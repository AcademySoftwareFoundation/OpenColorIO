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

#include <cmath>
#include <cstring>
#include <sstream>
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "ExponentOps.h"
#include "GpuShaderUtils.h"
#include "MathUtils.h"

OCIO_NAMESPACE_ENTER
{
    namespace DefaultValues
    {
        const int FLOAT_DECIMALS = 7;
    }

    ExponentOpData::ExponentOpData()
        :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    {
        for(unsigned i=0; i<4; ++i)
        {
            m_exp4[i] = 1.0;
        }
    }

    ExponentOpData::ExponentOpData(const double * exp4)
        :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    {
        memcpy(m_exp4, exp4, 4*sizeof(double)); 
    }

    ExponentOpData & ExponentOpData::operator = (const ExponentOpData & rhs)
    {
        if(this!=&rhs)
        {
            memcpy(m_exp4, rhs.m_exp4, sizeof(double)*4);
        }

        return *this;
    }

    bool ExponentOpData::isNoOp() const
    {
        return (getInputBitDepth() == getOutputBitDepth()) && isIdentity();
    }

    bool ExponentOpData::isIdentity() const
    {
        return IsVecEqualToOne(m_exp4, 4);
    }

    void ExponentOpData::finalize()
    {
        AutoMutex lock(m_mutex);

        std::ostringstream cacheIDStream;
        cacheIDStream << getID();

        cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);
        for(int i=0; i<4; ++i)
        {
            cacheIDStream << m_exp4[i] << " ";
        }

        m_cacheID = cacheIDStream.str();
    }


    namespace
    {
        void ApplyClampExponent(const void * inImg, void * outImg, long numPixels,
                                ConstExponentOpDataRcPtr & exp)
        {
            const float * in = (const float *)inImg;
            float * out = (float *)outImg;

            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                out[0] = powf( std::max(0.0f, in[0]), float(exp->m_exp4[0]));
                out[1] = powf( std::max(0.0f, in[1]), float(exp->m_exp4[1]));
                out[2] = powf( std::max(0.0f, in[2]), float(exp->m_exp4[2]));
                out[3] = powf( std::max(0.0f, in[3]), float(exp->m_exp4[3]));

                in  += 4;
                out += 4;
            }
        }
    }
    
   
    namespace
    {
        class ExponentOp : public Op
        {
        public:
            ExponentOp(const double * exp4);
            ExponentOp(ExponentOpDataRcPtr & exp);

            virtual ~ExponentOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            
            virtual bool isSameType(ConstOpRcPtr & op) const;
            virtual bool isInverse(ConstOpRcPtr & op) const;
            
            virtual bool canCombineWith(ConstOpRcPtr & op) const;
            virtual void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const;
            
            virtual void finalize();
            virtual void apply(void * img, long numPixels) const;
            virtual void apply(const void * inImg, void * outImg, long numPixels) const;
            
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

        protected:
            ConstExponentOpDataRcPtr expData() const { return DynamicPtrCast<const ExponentOpData>(data()); }
            ExponentOpDataRcPtr expData() { return DynamicPtrCast<ExponentOpData>(data()); }

        };
        
        typedef OCIO_SHARED_PTR<ExponentOp> ExponentOpRcPtr;
        typedef OCIO_SHARED_PTR<const ExponentOp> ConstExponentOpRcPtr;
        
        ExponentOp::ExponentOp(const double * exp4)
            : Op()
        {
            data().reset(new ExponentOpData(exp4));
        }

        ExponentOp::ExponentOp(ExponentOpDataRcPtr & exp)
            : Op()
        {
            data() = exp;
        }

        OpRcPtr ExponentOp::clone() const
        {
            return std::make_shared<ExponentOp>(expData()->m_exp4);
        }
        
        ExponentOp::~ExponentOp()
        { }
        
        std::string ExponentOp::getInfo() const
        {
            return "<ExponentOp>";
        }
        
        bool ExponentOp::isSameType(ConstOpRcPtr & op) const
        {
            ConstExponentOpRcPtr typedRcPtr = DynamicPtrCast<const ExponentOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool ExponentOp::isInverse(ConstOpRcPtr & op) const
        {
            ConstExponentOpRcPtr typedRcPtr = DynamicPtrCast<const ExponentOp>(op);
            if(!typedRcPtr) return false;
            
            double combined[4] 
                = { expData()->m_exp4[0]*typedRcPtr->expData()->m_exp4[0],
                    expData()->m_exp4[1]*typedRcPtr->expData()->m_exp4[1],
                    expData()->m_exp4[2]*typedRcPtr->expData()->m_exp4[2],
                    expData()->m_exp4[3]*typedRcPtr->expData()->m_exp4[3] };
            
            return IsVecEqualToOne(combined, 4);
        }

        bool ExponentOp::canCombineWith(ConstOpRcPtr & op) const
        {
            return isSameType(op);
        }

        void ExponentOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
        {
            ConstExponentOpRcPtr typedRcPtr = DynamicPtrCast<const ExponentOp>(secondOp);
            if(!typedRcPtr)
            {
                std::ostringstream os;
                os << "ExponentOp can only be combined with other ";
                os << "ExponentOps.  secondOp:" << secondOp->getInfo();
                throw Exception(os.str().c_str());
            }
            
            const double combined[4] 
                = { expData()->m_exp4[0]*typedRcPtr->expData()->m_exp4[0],
                    expData()->m_exp4[1]*typedRcPtr->expData()->m_exp4[1],
                    expData()->m_exp4[2]*typedRcPtr->expData()->m_exp4[2],
                    expData()->m_exp4[3]*typedRcPtr->expData()->m_exp4[3] };

            if(!IsVecEqualToOne(combined, 4))
            {
                ops.push_back(std::make_shared<ExponentOp>(combined) );
            }
        }

        void ExponentOp::finalize()
        {
            expData()->finalize();

            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<ExponentOp ";
            cacheIDStream << expData()->getCacheID() << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void ExponentOp::apply(void * img, long numPixels) const
        {
            ConstExponentOpDataRcPtr expOpData = expData();
            ApplyClampExponent(img, img, numPixels, expOpData);
        }

        void ExponentOp::apply(const void * inImg, void * outImg, long numPixels) const
        {
            ConstExponentOpDataRcPtr expOpData = expData();
            ApplyClampExponent(inImg, outImg, numPixels, expOpData);
        }

        void ExponentOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const 
        {
            if(getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            // outColor = pow(max(outColor, 0.), exp);

            ss.newLine()
                << shaderDesc->getPixelName() 
                << " = pow( "
                << "max( " << shaderDesc->getPixelName() 
                << ", " << ss.vec4fConst(0.0f) << " )"
                << ", " << ss.vec4fConst(expData()->m_exp4[0], expData()->m_exp4[1], 
                                         expData()->m_exp4[2], expData()->m_exp4[3]) << " );";

            shaderDesc->addToFunctionShaderCode(ss.string().c_str());
        }
        
    }  // Anon namespace
    
    
    
    void CreateExponentOp(OpRcPtrVec & ops,
                          const double(&vec4)[4],
                          TransformDirection direction)
    {
        ExponentOpDataRcPtr expData = std::make_shared<ExponentOpData>(vec4);
        CreateExponentOp(ops, expData, direction);
    }

    void CreateExponentOp(OpRcPtrVec & ops,
                          ExponentOpDataRcPtr & expData,
                          TransformDirection direction)
    {
        if (!IsVecEqualToOne(expData->m_exp4, 4))
        {
            if (direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot create ExponentOp with unspecified transform direction.");
            }
            else if (direction == TRANSFORM_DIR_INVERSE)
            {
                double values[4];
                for (int i = 0; i<4; ++i)
                {
                    if (!IsScalarEqualToZero(expData->m_exp4[i]))
                    {
                        values[i] = 1.0 / expData->m_exp4[i];
                    }
                    else
                    {
                        throw Exception("Cannot apply ExponentOp op, Cannot apply 0.0 exponent in the inverse.");
                    }
                }
                ExponentOpDataRcPtr expInv = std::make_shared<ExponentOpData>(values);
                ops.push_back(std::make_shared<ExponentOp>(expInv));
            }
            else
            {
                ops.push_back(std::make_shared<ExponentOp>(expData));
            }
        }
    }

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include "unittest.h"
#include "ops/NoOp/NoOps.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(ExponentOps, Value)
{
    const double exp1[4] = { 1.2, 1.3, 1.4, 1.5 };
    
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    
    for(unsigned int i=0; i<ops.size(); ++i)
    {
        OIIO_CHECK_NO_THROW(ops[i]->finalize());
    }
    
    float error = 1e-6f;
    
    const float source[] = {  0.1f, 0.3f, 0.9f, 0.5f, };
    
    const float result1[] = { 0.0630957261f, 0.209053621f,
        0.862858355f, 0.353553385f };
    
    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, tmp, 1);
    
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], result1[i], error);
    }
    
    ops[1]->apply(tmp, tmp, 1);
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], source[i], error);
    }
}

