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
#include "CDLOps.h"
#include "MathUtils.h"
#include "BitDepthUtils.h"

#include "cpu/CPUCDLOp.h"

#include <cstring>
#include <sstream>


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class CDLOp : public Op
        {
        public:
            CDLOp();

            CDLOp(OpData::OpDataCDLRcPtr & cdl, TransformDirection direction);

            CDLOp(OpData::CDL::CDLStyle style,
                  const double * slope3, 
                  const double * offset3,
                  const double * power3,
                  double saturation,
                  TransformDirection direction);

            virtual ~CDLOp();
            
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
            
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

            // The CDL Data
            OpData::OpDataCDLRcPtr m_data;
        
        private:
            // The range direction
            TransformDirection m_direction;
            // The computed cache identifier
            std::string m_cacheID;
            // The CPU processor
            CPUOpRcPtr m_cpu;
        };


        typedef OCIO_SHARED_PTR<CDLOp> CDLOpRcPtr;

        CDLOp::CDLOp()
            :   Op()
            ,   m_data(new OpData::CDL)
            ,   m_direction(TRANSFORM_DIR_FORWARD)
        {
        }

        CDLOp::CDLOp(OpData::OpDataCDLRcPtr & cdl, TransformDirection direction)
            :   Op()
            ,   m_data(cdl)
            ,   m_direction(direction)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create CDLOp with unspecified transform direction.");
            }
        }

        CDLOp::CDLOp(OpData::CDL::CDLStyle style,
                     const double * slope3, 
                     const double * offset3,
                     const double * power3,
                     double saturation,
                     TransformDirection direction)
            :   Op()
            ,   m_direction(direction)
            ,   m_cpu(new CPUNoOp)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create CDLOp with unspecified transform direction.");
            }

            if (!slope3 || !offset3 || !power3)
            {
                throw Exception("CDLOp: NULL pointer.");
            }

            m_data.reset(
                new OpData::CDL(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                style, 
                                OpData::CDL::ChannelParams(slope3[0], slope3[1], slope3[2]), 
                                OpData::CDL::ChannelParams(offset3[0], offset3[1], offset3[2]), 
                                OpData::CDL::ChannelParams(power3[0], power3[1], power3[2]), 
                                saturation));
        }

        OpRcPtr CDLOp::clone() const
        {
            OpData::OpDataCDLRcPtr
                cdl(dynamic_cast<OpData::CDL*>(m_data->clone(OpData::OpData::DO_DEEP_COPY)));
            return OpRcPtr(new CDLOp(cdl, m_direction));
        }

        CDLOp::~CDLOp()
        {
        }
        
        std::string CDLOp::getInfo() const
        {
            return "<CDLOp>";
        }
        
        std::string CDLOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        BitDepth CDLOp::getInputBitDepth() const
        {
            return m_data->getInputBitDepth();
        }

        BitDepth CDLOp::getOutputBitDepth() const
        {
            return m_data->getOutputBitDepth();
        }

        void CDLOp::setInputBitDepth(BitDepth bitdepth)
        {
            m_data->setInputBitDepth(bitdepth);
        }

        void CDLOp::setOutputBitDepth(BitDepth bitdepth)
        {
            m_data->setOutputBitDepth(bitdepth);
        }

        bool CDLOp::isNoOp() const
        {
            return m_data->isNoOp();
        }

        bool CDLOp::isIdentity() const
        {
            return m_data->isIdentity();
        }

        bool CDLOp::isSameType(const OpRcPtr & op) const
        {
            CDLOpRcPtr typedRcPtr = DynamicPtrCast<CDLOp>(op);
            return (bool)typedRcPtr;
        }
        
        bool CDLOp::isInverse(const OpRcPtr & op) const
        {
            CDLOpRcPtr typedRcPtr = DynamicPtrCast<CDLOp>(op);
            if(!typedRcPtr) return false;

            if(GetInverseTransformDirection(m_direction)==typedRcPtr->m_direction
                && *m_data==*typedRcPtr->m_data)
            {
                return true;
            }

            return m_data->isInverse(typedRcPtr->m_data);
        }
        
        bool CDLOp::canCombineWith(const OpRcPtr & /*op*/) const
        {
            // TODO: Allow combining with LUTs.
            // TODO: Allow combining with matrices.
            return false;
        }
        
        void CDLOp::combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const
        {
            if(!canCombineWith(secondOp))
            {
                std::ostringstream os;
                os << "CDLOp can only be combined with other ";
                os << "CDLOps.  secondOp:" << secondOp->getInfo();
                throw Exception(os.str().c_str());
            }

            const CDLOpRcPtr typedRcPtr = DynamicPtrCast<CDLOp>(secondOp);

            OpData::OpDataCDLRcPtr data = m_data->compose(typedRcPtr->m_data);

            if(data->isNoOp()) return;

            ops.push_back(CDLOpRcPtr(new CDLOp(data, TRANSFORM_DIR_FORWARD)));
        }

        bool CDLOp::hasChannelCrosstalk() const
        {
            return m_data->hasChannelCrosstalk();
        }

        void CDLOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                OpData::OpDataVec v;
                m_data->inverse(v);
                m_data.reset(dynamic_cast<OpData::CDL*>(v.remove(0)));
                m_direction = TRANSFORM_DIR_FORWARD;
            }

            // Only the 32f processing is natively supported
            m_data->setInputBitDepth(BIT_DEPTH_F32);
            m_data->setOutputBitDepth(BIT_DEPTH_F32);

            m_data->validate();

            m_cpu = CPUCDLOp::GetRenderer(m_data);

            // Rebuild the cache identifier
            //
            const double limits[13] 
                = { m_data->getSlopeParams()[0], 
                    m_data->getSlopeParams()[1], 
                    m_data->getSlopeParams()[2], 
                    m_data->getSlopeParams()[3],

                    m_data->getOffsetParams()[0], 
                    m_data->getOffsetParams()[1], 
                    m_data->getOffsetParams()[2], 
                    m_data->getOffsetParams()[3],

                    m_data->getPowerParams()[0], 
                    m_data->getPowerParams()[1], 
                    m_data->getPowerParams()[2], 
                    m_data->getPowerParams()[3],

                    m_data->getSaturation() 
                };

            // Create the cacheID
            md5_state_t state;
            md5_byte_t digest[13*sizeof(double)];
            md5_init(&state);
            md5_append(&state, (const md5_byte_t *)&limits[0], (int)(13*sizeof(double)));
            md5_finish(&state, digest);

            std::ostringstream cacheIDStream;
            cacheIDStream << "<CDLOp ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << BitDepthToString(m_data->getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(m_data->getOutputBitDepth()) << " ";
            cacheIDStream << OpData::CDL::GetCDLStyleName(m_data->getCDLStyle()) << " ";
            cacheIDStream << ">";

            m_cacheID = cacheIDStream.str();
        }

        void CDLOp::apply(float * rgbaBuffer, long numPixels) const
        {
            m_cpu->apply(rgbaBuffer, (unsigned int)numPixels);
        }

        void CDLOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
