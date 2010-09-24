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
#include "LogOps.h"
#include "MathUtils.h"

#include <cmath>

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        // TODO: make these user paramaters for each channel (rgb)
        const float LOG_LAD_REF = 455.f;
        const float LIN_LAD_REF = 0.18f;
        const float NEG_GAMMA   = 0.6f;
        
        inline float
        JPLog2Lin (float x)
        {
            return LIN_LAD_REF * std::pow(10.f, (1023.f * x - LOG_LAD_REF) * 0.002f / NEG_GAMMA);
        }
        
        inline float
        JPLin2Log (float x)
        {
            if (x < 1e-10) x = 1e-10;
            return (LOG_LAD_REF + std::log10(x / LIN_LAD_REF) * NEG_GAMMA / 0.002f) / 1023.f;
        }
        
        // Forward
        void
        ApplyJPLog2Lin_NoAlpha(float* rgbaBuffer, long numPixels) 
        {
            for(long pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = JPLog2Lin(rgbaBuffer[0]);
                rgbaBuffer[1] = JPLog2Lin(rgbaBuffer[1]);
                rgbaBuffer[2] = JPLog2Lin(rgbaBuffer[2]);
                rgbaBuffer += 4;
            }
        }
        
        // Inverse
        void
        ApplyJPLin2Log_NoAlpha(float* rgbaBuffer, long numPixels) 
        {
            for(long pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = JPLin2Log(rgbaBuffer[0]);
                rgbaBuffer[1] = JPLin2Log(rgbaBuffer[1]);
                rgbaBuffer[2] = JPLin2Log(rgbaBuffer[2]);
                rgbaBuffer += 4;
            }
        }
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    namespace
    {
        class JPLogOp : public Op
        {
            
        public:
            
            JPLogOp(TransformDirection direction);
            
            virtual
            ~JPLogOp();
            
            virtual OpRcPtr
            clone() const;
            
            virtual std::string
            getInfo() const;
            
            virtual std::string
            getCacheID() const;
            
            virtual bool
            isNoOp() const;
            
            virtual void
            finalize();
            
            virtual void
            apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool
            supportsGpuShader() const;
            
            virtual void
            writeGpuShader(std::ostringstream & shader,
                           const std::string & pixelName,
                           const GpuShaderDesc & shaderDesc) const;
            
            virtual bool
            definesGpuAllocation() const;
            
            virtual GpuAllocationData
            getGpuAllocation() const;
            
        private:
            
            TransformDirection m_direction;
            std::string m_cacheID;
            
        };
        
        JPLogOp::JPLogOp(TransformDirection direction): Op(), m_direction(direction)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply JPLogOp op, unspecified transform direction.");
            }
        }
        
        OpRcPtr
        JPLogOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new JPLogOp(m_direction));
            return op;
        }
        
        JPLogOp::~JPLogOp()
        { }
        
        std::string
        JPLogOp::getInfo() const
        {
            return "<JPLogOp>";
        }
        
        std::string
        JPLogOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        bool
        JPLogOp::isNoOp() const
        {
            return false;
        }
        
        void
        JPLogOp::finalize()
        {
            std::ostringstream cacheIDStream;
            cacheIDStream << "<JPLogOp ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void
        JPLogOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                ApplyJPLog2Lin_NoAlpha(rgbaBuffer, numPixels);
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                ApplyJPLin2Log_NoAlpha(rgbaBuffer, numPixels);
            }
        }
        
        bool
        JPLogOp::supportsGpuShader() const
        {
            return false;
        }
        
        void
        JPLogOp::writeGpuShader(std::ostringstream &  /* shader     */,
                                const std::string &   /* pixelName  */,
                                const GpuShaderDesc & /* shaderDesc */) const
        {
            throw Exception("JPLogOp does not support analytical shader generation.");
        }
        
        bool
        JPLogOp::definesGpuAllocation() const
        {
            return false;
        }
        
        GpuAllocationData
        JPLogOp::getGpuAllocation() const
        {
            throw Exception("JPLogOp does not define a Gpu Allocation.");
        }
        
    } // namespace
    
    void
    CreateJPLogOp(OpRcPtrVec & ops, TransformDirection direction)
    {
        ops.push_back( OpRcPtr(new JPLogOp(direction)) );
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include <boost/test/unit_test.hpp>

#include <iostream>

// check if two float values match
inline bool
is_equal (float x, float y)
{
    return std::abs(x - y) <= 1e-05 * std::abs(x); // 1e-05
}

// luminance ramp from -0.1 -> 14.0 used in tests
static float
LUMINANCE_Linear[142] = {
-0.100000,  0.000000,  0.100000,  0.200000,  0.300000,  0.400000,  0.500000, 
 0.600000,  0.700000,  0.800000,  0.900000,  1.000000,  1.100000,  1.200000, 
 1.300000,  1.400000,  1.500000,  1.600000,  1.700000,  1.800000,  1.900000, 
 2.000000,  2.100000,  2.200000,  2.300000,  2.400000,  2.500000,  2.600000, 
 2.700000,  2.800000,  2.900000,  3.000000,  3.100000,  3.200000,  3.300000, 
 3.400000,  3.500000,  3.600000,  3.700000,  3.800000,  3.900000,  4.000000, 
 4.100000,  4.200000,  4.300000,  4.400000,  4.500000,  4.600000,  4.700000, 
 4.800000,  4.900000,  5.000000,  5.100000,  5.200000,  5.300000,  5.400000, 
 5.500000,  5.600000,  5.700000,  5.800000,  5.900000,  6.000000,  6.100000, 
 6.200000,  6.300000,  6.400000,  6.500000,  6.600000,  6.700000,  6.800000, 
 6.900000,  7.000000,  7.100000,  7.200000,  7.300000,  7.400000,  7.500000, 
 7.600000,  7.700000,  7.800000,  7.900000,  8.000000,  8.100000,  8.200000, 
 8.300000,  8.400000,  8.500000,  8.600000,  8.700000,  8.800000,  8.900000, 
 9.000000,  9.100000,  9.200000,  9.300000,  9.400000,  9.500000,  9.600000, 
 9.700000,  9.800000,  9.900000, 10.000000, 10.100000, 10.200000, 10.300000, 
10.400000, 10.500000, 10.600000, 10.700000, 10.800000, 10.900000, 11.000000, 
11.100000, 11.200000, 11.300000, 11.400000, 11.500000, 11.600000, 11.700000, 
11.800000, 11.900000, 12.000000, 12.100000, 12.200000, 12.300000, 12.400000, 
12.500000, 12.600000, 12.700000, 12.800000, 12.900000, 13.000000, 13.100000, 
13.200000, 13.200000, 13.400000, 13.500000, 13.600000, 13.700000, 13.800000, 
13.900000, 14.000000
};

// JPLog luminance ramp
static float
LUMINANCE_JPLog[142] = {
-2.269386, -2.269386,  0.369910,  0.458189,  0.509829,  0.546467,  0.574887, 
 0.598107,  0.617740,  0.634746,  0.649747,  0.663165,  0.675304,  0.686386, 
 0.696580,  0.706018,  0.714805,  0.723025,  0.730746,  0.738025,  0.744911, 
 0.751444,  0.757658,  0.763583,  0.769244,  0.774664,  0.779863,  0.784859, 
 0.789665,  0.794297,  0.798766,  0.803084,  0.807260,  0.811303,  0.815222, 
 0.819024,  0.822716,  0.826304,  0.829794,  0.833190,  0.836498,  0.839723, 
 0.842867,  0.845936,  0.848933,  0.851861,  0.854723,  0.857523,  0.860262, 
 0.862943,  0.865569,  0.868142,  0.870664,  0.873137,  0.875563,  0.877944, 
 0.880281,  0.882575,  0.884830,  0.887045,  0.889222,  0.891362,  0.893467, 
 0.895538,  0.897576,  0.899582,  0.901556,  0.903501,  0.905416,  0.907303, 
 0.909162,  0.910995,  0.912801,  0.914583,  0.916339,  0.918072,  0.919782, 
 0.921469,  0.923133,  0.924777,  0.926399,  0.928001,  0.929583,  0.931146, 
 0.932690,  0.934215,  0.935722,  0.937212,  0.938684,  0.940140,  0.941579, 
 0.943002,  0.944409,  0.945801,  0.947178,  0.948540,  0.949888,  0.951222, 
 0.952541,  0.953848,  0.955141,  0.956421,  0.957688,  0.958943,  0.960185, 
 0.961416,  0.962634,  0.963842,  0.965038,  0.966222,  0.967396,  0.968559, 
 0.969712,  0.970854,  0.971986,  0.973108,  0.974221,  0.975323,  0.976416, 
 0.977500,  0.978575,  0.979641,  0.980698,  0.981746,  0.982786,  0.983817, 
 0.984840,  0.985855,  0.986862,  0.987860,  0.988852,  0.989835,  0.990811, 
 0.991780,  0.991780,  0.993695,  0.994642,  0.995582,  0.996515,  0.997441, 
 0.998360,  0.999273,
};

static int STEPS = sizeof(LUMINANCE_Linear) / sizeof(float);

BOOST_AUTO_TEST_SUITE( JPLogOp_Unit_Tests )

BOOST_AUTO_TEST_CASE ( test_jplog_simple )
{
    
    OCIO::JPLogOp simple_fwd(OCIO::TRANSFORM_DIR_FORWARD);
    // NOTE: skip the first entries
    for(int y = 2; y < STEPS; y++) {
        float pixel[4] = {LUMINANCE_JPLog[y],
                          LUMINANCE_JPLog[y],
                          LUMINANCE_JPLog[y], 1.f};
        simple_fwd.apply(pixel, 1);
        // DEBUG
        //std::cout << "p: " << pixel[0] << "," << pixel[1] << "," << pixel[2] << "," << pixel[3] << "\n";
        for(int c = 0; c < 3; c++)
            BOOST_CHECK_EQUAL (is_equal(pixel[c], LUMINANCE_Linear[y]), true);
        BOOST_CHECK_EQUAL (is_equal(pixel[3], 1.f), true);
    }
    
    OCIO::JPLogOp simple_inv(OCIO::TRANSFORM_DIR_INVERSE);
    for(int y = 0; y < STEPS; y++) {
        float pixel[4] = {LUMINANCE_Linear[y],
                          LUMINANCE_Linear[y],
                          LUMINANCE_Linear[y], 1.f};
        simple_inv.apply(pixel, 1);
        // DEBUG
        //std::cout << "p: " << pixel[0] << "," << pixel[1] << "," << pixel[2] << "," << pixel[3] << "\n";
        for(int c = 0; c < 3; c++)
            BOOST_CHECK_EQUAL (is_equal(pixel[c], LUMINANCE_JPLog[y]), true);
        BOOST_CHECK_EQUAL (is_equal(pixel[3], 1.f), true);
    }
    
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_BUILD_TESTS