void ValidateOp(const float * source, const OpRcPtr op, const float * result, const float error)
{
    float tmp[4];
    memcpy(tmp, source, 4 * sizeof(float));
    op->apply(tmp, tmp, 1);

    for (unsigned int i = 0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], result[i], error);
    }
}

OIIO_ADD_TEST(ExponentOps, ValueLimits)
{
    const double exp1[4] = { 0., 2., -2., 1.5 };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    float error = 1e-6f;

    const float source1[] = { 1.0f, 1.0f, 1.0f, 1.0f, };
    const float result1[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ValidateOp(source1, ops[0], result1, error);

    const float source2[] = { 2.0f, 2.0f, 2.0f, 2.0f, };
    const float result2[] = { 1.0f, 4.0f, 0.25f, 2.82842708f };
    ValidateOp(source2, ops[0], result2, error);

    const float source3[] = { -2.0f, -2.0f, 1.0f, -2.0f, };
    const float result3[] = { 1.0f, 0.0f, 1.0f, 0.0f };
    ValidateOp(source3, ops[0], result3, error);

    const float source4[] = { 0.0f, 0.0f, 1.0f, 0.0f, };
    const float result4[] = { 1.0f, 0.0f, 1.0f, 0.0f };
    ValidateOp(source4, ops[0], result4, error);
}

OIIO_ADD_TEST(ExponentOps, Inverse)
{
    const double exp1[4] = { 2.0, 1.02345, 5.651321, 0.12345678910 };
    const double exp2[4] = { 2.0, 2.0,     2.0,      2.0 };
    
    OpRcPtrVec ops;
    
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp2, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp2, TRANSFORM_DIR_INVERSE));
    
    OIIO_REQUIRE_EQUAL(ops.size(), 4);
    ConstOpRcPtr op0 = ops[0];
    ConstOpRcPtr op1 = ops[1];
    ConstOpRcPtr op2 = ops[2];
    ConstOpRcPtr op3 = ops[3];

    OIIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(op2));
    ConstOpRcPtr op3Cloned = ops[3]->clone();
    OIIO_CHECK_ASSERT(ops[0]->isSameType(op3Cloned));
    
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op0), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op1), true);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(op0), true);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op2), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op3), false);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(op0), false);
    OIIO_CHECK_EQUAL(ops[2]->isInverse(op3), true);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(op2), true);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(op3), false);
}