#ifndef NDEBUG
            if(getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception(
                    "Only 32F bit depth is supported for the GPU shader");
            }
#endif

            CDLOpUtil::RenderParams params;
            params.update(m_data);

            const float * slope    = params.getSlope();
            const float * offset   = params.getOffset();
            const float * power    = params.getPower();
            const float saturation = params.getSaturation();

            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            ss.newLine() << "";
            ss.newLine() << "// Add a CDL processing";
            ss.newLine() << "";

            ss.newLine() << "{";
            ss.indent();

            // Since alpha is not affected, only need to use the RGB components
            ss.declareVec3f("lumaWeights", 0.2126f,   0.7152f,   0.0722f  );
            ss.declareVec3f("slope",       slope [0], slope [1], slope [2]);
            ss.declareVec3f("offset",      offset[0], offset[1], offset[2]);
            ss.declareVec3f("power",       power [0], power [1], power [2]);

            ss.declareVar("saturation" , saturation);

            ss.newLine() << ss.vec3fDecl("pix") << " = " 
                         << shaderDesc->getPixelName() << ".xyz;";

            if ( !params.isReverse() )
            {
                // Forward style

                // Slope
                ss.newLine() << "pix = pix * slope;";

                // Offset
                ss.newLine() << "pix = pix + offset;";

                // Power
                if ( !params.isNoClamp() )
                {
                    ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
                    ss.newLine() << "pix = pow(pix, power);";
                }
                else
                {
                    ss.newLine() << ss.vec3fDecl("posPix") << " = step(0.0, pix);";
                    ss.newLine() << ss.vec3fDecl("pixPower") << " = pow(abs(pix), power);";
                    ss.newLine() << "pix = " << ss.lerp("pix", "pixPower", "posPix") << ";";
                }

                // Saturation
                ss.newLine() << "float luma = dot(pix, lumaWeights);";
                ss.newLine() << "pix = luma + saturation * (pix - luma);";

                // Post-saturation clamp
                if ( !params.isNoClamp() )
                {
                    ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
                }
            }
            else
            {
                // Reverse style

                // Pre-saturation clamp
                if ( !params.isNoClamp() )
                {
                    ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
                }

                // Saturation
                ss.newLine() << "float luma = dot(pix, lumaWeights);";
                ss.newLine() << "pix = luma + saturation * (pix - luma);";

                // Power
                if ( !params.isNoClamp() )
                {
                    ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
                    ss.newLine() << "pix = pow(pix, power);";
                }
                else
                {
                    ss.newLine() << ss.vec3fDecl("posPix") << " = step(0.0, pix);";
                    ss.newLine() << ss.vec3fDecl("pixPower") << " = pow(abs(pix), power);";
                    ss.newLine() << "pix = " << ss.lerp("pix", "pixPower", "posPix") << ";";
                }

                // Offset
                ss.newLine() << "pix = pix + offset;";

                // Slope
                ss.newLine() << "pix = pix * slope;";

                // Post-slope clamp
                if ( !params.isNoClamp() )
                {
                    ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
                }
            }

            ss.newLine() << shaderDesc->getPixelName() << ".xyz = pix;";

            ss.dedent();
            ss.newLine() << "}";

            shaderDesc->addToFunctionShaderCode(ss.string().c_str());
        }

    }  // Anon namespace
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    void CreateCDLOp(OpRcPtrVec & ops,
                     OpData::OpDataCDLRcPtr & cdl, 
                     TransformDirection direction)
    {
        ops.push_back(CDLOpRcPtr(new CDLOp(cdl, direction)));
    }


    void CreateCDLOp(OpRcPtrVec & ops,
                     OpData::CDL::CDLStyle style,
                     const double * slope3, 
                     const double * offset3,
                     const double * power3,
                     double saturation,
                     TransformDirection direction)
    {
        ops.push_back(
            CDLOpRcPtr(new CDLOp(style, slope3, offset3, power3, saturation, direction)));
    }

}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_NAMESPACE_USING


