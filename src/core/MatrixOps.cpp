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
#include "MatrixOps.h"
#include "MathUtils.h"

#include <cstring>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        void ApplyScaleNoAlpha(float* rgbaBuffer, long numPixels,
                               const float* scale4)
        {
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] *= scale4[0];
                rgbaBuffer[1] *= scale4[1];
                rgbaBuffer[2] *= scale4[2];
                
                rgbaBuffer += 4;
            }
        }
        
        void ApplyOffsetNoAlpha(float* rgbaBuffer, long numPixels,
                                const float* offset4)
        {
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] += offset4[0];
                rgbaBuffer[1] += offset4[1];
                rgbaBuffer[2] += offset4[2];
                
                rgbaBuffer += 4;
            }
        }
        
        void ApplyMatrixNoAlpha(float* rgbaBuffer, long numPixels,
                                const float* mat44)
        {
            float r,g,b;
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                r = rgbaBuffer[0];
                g = rgbaBuffer[1];
                b = rgbaBuffer[2];
                
                rgbaBuffer[0] = r*mat44[0] + g*mat44[1] + b*mat44[2];
                rgbaBuffer[1] = r*mat44[4] + g*mat44[5] + b*mat44[6];
                rgbaBuffer[2] = r*mat44[8] + g*mat44[9] + b*mat44[10];
                
                rgbaBuffer += 4;
            }
        }
    }
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    
    
    
    
    namespace
    {
        class MatrixOffsetOp : public Op
        {
        public:
            MatrixOffsetOp(const float * m44,
                           const float * offset4,
                           TransformDirection direction);
            virtual ~MatrixOffsetOp();
            
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
            float m_m44[16];
            float m_offset4[4];
            TransformDirection m_direction;
            
            bool m_m44IsIdentity;
            bool m_m44IsDiagonal;
            bool m_offset4IsIdentity;
            float m_m44_inv[16];
            float m_offset4_inv[4];
            
            std::string m_cacheID;
        };
        
        MatrixOffsetOp::MatrixOffsetOp(const float * m44,
                                       const float * offset4,
                                       TransformDirection direction):
                                       Op(),
                                       m_direction(direction),
                                       m_m44IsIdentity(false),
                                       m_offset4IsIdentity(false)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply MatrixOffsetOp op, unspecified transform direction.");
            }
            
            memcpy(m_m44, m44, 16*sizeof(float));
            memcpy(m_offset4, offset4, 4*sizeof(float));
            
            memset(m_m44_inv, 0, 16*sizeof(float));
            memset(m_offset4_inv, 0, 4*sizeof(float));
        }
        
        OpRcPtr MatrixOffsetOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new MatrixOffsetOp(m_m44, m_offset4, m_direction));
            return op;
        }
        
        MatrixOffsetOp::~MatrixOffsetOp()
        { }
        
        std::string MatrixOffsetOp::getInfo() const
        {
            return "<MatrixOffsetOp>";
        }
        
        std::string MatrixOffsetOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        void MatrixOffsetOp::finalize()
        {
            m_offset4IsIdentity = IsVecEqualToZero(m_offset4, 4);
            m_m44IsIdentity = IsM44Identity(m_m44);
            m_m44IsDiagonal = IsM44Diagonal(m_m44);
            
            if(TRANSFORM_DIR_INVERSE)
            {
                if(!GetM44Inverse(m_m44_inv, m_m44))
                {
                    std::ostringstream os;
                    os << "Cannot apply MatrixOffsetOp op. ";
                    os << "Matrix inverse does not exist for m44 (";
                    for(int i=0; i<16; ++i) os << m_m44[i] << " ";
                    os << ").";
                    throw Exception(os.str().c_str());
                }
                
                for(int i=0; i<4; ++i)
                {
                    m_offset4_inv[i] = -m_offset4_inv[i];
                }
            }
            
            // Create the cacheID
            md5_state_t state;
            md5_byte_t digest[16];
            md5_init(&state);
            md5_append(&state, (const md5_byte_t *)m_m44, 16*sizeof(float));
            md5_append(&state, (const md5_byte_t *)m_offset4, 4*sizeof(float));
            md5_finish(&state, digest);
            
            std::ostringstream cacheIDStream;
            cacheIDStream << "<MatrixOffsetOp ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        void MatrixOffsetOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        float scale[4];
                        GetM44Diagonal(scale, m_m44);
                        ApplyScaleNoAlpha(rgbaBuffer, numPixels, scale);
                    }
                    else
                    {
                        ApplyMatrixNoAlpha(rgbaBuffer, numPixels, m_m44);
                    }
                }
                
                if(!m_offset4IsIdentity)
                {
                    ApplyOffsetNoAlpha(rgbaBuffer, numPixels, m_offset4);
                }
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(!m_offset4IsIdentity)
                {
                    ApplyOffsetNoAlpha(rgbaBuffer, numPixels, m_offset4_inv);
                }
                
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        float scale[4];
                        GetM44Diagonal(scale, m_m44_inv);
                        ApplyScaleNoAlpha(rgbaBuffer, numPixels, scale);
                    }
                    else
                    {
                        ApplyMatrixNoAlpha(rgbaBuffer, numPixels, m_m44_inv);
                    }
                }
            }
        } // Op::process
        
        bool MatrixOffsetOp::supportsGpuShader() const
        {
            return true;
        }
        
        void MatrixOffsetOp::writeGpuShader(std::ostringstream & shader,
                                            const std::string & pixelName,
                                            const GpuShaderDesc & shaderDesc) const
        {
            GpuLanguage lang = shaderDesc.getLanguage();
            
            // TODO: This should not act upon alpha,
            // since we dont apply it on the CPU?
            
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        shader << pixelName << " = ";
                        float scale[4];
                        GetM44Diagonal(scale, m_m44);
                        Write_half4(&shader, scale, lang);
                        shader << " * " << pixelName << ";\n";
                    }
                    else
                    {
                        shader << pixelName << " = ";
                        Write_mtx_x_vec(&shader,
                                        GpuTextHalf4x4(m_m44, lang), pixelName,
                                        lang);
                        shader << ";\n";
                    }
                }
                
                if(!m_offset4IsIdentity)
                {
                    shader << pixelName << " = ";
                    Write_half4(&shader, m_offset4, lang);
                    shader << " + " << pixelName << ";\n";
                }
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(!m_offset4IsIdentity)
                {
                    shader << pixelName << " = ";
                    Write_half4(&shader, m_offset4_inv, lang);
                    shader << " + " << pixelName << ";\n";
                }
                
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        shader << pixelName << " = ";
                        float scale[4];
                        GetM44Diagonal(scale, m_m44_inv);
                        Write_half4(&shader, scale, lang);
                        shader << " * " << pixelName << ";\n";
                    }
                    else
                    {
                        shader << pixelName << " = ";
                        Write_mtx_x_vec(&shader,
                                        GpuTextHalf4x4(m_m44_inv, lang), pixelName,
                                        lang);
                        shader << ";\n";
                    }
                }
            }
        }
        
        bool MatrixOffsetOp::definesGpuAllocation() const
        {
            return false;
        }
        
        GpuAllocationData MatrixOffsetOp::getGpuAllocation() const
        {
            throw Exception("MatrixOffsetOp does not define a Gpu Allocation.");
        }
        
    }  // Anon namespace
    
    
    
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    
    
    
    
    
    void CreateScaleOp(OpRcPtrVec & ops,
                       const float * scale4,
                       TransformDirection direction)
    {
        float offset4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        CreateScaleOffsetOp(ops, scale4, offset4, direction);
    }
    
    void CreateMatrixOp(OpRcPtrVec & ops,
                        const float * m44,
                        TransformDirection direction)
    {
        float offset4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        CreateMatrixOffsetOp(ops, m44, offset4, direction);
    }
    
    void CreateOffsetOp(OpRcPtrVec & ops,
                        const float * offset4,
                        TransformDirection direction)
    {
        float scale4[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        CreateScaleOffsetOp(ops, scale4, offset4, direction);
    }
    
    void CreateScaleOffsetOp(OpRcPtrVec & ops,
                             const float * scale4, const float * offset4,
                             TransformDirection direction)
    {
        float m44[16];
        memset(m44, 0, 16*sizeof(float));
        
        m44[0] = scale4[0];
        m44[5] = scale4[1];
        m44[10] = scale4[2];
        m44[15] = scale4[3];
        
        CreateMatrixOffsetOp(ops,
                             m44, offset4,
                             direction);
    }
    
    void CreateSaturationOp(OpRcPtrVec & ops,
                            float sat,
                            const float * lumaCoef3,
                            TransformDirection direction)
    {
        float m44[16];
        
        m44[0] = (1 - sat) * lumaCoef3[0] + sat;
        m44[1] = (1 - sat) * lumaCoef3[1];
        m44[2] = (1 - sat) * lumaCoef3[2];
        m44[3] = 0.0f;
        
        m44[4] = (1 - sat) * lumaCoef3[0];
        m44[5] = (1 - sat) * lumaCoef3[1] + sat;
        m44[6] = (1 - sat) * lumaCoef3[2];
        m44[7] = 0.0f;
        
        m44[8] = (1 - sat) * lumaCoef3[0];
        m44[9] = (1 - sat) * lumaCoef3[1];
        m44[10] = (1 - sat) * lumaCoef3[2] + sat;
        m44[11] = 0.0f;
        
        m44[12] = 0.0f;
        m44[13] = 0.0f;
        m44[14] = 0.0f;
        m44[15] = 1.0f;
        
        CreateMatrixOp(ops, m44, direction);
    }
    
    void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                              const float * m44, const float * offset4,
                              TransformDirection direction)
    {
        bool mtxIsIdentity = IsM44Identity(m44);
        bool offsetIsIdentity = IsVecEqualToZero(offset4, 4);
        if(mtxIsIdentity && offsetIsIdentity) return;
        
        ops.push_back( OpRcPtr(new MatrixOffsetOp(m44,
            offset4, direction)) );
    }
    
    /*
    Fit is canonically formulated as:
    out = newmin + ((value-oldmin)/(oldmax-oldmin)*(newmax-newmin))
    I.e., subtract the old offset, descale into the [0,1] range,
          scale into the new range, and add the new offset
    
    We algebraiclly manipulate the terms into y = mx + b form as:
    m = (newmax-newmin)/(oldmax-oldmin)
    b = (newmin*oldmax - newmax*oldmin) / (oldmax-oldmin)
    */
    
    void CreateFitOp(OpRcPtrVec & ops,
                     const float * oldmin4, const float * oldmax4,
                     const float * newmin4, const float * newmax4,
                     TransformDirection direction)
    {
        float scale[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float offset[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        
        for(int i=0; i<4; ++i)
        {
            float denom = oldmax4[i] - oldmin4[i];
            if(IsScalarEqualToZero(denom))
            {
                std::ostringstream os;
                os << "Cannot create Fit operatir. ";
                os << "Max value equals min value '";
                os << oldmax4[i] << "' in channel index ";
                os << i << ".";
                throw Exception(os.str().c_str());
            }
            
            scale[i] = (newmax4[i]-newmin4[i]) / denom;
            offset[i] = (newmin4[i]*oldmax4[i] - newmax4[i]*oldmin4[i]) / denom;
        }
        
        return CreateScaleOffsetOp(ops,
                                   scale, offset,
                                   direction);
    }
}
OCIO_NAMESPACE_EXIT