OIIO_ADD_TEST(ExponentOps, Combining)
{
    const float error = 1e-6f;
    {
    const double exp1[4] = { 2.0, 2.0, 2.0, 1.0 };
    const double exp2[4] = { 1.2, 1.2, 1.2, 1.0 };
    
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp2, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    ConstOpRcPtr op1 = ops[1];

    const float source[] = {  0.9f, 0.4f, 0.1f, 0.5f, };
    const float result[] = { 0.776572466f, 0.110903174f,
        0.00398107106f, 0.5f };
    
    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, tmp, 1);
    ops[1]->apply(tmp, tmp, 1);
    
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], result[i], error);
    }
    
    OpRcPtrVec combined;
    OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
    OIIO_CHECK_EQUAL(combined.size(), 1);
    
    float tmp2[4];
    memcpy(tmp2, source, 4*sizeof(float));
    combined[0]->apply(tmp2, tmp2, 1);
    
    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp2[i], result[i], error);
    }
    }
    
    {
    
    const double exp1[4] = {1.037289, 1.019015, 0.966082, 1.0};
    
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    ConstOpRcPtr op1 = ops[1];

    const bool isInverse = ops[0]->isInverse(op1);
    OIIO_CHECK_EQUAL(isInverse, true);

    OpRcPtrVec combined;
    OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
    OIIO_CHECK_EQUAL(combined.empty(), true);

    }

    {
    const double exp1[4] = { 1.037289, 1.019015, 0.966082, 1.0 };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 3);

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
    OIIO_CHECK_EQUAL(ops.size(), 3);

    const float source[] = { 0.1f, 0.5f, 0.9f, 0.5f, };
    const float result[] = { 0.0765437484f, 0.480251998f,
        0.909373641f, 0.5f };

    float tmp[4];
    memcpy(tmp, source, 4 * sizeof(float));
    ops[0]->apply(tmp, tmp, 1);
    ops[1]->apply(tmp, tmp, 1);
    ops[2]->apply(tmp, tmp, 1);

    for (unsigned int i = 0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], result[i], error);
    }

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    memcpy(tmp, source, 4 * sizeof(float));
    ops[0]->apply(tmp, tmp, 1);

    for (unsigned int i = 0; i<4; ++i)
    {
        OIIO_CHECK_CLOSE(tmp[i], result[i], error);
    }
    }
}