void ApplyCDL(float * in, const float * ref, unsigned numPixels,
              const double * slope, const double * offset,
              const double * power, double saturation,
              OCIO::OpData::CDL::CDLStyle style,
              float errorThreshold)
{
    OCIO::CDLOp cdlOp(style, slope, offset, power, saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_NO_THROW(cdlOp.finalize());

    cdlOp.apply(in, numPixels);

    for(unsigned idx=0; idx<(numPixels*4); ++idx)
    {
        OIIO_CHECK_CLOSE(in[idx], ref[idx], errorThreshold);
    }

}


// TODO: CDL Unit tests with bit depth are missing.


namespace CDL_DATA_1
{
    const double slope[3]  = { 1.35,  1.1,  0.071 };
    const double offset[3] = { 0.05, -0.23, 0.11  };
    const double power[3]  = { 0.93,  0.81, 1.27  };
    const double saturation = 1.23;
};

OIIO_ADD_TEST(CDLOps, Basic)
{
    OpRcPtrVec ops;

    CreateCDLOp(ops, OCIO::OpData::CDL::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);

    CreateCDLOp(ops, OCIO::OpData::CDL::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation, 
                OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_ASSERT(ops[0]->isInverse(ops[1]));
    OIIO_CHECK_ASSERT(ops[1]->isInverse(ops[0]));

    CreateCDLOp(ops, OCIO::OpData::CDL::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset, 
                CDL_DATA_1::power, 1.30, 
                OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 3);

    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[2]));
    OIIO_CHECK_ASSERT(!ops[1]->isInverse(ops[2]));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(ops[0]));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(ops[1]));
}

