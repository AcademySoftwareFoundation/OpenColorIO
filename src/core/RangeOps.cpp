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

#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "RangeOps.h"
#include "MathUtils.h"
#include "BitDepthUtils.h"

#include "cpu/CPURangeOp.h"

#include <cstring>
#include <sstream>


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class RangeOp : public Op
        {
        public:
            RangeOp();

            RangeOp(OpData::OpDataRangeRcPtr & range, TransformDirection direction);

            RangeOp(double minInValue, double maxInValue,
                    double minOutValue, double maxOutValue,
                    TransformDirection direction);

            virtual ~RangeOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual BitDepth getInputBitDepth() const;
            virtual BitDepth getOutputBitDepth() const;
            virtual void setInputBitDepth(BitDepth bitdepth);
            virtual void setOutputBitDepth(BitDepth bitdepth);

            virtual bool isNoOp() const;
            virtual bool isIdentity() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;

            virtual bool canCombineWith(const OpRcPtr & op) const;
            virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;
            
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float * rgbaBuffer, long numPixels) const;
            
            virtual bool supportedByLegacyShader() const { return false; }
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

            // The Range Data
            OpData::OpDataRangeRcPtr m_data;

        private:            
            // The range direction
            TransformDirection m_direction;
            // The computed cache identifier
            std::string m_cacheID;
            // The CPU processor
            CPUOpRcPtr m_cpu;
        };


        typedef OCIO_SHARED_PTR<RangeOp> RangeOpRcPtr;

        RangeOp::RangeOp()
            :   Op()
            ,   m_data(new OpData::Range)
            ,   m_direction(TRANSFORM_DIR_FORWARD)
            ,   m_cpu(new CPUNoOp)
        {
        }

        RangeOp::RangeOp(OpData::OpDataRangeRcPtr & range, TransformDirection direction)
            :   Op()
            ,   m_data(range)
            ,   m_direction(direction)
            ,   m_cpu(new CPUNoOp)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create RangeOp with unspecified transform direction.");
            }
        }

        RangeOp::RangeOp(double minInValue, double maxInValue,
                         double minOutValue, double maxOutValue,
                         TransformDirection direction)
            :   Op()
            ,   m_data(new OpData::Range(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                         minInValue, maxInValue,
                                         minOutValue, maxOutValue))
            ,   m_direction(direction)
            ,   m_cpu(new CPUNoOp)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create RangeOp with unspecified transform direction.");
            }
        }

        OpRcPtr RangeOp::clone() const
        {
            OpData::OpDataRangeRcPtr
                range(dynamic_cast<OpData::Range*>(m_data->clone(OpData::OpData::DO_DEEP_COPY)));
            return OpRcPtr(new RangeOp(range, m_direction));
        }

        RangeOp::~RangeOp()
        {
        }
        
        std::string RangeOp::getInfo() const
        {
            return "<RangeOp>";
        }
        
        std::string RangeOp::getCacheID() const
        {
            return m_cacheID;
        }

        BitDepth RangeOp::getInputBitDepth() const
        {
            return m_data->getInputBitDepth();
        }

        BitDepth RangeOp::getOutputBitDepth() const
        {
            return m_data->getOutputBitDepth();
        }

        void RangeOp::setInputBitDepth(BitDepth bitdepth)
        {
            m_data->setInputBitDepth(bitdepth);
        }

        void RangeOp::setOutputBitDepth(BitDepth bitdepth)
        {
            m_data->setOutputBitDepth(bitdepth);
        }

        bool RangeOp::isNoOp() const
        {
            return m_data->isNoOp();
        }

        bool RangeOp::isIdentity() const
        {
            return m_data->isIdentity();
        }

        bool RangeOp::isSameType(const OpRcPtr & op) const
        {
            const RangeOpRcPtr typedRcPtr = DynamicPtrCast<RangeOp>(op);
            return (bool)typedRcPtr;
        }
        
        bool RangeOp::isInverse(const OpRcPtr & op) const
        {
            RangeOpRcPtr typedRcPtr = DynamicPtrCast<RangeOp>(op);
            if(!typedRcPtr) return false;

            if(GetInverseTransformDirection(m_direction)==typedRcPtr->m_direction
                && *m_data==*(typedRcPtr->m_data))
            {
                return true;
            }

            return m_data->isInverse(typedRcPtr->m_data);
        }
        
        bool RangeOp::canCombineWith(const OpRcPtr & /*op*/) const
        {
            // TODO: Implement RangeOp::canCombineWith()
            return false;
        }
        
        void RangeOp::combineWith(OpRcPtrVec & /*ops*/, const OpRcPtr & secondOp) const
        {
            if(!canCombineWith(secondOp))
            {
                throw Exception("TODO: Range can't be combined.");
            }

            // TODO: Implement RangeOp::combineWith()
        }

        bool RangeOp::hasChannelCrosstalk() const
        {
            return false;
        }
        
        void RangeOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                OpData::OpDataVec v;
                m_data->inverse(v);
                m_data.reset(dynamic_cast<OpData::Range*>(v.remove(0)));
                m_direction = TRANSFORM_DIR_FORWARD;
            }

            // In this initial implementation, only 32f processing is
            // natively supported.
            m_data->setInputBitDepth(BIT_DEPTH_F32);
            m_data->setOutputBitDepth(BIT_DEPTH_F32);

            m_data->validate();

            m_cpu = CPURangeOp::GetRenderer(m_data);

            // Rebuild the cache identifier
            //
            const double limits[4] 
                = { m_data->getMinInValue(), m_data->getMaxInValue(), 
                    m_data->getMinOutValue(), m_data->getMaxOutValue() };

            // Create the cacheID
            md5_state_t state;
            md5_byte_t digest[16];
            md5_init(&state);
            md5_append(&state, (const md5_byte_t *)&limits[0], (int)(4*sizeof(double)));
            md5_finish(&state, digest);
            
            std::ostringstream cacheIDStream;
            cacheIDStream << "<RangeOp ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << BitDepthToString(m_data->getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(m_data->getOutputBitDepth()) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }

        void RangeOp::apply(float * rgbaBuffer, long numPixels) const
        {
#ifndef NDEBUG
            // Skip validation in release
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                throw Exception("RangeOp direction should have been"
                    " set to forward by finalize");
            }
