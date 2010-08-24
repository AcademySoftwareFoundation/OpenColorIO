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
        const float LOG2INV = 1.0f / logf(2.0f);
        const float FLTMIN = std::numeric_limits<float>::min();
        inline float log2(float f) { return logf(std::max(f, FLTMIN)) * LOG2INV; }
        
        void ApplyLog2NoAlpha_Forward(float* rgbaBuffer, long numPixels)
        {
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = log2(rgbaBuffer[0]);
                rgbaBuffer[1] = log2(rgbaBuffer[1]);
                rgbaBuffer[2] = log2(rgbaBuffer[2]);
                
                rgbaBuffer += 4;
            }
        }
        
        void ApplyLog2NoAlpha_Inverse(float* rgbaBuffer, long numPixels)
        {
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = powf(2.0f, rgbaBuffer[0]);
                rgbaBuffer[1] = powf(2.0f, rgbaBuffer[1]);
                rgbaBuffer[2] = powf(2.0f, rgbaBuffer[2]);
                
                rgbaBuffer += 4;
            }
        }
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    
    
    
    
    namespace
    {
        class Log2Op : public Op
        {
        public:
            Log2Op(TransformDirection direction);
            virtual ~Log2Op();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostringstream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
            
            virtual bool definesGpuAllocation() const;
            virtual GpuAllocationData getGpuAllocation() const;
        
        private:
            TransformDirection m_direction;
            
            std::string m_cacheID;
        };
        
        // TODO: Optimize the sub-cases. (dispatch to different calls based on
        // matrix analysis
        
        Log2Op::Log2Op(TransformDirection direction):
                                       Op(),
                                       m_direction(direction)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply Log2Op op, unspecified transform direction.");
            }
        }
        
        OpRcPtr Log2Op::clone() const
        {
            OpRcPtr op = OpRcPtr(new Log2Op(m_direction));
            return op;
        }
        
        Log2Op::~Log2Op()
        { }
        
        std::string Log2Op::getInfo() const
        {
            return "<Log2Op>";
        }
        
        std::string Log2Op::getCacheID() const
        {
            return m_cacheID;
        }
        
        void Log2Op::finalize()
        {
            std::ostringstream cacheIDStream;
            cacheIDStream << "<Log2Op ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        void Log2Op::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                ApplyLog2NoAlpha_Forward(rgbaBuffer, numPixels);
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                ApplyLog2NoAlpha_Inverse(rgbaBuffer, numPixels);
            }
        } // Op::process
        
        bool Log2Op::supportsGpuShader() const
        {
            return true;
        }
        
        void Log2Op::writeGpuShader(std::ostringstream & shader,
                                            const std::string & pixelName,
                                            const GpuShaderDesc & shaderDesc) const
        {
            GpuLanguage lang = shaderDesc.getLanguage();
            
            float clampMin[3] = { FLTMIN, FLTMIN, FLTMIN };
            
            // TODO: Switch to f32 for internal processing?
            if(lang == GPU_LANGUAGE_CG)
            {
                clampMin[0] = static_cast<float>(GetHalfNormMin());
                clampMin[1] = static_cast<float>(GetHalfNormMin());
                clampMin[2] = static_cast<float>(GetHalfNormMin());
            }
            
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                shader << pixelName << ".rgb = ";
                shader << "log2(max(";
                shader << pixelName << ".rgb,";
                Write_half3(&shader, clampMin, lang);
                shader << ")).rgb;\n";
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                float base[] = { 2.0f, 2.0f, 2.0f };
                
                shader << pixelName << ".rgb = pow(";
                Write_half3(&shader, base, lang);
                
                shader << ", " << pixelName << ".rgb).rgb;\n";
            }
        }
        
        bool Log2Op::definesGpuAllocation() const
        {
            return false;
        }
        
        GpuAllocationData Log2Op::getGpuAllocation() const
        {
            throw Exception("Log2Op does not define a Gpu Allocation.");
        }
        
    }  // Anon namespace
    
    
    
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void CreateLog2Op(OpRcPtrVec & ops,
                      TransformDirection direction)
    {
        ops.push_back( OpRcPtr(new Log2Op(direction)) );
    }
}
OCIO_NAMESPACE_EXIT