OIIO_ADD_TEST(CDLOps, Apply_clamp_fwd)
{
    float input_32f[] = {
         0.3278f, 0.01f, 1.0f,  0.0f,
         0.25f,   0.5f,  0.75f, 1.0f,
         1.25f,   1.5f,  1.75f, 0.75f,
        -0.2f,    0.5f,  1.4f,  0.0f,
        -0.25f,  -0.5f, -0.75f, 0.25f,
         0.0f,    0.8f,  0.99f, 0.5f };

    const float expected_32f[] = {
        0.609399f, 0.000000f, 0.113130f,   0.000000f,
        0.422056f, 0.401466f, 0.035820f,   1.000000f,
        1.000000f, 1.000000f, 0.000000f,   0.750000f,
        0.000000f, 0.421098f, 0.101226f,   0.000000f,
        0.000000f, 0.000000f, 0.031735f,   0.250000f,
        0.000000f, 0.746748f, 0.01869086f, 0.5000f  };

    ApplyCDL(input_32f, expected_32f, 6,
             CDL_DATA_1::slope, CDL_DATA_1::offset, 
             CDL_DATA_1::power, CDL_DATA_1::saturation, 
             OCIO::OpData::CDL::CDL_V1_2_FWD,
// TODO: Investigate that error threshold
             1e-5f);
}

OIIO_ADD_TEST(CDLOps, Apply_clamp_rev)
{
    float input_32f[] = {
       0.609399f,  0.100000f,  0.113130f, 0.000000f,
       0.001000f,  0.746748f,  0.018691f, 0.500000f,
       0.422056f,  0.401466f,  0.035820f, 1.000000f,
      -0.25f,     -0.5f,      -0.75f,     0.25f,
       1.25f,      1.5f,       1.75f,     0.75f,
      -0.2f,       0.5f,       1.4f,      0.0f };

    const float expected_32f[] = {
      0.340707f, 0.275725f, 1.000000f, 0.000000f,
      0.025902f, 0.801892f, 1.000000f, 0.500000f,
// TODO: changed 0.75 to 0.750010 to keep 1e-6f as error threshold
      0.250000f, 0.500000f, 0.750010f, 1.000000f,
      0.000000f, 0.209091f, 0.000000f, 0.250000f,
      0.703713f, 1.000000f, 1.000000f, 0.750000f,
      0.012206f, 0.582940f, 1.000000f, 0.000000f };

    ApplyCDL(input_32f, expected_32f, 6,
             CDL_DATA_1::slope, CDL_DATA_1::offset, 
             CDL_DATA_1::power, CDL_DATA_1::saturation, 
             OCIO::OpData::CDL::CDL_V1_2_REV,
             1e-6f);
}

OIIO_ADD_TEST(CDLOps, Apply_noclamp_fwd)
{
    float input_32f[] = {
       0.3278f, 0.01f, 1.0f,  0.0f,
       0.0f,    0.8f,  0.99f, 0.5f,
       0.25f,   0.5f,  0.75f, 1.0f,
      -0.25f,  -0.5f, -0.75f, 0.25f,
       1.25f,   1.5f,  1.75f, 0.75f,
      -0.2f,    0.5f,  1.4f,  0.0f };

    const float expected_32f[] = {
       0.645421f, -0.260548f,  0.149153f, 0.000000f,
      -0.045094f,  0.746751f,  0.018689f, 0.500000f,
       0.422053f,  0.401468f,  0.035820f, 1.000000f,
      -0.211694f, -0.817469f,  0.174100f, 0.250000f,
       1.753178f,  1.331124f, -0.108181f, 0.750000f,
      -0.327485f,  0.431855f,  0.111984f, 0.000000f };

    ApplyCDL(input_32f, expected_32f, 6,
             CDL_DATA_1::slope, CDL_DATA_1::offset, 
             CDL_DATA_1::power, CDL_DATA_1::saturation, 
             OCIO::OpData::CDL::CDL_NO_CLAMP_FWD,
             1e-6f);
}

