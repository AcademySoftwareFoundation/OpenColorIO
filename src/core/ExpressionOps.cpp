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

#if OCIO_USE_BOOST_PTR
#define EXPRESSIONS_USE_BOOST_PTR
#endif

#include <expressions/expressions.h>

#include "ExpressionOps.h"



OCIO_NAMESPACE_ENTER
{
    namespace
    {
        std::string GenerateGpuShader(expr::ASTNodePtr ast, GpuLanguage lang)
        {
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

            try
            {
                shader = ast ? generator.generate(ast, vers) : "v";
            }
            catch (expr::GeneratorException &e)
            {
                throw Exception(e.what());
            }

            return shader;
        }

        bool usesOtherChannels(expr::ASTNodePtr ast, std::string channel)
        {
            if (ast->type() == expr::ASTNode::NUMBER)
            {
                return false;
            }
            if(ast->type() == expr::ASTNode::VARIABLE)
            {
                SHARED_PTR<expr::VariableASTNode<float> > v = STATIC_POINTER_CAST<expr::VariableASTNode<float> >(ast);
                if (v->variable() != channel && v->variable() != "v")
                {
                    return true;
                }
                return false;
            }
            else if (ast->type() == expr::ASTNode::OPERATION)
            {
                SHARED_PTR<expr::OperationASTNode> op = STATIC_POINTER_CAST<expr::OperationASTNode>(ast);
                bool v1 = usesOtherChannels(op->right(), channel);
                bool v2 = usesOtherChannels(op->left(), channel);
                return v1 || v2;
            }
            else if (ast->type() == expr::ASTNode::FUNCTION1)
            {
                SHARED_PTR<expr::Function1ASTNode> f = STATIC_POINTER_CAST<expr::Function1ASTNode>(ast);
                return usesOtherChannels(f->left(), channel);
            }
            else if (ast->type() == expr::ASTNode::FUNCTION2)
            {
                SHARED_PTR<expr::Function2ASTNode> f = STATIC_POINTER_CAST<expr::Function2ASTNode>(ast);
                bool v1 = usesOtherChannels(f->right(), channel);
                bool v2 = usesOtherChannels(f->left(), channel);
                return v1 || v2;
            }
            else if (ast->type() == expr::ASTNode::COMPARISON)
            {
                SHARED_PTR<expr::ComparisonASTNode> c = STATIC_POINTER_CAST<expr::ComparisonASTNode>(ast);
                bool v1 = usesOtherChannels(c->right(), channel);
                bool v2 = usesOtherChannels(c->left(), channel);
                return v1 || v2;
            }
            else if (ast->type() == expr::ASTNode::LOGICAL)
            {
                SHARED_PTR<expr::LogicalASTNode> l = STATIC_POINTER_CAST<expr::LogicalASTNode>(ast);
                bool v1 = usesOtherChannels(l->right(), channel);
                bool v2 = usesOtherChannels(l->left(), channel);
                return v1 || v2;
            }
            else if (ast->type() == expr::ASTNode::BRANCH)
            {
                SHARED_PTR<expr::BranchASTNode> b = STATIC_POINTER_CAST<expr::BranchASTNode>(ast);
                bool v1 = usesOtherChannels(b->yes(), channel);
                bool v2 = usesOtherChannels(b->no(), channel);
                bool v3 = usesOtherChannels(b->condition(), channel);
                return v1 || v2 || v3;
            }
            throw Exception("Incorrect syntax tree!");
        }

        const int FLOAT_DECIMALS = 7;
    }

    namespace
    {
        class ExpressionOp : public Op
        {
        public:

            ExpressionOp(const std::string expressionR,
                         const std::string expressionG,
                         const std::string expressionB,
                         const std::string expressionA,
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
            std::string m_expressionA;

            expr::ASTNodePtr m_astR;
            expr::ASTNodePtr m_astG;
            expr::ASTNodePtr m_astB;
            expr::ASTNodePtr m_astA;


#ifdef USE_LLVM
            typedef expr::LLVMEvaluator Evaluator;
            typedef expr::LLVMEvaluator::VariableMap VariableMap;
#else
            typedef expr::Evaluator<float> Evaluator;
            typedef expr::Evaluator<float>::VariableMap VariableMap;
#endif

            mutable VariableMap m_vm;

            // Set in finalize

            Evaluator* m_evaluatorR;
            Evaluator* m_evaluatorG;
            Evaluator* m_evaluatorB;
            Evaluator* m_evaluatorA;

            std::string m_cacheID;
        };

        typedef OCIO_SHARED_PTR<ExpressionOp> ExpressionOpRcPtr;


        ExpressionOp::ExpressionOp(const std::string expressionR,
                                   const std::string expressionG,
                                   const std::string expressionB,
                                   const std::string expressionA,
                                   TransformDirection direction):
            Op(),
            m_expressionR(expressionR),
            m_expressionG(expressionG),
            m_expressionB(expressionB),
            m_expressionA(expressionA),
            m_evaluatorR(NULL),
            m_evaluatorG(NULL),
            m_evaluatorB(NULL),
            m_evaluatorA(NULL)
        {
            if (direction != TRANSFORM_DIR_FORWARD)
            {
                throw Exception("ExpressionOp only supports forward direction");
            }

            // Add some default variables to the variablemap
            m_vm["pi"] = 3.14159265359f;
            m_vm["r"] = 0.0f;
            m_vm["g"] = 0.0f;
            m_vm["b"] = 0.0f;
            m_vm["a"] = 0.0f;
            m_vm["v"] = 0.0f;

            // Construct the AST's for each of the expressions

            expr::Parser<float> parser;

            try
            {
                m_astR = parser.parse(expressionR.c_str());
                m_astG = parser.parse(expressionG.c_str());
                m_astB = parser.parse(expressionB.c_str());
                m_astA = parser.parse(expressionA.c_str());
            }
            catch (expr::ParserException &e)
            {
                throw Exception(e.what());
            }
        }

        OpRcPtr ExpressionOp::clone() const
        {
            ExpressionOp * op = new ExpressionOp(m_expressionR,
                                                 m_expressionG,
                                                 m_expressionB,
                                                 m_expressionA,
                                                 TRANSFORM_DIR_FORWARD);
            return OpRcPtr(op);
        }

        ExpressionOp::~ExpressionOp()
        {
            delete m_evaluatorR;
            delete m_evaluatorG;
            delete m_evaluatorB;
            delete m_evaluatorA;
        }

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
            return (!m_astR && !m_astG && !m_astB && !m_astA);
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
            typedef std::map<std::string, expr::ASTNodePtr> NodeMap;
            NodeMap channels;
            channels["r"] = m_astR;
            channels["g"] = m_astG;
            channels["b"] = m_astB;
            channels["a"] = m_astA;

            for (NodeMap::iterator it=channels.begin(); it!=channels.end(); ++it)
            {
                // Need to walk the AST to look for Variable nodes that are set to other channel names
                if (usesOtherChannels(it->second, it->first))
                {
                    return true;
                }
            }
            return false;
        }

        void ExpressionOp::finalize()
        {
            std::ostringstream cacheIDStream;
            cacheIDStream << "<ExpressionOp ";
            cacheIDStream.precision(FLOAT_DECIMALS);
            cacheIDStream << m_expressionR << " ";
            cacheIDStream << m_expressionG << " ";
            cacheIDStream << m_expressionB << " ";
            cacheIDStream << m_expressionA << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();

            // Build the evaluators
            m_evaluatorR = m_astR ? new Evaluator(m_astR, &m_vm) : NULL;
            m_evaluatorG = m_astG ? new Evaluator(m_astG, &m_vm) : NULL;
            m_evaluatorB = m_astB ? new Evaluator(m_astB, &m_vm) : NULL;
            m_evaluatorA = m_astA ? new Evaluator(m_astA, &m_vm) : NULL;
        }

        void ExpressionOp::apply(float*rgbaBuffer, long numPixels) const
        {
            if (!rgbaBuffer) return;

            float &r = m_vm["r"];
            float &g = m_vm["g"];
            float &b = m_vm["b"];
            float &a = m_vm["a"];
            float &v = m_vm["v"];

            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                r = rgbaBuffer[0];
                g = rgbaBuffer[1];
                b = rgbaBuffer[2];
                a = rgbaBuffer[3];

                try
                {
                    // Set v to the current channel before evaluation
                    // just in case people are using the v style expression
                    if (m_evaluatorR)
                    {
                        v = r; rgbaBuffer[0] = m_evaluatorR->evaluate();
                    }
                    if (m_evaluatorG)
                    {
                        v = g; rgbaBuffer[1] = m_evaluatorG->evaluate();
                    }
                    if (m_evaluatorB)
                    {
                        v = b; rgbaBuffer[2] = m_evaluatorB->evaluate();
                    }
                    if (m_evaluatorA)
                    {
                        v = a; rgbaBuffer[3] = m_evaluatorA->evaluate();
                    }
                }
                catch (expr::EvaluatorException &e)
                {
                    throw Exception(e.what());
                }

                rgbaBuffer += 4;
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


            shader << "v = inPixel(r); " << pixelName << ".r = " << GenerateGpuShader(m_astR, lang) <<";\n";
            shader << "v = inPixel(g); " << pixelName << ".g = " << GenerateGpuShader(m_astG, lang) <<";\n";
            shader << "v = inPixel(b); " << pixelName << ".b = " << GenerateGpuShader(m_astB, lang) <<";\n";
            shader << "v = inPixel(a); " << pixelName << ".a = " << GenerateGpuShader(m_astA, lang) <<";\n";
        }

    } // Anon namespace


    void CreateExpressionOp(OpRcPtrVec & ops,
                          const std::string expressionR,
                          const std::string expressionG,
                          const std::string expressionB,
                          const std::string expressionA,
                          TransformDirection direction)
    {
        ExpressionOp *op = new ExpressionOp(expressionR,
                                            expressionG,
                                            expressionB,
                                            expressionA,
                                            direction);

        ops.push_back( ExpressionOpRcPtr(op));
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include "UnitTest.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(ExpressionOps, Value)
{
    OpRcPtrVec ops;
    CreateExpressionOp(ops, "v+0.5", "v+0.5", "v+0.5", "", TRANSFORM_DIR_FORWARD); // 1d
    CreateExpressionOp(ops, "v+0.5", "v+0.1", "v+0.2", "", TRANSFORM_DIR_FORWARD); // 3d
    CreateExpressionOp(ops, "g+0.5", "r+0.1", "b+0.4", "a-0.5", TRANSFORM_DIR_FORWARD); // crosstalk
    CreateExpressionOp( ops, 
                        "v > 0.1496582 ? (pow(10.0, (v - 0.385537) / 0.2471896) - 0.052272) / 5.555556 : (v -0.092809) / 5.367655",
                        "(v < 0.0404482362771082) ? v/12.92 : ((v+0.055)/1.055)^2.4",
                        "v<0.081 ? v/4.5 : ((v+0.099)/1.099)^(1/0.45)",
                        "", 
                        TRANSFORM_DIR_FORWARD); // complex
    OIIO_CHECK_EQUAL(ops.size(), 4);

    for(unsigned int i=0; i<ops.size(); ++i)
    {
        ops[i]->finalize();
    }

    float error = 1e-6f;

    float tmp[4];
    const float source[] = {0.1f, 0.2f, 0.3f, 1.0f};

    const float result1[] = {0.6f, 0.7f, 0.8f, 1.0f};
    const float result2[] = {0.6f, 0.3f, 0.5f, 1.0f};
    const float result3[] = {0.7f, 0.2f, 0.7f, 0.5f};
    const float result4[] = {0.00133969f, 0.0331048f, 0.105237f, 1.0f};

    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, 1);

    for (unsigned int i=0; i<4; i++)
    {
        OIIO_CHECK_CLOSE(tmp[i], result1[i], error);
    }

    memcpy(tmp, source, 4*sizeof(float));
    ops[1]->apply(tmp, 1);
    for (unsigned int i=0; i<4; i++)
    {
        OIIO_CHECK_CLOSE(tmp[i], result2[i], error);
    }

    memcpy(tmp, source, 4*sizeof(float));
    ops[2]->apply(tmp, 1);
    for (unsigned int i=0; i<4; i++)
    {
        OIIO_CHECK_CLOSE(tmp[i], result3[i], error);
    }

    memcpy(tmp, source, 4*sizeof(float));
    ops[3]->apply(tmp, 1);
    for (unsigned int i=0; i<4; i++)
    {
        OIIO_CHECK_CLOSE(tmp[i], result4[i], error);
    }
}


OIIO_ADD_TEST(ExpressionOps, Crosstalk)
{
    OpRcPtrVec ops;
    CreateExpressionOp(ops, "g+0.5", "r+0.1", "b+0.4", "a-0.5", TRANSFORM_DIR_FORWARD); // crosstalk
    CreateExpressionOp(ops, "v+0.5", "v+0.1", "b+0.4", "a-0.5", TRANSFORM_DIR_FORWARD); // no crosstalk
    OIIO_CHECK_EQUAL(ops.size(), 2);

    for(unsigned int i=0; i<ops.size(); ++i)
    {
        ops[i]->finalize();
    }

    OIIO_CHECK_EQUAL(ops[0]->hasChannelCrosstalk(), true);
    OIIO_CHECK_EQUAL(ops[1]->hasChannelCrosstalk(), false);
}

#endif // OCIO_UNIT_TEST
