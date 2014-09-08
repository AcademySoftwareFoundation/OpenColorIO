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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ExponentOps.h"
#include "GpuShaderUtils.h"
#include "MathUtils.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        void ApplyClampExponent(float* rgbaBuffer, long numPixels,
                                const float* exp4)
        {
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = powf( std::max(0.0f, rgbaBuffer[0]), exp4[0]);
                rgbaBuffer[1] = powf( std::max(0.0f, rgbaBuffer[1]), exp4[1]);
                rgbaBuffer[2] = powf( std::max(0.0f, rgbaBuffer[2]), exp4[2]);
                rgbaBuffer[3] = powf( std::max(0.0f, rgbaBuffer[3]), exp4[3]);
                rgbaBuffer += 4;
            }
        }
        
        const int FLOAT_DECIMALS = 7;
    }
    
    
    namespace
    {
        class ExponentOp : public Op
        {
        public:
            ExponentOp(const double * exp4,
                       TransformDirection direction);
            virtual ~ExponentOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual bool isNoOp() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            
            virtual bool canCombineWith(const OpRcPtr & op) const;
            virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;
            
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
        private:
            double m_exp4[4];

            // Set in finalize
            std::string m_cacheID;
        };
        
        typedef OCIO_SHARED_PTR<ExponentOp> ExponentOpRcPtr;
        
        
        ExponentOp::ExponentOp(const double * exp4,
                               TransformDirection direction):
                               Op()
        {
            if(direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot create ExponentOp with unspecified transform direction.");
            }
            
            if(direction == TRANSFORM_DIR_INVERSE)
            {
                for(int i=0; i<4; ++i)
                {
                    if(!IsScalarEqualToZeroFlt(exp4[i]))
                    {
                        m_exp4[i] = 1.0 / exp4[i];
                    }
                    else
                    {
                        throw Exception("Cannot apply ExponentOp op, Cannot apply 0.0 exponent in the inverse.");
                    }
                }
            }
            else
            {
                memcpy(m_exp4, exp4, 4*sizeof(double));
            }
        }
        
        OpRcPtr ExponentOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new ExponentOp(m_exp4, TRANSFORM_DIR_FORWARD));
            return op;
        }
        
        ExponentOp::~ExponentOp()
        { }
        
        std::string ExponentOp::getInfo() const
        {
            return "<ExponentOp>";
        }
        
        std::string ExponentOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        bool ExponentOp::isNoOp() const
        {
            return IsVecEqualToOneFlt(m_exp4, 4);
        }

        bool ExponentOp::isSameType(const OpRcPtr & op) const
        {
            ExponentOpRcPtr typedRcPtr = DynamicPtrCast<ExponentOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool ExponentOp::isInverse(const OpRcPtr & op) const
        {
            ExponentOpRcPtr typedRcPtr = DynamicPtrCast<ExponentOp>(op);
            if(!typedRcPtr) return false;
            
            double combined[4] = { m_exp4[0]*typedRcPtr->m_exp4[0],
                                   m_exp4[1]*typedRcPtr->m_exp4[1],
                                   m_exp4[2]*typedRcPtr->m_exp4[2],
                                   m_exp4[3]*typedRcPtr->m_exp4[3] };
            
            return IsVecEqualToOneFlt(combined, 4);
        }

        bool ExponentOp::canCombineWith(const OpRcPtr & op) const
        {
            return isSameType(op);
        }

        void ExponentOp::combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const
        {
            ExponentOpRcPtr typedRcPtr = DynamicPtrCast<ExponentOp>(secondOp);
            if(!typedRcPtr)
            {
                std::ostringstream os;
                os << "ExponentOp can only be combined with other ";
                os << "ExponentOps.  secondOp:" << secondOp->getInfo();
                throw Exception(os.str().c_str());
            }
            
            double combined[4] = { m_exp4[0]*typedRcPtr->m_exp4[0],
                                   m_exp4[1]*typedRcPtr->m_exp4[1],
                                   m_exp4[2]*typedRcPtr->m_exp4[2],
                                   m_exp4[3]*typedRcPtr->m_exp4[3] };

            if(!IsVecEqualToOneFlt(combined, 4))
            {
                ops.push_back(
                    ExponentOpRcPtr(new ExponentOp(combined,
                        TRANSFORM_DIR_FORWARD)) );
            }
        }

        bool ExponentOp::hasChannelCrosstalk() const
        {
            return false;
        }
        
        void ExponentOp::finalize()
        {
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<ExponentOp ";
            cacheIDStream.precision(FLOAT_DECIMALS);
            for(int i=0; i<4; ++i)
            {
                cacheIDStream << m_exp4[i] << " ";
            }
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void ExponentOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(!rgbaBuffer) return;
            float exp[4] = { float(m_exp4[0]), float(m_exp4[1]),
                float(m_exp4[2]), float(m_exp4[3]) };
            ApplyClampExponent(rgbaBuffer, numPixels, exp);
        }
        
        bool ExponentOp::supportsGpuShader() const
        {
            return true;
        }
        
        void ExponentOp::writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const
        {
            float exp[4] = { float(m_exp4[0]), float(m_exp4[1]),
                float(m_exp4[2]), float(m_exp4[3]) };

            GpuLanguage lang = shaderDesc.getLanguage();
            float zerovec[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

            shader << pixelName << " = pow(";
            shader << "max(" << pixelName << ", " << GpuTextHalf4(zerovec, lang) << ")";
            shader << ", " << GpuTextHalf4(exp, lang) << ");\n";
        }
        
    }  // Anon namespace
    
    
    
    void CreateExponentOp(OpRcPtrVec & ops,
                          const float * exp4,
                          TransformDirection direction)
    {
        bool expIsIdentity = IsVecEqualToOne(exp4, 4);
        if(expIsIdentity) return;
        double d_exp[4] = { double(exp4[0]), double(exp4[1]),
                double(exp4[2]), double(exp4[3]) };
        ops.push_back( ExponentOpRcPtr(new ExponentOp(d_exp, direction)) );
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include "UnitTest.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(ExponentOps, Value)
{
    float exp1[4] = { 1.2f, 1.3f, 1.4f, 1.5f };
    
    OpRcPtrVec ops;
    CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD);
    CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    
    for(unsigned int i=0; i<ops.size(); ++i)
    {
        ops[i]->finalize();
    }
    
    float error = 1e-6f;
    
    const float source[] = {  0.5f, 0.5f, 0.5f, 0.5f, };
    
    const float result1[] = {  0.43527528164806206f, 0.40612619817811774f,
                               0.37892914162759955f, 0.35355339059327379f };
    
    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, 1);
    
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], result1[i], error);
    }
    
    ops[1]->apply(tmp, 1);
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], source[i], error);
    }
}