OIIO_ADD_TEST(ExponentOps, ThrowCreate)
{
    const double exp1[4] = { 0.0, 1.3, 1.4, 1.5 };

    OpRcPtrVec ops;
    OIIO_CHECK_THROW_WHAT(
        CreateExponentOp(ops, exp1, TRANSFORM_DIR_UNKNOWN),
        OpenColorIO::Exception, "unspecified transform direction");

    OIIO_CHECK_THROW_WHAT(
        CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE),
        OpenColorIO::Exception, "Cannot apply 0.0 exponent in the inverse");
}

OIIO_ADD_TEST(ExponentOps, ThrowCombine)
{
    const double exp1[4] = { 0.0, 1.3, 1.4, 1.5 };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    ConstOpRcPtr op1 = ops[1];

    OpRcPtrVec combinedOps;
    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OpenColorIO::Exception, "can only be combined with other ExponentOps");
}

OIIO_ADD_TEST(ExponentOps, NoOp)
{
    const double exp1[4] = { 1.0, 1.0, 1.0, 1.0 };

    // CreateExponentOp will not create a NoOp
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_INVERSE));

    OIIO_CHECK_EQUAL(ops.empty(), true);
}

OIIO_ADD_TEST(ExponentOps, CacheID)
{
    const double exp1[4] = { 2.0, 2.1, 3.0, 3.1 };
    const double exp2[4] = { 4.0, 4.1, 5.0, 5.1 };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp2, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateExponentOp(ops, exp1, TRANSFORM_DIR_FORWARD));

    OIIO_CHECK_EQUAL(ops.size(), 3);


    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));

    const std::string opCacheID0 = ops[0]->getCacheID();
    const std::string opCacheID1 = ops[1]->getCacheID();
    const std::string opCacheID2 = ops[2]->getCacheID();

    OIIO_CHECK_EQUAL(opCacheID0, opCacheID2);
    OIIO_CHECK_NE(opCacheID0, opCacheID1);
}


#endif // OCIO_UNIT_TEST