OIIO_ADD_TEST(CDLOps, Apply_noclamp_rev)
{
    float input_32f[] = {
       0.609399f,  0.100000f,  0.113130f, 0.000000f,
       0.001000f,  0.746748f,  0.018691f, 0.500000f,
       0.422056f,  0.401466f,  0.035820f, 1.000000f,
      -0.25f,     -0.5f,      -0.75f,     0.25f,
       1.25f,      1.5f,       1.75f,     0.75f,
      -0.2f,       0.5f,       1.4f,      0.0f };

    const float expected_32f[] = {
       0.340707f,  0.275725f,  1.294803f,  0.000000f,
       0.025902f,  0.801892f,  1.022215f,  0.500000f,
// TODO: changed 0.75 to 0.750010 to keep 1e-6f as error threshold
       0.250000f,  0.500000f,  0.750010f,  1.000000f,
      -0.251989f, -0.239488f, -11.361813f, 0.250000f,
       0.937168f,  1.700675f,  19.807320f, 0.750000f,
      -0.099839f,  0.580523f,  14.880430f, 0.000000f };

    ApplyCDL(input_32f, expected_32f, 6,
             CDL_DATA_1::slope, CDL_DATA_1::offset, 
             CDL_DATA_1::power, CDL_DATA_1::saturation, 
             OCIO::OpData::CDL::CDL_NO_CLAMP_REV,
             1e-6f);
}

namespace CDL_DATA_2
{
    const double slope[3]  = { 1.15, 1.10, 0.9  };
    const double offset[3] = { 0.05, 0.02, 0.07 };
    const double power[3]  = { 1.2,  0.95, 1.13 };
    const double saturation = 0.87;
};

OIIO_ADD_TEST(CDLOps, Apply_clamp_fwd_2)
{
    float input_32f[] = {
      0.65f, 0.55f, 0.20f, 0.0f,
      0.41f, 0.81f, 0.39f, 0.5f,
      0.25f, 0.50f, 0.75f, 1.0f };

    const float expected_32f[] = {
      0.745641f, 0.639203f, 0.264151f, 0.000000f,
      0.499592f, 0.897559f, 0.428593f, 0.500000f,
      0.305034f, 0.578780f, 0.692552f, 1.000000f };

    ApplyCDL(input_32f, expected_32f, 3,
             CDL_DATA_2::slope, CDL_DATA_2::offset, 
             CDL_DATA_2::power, CDL_DATA_2::saturation, 
             OCIO::OpData::CDL::CDL_V1_2_FWD,
             1e-6f);
}

namespace CDL_DATA_3
{
    const double slope[3]  = {  3.405,  1.0,    1.0   };
    const double offset[3] = { -0.178, -0.178, -0.178 };
    const double power[3]  = {  1.095,  1.095,  1.095 };
    const double saturation = 0.99;
};