OIIO_ADD_TEST(ExponentOps, Inverse)
{
    float exp1[4] = { 2.0f, 1.02345f, 5.651321f, 0.12345678910f };
    float exp2[4] = { 2.0f, 2.0f, 2.0f, 2.0f };
    
    OpRcPtrVec ops;
    
    CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD);
    CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE);
    CreateExponentOp(ops, exp2, TRANSFORM_DIR_FORWARD);
    CreateExponentOp(ops, exp2, TRANSFORM_DIR_INVERSE);
    
    OIIO_CHECK_EQUAL(ops.size(), 4);
    
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[2]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[3]->clone()));
    
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[0]), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(ops[0]), true);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[3]), false);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(ops[0]), false);
    OIIO_CHECK_EQUAL(ops[2]->isInverse(ops[3]), true);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(ops[2]), true);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(ops[3]), false);
}

OIIO_ADD_TEST(ExponentOps, Combining)
{
    {
    float exp1[4] = { 2.0f, 2.0f, 2.0f, 1.0f };
    float exp2[4] = { 1.2f, 1.2f, 1.2f, 1.0f };
    
    OpRcPtrVec ops;
    CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD);
    CreateExponentOp(ops, exp2, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    
    float error = 1e-6f;
    const float source[] = {  0.5f, 0.5f, 0.5f, 0.5f, };
    const float result[] = {  0.18946457081379978f, 0.18946457081379978f,
                               0.18946457081379978f, 0.5f };
    
    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, 1);
    ops[1]->apply(tmp, 1);
    
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], result[i], error);
    }
    
    
    OpRcPtrVec combined;
    ops[0]->combineWith(combined, ops[1]);
    OIIO_CHECK_EQUAL(combined.size(), 1);
    
    float tmp2[4];
    memcpy(tmp2, source, 4*sizeof(float));
    combined[0]->apply(tmp2, 1);
    
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp2[i], result[i], error);
    }
    }
    
    {
    
    float exp1[4] = {1.037289f, 1.019015f, 0.966082f, 1.0f};
    
    OpRcPtrVec ops;
    CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD);
    CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    
    bool isInverse = ops[0]->isInverse(ops[1]);
    OIIO_CHECK_EQUAL(isInverse, true);
    }
}

#endif // OCIO_UNIT_TEST
