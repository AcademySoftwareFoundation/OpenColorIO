/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OpenColorIO/OpenColorIO.h>

#include <expressions/expressions.h>

#include "ExpressionOps.h"



OCIO_NAMESPACE_ENTER
{
    namespace
    {
        void Apply1DExpression(float*rgbaBuffer, long numPixels,
                             const std::string expression)
        {
            expr::Parser<float> parser;
            expr::Parser<float>::VariableMap vm;
            vm["pi"] = 3.14159f;

            expr::ASTNode* ast = parser.parse(expression.c_str(), &vm);
            expr::Evaluator<float> evaluator;

            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                vm["r"] = rgbaBuffer[0];
                vm["g"] = rgbaBuffer[1];
                vm["b"] = rgbaBuffer[2];
                vm["a"] = rgbaBuffer[3];

                // Skip the alpha channel

                for(unsigned short subpixel=0; subpixel<3; subpixel++)
                {
                    float v = rgbaBuffer[subpixel]; vm["v"] = v;
                    rgbaBuffer[subpixel] = ast ? evaluator.evaluate(ast) : v;
                }

                rgbaBuffer += 4;
            }

            delete ast;

        }



        void Apply3DExpression(float*rgbaBuffer, long numPixels,
                             const std::string expressionR,
                             const std::string expressionG,
                             const std::string expressionB)
        {
            expr::Parser<float> parser;
            expr::Parser<float>::VariableMap vm;
            vm["pi"] = 3.14159f;

            expr::ASTNode* astR = parser.parse(expressionR.c_str(), &vm);
            expr::ASTNode* astG = parser.parse(expressionG.c_str(), &vm);
            expr::ASTNode* astB = parser.parse(expressionB.c_str(), &vm);

            expr::Evaluator<float> evaluator;

            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                float r = rgbaBuffer[0]; vm["r"] = r;
                float g = rgbaBuffer[1]; vm["g"] = g;
                float b = rgbaBuffer[2]; vm["b"] = b;
                float a = rgbaBuffer[3]; vm["a"] = a;

                // Set v to the current channel before evaluation
                // just in case people are using the v style expression
                vm["v"] = r; rgbaBuffer[0] = astR ? evaluator.evaluate(astR) : r;
                vm["v"] = g; rgbaBuffer[1] = astG ? evaluator.evaluate(astG) : g;
                vm["v"] = b; rgbaBuffer[2] = astB ? evaluator.evaluate(astB) : b;

                // Skip alpha
                rgbaBuffer += 4;
            }

            delete astR;
            delete astG;
            delete astB;

        }



        std::string GenerateGpuShader(const std::string expression, GpuLanguage lang)
        {
            expr::Parser<float> parser;
            expr::Parser<float>::VariableMap vm;
            vm["pi"] = 3.14159f;

            expr::ASTNode* ast = parser.parse(expression.c_str(), &vm);
            expr::ShaderGenerator<float> generator;
            expr::ShaderGenerator<float>::Language vers;
            std::string shader;

            if(lang == GPU_LANGUAGE_GLSL_1_0)
            {
				vers = expr::ShaderGenerator<float>::GLSLv1_0;
            }
            else if(lang == GPU_LANGUAGE_GLSL_1_3)
            {
				vers = expr::ShaderGenerator<float>::GLSLv1_3;
            }
            else
            {
                throw Exception("Unsupported shader language.");
            }
			
            shader = ast ? generator.generate(ast, vers) : "v";
            return shader;
        }


        const int FLOAT_DECIMALS = 7;
    }

    namespace
    {
        class ExpressionOp : public Op
        {
        public:
            ExpressionOp(const std::string expression,
                        TransformDirection direction);

            ExpressionOp(const std::string expressionR,
                         const std::string expressionG,
                         const std::string expressionB,
                        TransformDirection direction);


            virtual ~ExpressionOp();

            virtual OpRcPtr clone() const;

            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;

            virtual bool isNoOp() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;

            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;

            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
        private:

            std::string m_expressionR;
            std::string m_expressionG;
            std::string m_expressionB;
            bool m_is3d;

            // Set in finalize
            std::string m_cacheID;
        };

        typedef OCIO_SHARED_PTR<ExpressionOp> ExpressionOpRcPtr;


        ExpressionOp::ExpressionOp(const std::string expression,
                                   TransformDirection direction):
                                   Op(),
                                   m_expressionR(expression),
                                   m_expressionG(""),
                                   m_expressionB(""),
                                   m_is3d(false)
        {
            if (direction != TRANSFORM_DIR_FORWARD)
            {
                throw Exception("ExpressionOp only supports forward direction");
            }
        }

        ExpressionOp::ExpressionOp(const std::string expressionR,
                                   const std::string expressionG,
                                   const std::string expressionB,
                                   TransformDirection direction):
                                   Op(),
                                   m_expressionR(expressionR),
                                   m_expressionG(expressionG),
                                   m_expressionB(expressionB),
                                   m_is3d(true)
        {
            if (direction != TRANSFORM_DIR_FORWARD)
            {
                throw Exception("ExpressionOp only supports forward direction");
            }
        }

        OpRcPtr ExpressionOp::clone() const
        {
            ExpressionOp * op = new ExpressionOp(m_expressionR, TRANSFORM_DIR_FORWARD);
            op->m_expressionG = m_expressionG;
            op->m_expressionB = m_expressionB;
            op->m_is3d = m_is3d;
            return OpRcPtr(op);
        }

        ExpressionOp::~ExpressionOp()
        { }

        std::string ExpressionOp::getInfo() const
        {
            return "<ExpressionOp>";
        }

        std::string ExpressionOp::getCacheID() const
        {
            return m_cacheID;
        }

        bool ExpressionOp::isNoOp() const
        {
            if (m_is3d)
            {
                return m_expressionR == "" && m_expressionG == "" && m_expressionB == "";
            }

            return m_expressionR == "";
        }

        bool ExpressionOp::isSameType(const OpRcPtr & op) const
        {
            ExpressionOpRcPtr typedRcPtr = DynamicPtrCast<ExpressionOp>(op);
            if (!typedRcPtr) return false;
            return true;
        }

        bool ExpressionOp::isInverse(const OpRcPtr & op) const
        {
            if (!op) return false;
            // There is no way to determine whether another expression
            // is the functional inverse of this expression.
            // return false to be on the safe side
            return false;
        }

        bool ExpressionOp::hasChannelCrosstalk() const
        {
            return m_is3d;
        }

        void ExpressionOp::finalize()
        {
            std::ostringstream cacheIDStream;
            cacheIDStream << "<ExpressionOp ";
            cacheIDStream.precision(FLOAT_DECIMALS);
            cacheIDStream << m_expressionR << " ";
            cacheIDStream << m_expressionG << " ";
            cacheIDStream << m_expressionB << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }

        void ExpressionOp::apply(float*rgbaBuffer, long numPixels) const
        {
            if (!rgbaBuffer) return;

            if(m_is3d)
            {
                Apply3DExpression(rgbaBuffer, numPixels, m_expressionR, m_expressionG, m_expressionB);
            }
            else
            {
                Apply1DExpression(rgbaBuffer, numPixels, m_expressionR);
            }
        }

        bool ExpressionOp::supportsGpuShader() const
        {
            return true;
        }

        void ExpressionOp::writeGpuShader(std::ostream & shader,
                                          const std::string & pixelName,
                                          const GpuShaderDesc & shaderDesc) const
        {
            GpuLanguage lang = shaderDesc.getLanguage();

            shader << "float r = inPixel.r;\n"
                   << "float g = inPixel.g;\n"
                   << "float b = inPixel.b;\n"
                   << "float a = inPixel.a;\n"
                   << "float v = inPixel.r;\n"
                   << "float pi = 3.14159f;\n";
		

            if (m_is3d)
            {
				shader << "v = inPixel(r); " << pixelName << ".r = " << GenerateGpuShader(m_expressionR, lang) <<";\n";
				shader << "v = inPixel(g); " << pixelName << ".g = " << GenerateGpuShader(m_expressionG, lang) <<";\n";
				shader << "v = inPixel(b); " << pixelName << ".b = " << GenerateGpuShader(m_expressionB, lang) <<";\n";
            }
            else
            {
				shader << "v = inPixel(r); " << pixelName << ".r = " << GenerateGpuShader(m_expressionR, lang) <<";\n";
				shader << "v = inPixel(g); " << pixelName << ".g = " << GenerateGpuShader(m_expressionR, lang) <<";\n";
				shader << "v = inPixel(b); " << pixelName << ".b = " << GenerateGpuShader(m_expressionR, lang) <<";\n";
            }
        }

    } // Anon namespace


    void CreateExpressionOp(OpRcPtrVec & ops,
                          const std::string expression,
                          TransformDirection direction)
    {
        ExpressionOp *op = new ExpressionOp(expression, direction);
        ops.push_back( ExpressionOpRcPtr(op));
    }


    void CreateExpressionOp(OpRcPtrVec & ops,
                          const std::string expressionR,
                          const std::string expressionG,
                          const std::string expressionB,
                          TransformDirection direction)
    {
        ExpressionOp *op = new ExpressionOp(expressionR,
                                            expressionG,
                                            expressionB,
                                            direction);

        ops.push_back( ExpressionOpRcPtr(op));
    }
}
OCIO_NAMESPACE_EXIT

