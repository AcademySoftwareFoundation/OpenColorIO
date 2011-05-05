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

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "MatrixOps.h"
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
            ExponentOp(const float * exp4,
                       TransformDirection direction);
            virtual ~ExponentOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual bool isNoOp() const;
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
            
            virtual bool definesAllocation() const;
            virtual AllocationData getAllocation() const;
        
        private:
            float m_exp4[4];
            float m_finalExp4[4];
            TransformDirection m_direction;
            std::string m_cacheID;
        };
        
        ExponentOp::ExponentOp(const float * exp4,
                               TransformDirection direction):
                               Op(),
                               m_direction(direction)
        {
            memcpy(m_exp4, exp4, 4*sizeof(float));
            memset(m_finalExp4, 0, 4*sizeof(float));
        }
        
        OpRcPtr ExponentOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new ExponentOp(m_exp4, m_direction));
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
            return IsVecEqualToOne(m_exp4, 4);
        }
        
        bool ExponentOp::hasChannelCrosstalk() const
        {
            return false;
        }
        
        void ExponentOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply ExponentOp op, unspecified transform direction.");
            }
            
            if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                for(int i=0; i<4; ++i)
                {
                    if(!IsScalarEqualToZero(m_exp4[i]))
                    {
                        m_finalExp4[i] = 1.0f / m_exp4[i];
                    }
                    else
                    {
                        throw Exception("Cannot apply ExponentOp op, Cannot apply 0.0 exponent in the inverse.");
                    }
                }
            }
            else
            {
                memcpy(m_finalExp4, m_exp4, 4*sizeof(float));
            }
            
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<ExponentOp ";
            cacheIDStream.precision(FLOAT_DECIMALS);
            for(int i=0; i<4; ++i)
            {
                cacheIDStream << m_finalExp4[i] << " ";
            }
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void ExponentOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(!rgbaBuffer) return;
            
            ApplyClampExponent(rgbaBuffer, numPixels, m_finalExp4);
        }
        
        bool ExponentOp::supportsGpuShader() const
        {
            return true;
        }
        
        void ExponentOp::writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const
        {
            GpuLanguage lang = shaderDesc.getLanguage();
            float zerovec[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            
            shader << pixelName << " = pow(";
            shader << "max(" << pixelName << ", " << GpuTextHalf4(zerovec, lang) << ")";
            shader << ", " << GpuTextHalf4(m_finalExp4, lang) << ");\n";
        }
        
        bool ExponentOp::definesAllocation() const
        {
            return false;
        }
        
        AllocationData ExponentOp::getAllocation() const
        {
            throw Exception("ExponentOp does not define an allocation.");
        }
        
    }  // Anon namespace
    
    
    
    void CreateExponentOp(OpRcPtrVec & ops,
                          const float * exp4,
                          TransformDirection direction)
    {
        bool expIsIdentity = IsVecEqualToOne(exp4, 4);
        if(expIsIdentity) return;
        
        ops.push_back( OpRcPtr(new ExponentOp(exp4, direction)) );
    }
}
OCIO_NAMESPACE_EXIT