OIIO_ADD_TEST(CDLOps, Apply_clamp_fwd_3)
{
    float input_32f[] = {
      0.02f, 0.0f, 0.0f, 0.0f,
      0.17f, 0.0f, 0.0f, 0.0f,
      0.65f, 0.0f, 0.0f, 0.0f,
      0.97f, 0.0f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.0f, 0.0f,
      0.17f, 0.13f, 0.0f, 0.0f,
      0.65f, 0.13f, 0.0f, 0.0f,
      0.97f, 0.13f, 0.0f, 0.0f,

      0.02f, 0.23f, 0.0f, 0.0f,
      0.17f, 0.23f, 0.0f, 0.0f,
      0.65f, 0.23f, 0.0f, 0.0f,
      0.97f, 0.23f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.23f, 0.0f,
      0.17f, 0.13f, 0.23f, 0.0f,
      0.65f, 0.13f, 0.23f, 0.0f,
      0.97f, 0.13f, 0.23f, 0.0f };

    const float expected_32f[] = {
      0.000000f, 0.000000f, 0.000000f, 0.000000f,
      0.364613f, 0.00078132f, 0.00078132f, 0.00f,
      0.992126f, 0.002126f, 0.002126f, 0.000000f,
      0.992126f, 0.002126f, 0.002126f, 0.000000f,

      0.000000f, 0.000000f, 0.000000f, 0.000000f,
      0.364613f, 0.00078132f, 0.00078132f, 0.00f,
      0.992126f, 0.002126f, 0.002126f, 0.000000f,
      0.992126f, 0.002126f, 0.002126f, 0.000000f,

      0.00028083f, 0.039155f, 0.00028083f, 0.00f,
      0.364894f, 0.039936f, 0.00106215f, 0.0000f,
      0.992407f, 0.041281f, 0.00240683f, 0.0000f,
      0.992407f, 0.041281f, 0.00240683f, 0.000000f,

      0.0000283505f, 0.0000283505f, 0.0389023f, 0.f,
      0.364641f, 0.00080967f, 0.039684f, 0.0000f,
      0.992154f, 0.00215435f, 0.041028f, 0.0000f,
      0.992154f, 0.00215435f, 0.041028f, 0.0000f };

    ApplyCDL(input_32f, expected_32f, 16,
             CDL_DATA_3::slope, CDL_DATA_3::offset, 
             CDL_DATA_3::power, CDL_DATA_3::saturation, 
             OCIO::OpData::CDL::CDL_V1_2_FWD,
// TODO: Investigate that error threshold
             2e-5f);
}

OIIO_ADD_TEST(CDLOps, Apply_noclamp_fwd_2)
{
    float input_32f[] = {
      0.02f, 0.0f, 0.0f, 0.0f,
      0.17f, 0.0f, 0.0f, 0.0f,
      0.65f, 0.0f, 0.0f, 0.0f,
      0.97f, 0.0f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.0f, 0.0f,
      0.17f, 0.13f, 0.0f, 0.0f,
      0.65f, 0.13f, 0.0f, 0.0f,
      0.97f, 0.13f, 0.0f, 0.0f,

      0.02f, 0.23f, 0.0f, 0.0f,
      0.17f, 0.23f, 0.0f, 0.0f,
      0.65f, 0.23f, 0.0f, 0.0f,
      0.97f, 0.23f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.23f, 0.0f,
      0.17f, 0.13f, 0.23f, 0.0f,
      0.65f, 0.13f, 0.23f, 0.0f,
      0.97f, 0.13f, 0.23f, 0.0f };

    const float expected_32f[] = {
      -0.110436f, -0.177855f, -0.177855f, 0.000000f,
       0.363210f, -0.176840f, -0.176840f, 0.000000f,
       2.158838f, -0.172992f, -0.172992f, 0.000000f,
       3.453242f, -0.170219f, -0.170219f, 0.000000f,

      -0.109506f, -0.048225f, -0.176925f, 0.000000f,
       0.364140f, -0.047210f, -0.175911f, 0.000000f,
       2.159768f, -0.043363f, -0.172063f, 0.000000f,
       3.454171f, -0.040589f, -0.169289f, 0.000000f,

      -0.108882f, 0.038793f, -0.176301f, 0.000000f,
       0.364764f, 0.039808f, -0.175286f, 0.000000f,
       2.160392f, 0.043656f, -0.171439f, 0.000000f,
       3.454796f, 0.046430f, -0.168665f, 0.000000f,

      -0.109350f, -0.048069f, 0.038326f, 0.000000f,
       0.364297f, -0.047054f, 0.039341f, 0.000000f,
       2.159925f, -0.043206f, 0.043188f, 0.000000f,
       3.454328f, -0.040432f, 0.045962f, 0.000000f };

    ApplyCDL(input_32f, expected_32f, 16,
             CDL_DATA_3::slope, CDL_DATA_3::offset, 
             CDL_DATA_3::power, CDL_DATA_3::saturation, 
             OCIO::OpData::CDL::CDL_NO_CLAMP_FWD,
             1e-6f);
}

#endif