#endif
            m_cpu->apply(rgbaBuffer, (unsigned int)numPixels);
        }

        void RangeOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            // Skip validation in release
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                throw Exception("RangeOp direction should have been"
                    " set to forward by finalize");
            }

            if(getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception(
                    "Only 32F bit depth is supported for the GPU shader");
            }

            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            ss.newLine() << "";
            ss.newLine() << "// Add a Range processing";
            ss.newLine() << "";

            if(m_data->scales(true))
            {
                const float scale[4] 
                    = { (float)m_data->getScale(), 
                        (float)m_data->getScale(), 
                        (float)m_data->getScale(), 
                        (float)m_data->getAlphaScale() };

                const float offset[4] 
                    = { (float)m_data->getOffset(), 
                        (float)m_data->getOffset(), 
                        (float)m_data->getOffset(), 
                        0.0f };

                ss.newLine() << shaderDesc->getPixelName() << " = "
                             << shaderDesc->getPixelName()
                             << " * "
                             << ss.vec4fConst(scale[0], scale[1], scale[2], scale[3])
                             << " + "
                             << ss.vec4fConst(offset[0], offset[1], offset[2], offset[3])
                             << ";";
            }

            if(m_data->minClips())
            {
                const float lowerBound[4] 
                    = { (float)m_data->getLowBound(), 
                        (float)m_data->getLowBound(), 
                        (float)m_data->getLowBound(), 
                        -1.0f * (float)HALF_MAX };

                ss.newLine() << shaderDesc->getPixelName() << " = "
                             << "max(" << shaderDesc->getPixelName() << ", "
                             << ss.vec4fConst(lowerBound[0], lowerBound[1], lowerBound[2], lowerBound[3])
                             << ");";
            }

            if(m_data->maxClips())
            {
                const float upperBound[4] 
                    = { (float)m_data->getHighBound(), 
                        (float)m_data->getHighBound(), 
                        (float)m_data->getHighBound(), 
                        (float)HALF_MAX };

                ss.newLine() << shaderDesc->getPixelName() << " = "
                             << "min(" << shaderDesc->getPixelName() << ", "
                             << ss.vec4fConst(upperBound[0], upperBound[1], upperBound[2], upperBound[3])
                             << ");";
            }

            shaderDesc->addToFunctionShaderCode(ss.string().c_str());
        }

    }  // Anon namespace
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    void CreateRangeOp(OpRcPtrVec & ops, OpData::OpDataRangeRcPtr & range, TransformDirection direction)
    {
        ops.push_back(RangeOpRcPtr(new RangeOp(range, direction)));
    }


    void CreateRangeOp(OpRcPtrVec & ops, 
                       double minInValue, double maxInValue,
                       double minOutValue, double maxOutValue,
                       TransformDirection direction)
    {
        ops.push_back(RangeOpRcPtr(
            new RangeOp(minInValue, maxInValue, minOutValue, maxOutValue, direction)));
    }


}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "MatrixOps.h"

