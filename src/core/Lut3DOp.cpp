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

#include "HashUtils.h"
#include "Lut3DOp.h"
#include "MathUtils.h"

#include <cmath>
#include <limits>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    Lut3D::Lut3D()
    {
        for(int i=0; i<3; ++i)
        {
            from_min[i] = 0.0f;
            from_max[i] = 1.0f;
            size[i] = 0;
        }
    }
    
    Lut3DRcPtr Lut3D::Create()
    {
        return Lut3DRcPtr(new Lut3D());
    }

    std::string Lut3D::getCacheID() const
    {
        AutoMutex lock(m_cacheidMutex);
        
        if(lut.empty())
            throw Exception("Cannot compute cacheID of invalid Lut3D");
        
        if(!m_cacheID.empty())
            return m_cacheID;
        
        md5_state_t state;
        md5_byte_t digest[16];
        
        md5_init(&state);
        
        md5_append(&state, (const md5_byte_t *)from_min, 3*sizeof(float));
        md5_append(&state, (const md5_byte_t *)from_max, 3*sizeof(float));
        md5_append(&state, (const md5_byte_t *)size, 3*sizeof(int));
        md5_append(&state, (const md5_byte_t *)&lut[0], (int) (lut.size()*sizeof(float)));
        
        md5_finish(&state, digest);
        
        m_cacheID = GetPrintableHash(digest);
        
        return m_cacheID;
    }
    
    
    namespace
    {
        // Linear
        inline float lerp(float a, float b, float z)
            { return (b - a) * z + a; }
        
        inline void lerp_rgb(float* out, float* a, float* b, float* z)
        {
            out[0] = (b[0] - a[0]) * z[0] + a[0];
            out[1] = (b[1] - a[1]) * z[1] + a[1];
            out[2] = (b[2] - a[2]) * z[2] + a[2];
        }
        
        // Bilinear
        inline float lerp(float a, float b, float c, float d, float y, float z)
            { return lerp(lerp(a, b, z), lerp(c, d, z), y); }
        
        inline void lerp_rgb(float* out, float* a, float* b, float* c,
                             float* d, float* y, float* z)
        {
            float v1[3];
            float v2[3];
            
            lerp_rgb(v1, a, b, z);
            lerp_rgb(v2, c, d, z);
            lerp_rgb(out, v1, v2, y);
        }
        
        // Trilinear
        inline float lerp(float a, float b, float c, float d,
                          float e, float f, float g, float h,
                          float x, float y, float z)
            { return lerp(lerp(a,b,c,d,y,z), lerp(e,f,g,h,y,z), x); }
        
        inline void lerp_rgb(float* out, float* a, float* b, float* c, float* d,
                             float* e, float* f, float* g, float* h,
                             float* x, float* y, float* z)
        {
            float v1[3];
            float v2[3];
            
            lerp_rgb(v1, a,b,c,d,y,z);
            lerp_rgb(v2, e,f,g,h,y,z);
            lerp_rgb(out, v1, v2, x);
        }
        
        inline float lookupNearest_3D(int rIndex, int gIndex, int bIndex,
                                      int size_red, int size_green, int size_blue,
                                      const float* simple_rgb_lut, int channelIndex)
        {
            return simple_rgb_lut[ GetLut3DIndex_B(rIndex, gIndex, bIndex,
                size_red, size_green, size_blue) + channelIndex];
        }
        
        inline void lookupNearest_3D_rgb(float* rgb,
                                         int rIndex, int gIndex, int bIndex,
                                         int size_red, int size_green, int size_blue,
                                         const float* simple_rgb_lut)
        {
            int offset = GetLut3DIndex_B(rIndex, gIndex, bIndex, size_red, size_green, size_blue);
            rgb[0] = simple_rgb_lut[offset];
            rgb[1] = simple_rgb_lut[offset+1];
            rgb[2] = simple_rgb_lut[offset+2];
        }
        
        // Note: This function assumes that minVal is less than maxVal
        inline int clamp(float k, float minVal, float maxVal)
        {
            return static_cast<int>(roundf(std::max(std::min(k, maxVal), minVal)));
        }
        
        ///////////////////////////////////////////////////////////////////////
        // Nearest Forward
        
        void Lut3D_Nearest(float* rgbaBuffer, long numPixels, const Lut3D & lut)
        {
            float maxIndex[3];
            float mInv[3];
            float b[3];
            float mInv_x_maxIndex[3];
            int lutSize[3];
            const float* startPos = &(lut.lut[0]);
            
            for(int i=0; i<3; ++i)
            {
                maxIndex[i] = (float) (lut.size[i] - 1);
                mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
                b[i] = lut.from_min[i];
                mInv_x_maxIndex[i] = (float) (mInv[i] * maxIndex[i]);
                
                lutSize[i] = lut.size[i];
            }
            
            int localIndex[3];
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                if(isnan(rgbaBuffer[0]) || isnan(rgbaBuffer[1]) || isnan(rgbaBuffer[2]))
                {
                    rgbaBuffer[0] = std::numeric_limits<float>::quiet_NaN();
                    rgbaBuffer[1] = std::numeric_limits<float>::quiet_NaN();
                    rgbaBuffer[2] = std::numeric_limits<float>::quiet_NaN();
                }
                else
                {
                    localIndex[0] = clamp(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), 0.0f, maxIndex[0]);
                    localIndex[1] = clamp(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), 0.0f, maxIndex[1]);
                    localIndex[2] = clamp(mInv_x_maxIndex[2] * (rgbaBuffer[2] - b[2]), 0.0f, maxIndex[2]);
                    
                    lookupNearest_3D_rgb(rgbaBuffer, localIndex[0], localIndex[1], localIndex[2],
                                                     lutSize[0], lutSize[1], lutSize[2], startPos);
                }
                
                rgbaBuffer += 4;
            }
        }
        
        ///////////////////////////////////////////////////////////////////////
        // Linear Forward
        
        void Lut3D_Linear(float* rgbaBuffer, long numPixels, const Lut3D & lut)
        {
            float maxIndex[3];
            float mInv[3];
            float b[3];
            float mInv_x_maxIndex[3];
            int lutSize[3];
            const float* startPos = &(lut.lut[0]);
            
            for(int i=0; i<3; ++i)
            {
                maxIndex[i] = (float) (lut.size[i] - 1);
                mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
                b[i] = lut.from_min[i];
                mInv_x_maxIndex[i] = (float) (mInv[i] * maxIndex[i]);
                
                lutSize[i] = lut.size[i];
            }
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                
                if(isnan(rgbaBuffer[0]) || isnan(rgbaBuffer[1]) || isnan(rgbaBuffer[2]))
                {
                    rgbaBuffer[0] = std::numeric_limits<float>::quiet_NaN();
                    rgbaBuffer[1] = std::numeric_limits<float>::quiet_NaN();
                    rgbaBuffer[2] = std::numeric_limits<float>::quiet_NaN();
                }
                else
                {
                    float localIndex[3];
                    int indexLow[3];
                    int indexHigh[3];
                    float delta[3];
                    float a[3];
                    float b_[3];
                    float c[3];
                    float d[3];
                    float e[3];
                    float f[3];
                    float g[3];
                    float h[4];
                    float x[4];
                    float y[4];
                    float z[4];
                    
                    localIndex[0] = std::max(std::min(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0]), 0.0f);
                    localIndex[1] = std::max(std::min(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1]), 0.0f);
                    localIndex[2] = std::max(std::min(mInv_x_maxIndex[2] * (rgbaBuffer[2] - b[2]), maxIndex[2]), 0.0f);
                    
                    indexLow[0] =  static_cast<int>(std::floor(localIndex[0]));
                    indexLow[1] =  static_cast<int>(std::floor(localIndex[1]));
                    indexLow[2] =  static_cast<int>(std::floor(localIndex[2]));
                    
                    indexHigh[0] =  static_cast<int>(std::ceil(localIndex[0]));
                    indexHigh[1] =  static_cast<int>(std::ceil(localIndex[1]));
                    indexHigh[2] =  static_cast<int>(std::ceil(localIndex[2]));
                    
                    delta[0] = localIndex[0] - static_cast<float>(indexLow[0]);
                    delta[1] = localIndex[1] - static_cast<float>(indexLow[1]);
                    delta[2] = localIndex[2] - static_cast<float>(indexLow[2]);
                    
                    // Lookup 8 corners of cube
                    lookupNearest_3D_rgb(a, indexLow[0],  indexLow[1],  indexLow[2],  lutSize[0], lutSize[1], lutSize[2], startPos);
                    lookupNearest_3D_rgb(b_, indexLow[0],  indexLow[1],  indexHigh[2], lutSize[0], lutSize[1], lutSize[2], startPos);
                    lookupNearest_3D_rgb(c, indexLow[0],  indexHigh[1], indexLow[2],  lutSize[0], lutSize[1], lutSize[2], startPos);
                    lookupNearest_3D_rgb(d, indexLow[0],  indexHigh[1], indexHigh[2], lutSize[0], lutSize[1], lutSize[2], startPos);
                    lookupNearest_3D_rgb(e, indexHigh[0], indexLow[1],  indexLow[2],  lutSize[0], lutSize[1], lutSize[2], startPos);
                    lookupNearest_3D_rgb(f, indexHigh[0], indexLow[1],  indexHigh[2], lutSize[0], lutSize[1], lutSize[2], startPos);
                    lookupNearest_3D_rgb(g, indexHigh[0], indexHigh[1], indexLow[2],  lutSize[0], lutSize[1], lutSize[2], startPos);
                    lookupNearest_3D_rgb(h, indexHigh[0], indexHigh[1], indexHigh[2], lutSize[0], lutSize[1], lutSize[2], startPos);
                    
                    // Also store the 3d interpolation coordinates
                    x[0] = delta[0]; x[1] = delta[0]; x[2] = delta[0];
                    y[0] = delta[1]; y[1] = delta[1]; y[2] = delta[1];
                    z[0] = delta[2]; z[1] = delta[2]; z[2] = delta[2];
                    
                    // Do a trilinear interpolation of the 8 corners
                    // 4726.8 scanlines/sec
                    
                    lerp_rgb(rgbaBuffer, a, b_, c, d, e, f, g, h,
                                         x, y, z);
                }
                
                rgbaBuffer += 4;
            }
        }
    }
    
    
    void Lut3D_Tetrahedral(float* rgbaBuffer, long numPixels, const Lut3D & lut)
    {
        // Tetrahedral interoplation, as described by:
        // http://www.filmlight.ltd.uk/pdf/whitepapers/FL-TL-TN-0057-SoftwareLib.pdf
        // http://blogs.mathworks.com/steve/2006/11/24/tetrahedral-interpolation-for-colorspace-conversion/
        // http://www.hpl.hp.com/techreports/98/HPL-98-95.html

        float maxIndex[3];
        float mInv[3];
        float b[3];
        float mInv_x_maxIndex[3];
        int lutSize[3];
        const float* startPos = &(lut.lut[0]);

        for(int i=0; i<3; ++i)
        {
            maxIndex[i] = (float) (lut.size[i] - 1);
            mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
            b[i] = lut.from_min[i];
            mInv_x_maxIndex[i] = (float) (mInv[i] * maxIndex[i]);

            lutSize[i] = lut.size[i];
        }

        for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
        {

            if(isnan(rgbaBuffer[0]) || isnan(rgbaBuffer[1]) || isnan(rgbaBuffer[2]))
            {
                rgbaBuffer[0] = std::numeric_limits<float>::quiet_NaN();
                rgbaBuffer[1] = std::numeric_limits<float>::quiet_NaN();
                rgbaBuffer[2] = std::numeric_limits<float>::quiet_NaN();
            }
            else
            {
                float localIndex[3];
                int indexLow[3];
                int indexHigh[3];
                float delta[3];

                // Same index/delta calculation as linear interpolation
                localIndex[0] = std::max(std::min(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0]), 0.0f);
                localIndex[1] = std::max(std::min(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1]), 0.0f);
                localIndex[2] = std::max(std::min(mInv_x_maxIndex[2] * (rgbaBuffer[2] - b[2]), maxIndex[2]), 0.0f);

                indexLow[0] =  static_cast<int>(std::floor(localIndex[0]));
                indexLow[1] =  static_cast<int>(std::floor(localIndex[1]));
                indexLow[2] =  static_cast<int>(std::floor(localIndex[2]));

                indexHigh[0] =  static_cast<int>(std::ceil(localIndex[0]));
                indexHigh[1] =  static_cast<int>(std::ceil(localIndex[1]));
                indexHigh[2] =  static_cast<int>(std::ceil(localIndex[2]));

                delta[0] = localIndex[0] - static_cast<float>(indexLow[0]);
                delta[1] = localIndex[1] - static_cast<float>(indexLow[1]);
                delta[2] = localIndex[2] - static_cast<float>(indexLow[2]);

                // Rebind for consistency with Truelight paper
                float fx = delta[0];
                float fy = delta[1];
                float fz = delta[2];

                // Compute index into LUT for surrounding corners
                const int n000 = GetLut3DIndex_B(indexLow[0], indexLow[1], indexLow[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);
                const int n100 = GetLut3DIndex_B(indexHigh[0], indexLow[1], indexLow[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);
                const int n010 = GetLut3DIndex_B(indexLow[0], indexHigh[1], indexLow[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);
                const int n001 = GetLut3DIndex_B(indexLow[0], indexLow[1], indexHigh[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);
                const int n110 = GetLut3DIndex_B(indexHigh[0], indexHigh[1], indexLow[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);
                const int n101 = GetLut3DIndex_B(indexHigh[0], indexLow[1], indexHigh[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);
                const int n011 = GetLut3DIndex_B(indexLow[0], indexHigh[1], indexHigh[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);
                const int n111 = GetLut3DIndex_B(indexHigh[0], indexHigh[1], indexHigh[2],
                                                 lutSize[0], lutSize[1], lutSize[2]);

                if (fx > fy) {
                    if (fy > fz) {
                       rgbaBuffer[0] =
                           (1-fx)  * startPos[n000] +
                           (fx-fy) * startPos[n100] +
                           (fy-fz) * startPos[n110] +
                           (fz)    * startPos[n111];

                       rgbaBuffer[1] =
                           (1-fx)  * startPos[n000+1] +
                           (fx-fy) * startPos[n100+1] +
                           (fy-fz) * startPos[n110+1] +
                           (fz)    * startPos[n111+1];

                       rgbaBuffer[2] =
                           (1-fx)  * startPos[n000+2] +
                           (fx-fy) * startPos[n100+2] +
                           (fy-fz) * startPos[n110+2] +
                           (fz)    * startPos[n111+2];
                    }
                    else if (fx > fz)
                    {
                        rgbaBuffer[0] =
                            (1-fx)  * startPos[n000] +
                            (fx-fz) * startPos[n100] +
                            (fz-fy) * startPos[n101] +
                            (fy)    * startPos[n111];

                        rgbaBuffer[1] =
                            (1-fx)  * startPos[n000+1] +
                            (fx-fz) * startPos[n100+1] +
                            (fz-fy) * startPos[n101+1] +
                            (fy)    * startPos[n111+1];

                        rgbaBuffer[2] =
                            (1-fx)  * startPos[n000+2] +
                            (fx-fz) * startPos[n100+2] +
                            (fz-fy) * startPos[n101+2] +
                            (fy)    * startPos[n111+2];
                    }
                    else
                    {
                        rgbaBuffer[0] =
                            (1-fz)  * startPos[n000] +
                            (fz-fx) * startPos[n001] +
                            (fx-fy) * startPos[n101] +
                            (fy)    * startPos[n111];

                        rgbaBuffer[1] =
                            (1-fz)  * startPos[n000+1] +
                            (fz-fx) * startPos[n001+1] +
                            (fx-fy) * startPos[n101+1] +
                            (fy)    * startPos[n111+1];

                        rgbaBuffer[2] =
                            (1-fz)  * startPos[n000+2] +
                            (fz-fx) * startPos[n001+2] +
                            (fx-fy) * startPos[n101+2] +
                            (fy)    * startPos[n111+2];
                    }
                }
                else
                {
                    if (fz > fy)
                    {
                        rgbaBuffer[0] =
                            (1-fz)  * startPos[n000] +
                            (fz-fy) * startPos[n001] +
                            (fy-fx) * startPos[n011] +
                            (fx)    * startPos[n111];

                        rgbaBuffer[1] =
                            (1-fz)  * startPos[n000+1] +
                            (fz-fy) * startPos[n001+1] +
                            (fy-fx) * startPos[n011+1] +
                            (fx)    * startPos[n111+1];

                        rgbaBuffer[2] =
                            (1-fz)  * startPos[n000+2] +
                            (fz-fy) * startPos[n001+2] +
                            (fy-fx) * startPos[n011+2] +
                            (fx)    * startPos[n111+2];
                    }
                    else if (fz > fx)
                    {
                        rgbaBuffer[0] =
                            (1-fy)  * startPos[n000] +
                            (fy-fz) * startPos[n010] +
                            (fz-fx) * startPos[n011] +
                            (fx)    * startPos[n111];

                        rgbaBuffer[1] =
                            (1-fy)  * startPos[n000+1] +
                            (fy-fz) * startPos[n010+1] +
                            (fz-fx) * startPos[n011+1] +
                            (fx)    * startPos[n111+1];

                        rgbaBuffer[2] =
                            (1-fy)  * startPos[n000+2] +
                            (fy-fz) * startPos[n010+2] +
                            (fz-fx) * startPos[n011+2] +
                            (fx)    * startPos[n111+2];
                    }
                    else
                    {
                        rgbaBuffer[0] =
                            (1-fy)  * startPos[n000] +
                            (fy-fx) * startPos[n010] +
                            (fx-fz) * startPos[n110] +
                            (fz)    * startPos[n111];

                        rgbaBuffer[1] =
                            (1-fy)  * startPos[n000+1] +
                            (fy-fx) * startPos[n010+1] +
                            (fx-fz) * startPos[n110+1] +
                            (fz)    * startPos[n111+1];

                        rgbaBuffer[2] =
                            (1-fy)  * startPos[n000+2] +
                            (fy-fx) * startPos[n010+2] +
                            (fx-fz) * startPos[n110+2] +
                            (fz)    * startPos[n111+2];
                    }
                }
            } // !isnan
            
            rgbaBuffer += 4;
        }
    }


    void GenerateIdentityLut3D(float* img, int edgeLen, int numChannels, Lut3DOrder lut3DOrder)
    {
        if(!img) return;
        if(numChannels < 3)
        {
            throw Exception("Cannot generate idenitity 3d lut with less than 3 channels.");
        }
        
        float c = 1.0f / ((float)edgeLen - 1.0f);
        
        if(lut3DOrder == LUT3DORDER_FAST_RED)
        {
            for(int i=0; i<edgeLen*edgeLen*edgeLen; i++)
            {
                img[numChannels*i+0] = (float)(i%edgeLen) * c;
                img[numChannels*i+1] = (float)((i/edgeLen)%edgeLen) * c;
                img[numChannels*i+2] = (float)((i/edgeLen/edgeLen)%edgeLen) * c;
            }
        }
        else if(lut3DOrder == LUT3DORDER_FAST_BLUE)
        {
            for(int i=0; i<edgeLen*edgeLen*edgeLen; i++)
            {
                img[numChannels*i+0] = (float)((i/edgeLen/edgeLen)%edgeLen) * c;
                img[numChannels*i+1] = (float)((i/edgeLen)%edgeLen) * c;
                img[numChannels*i+2] = (float)(i%edgeLen) * c;
            }
        }
        else
        {
            throw Exception("Unknown Lut3DOrder.");
        }
    }
    
    
    int Get3DLutEdgeLenFromNumPixels(int numPixels)
    {
        int dim = static_cast<int>(roundf(powf((float) numPixels, 1.0f/3.0f)));
        
        if(dim*dim*dim != numPixels)
        {
            std::ostringstream os;
            os << "Cannot infer 3D Lut size. ";
            os << numPixels << " element(s) does not correspond to a ";
            os << "unform cube edge length. (nearest edge length is ";
            os << dim << ").";
            throw Exception(os.str().c_str());
        }
        
        return dim;
    }
    
    namespace
    {
        class Lut3DOp : public Op
        {
        public:
            Lut3DOp(Lut3DRcPtr lut,
                    Interpolation interpolation,
                    TransformDirection direction);
            virtual ~Lut3DOp();
            
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
            Lut3DRcPtr m_lut;
            Interpolation m_interpolation;
            TransformDirection m_direction;
            
            // Set in finalize
            std::string m_cacheID;
        };
        
        typedef OCIO_SHARED_PTR<Lut3DOp> Lut3DOpRcPtr;
        
        
        Lut3DOp::Lut3DOp(Lut3DRcPtr lut,
                         Interpolation interpolation,
                         TransformDirection direction):
                            Op(),
                            m_lut(lut),
                            m_interpolation(interpolation),
                            m_direction(direction)
        {
        }
        
        OpRcPtr Lut3DOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new Lut3DOp(m_lut, m_interpolation, m_direction));
            return op;
        }
        
        Lut3DOp::~Lut3DOp()
        { }
        
        std::string Lut3DOp::getInfo() const
        {
            return "<Lut3DOp>";
        }
        
        std::string Lut3DOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        // TODO: compute real value for isNoOp
        bool Lut3DOp::isNoOp() const
        {
            return false;
        }
        
        bool Lut3DOp::isSameType(const OpRcPtr & op) const
        {
            Lut3DOpRcPtr typedRcPtr = DynamicPtrCast<Lut3DOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool Lut3DOp::isInverse(const OpRcPtr & op) const
        {
            Lut3DOpRcPtr typedRcPtr = DynamicPtrCast<Lut3DOp>(op);
            if(!typedRcPtr) return false;
            
            if(GetInverseTransformDirection(m_direction) != typedRcPtr->m_direction)
                return false;
            
            return (m_lut->getCacheID() == typedRcPtr->m_lut->getCacheID());
        }
        
        // TODO: compute real value for hasChannelCrosstalk
        bool Lut3DOp::hasChannelCrosstalk() const
        {
            return true;
        }
        
        void Lut3DOp::finalize()
        {
            if(m_direction != TRANSFORM_DIR_FORWARD)
            {
                std::ostringstream os;
                os << "3D Luts can only be applied in the forward direction. ";
                os << "(" << TransformDirectionToString(m_direction) << ")";
                os << " specified.";
                throw Exception(os.str().c_str());
            }
            
            // Validate the requested interpolation type
            switch(m_interpolation)
            {
                 // These are the allowed values.
                case INTERP_NEAREST:
                case INTERP_LINEAR:
                case INTERP_TETRAHEDRAL:
                    break;
                case INTERP_BEST:
                    m_interpolation = INTERP_LINEAR;
                    break;
                case INTERP_UNKNOWN:
                    throw Exception("Cannot apply Lut3DOp, unspecified interpolation.");
                    break;
                default:
                    throw Exception("Cannot apply Lut3DOp, invalid interpolation specified.");
            }
            
            for(int i=0; i<3; ++i)
            {
                if(m_lut->size[i] == 0)
                {
                    throw Exception("Cannot apply Lut3DOp, lut object is empty.");
                }
                // TODO if from_min[i] == from_max[i]
            }
            
            if(m_lut->size[0]*m_lut->size[1]*m_lut->size[2] * 3 != (int)m_lut->lut.size())
            {
                throw Exception("Cannot apply Lut3DOp, specified size does not match data.");
            }
            
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut3DOp ";
            cacheIDStream << m_lut->getCacheID() << " ";
            cacheIDStream << InterpolationToString(m_interpolation) << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void Lut3DOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_interpolation == INTERP_NEAREST)
            {
                Lut3D_Nearest(rgbaBuffer, numPixels, *m_lut);
            }
            else if(m_interpolation == INTERP_LINEAR)
            {
                Lut3D_Linear(rgbaBuffer, numPixels, *m_lut);
            }
            else if(m_interpolation == INTERP_TETRAHEDRAL)
            {
                Lut3D_Tetrahedral(rgbaBuffer, numPixels, *m_lut);
            }
        }
        
        bool Lut3DOp::supportsGpuShader() const
        {
            return false;
        }
        
        void Lut3DOp::writeGpuShader(std::ostream & /*shader*/,
                                     const std::string & /*pixelName*/,
                                     const GpuShaderDesc & /*shaderDesc*/) const
        {
            throw Exception("Lut3DOp does not support analytical shader generation.");
        }
    }
    
    void CreateLut3DOp(OpRcPtrVec & ops,
                       Lut3DRcPtr lut,
                       Interpolation interpolation,
                       TransformDirection direction)
    {
        ops.push_back( Lut3DOpRcPtr(new Lut3DOp(lut, interpolation, direction)) );
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include <cstring>
#include <cstdlib>
#ifndef WIN32
#include <sys/time.h>
#endif

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(Lut3DOp, NanInfValueCheck)
{
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    
    lut->size[0] = 3;
    lut->size[1] = 3;
    lut->size[2] = 3;
    
    lut->lut.resize(lut->size[0]*lut->size[1]*lut->size[2]*3);
    
    GenerateIdentityLut3D(&lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED);
    for(unsigned int i=0; i<lut->lut.size(); ++i)
    {
        lut->lut[i] = powf(lut->lut[i], 2.0f);
    }
    
    const float reference[4] = {  std::numeric_limits<float>::signaling_NaN(),
                                  std::numeric_limits<float>::quiet_NaN(),
                                  std::numeric_limits<float>::infinity(),
                                  -std::numeric_limits<float>::infinity() };
    float color[4];
    
    memcpy(color, reference, 4*sizeof(float));
    OCIO::Lut3D_Nearest(color, 1, *lut);
    
    memcpy(color, reference, 4*sizeof(float));
    OCIO::Lut3D_Linear(color, 1, *lut);
}


OIIO_ADD_TEST(Lut3DOp, ValueCheck)
{
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    
    lut->size[0] = 32;
    lut->size[1] = 32;
    lut->size[2] = 32;
    
    lut->lut.resize(lut->size[0]*lut->size[1]*lut->size[2]*3);
    GenerateIdentityLut3D(&lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED);
    for(unsigned int i=0; i<lut->lut.size(); ++i)
    {
        lut->lut[i] = powf(lut->lut[i], 2.0f);
    }
    
    const float reference[] = {  0.0f, 0.2f, 0.3f, 1.0f,
                                 0.1234f, 0.4567f, 0.9876f, 1.0f,
                                 11.0f, -0.5f, 0.5010f, 1.0f
                                };
    const float nearest[] = { 0.0f, 0.03746097535f, 0.0842871964f, 1.0f,
                              0.01664932258f, 0.2039542049f, 1.0f, 1.0f,
                              1.0f, 0.0f, 0.2663891613f, 1.0f
                            };
    const float linear[] = { 0.0f, 0.04016649351f, 0.09021852165f, 1.0f,
                              0.01537752338f, 0.2087130845f, 0.9756000042f, 1.0f,
                              1.0f, 0.0f, 0.2512601018f, 1.0f
                            };
    float color[12];
    
    // Check nearest
    memcpy(color, reference, 12*sizeof(float));
    OCIO::Lut3D_Nearest(color, 3, *lut);
    for(unsigned int i=0; i<12; ++i)
    {
        OIIO_CHECK_CLOSE(color[i], nearest[i], 1e-8);
    }
    
    // Check linear
    memcpy(color, reference, 12*sizeof(float));
    OCIO::Lut3D_Linear(color, 3, *lut);
    for(unsigned int i=0; i<12; ++i)
    {
        OIIO_CHECK_CLOSE(color[i], linear[i], 1e-8);
    }

    // Check tetrahedral
    memcpy(color, reference, 12*sizeof(float));
    OCIO::Lut3D_Tetrahedral(color, 3, *lut);
    for(unsigned int i=0; i<12; ++i)
    {
        OIIO_CHECK_CLOSE(color[i], linear[i], 1e-7); // Note, max delta lowered from 1e-8
    }
}



OIIO_ADD_TEST(Lut3DOp, InverseComparisonCheck)
{
    OCIO::Lut3DRcPtr lut_a = OCIO::Lut3D::Create();
    lut_a->from_min[0] = 0.0f;
    lut_a->from_min[1] = 0.0f;
    lut_a->from_min[2] = 0.0f;
    lut_a->from_max[0] = 1.0f;
    lut_a->from_max[1] = 1.0f;
    lut_a->from_max[2] = 1.0f;
    lut_a->size[0] = 32;
    lut_a->size[1] = 32;
    lut_a->size[2] = 32;
    lut_a->lut.resize(lut_a->size[0]*lut_a->size[1]*lut_a->size[2]*3);
    GenerateIdentityLut3D(&lut_a->lut[0], lut_a->size[0], 3, OCIO::LUT3DORDER_FAST_RED);
    
    OCIO::Lut3DRcPtr lut_b = OCIO::Lut3D::Create();
    lut_b->from_min[0] = 0.5f;
    lut_b->from_min[1] = 0.5f;
    lut_b->from_min[2] = 0.5f;
    lut_b->from_max[0] = 1.0f;
    lut_b->from_max[1] = 1.0f;
    lut_b->from_max[2] = 1.0f;
    lut_b->size[0] = 32;
    lut_b->size[1] = 32;
    lut_b->size[2] = 32;
    lut_b->lut.resize(lut_b->size[0]*lut_b->size[1]*lut_b->size[2]*3);
    GenerateIdentityLut3D(&lut_b->lut[0], lut_b->size[0], 3, OCIO::LUT3DORDER_FAST_RED);
    
    OCIO::OpRcPtrVec ops;
    CreateLut3DOp(ops, lut_a, OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD);
    CreateLut3DOp(ops, lut_a, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE);
    CreateLut3DOp(ops, lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    CreateLut3DOp(ops, lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE);
    
    OIIO_CHECK_EQUAL(ops.size(), 4);
    
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[2]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[3]->clone()));
    
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[3]), false);
    OIIO_CHECK_EQUAL( ops[2]->isInverse(ops[3]), true);
}


OIIO_ADD_TEST(Lut3DOp, PerformanceCheck)
{
    /*
    OCIO::Lut3D lut;
    
    lut.from_min[0] = 0.0f;
    lut.from_min[1] = 0.0f;
    lut.from_min[2] = 0.0f;
    
    lut.from_max[0] = 1.0f;
    lut.from_max[1] = 1.0f;
    lut.from_max[2] = 1.0f;
    
    lut.size[0] = 32;
    lut.size[1] = 32;
    lut.size[2] = 32;
    
    lut.lut.resize(lut.size[0]*lut.size[1]*lut.size[2]*3);
    GenerateIdentityLut3D(&lut.lut[0], lut.size[0], 3, OCIO::LUT3DORDER_FAST_RED);
    
    std::vector<float> img;
    int xres = 2048;
    int yres = 1;
    int channels = 4;
    img.resize(xres*yres*channels);
    
    srand48(0);
    
    // create random values from -0.05 to 1.05
    // (To simulate clipping performance)
    
    for(unsigned int i=0; i<img.size(); ++i)
    {
        float uniform = (float)drand48();
        img[i] = uniform*1.1f - 0.05f;
    }
    
    timeval t;
    gettimeofday(&t, 0);
    double starttime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    
    int numloops = 1024;
    for(int i=0; i<numloops; ++i)
    {
        //OCIO::Lut3D_Nearest(&img[0], xres*yres, lut);
        OCIO::Lut3D_Linear(&img[0], xres*yres, lut);
    }
    
    gettimeofday(&t, 0);
    double endtime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    double totaltime_a = (endtime-starttime)/numloops;
    
    printf("Linear: %0.1f ms  - %0.1f fps\n", totaltime_a*1000.0, 1.0/totaltime_a);


    // Tetrahedral
    gettimeofday(&t, 0);
    starttime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;

    for(int i=0; i<numloops; ++i)
    {
        OCIO::Lut3D_Tetrahedral(&img[0], xres*yres, lut);
    }

    gettimeofday(&t, 0);
    endtime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    double totaltime_b = (endtime-starttime)/numloops;

    printf("Tetra: %0.1f ms  - %0.1f fps\n", totaltime_b*1000.0, 1.0/totaltime_b);

    double speed_diff = totaltime_a/totaltime_b;
    printf("Tetra is %.04f speed of Linear\n", speed_diff);
    */
}
#endif // OCIO_UNIT_TEST