OCIO_NAMESPACE_USING

const float g_error = 1e-7f;


OIIO_ADD_TEST(RangeOps, Accessors)
{
    OCIO::RangeOp range(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_EQUAL(range.m_data->getMinInValue(), 0.);
    OIIO_CHECK_EQUAL(range.m_data->getMaxInValue(), 1.);
    OIIO_CHECK_EQUAL(range.m_data->getMinOutValue(), 0.5);
    OIIO_CHECK_EQUAL(range.m_data->getMaxOutValue(), 1.5);

    OIIO_CHECK_NO_THROW(range.m_data->setMinInValue(-0.05432));
    OIIO_CHECK_NO_THROW(range.finalize());
    OIIO_CHECK_EQUAL(range.m_data->getMinInValue(), -0.05432);

    OIIO_CHECK_NO_THROW(range.m_data->setMaxInValue(1.05432));
    OIIO_CHECK_NO_THROW(range.finalize());
    OIIO_CHECK_EQUAL(range.m_data->getMaxInValue(), 1.05432);

    OIIO_CHECK_NO_THROW(range.m_data->setMinOutValue(0.05432));
    OIIO_CHECK_NO_THROW(range.finalize());
    OIIO_CHECK_EQUAL(range.m_data->getMinOutValue(), 0.05432);

    OIIO_CHECK_NO_THROW(range.m_data->setMaxOutValue(2.05432));
    OIIO_CHECK_NO_THROW(range.finalize());
    OIIO_CHECK_EQUAL(range.m_data->getMaxOutValue(), 2.05432);

    OIIO_CHECK_CLOSE(range.m_data->getScale(), 1.804012123, g_error);
    OIIO_CHECK_CLOSE(range.m_data->getOffset(), 0.1523139385, g_error);
}

OIIO_ADD_TEST(RangeOps, Faulty)
{
    {
        OCIO::RangeOp range(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_NO_THROW(range.finalize());

        OIIO_CHECK_NO_THROW(range.m_data->unsetMinInValue()); 
        OIIO_CHECK_THROW_WHAT(range.finalize(), 
                              OCIO::Exception,
                              "In and out minimum limits must be both set or both missing");
    }

    {
        OCIO::RangeOp range(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_NO_THROW(range.finalize());

        OIIO_CHECK_NO_THROW(range.m_data->unsetMinInValue()); 
        OIIO_CHECK_NO_THROW(range.m_data->unsetMinOutValue()); 
        OIIO_CHECK_NO_THROW(range.finalize());
    }

    {
        OCIO::RangeOp range(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_NO_THROW(range.finalize());

        OIIO_CHECK_NO_THROW(range.m_data->unsetMaxInValue()); 
        OIIO_CHECK_THROW_WHAT(range.finalize(), 
                              OCIO::Exception,
                              "In and out maximum limits must be both set or both missing");
    }

    {
        OCIO::RangeOp range(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_NO_THROW(range.finalize());

        OIIO_CHECK_NO_THROW(range.m_data->unsetMaxInValue()); 
        OIIO_CHECK_NO_THROW(range.m_data->unsetMaxOutValue()); 
        OIIO_CHECK_NO_THROW(range.finalize());
    }

    {
        OCIO::RangeOp range(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_NO_THROW(range.finalize());

        OIIO_CHECK_NO_THROW(range.m_data->setMaxInValue(-125.)); 
        OIIO_CHECK_THROW_WHAT(range.finalize(), 
                              OCIO::Exception,
                              "Range maximum input value is less than minimum input value");
    }

    {
        OCIO::RangeOp range(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_NO_THROW(range.finalize());

        OIIO_CHECK_NO_THROW(range.m_data->setMaxOutValue(-125.)); 
        OIIO_CHECK_THROW_WHAT(range.finalize(), 
                              OCIO::Exception,
                              "Range maximum output value is less than minimum output value");
    }
}

OIIO_ADD_TEST(RangeOps, Identity)
{
    OCIO::RangeOp r;

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.finalize());
    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  -0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  -0.25f, g_error);
    OIIO_CHECK_CLOSE(image[2],   0.50f, g_error);
    OIIO_CHECK_CLOSE(image[3],   0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],   0.75f, g_error);
    OIIO_CHECK_CLOSE(image[5],   1.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],   1.25f, g_error);
    OIIO_CHECK_CLOSE(image[7],   1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],   1.25f, g_error);
    OIIO_CHECK_CLOSE(image[9],   1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[11],  0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, Scale_with_low_and_high_clippings)
{
    OCIO::RangeOp r(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.50f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, Scale_with_low_clipping)
{
    OCIO::RangeOp r(0.0f, OpData::Range::EmptyValue(), 
                    0.5f, OpData::Range::EmptyValue(), 
                    OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[9],  2.00f, g_error);
    OIIO_CHECK_CLOSE(image[10], 2.25f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, Scale_with_high_clipping)
{
    OCIO::RangeOp r(OpData::Range::EmptyValue(), 1.0f, 
                    OpData::Range::EmptyValue(), 1.5f, 
                    OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.25f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.50f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, Scale_with_low_and_high_clippings_2)
{
    OCIO::RangeOp r(0.0f, 1.0f, 0.0f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[2],  0.750f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.125f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.000f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.500f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.000f, g_error);
}

OIIO_ADD_TEST(RangeOps, Apply_Arbitrary)
{
    OCIO::RangeOp r(0.0f, 1.0f, 0.0f, 1.0f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[2],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],  0.75f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.00f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, Combining)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0.0f, 0.5f, 0.5f, 1.0f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OCIO::CreateRangeOp(ops, 0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    // TODO: implement Range combine
    OIIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, ops[1]), OCIO::Exception, "TODO: Range can't be combined");
    OIIO_CHECK_EQUAL(ops.size(), 2);

}

OIIO_ADD_TEST(RangeOps, CombiningInverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OCIO::CreateRangeOp(ops, 0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    // TODO: implement Range combine
    OIIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, ops[1]), OCIO::Exception, "TODO: Range can't be combined");
    OIIO_CHECK_EQUAL(ops.size(), 2);

}

OIIO_ADD_TEST(RangeOps, BasicType)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0.0f, 0.5f, 0.5f, 1.0f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    // Skip finalize so that inverse direction is kept
    OCIO::CreateRangeOp(ops, 0.0f, 0.5f, 0.5f, 1.0f, OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);

    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 3);

    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(!ops[0]->isSameType(ops[2]));

    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[2]));
    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[0]));
    // Inverse based on op direction
    OIIO_CHECK_ASSERT(ops[0]->isInverse(ops[1]));
}

#endif
