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
#include "GpuShaderUtils.h"
#include "MatrixOps.h"

#include "opdata/OpDataInvLut3D.h"

#include "cpu/CPULut3DOp.h"
#include "cpu/CPUInvLut3DOp.h"
#include "cpu/CPULutUtils.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <algorithm>

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
        
        md5_append(&state, (const md5_byte_t *)from_min, (int)(3*sizeof(float)));
        md5_append(&state, (const md5_byte_t *)from_max, (int)(3*sizeof(float)));
        md5_append(&state, (const md5_byte_t *)size,     (int)(3*sizeof(int)));
        md5_append(&state, (const md5_byte_t *)&lut[0],  (int)(lut.size()*sizeof(float)));
        
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
            return simple_rgb_lut[GetLut3DIndex_RedFast(rIndex, gIndex, bIndex,
                size_red, size_green, size_blue) + channelIndex];
        }
        
        inline void lookupNearest_3D_rgb(float* rgb,
                                         int rIndex, int gIndex, int bIndex,
                                         int size_red, int size_green, int size_blue,
                                         const float* simple_rgb_lut)
        {
            int offset = GetLut3DIndex_RedFast(rIndex, gIndex, bIndex, size_red, size_green, size_blue);
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
        
#ifdef OCIO_UNIT_TEST
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
#endif
    }
    
#ifdef OCIO_UNIT_TEST
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
                const int n000 =
                    GetLut3DIndex_RedFast(indexLow[0], indexLow[1], indexLow[2],
                                          lutSize[0], lutSize[1], lutSize[2]);
                const int n100 =
                    GetLut3DIndex_RedFast(indexHigh[0], indexLow[1], indexLow[2],
                                          lutSize[0], lutSize[1], lutSize[2]);
                const int n010 =
                    GetLut3DIndex_RedFast(indexLow[0], indexHigh[1], indexLow[2],
                                          lutSize[0], lutSize[1], lutSize[2]);
                const int n001 =
                    GetLut3DIndex_RedFast(indexLow[0], indexLow[1], indexHigh[2],
                                          lutSize[0], lutSize[1], lutSize[2]);
                const int n110 =
                    GetLut3DIndex_RedFast(indexHigh[0], indexHigh[1], indexLow[2],
                                          lutSize[0], lutSize[1], lutSize[2]);
                const int n101 =
                    GetLut3DIndex_RedFast(indexHigh[0], indexLow[1], indexHigh[2],
                                          lutSize[0], lutSize[1], lutSize[2]);
                const int n011 =
                    GetLut3DIndex_RedFast(indexLow[0], indexHigh[1], indexHigh[2],
                                          lutSize[0], lutSize[1], lutSize[2]);
                const int n111 =
                    GetLut3DIndex_RedFast(indexHigh[0], indexHigh[1], indexHigh[2],
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
#endif

    void GenerateIdentityLut3D(float* img, int edgeLen, int numChannels, Lut3DOrder lut3DOrder)
    {
        if(!img) return;
        if(numChannels < 3)
        {
            throw Exception("Cannot generate idenitity 3d LUT with less than 3 channels.");
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
            os << "Cannot infer 3D LUT size. ";
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
            Lut3DOp(OpData::OpDataLut3DRcPtr & data);
            virtual ~Lut3DOp();

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
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;

            virtual bool supportedByLegacyShader() const { return false; }
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

            OpData::OpDataLut3DRcPtr m_data;

        private:
            // Lut3DOp does not record the direction. It can only be forward.

            // The computed cache identifier
            std::string m_cacheID;
            // The CPU processor
            CPUOpRcPtr m_cpu;

            Lut3DOp();
        };

        typedef OCIO_SHARED_PTR<Lut3DOp> Lut3DOpRcPtr;

        class InvLut3DOp : public Op
        {
        public:
            InvLut3DOp(OpData::OpDataInvLut3DRcPtr & data);
            virtual ~InvLut3DOp();

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
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;

            virtual bool supportedByLegacyShader() const { return false; }
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

            OpData::OpDataInvLut3DRcPtr m_data;

        private:
            // InvLut3DOp does not record the direction. It can only be forward
            // (ie an inverse Lut3DOp)

            // The computed cache identifier
            std::string m_cacheID;
            // The CPU processor
            CPUOpRcPtr m_cpu;

            InvLut3DOp();
        };

        typedef OCIO_SHARED_PTR<InvLut3DOp> InvLut3DOpRcPtr;

        Lut3DOp::Lut3DOp(OpData::OpDataLut3DRcPtr & data)
            : Op()
            , m_data(data)
            , m_cpu(new CPUNoOp)
        {
        }

        Lut3DOp::~Lut3DOp()
        {
        }

        OpRcPtr Lut3DOp::clone() const
        {
            OpData::OpDataLut3DRcPtr
                lut(dynamic_cast<OpData::Lut3D*>(
                    m_data->clone(OpData::OpData::DO_DEEP_COPY)));

            return OpRcPtr(new Lut3DOp(lut));
        }

        std::string Lut3DOp::getInfo() const
        {
            return "<Lut3DOp>";
        }

        std::string Lut3DOp::getCacheID() const
        {
            return m_cacheID;
        }

        BitDepth Lut3DOp::getInputBitDepth() const
        {
            return m_data->getInputBitDepth();
        }

        BitDepth Lut3DOp::getOutputBitDepth() const
        {
            return m_data->getOutputBitDepth();
        }

        void Lut3DOp::setInputBitDepth(BitDepth bitdepth)
        {
            m_data->setInputBitDepth(bitdepth);
        }

        void Lut3DOp::setOutputBitDepth(BitDepth bitdepth)
        {
            m_data->setOutputBitDepth(bitdepth);
        }

        bool Lut3DOp::isNoOp() const
        {
            return m_data->isNoOp();
        }

        bool Lut3DOp::isIdentity() const
        {
            return m_data->isIdentity();
        }

        bool Lut3DOp::isSameType(const OpRcPtr & op) const
        {
            // NB: InvLut3D and Lut3D have the same type.
            //     One is the inverse of the other one.
            const Lut3DOpRcPtr fwdTypedRcPtr = DynamicPtrCast<Lut3DOp>(op);
            const InvLut3DOpRcPtr invTypedRcPtr = DynamicPtrCast<InvLut3DOp>(op);
            return (bool)fwdTypedRcPtr || (bool)invTypedRcPtr;
        }

        bool Lut3DOp::isInverse(const OpRcPtr & op) const
        {
            InvLut3DOpRcPtr typedRcPtr = DynamicPtrCast<InvLut3DOp>(op);
            if (typedRcPtr)
            {
                // m_data is OpData::Lut3D (and not OpData:InvLut3D)
                // typedRcPtr->m_data is OpData:InvLut3D
                return m_data->isInverse(*typedRcPtr->m_data);
            }

            return false;
        }

        bool Lut3DOp::hasChannelCrosstalk() const
        {
            return m_data->hasChannelCrosstalk();
        }

        void Lut3DOp::finalize()
        {
            // TODO: Only the 32f processing is natively supported
            m_data->setInputBitDepth(BIT_DEPTH_F32);
            m_data->setOutputBitDepth(BIT_DEPTH_F32);

            m_data->validate();

            m_cpu = Lut3DRenderer::GetRenderer(m_data);


            // Rebuild the cache identifier
            //

            md5_state_t state;
            md5_byte_t digest[16];

            md5_init(&state);
            md5_append(&state,
                (const md5_byte_t *)&(m_data->getArray().getValues()[0]),
                (int)(m_data->getArray().getValues().size() * sizeof(float)));
            md5_finish(&state, digest);

            m_cacheID = GetPrintableHash(digest);

            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut3D ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << BitDepthToString(m_data->getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(m_data->getOutputBitDepth()) << " ";
            cacheIDStream << ">";

            m_cacheID = cacheIDStream.str();
        }

        void Lut3DOp::apply(float* rgbaBuffer, long numPixels) const
        {
            m_cpu->apply(rgbaBuffer, (unsigned int)numPixels);
        }

        void Lut3DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if(getInputBitDepth()!=BIT_DEPTH_F32 || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            std::ostringstream resName;
            resName << shaderDesc->getResourcePrefix()
                    << std::string("lut3d_")
                    << shaderDesc->getNum3DTextures();

            const std::string name(resName.str());

            shaderDesc->add3DTexture(GpuShaderText::getSamplerName(name).c_str(),
                                     m_cacheID.c_str(), m_data->getGridSize(),
                                     m_data->getConcreteInterpolation(), &m_data->getArray()[0]);

            {
                GpuShaderText ss(shaderDesc->getLanguage());
                ss.declareTex3D(name);
                shaderDesc->addToDeclareShaderCode(ss.string().c_str());
            }


            const float dim = (float)m_data->getGridSize();

            // incr = 1/dim (amount needed to increment one index in the grid)
            const float incr = 1.0f / dim;

            {
                GpuShaderText ss(shaderDesc->getLanguage());
                ss.indent();

                ss.newLine() << "";
                ss.newLine() << "// Add a LUT 3D processing for " << name;
                ss.newLine() << "";


                // Tetrahedral interpolation
                // The strategy is to use texture3d lookups with GL_NEAREST to fetch the
                // 4 corners of the cube (v1,v2,v3,v4), compute the 4 barycentric weights
                // (f1,f2,f3,f4), and then perform the interpolation manually.
                // One side benefit of this is that we are not subject to the 8-bit
                // quantization of the fractional weights that happens using GL_LINEAR.
                if (m_data->getConcreteInterpolation() == INTERP_TETRAHEDRAL)
                {
                    ss.newLine() << "{";
                    ss.indent();

                    ss.newLine() << ss.vec3fDecl("coords") << " = "
                                 << shaderDesc->getPixelName() << ".rgb * "
                                 << ss.vec3fConst(dim - 1) << "; ";

                    // baseInd is on [0,dim-1]
                    ss.newLine() << ss.vec3fDecl("baseInd") << " = floor(coords);";

                    // frac is on [0,1]
                    ss.newLine() << ss.vec3fDecl("frac") << " = coords - baseInd;";

                    // scale/offset baseInd onto [0,1] as usual for doing texture lookups
                    // we use zyx to flip the order since blue varies most rapidly
                    // in the grid array ordering
                    ss.newLine() << ss.vec3fDecl("f1, f4") << ";";

                    ss.newLine() << "baseInd = ( baseInd.zyx + " << ss.vec3fConst(0.5f) << " ) / " << ss.vec3fConst(dim) << ";";
                    ss.newLine() << ss.vec3fDecl("v1") << " = " << ss.sampleTex3D(name, "baseInd") << ".rgb;";

                    ss.newLine() << ss.vec3fDecl("nextInd") << " = baseInd + " << ss.vec3fConst(incr) << ";";
                    ss.newLine() << ss.vec3fDecl("v4") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

                    ss.newLine() << "if (frac.r >= frac.g)";
                    ss.newLine() << "{";
                    ss.indent();
                    ss.newLine() <<     "if (frac.g >= frac.b)";  // R > G > B
                    ss.newLine() <<     "{";
                    ss.indent();       
                                            // Note that compared to the CPU version of the algorithm,
                                            // we increment in inverted order since baseInd & nextInd
                                            // are essentially BGR rather than RGB.
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(0.0f, 0.0f, incr) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, incr) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "f1 = " << ss.vec3fConst("1. - frac.r") << ";";
                    ss.newLine() <<         "f4 = " << ss.vec3fConst("frac.b") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.r - frac.g") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.g - frac.b") << ";";
                                            
                    ss.newLine() <<         shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
                    ss.dedent();            
                    ss.newLine() <<     "}";
                    ss.newLine() <<     "else if (frac.r >= frac.b)";  // R > B > G
                    ss.newLine() <<     "{";
                    ss.indent();          
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(0.0f, 0.0f, incr) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, incr) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "f1 = " << ss.vec3fConst("1. - frac.r") << ";";
                    ss.newLine() <<         "f4 = " << ss.vec3fConst("frac.g") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.r - frac.b") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.b - frac.g") << ";";
                                            
                    ss.newLine() <<         shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
                    ss.dedent();            
                    ss.newLine() <<     "}";
                    ss.newLine() <<     "else";  // B > R > G
                    ss.newLine() <<     "{";
                    ss.indent();        
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, 0.0f) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, incr) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "f1 = " << ss.vec3fConst("1. - frac.b") << ";";
                    ss.newLine() <<         "f4 = " << ss.vec3fConst("frac.g") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.b - frac.r") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.r - frac.g") << ";";
                                            
                    ss.newLine() <<         shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
                    ss.dedent();            
                    ss.newLine() <<     "}";
                    ss.dedent();      
                    ss.newLine() << "}";
                    ss.newLine() << "else";
                    ss.newLine() << "{";
                    ss.indent();        
                    ss.newLine() <<     "if (frac.g <= frac.b)";  // B > G > R
                    ss.newLine() <<     "{";
                    ss.indent();        
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, 0.0f) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(incr, incr, 0.0f) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "f1 = " << ss.vec3fConst("1. - frac.b") << ";";
                    ss.newLine() <<         "f4 = " << ss.vec3fConst("frac.r") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.b - frac.g") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.g - frac.r") << ";";
                                            
                    ss.newLine() <<         shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
                    ss.dedent();          
                    ss.newLine() <<     "}";
                    ss.newLine() <<     "else if (frac.r >= frac.b)";  // G > R > B
                    ss.newLine() <<     "{";
                    ss.indent();        
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, 0.0f) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, incr) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "f1 = " << ss.vec3fConst("1. - frac.g") << ";";
                    ss.newLine() <<         "f4 = " << ss.vec3fConst("frac.b") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.g - frac.r") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.r - frac.b") << ";";
                                            
                    ss.newLine() <<         shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
                    ss.dedent();            
                    ss.newLine() <<     "}";
                    ss.newLine() <<     "else";  // G > B > R
                    ss.newLine() <<     "{";
                    ss.indent();        
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, 0.0f) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "nextInd = baseInd + " << ss.vec3fConst(incr, incr, 0.0f) << ";";
                    ss.newLine() <<         ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";
                                            
                    ss.newLine() <<         "f1 = " << ss.vec3fConst("1. - frac.g") << ";";
                    ss.newLine() <<         "f4 = " << ss.vec3fConst("frac.r") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.g - frac.b") << ";";
                    ss.newLine() <<         ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.b - frac.r") << ";";
                                            
                    ss.newLine() <<         shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
                    ss.dedent();            
                    ss.newLine() <<     "}";
                    ss.dedent();
                    ss.newLine() << "}";

                    ss.newLine() << shaderDesc->getPixelName()
                                 << ".rgb = "
                                 << shaderDesc->getPixelName()
                                 << ".rgb + (f1 * v1) + (f4 * v4);";

                    ss.dedent();
                    ss.newLine() << "}";
                }
                else
                {
                    // Trilinear interpolation
                    // Use texture3d and GL_LINEAR and the GPU's built-in trilinear algorithm.
                    // Note that the fractional components are quantized to 8-bits on current
                    // hardware, which introduces significant error with small grid sizes.

                    ss.newLine() << ss.vec3fDecl(name + "_coords")
                                 << " = (" << shaderDesc->getPixelName() << ".zyx * "
                                 << ss.vec3fConst(dim - 1) << " + "
                                 << ss.vec3fConst(0.5f) + ") / " 
                                 << ss.vec3fConst(dim) << ";";

                    ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                                 << ss.sampleTex3D(name, name + "_coords") << ".rgb;";
                }
                

                shaderDesc->addToFunctionShaderCode(ss.string().c_str());
            }

        }


        InvLut3DOp::InvLut3DOp(OpData::OpDataInvLut3DRcPtr & data)
            : Op()
            , m_data(data)
            , m_cpu(new CPUNoOp)
        {
        }

        InvLut3DOp::~InvLut3DOp()
        {
        }

        OpRcPtr InvLut3DOp::clone() const
        {
            OpData::OpDataInvLut3DRcPtr
                lut(dynamic_cast<OpData::InvLut3D*>(
                    m_data->clone(OpData::OpData::DO_DEEP_COPY)));

            return OpRcPtr(new InvLut3DOp(lut));
        }

        std::string InvLut3DOp::getInfo() const
        {
            return "<InvLut3DOp>";
        }

        std::string InvLut3DOp::getCacheID() const
        {
            return m_cacheID;
        }

        BitDepth InvLut3DOp::getInputBitDepth() const
        {
            return m_data->getInputBitDepth();
        }

        BitDepth InvLut3DOp::getOutputBitDepth() const
        {
            return m_data->getOutputBitDepth();
        }

        void InvLut3DOp::setInputBitDepth(BitDepth bitdepth)
        {
            m_data->setInputBitDepth(bitdepth);
        }

        void InvLut3DOp::setOutputBitDepth(BitDepth bitdepth)
        {
            m_data->setOutputBitDepth(bitdepth);
        }

        bool InvLut3DOp::isNoOp() const
        {
            return m_data->isNoOp();
        }

        bool InvLut3DOp::isIdentity() const
        {
            return m_data->isIdentity();
        }

        bool InvLut3DOp::isSameType(const OpRcPtr & op) const
        {
            // NB: InvLut3D and Lu31D have the same type.
            //     One is the inverse of the other one.
            const Lut3DOpRcPtr fwdTypedRcPtr = DynamicPtrCast<Lut3DOp>(op);
            const InvLut3DOpRcPtr invTypedRcPtr = DynamicPtrCast<InvLut3DOp>(op);
            return (bool)fwdTypedRcPtr || (bool)invTypedRcPtr;
        }

        bool InvLut3DOp::isInverse(const OpRcPtr & op) const
        {
            Lut3DOpRcPtr typedRcPtr = DynamicPtrCast<Lut3DOp>(op);
            if (typedRcPtr)
            {
                // m_data is OpData::InvLut3D
                // typedRcPtr->m_data is OpData:Lut3D (and not OpData:InvLut3D)
                return m_data->isInverse(*typedRcPtr->m_data);
            }

            return false;
        }

        bool InvLut3DOp::hasChannelCrosstalk() const
        {
            return m_data->hasChannelCrosstalk();
        }

        void InvLut3DOp::finalize()
        {
            // Only the 32f processing is natively supported
            m_data->setInputBitDepth(BIT_DEPTH_F32);
            m_data->setOutputBitDepth(BIT_DEPTH_F32);

            m_data->validate();

            // Get the CPU engine
            m_cpu = InvLut3DRenderer::GetRenderer(m_data);

            // Rebuild the cache identifier
            //

            md5_state_t state;
            md5_byte_t digest[16];

            md5_init(&state);
            md5_append(&state,
                (const md5_byte_t *)&(m_data->getArray().getValues()[0]),
                (int)(m_data->getArray().getValues().size() * sizeof(float)));
            md5_finish(&state, digest);

            m_cacheID = GetPrintableHash(digest);

            std::ostringstream cacheIDStream;
            cacheIDStream << "<InvLut1D ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << BitDepthToString(m_data->getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(m_data->getOutputBitDepth()) << " ";
            cacheIDStream << ">";

            m_cacheID = cacheIDStream.str();
        }

        void InvLut3DOp::apply(float* rgbaBuffer, long numPixels) const
        {
            m_cpu->apply(rgbaBuffer, (unsigned int)numPixels);
        }

        void InvLut3DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            OpData::OpDataLut3DRcPtr
                newLut = InvLutUtil::makeFastLut3D(m_data);

            OpRcPtrVec ops;
            CreateLut3DOp(ops, newLut, TRANSFORM_DIR_FORWARD);
            if (ops.size() != 1)
            {
                throw Exception("Cannot apply Lut3DOp, optimization failed.");
            }
            ops[0]->finalize();
            ops[0]->extractGpuShaderInfo(shaderDesc);
        }

    }
    
    void CreateLut3DOp(OpRcPtrVec & ops,
                       Lut3DRcPtr lut,
                       Interpolation interpolation,
                       TransformDirection direction)
    {
        if (direction != TRANSFORM_DIR_FORWARD
            && direction != TRANSFORM_DIR_INVERSE)
        {
            throw Exception("Cannot apply Lut3DOp op, "
                "unspecified transform direction.");
        }
        if (interpolation != INTERP_NEAREST
            && interpolation != INTERP_LINEAR
            && interpolation != INTERP_TETRAHEDRAL
            && interpolation != INTERP_DEFAULT
            && interpolation != INTERP_BEST)
        {
            throw Exception("Cannot apply LUT 3D op, "
                "invalid interpolation specified.");
        }
        if (!lut || lut->size[0]<2 
            || lut->size[0] != lut->size[1]
            || lut->size[0] != lut->size[2])
        {
            throw Exception("Cannot apply Lut3DOp op, "
                "invalid lut specified.");
        }

        // TODO: code assumes that LUT coming in are LUT3DORDER_FAST_RED
        // Need to transform to Blue fast for OpData::Lut3D
        unsigned lutSize = (unsigned)lut->size[0];
        OpData::OpDataLut3DRcPtr lutBF(new OpData::Lut3D(lutSize));
        lutBF->setInterpolation(interpolation);

        OpData::Array & lutArray = lutBF->getArray();

        for (unsigned b = 0; b < lutSize; ++b)
        {
            for (unsigned g = 0; g < lutSize; ++g)
            {
                for (unsigned r = 0; r < lutSize; ++r)
                {
                    // OpData::Lut3D Array index. b changes fastest.
                    const unsigned arrayIdx = 3 * ((r*lutSize + g)*lutSize + b);

                    // Lut3D struct index. r changes fastest.
                    const unsigned ocioIdx = 3 * ((b*lutSize + g)*lutSize + r);

                    lutArray[arrayIdx] = lut->lut[ocioIdx];
                    lutArray[arrayIdx + 1] = lut->lut[ocioIdx + 1];
                    lutArray[arrayIdx + 2] = lut->lut[ocioIdx + 2];
                }
            }
        }

        if (direction == TRANSFORM_DIR_FORWARD)
        {
            CreateMatrixOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_FORWARD);
            CreateLut3DOp(ops, lutBF, TRANSFORM_DIR_FORWARD);
        }
        else
        {
            CreateLut3DOp(ops, lutBF, TRANSFORM_DIR_INVERSE);
            CreateMatrixOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_INVERSE);
        }
    }


    void CreateLut3DOp(OpRcPtrVec & ops,
        OpData::OpDataLut3DRcPtr & lut,
        TransformDirection direction)
    {
        if (lut->isNoOp()) return;

        if (direction != TRANSFORM_DIR_FORWARD
            && direction != TRANSFORM_DIR_INVERSE)
        {
            throw Exception("Cannot apply Lut3DOp op, "
                "unspecified transform direction.");
        }

        if (lut->getOpType() != OpData::OpData::Lut3DType)
        {
            if (lut->getOpType() == OpData::OpData::InvLut3DType)
            {
                OpData::OpDataInvLut3DRcPtr typedRcPtr
                    = DynamicPtrCast<OpData::InvLut3D>(lut);
                CreateInvLut3DOp(ops, typedRcPtr, direction);
                return;
            }
            throw Exception("Cannot apply Lut3DOp op, "
                "Not a forward LUT 3D data");
        }

        if (direction == TRANSFORM_DIR_FORWARD)
        {
            ops.push_back(OpRcPtr(new Lut3DOp(lut)));
        }
        else
        {
            OpData::OpDataInvLut3DRcPtr data(new OpData::InvLut3D(*lut));
            ops.push_back(OpRcPtr(new InvLut3DOp(data)));
        }
    }

    void CreateInvLut3DOp(OpRcPtrVec & ops,
        OpData::OpDataInvLut3DRcPtr & lut,
        TransformDirection direction)
    {
        if (lut->isNoOp()) return;

        if (direction != TRANSFORM_DIR_FORWARD
            && direction != TRANSFORM_DIR_INVERSE)
        {
            throw Exception("Cannot apply Lut3DOp op, "
                "unspecified transform direction.");
        }

        if (lut->getOpType() != OpData::OpData::InvLut3DType)
        {
            throw Exception("Cannot apply InvLut3DOp op, "
                "Not a inverse LUT 3D data");
        }

        if (direction == TRANSFORM_DIR_FORWARD)
        {
            ops.push_back(OpRcPtr(new InvLut3DOp(lut)));
        }
        else
        {
            OpData::OpDataLut3DRcPtr data(new OpData::Lut3D(*lut));
            ops.push_back(OpRcPtr(new Lut3DOp(data)));
        }
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
#include "OpBuilders.h"
#include "opdata/OpDataTools.h"

OIIO_ADD_TEST(Lut3DOpStruct, NanInfValueCheck)
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
    
    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));
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


OIIO_ADD_TEST(Lut3DOpStruct, ValueCheck)
{
    const float error = 1e-5f;

    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();

    for (int i = 0; i < 3; ++i)
    {
        OIIO_CHECK_CLOSE(lut->from_min[i], 0.0f, error);
        OIIO_CHECK_CLOSE(lut->from_max[i], 1.0f, error);
    }

    lut->size[0] = 32;
    lut->size[1] = 32;
    lut->size[2] = 32;

    lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);
    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));
    for (unsigned int i = 0; i < lut->lut.size(); ++i)
    {
        lut->lut[i] = powf(lut->lut[i], 2.0f);
    }

    const float reference[] = { 0.0f, 0.2f, 0.3f, 1.0f,
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
    const float tetrahedral[] = { 0.0f, 0.0401664972f, 0.0902185217f,
                                 1.0f, 0.0153775234f, 0.208713099f,
                                 0.975600004f, 1.0f, 1.0f,
                                 0.0f, 0.251260102f, 1.0f
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
        OIIO_CHECK_CLOSE(color[i], tetrahedral[i], 1e-8);
    }
}



OIIO_ADD_TEST(Lut3DOpStruct, InverseComparisonCheck)
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
    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_a->lut[0], lut_a->size[0], 3, OCIO::LUT3DORDER_FAST_RED));
    
    OCIO::Lut3DRcPtr lut_b = OCIO::Lut3D::Create();
    lut_b->from_min[0] = 0.5f;
    lut_b->from_min[1] = 0.5f;
    lut_b->from_min[2] = 0.5f;
    lut_b->from_max[0] = 1.0f;
    lut_b->from_max[1] = 1.0f;
    lut_b->from_max[2] = 1.0f;
    lut_b->size[0] = 16;
    lut_b->size[1] = 16;
    lut_b->size[2] = 16;
    lut_b->lut.resize(lut_b->size[0]*lut_b->size[1]*lut_b->size[2]*3);
    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_b->lut[0], lut_b->size[0], 3, OCIO::LUT3DORDER_FAST_RED));
    
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_a, OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_a, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));

    // Add Matrix and LUT
    OIIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    
    // Add LUT and Matrix
    OIIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    
    OIIO_CHECK_EQUAL(ops.size(), 6);
    
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[3]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[4]->clone()));
    
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[3]), false);
    
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[4]), false);
    OIIO_CHECK_EQUAL( ops[3]->isInverse(ops[4]), true);
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

    lut.lut.resize(lut.size[0]*lut.size[1]*?lut.size[2]*3);
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
        ?//OCIO::Lut3D_Nearest(&img[0], xres*yres, lut);
        OCIO::Lut3D_Linear(&img[0], xres*yres, lut);
    }

    gettimeofday(&t, 0);
    double endtime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    double totaltime_a = (endtime-starttime)/numloops;

    printf("Linear: ?%0.1f ms  - ?%0.1f fps\n", totaltime_a*1000.0, 1.0/totaltime_a);


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

    printf("Tetra: ?%0.1f ms  - ?%0.1f fps\n", totaltime_b*1000.0, 1.0/totaltime_b);

    double speed_diff = totaltime_a/totaltime_b;
    printf("Tetra is ?%.04f speed of Linear\n", speed_diff);
*/
}

OIIO_ADD_TEST(Lut3DOpStruct, ThrowLut)
{
    // std::string Lut3D::getCacheID() const when LUT is empty
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    OIIO_CHECK_THROW_WHAT(lut->getCacheID(), OCIO::Exception, "invalid Lut3D");

    // GenerateIdentityLut3D with less than 3 channels
    lut->size[0] = 3;
    lut->size[1] = 3;
    lut->size[2] = 3;

    lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);

    OIIO_CHECK_THROW_WHAT(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 2, OCIO::LUT3DORDER_FAST_RED),
        OCIO::Exception, "less than 3 channels");

    // GenerateIdentityLut3D with unknown order
    OIIO_CHECK_THROW_WHAT(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, (OCIO::Lut3DOrder)42),
        OCIO::Exception, "Unknown Lut3DOrder");

    // Get3DLutEdgeLenFromNumPixels with not cubic size
    OIIO_CHECK_THROW_WHAT(OCIO::Get3DLutEdgeLenFromNumPixels(10),
        OCIO::Exception, "Cannot infer 3D LUT size");
}

OIIO_ADD_TEST(Lut3DOpStruct, ThrowLutOp)
{
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    lut->size[0] = 3;
    lut->size[1] = 3;
    lut->size[2] = 3;
    lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);

    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    // Inverse
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    ops.clear();

    // only valid interpolation
    OIIO_CHECK_THROW_WHAT(CreateLut3DOp(ops,
        lut, OCIO::INTERP_UNKNOWN, OCIO::TRANSFORM_DIR_FORWARD), OCIO::Exception, "invalid interpolation");
    ops.clear();

}

OIIO_ADD_TEST(Lut3DOpStruct, cacheID)
{
    OCIO::OpRcPtrVec ops;
    for (int i = 0; i < 2; ++i)
    {
        OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
        lut->size[0] = 3;
        lut->size[1] = 3;
        lut->size[2] = 3;
        lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);

        OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
            &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

        OIIO_CHECK_NO_THROW(CreateLut3DOp(ops,
            lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    }

    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops));

    const std::string cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);
    // same LUT have the same cacheID
    OIIO_CHECK_EQUAL(cacheID, ops[1]->getCacheID());
}

OIIO_ADD_TEST(Lut3DOpStruct, EdgeLenFromNumPixels)
{
    OIIO_CHECK_THROW_WHAT(OCIO::Get3DLutEdgeLenFromNumPixels(10),
        OCIO::Exception, "Cannot infer 3D LUT size");
    int expectedRes = 33;
    int res = 0;
    OIIO_CHECK_NO_THROW(res = OCIO::Get3DLutEdgeLenFromNumPixels(
        expectedRes*expectedRes*expectedRes));
    OIIO_CHECK_EQUAL(res, expectedRes);

    expectedRes = 1290; // maximum value such that v^3 is still an int
    OIIO_CHECK_NO_THROW(res = OCIO::Get3DLutEdgeLenFromNumPixels(
        expectedRes*expectedRes*expectedRes));
    OIIO_CHECK_EQUAL(res, expectedRes);
}

OIIO_ADD_TEST(Lut3DOpStruct, Lut3DOrder)
{
    OCIO::Lut3DRcPtr lut_r = OCIO::Lut3D::Create();

    lut_r->from_min[0] = 0.0f;
    lut_r->from_min[1] = 0.0f;
    lut_r->from_min[2] = 0.0f;

    lut_r->from_max[0] = 1.0f;
    lut_r->from_max[1] = 1.0f;
    lut_r->from_max[2] = 1.0f;

    lut_r->size[0] = 3;
    lut_r->size[1] = 3;
    lut_r->size[2] = 3;

    lut_r->lut.resize(lut_r->size[0] * lut_r->size[1] * lut_r->size[2] * 3);

    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_r->lut[0], lut_r->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

    // first 3 values have red changing
    OIIO_CHECK_EQUAL(lut_r->lut[0], 0.0f);
    OIIO_CHECK_EQUAL(lut_r->lut[3], 0.5f);
    OIIO_CHECK_EQUAL(lut_r->lut[6], 1.0f);
    // blue is all 0
    OIIO_CHECK_EQUAL(lut_r->lut[2], 0.0f);
    OIIO_CHECK_EQUAL(lut_r->lut[5], 0.0f);
    OIIO_CHECK_EQUAL(lut_r->lut[8], 0.0f);
    // last 3 values have red changing
    OIIO_CHECK_EQUAL(lut_r->lut[72], 0.0f);
    OIIO_CHECK_EQUAL(lut_r->lut[75], 0.5f);
    OIIO_CHECK_EQUAL(lut_r->lut[78], 1.0f);
    // blue is all 1
    OIIO_CHECK_EQUAL(lut_r->lut[74], 1.0f);
    OIIO_CHECK_EQUAL(lut_r->lut[77], 1.0f);
    OIIO_CHECK_EQUAL(lut_r->lut[80], 1.0f);

    OCIO::Lut3DRcPtr lut_b = OCIO::Lut3D::Create();

    lut_b->from_min[0] = 0.0f;
    lut_b->from_min[1] = 0.0f;
    lut_b->from_min[2] = 0.0f;

    lut_b->from_max[0] = 1.0f;
    lut_b->from_max[1] = 1.0f;
    lut_b->from_max[2] = 1.0f;

    lut_b->size[0] = 3;
    lut_b->size[1] = 3;
    lut_b->size[2] = 3;

    lut_b->lut.resize(lut_b->size[0] * lut_b->size[1] * lut_b->size[2] * 3);

    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_b->lut[0], lut_b->size[0], 3, OCIO::LUT3DORDER_FAST_BLUE));

    // first 3 values have blue changing
    OIIO_CHECK_EQUAL(lut_b->lut[2], 0.0f);
    OIIO_CHECK_EQUAL(lut_b->lut[5], 0.5f);
    OIIO_CHECK_EQUAL(lut_b->lut[8], 1.0f);
    // red is all 0
    OIIO_CHECK_EQUAL(lut_b->lut[0], 0.0f);
    OIIO_CHECK_EQUAL(lut_b->lut[3], 0.0f);
    OIIO_CHECK_EQUAL(lut_b->lut[6], 0.0f);
    // last 3 values have blue changing
    OIIO_CHECK_EQUAL(lut_b->lut[74], 0.0f);
    OIIO_CHECK_EQUAL(lut_b->lut[77], 0.5f);
    OIIO_CHECK_EQUAL(lut_b->lut[80], 1.0f);
    // red is all 1
    OIIO_CHECK_EQUAL(lut_b->lut[72], 1.0f);
    OIIO_CHECK_EQUAL(lut_b->lut[75], 1.0f);
    OIIO_CHECK_EQUAL(lut_b->lut[78], 1.0f);

}

OIIO_ADD_TEST(Lut3DOp, ConvertLut)
{
    OCIO::Lut3DRcPtr lut_r = OCIO::Lut3D::Create();

    lut_r->from_min[0] = 0.0f;
    lut_r->from_min[1] = 0.0f;
    lut_r->from_min[2] = 0.0f;

    lut_r->from_max[0] = 1.0f;
    lut_r->from_max[1] = 1.0f;
    lut_r->from_max[2] = 1.0f;

    lut_r->size[0] = 3;
    lut_r->size[1] = 3;
    lut_r->size[2] = 3;

    lut_r->lut.resize(lut_r->size[0] * lut_r->size[1] * lut_r->size[2] * 3);

    OIIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_r->lut[0], lut_r->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_r, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));

    const OCIO::Lut3DOpRcPtr pLB = OCIO::DynamicPtrCast<OCIO::Lut3DOp>(ops[0]);

    // first 3 values have blue changing
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[2], 0.0f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[5], 0.5f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[8], 1.0f);
    // red is all 0     
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[0], 0.0f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[3], 0.0f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[6], 0.0f);
    // last 3 values have blue changing
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[74], 0.0f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[77], 0.5f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[80], 1.0f);
    // red is all 1     
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[72], 1.0f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[75], 1.0f);
    OIIO_CHECK_EQUAL(pLB->m_data->getArray().getValues()[78], 1.0f);
}

#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));


void BuildOps(const std::string & fileName,
    OCIO::OpRcPtrVec & fileOps)
{
    // This example uses a profile with a 1024-entry LUT for the TRC.
    const std::string filePath(ocioTestFilesDir + "/" + fileName);

    // Create a FileTransform
    OCIO::FileTransformRcPtr pFileTransform
        = OCIO::FileTransform::Create();
    // A transform file does not define any interpolation (contrary to config
    // file), this is to avoid exception when creating the operation.
    pFileTransform->setInterpolation(OCIO::INTERP_BEST);
    pFileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    pFileTransform->setSrc(filePath.c_str());

    // Create empty Config to use
    OCIO::ConfigRcPtr pConfig = OCIO::Config::Create();

    OCIO::ContextRcPtr pContext = OCIO::Context::Create();

    OCIO::BuildFileOps(fileOps, *(pConfig.get()), pContext,
        *(pFileTransform.get()), OCIO::TRANSFORM_DIR_FORWARD);
}

OIIO_ADD_TEST(Lut3DOp, Lut3D_compose)
{
    const std::string fileNameLut3d("lut3d_bizarre.clf");
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(BuildOps(fileNameLut3d, ops));
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, false));
    OIIO_CHECK_EQUAL(2, ops.size());

    OIIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
    OIIO_CHECK_EQUAL("<Lut3DOp>", ops[1]->getInfo());

    const std::string fileNameLut3d2("lut3d_example.clf");
    OIIO_CHECK_NO_THROW(BuildOps(fileNameLut3d2, ops));

    // TODO: BitDepth support: this is currently switching ops to 32F
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, false));

    OIIO_CHECK_EQUAL(4, ops.size());

    OIIO_CHECK_EQUAL("<FileNoOp>", ops[2]->getInfo());
    OIIO_CHECK_EQUAL("<Lut3DOp>", ops[3]->getInfo());

    const OCIO::Lut3DOpRcPtr pL0 = OCIO::DynamicPtrCast<OCIO::Lut3DOp>(ops[1]);
    const OCIO::Lut3DOpRcPtr pL1 = OCIO::DynamicPtrCast<OCIO::Lut3DOp>(ops[3]);

    OCIO::OpData::OpDataLut3DRcPtr pComposed = OCIO::OpData::Compose(pL0->m_data, pL1->m_data);

    const float error = 1e-6f;

    OIIO_CHECK_EQUAL(pComposed->getArray().getLength(), 17);
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[6], 0.0f, error);
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[7], 0.0100250980f, error);
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[8], 0.0644531101f, error);
    
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[8289], 0.3781897128f, error);
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[8290], 0.5108838081f, error);
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[8291], 0.5439114571f, error);

    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[14736], 0.9995302558f, error);
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[14737], 0.9941929579f, error);
    OIIO_CHECK_CLOSE(pComposed->getArray().getValues()[14738], 0.9985907078f, error);

    // TODO: when bitdepth support is available:
    // Check that if the connecting bit-depths don't match, it's an exception.
/*    try
    {
    // Tried to do this with a shared ptr but it prevented the catch from being called.
    std::auto_ptr<Color::Lut3DOp> pLe(Color::TransformTools::compose(*pL1, *pL0));
    }
    catch (Color::Exception )
    {
    return;
    }
    BOOST_CHECK(false);*/
}


OIIO_ADD_TEST(Lut3DOp, CPURendererLut3D)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'
    OCIO::OpData::OpDataLut3DRcPtr
        lutData(
            new OCIO::OpData::Lut3D(bitDepth, bitDepth,
                "", "", OCIO::OpData::Descriptions(),
                OCIO::INTERP_LINEAR, 33));

    OCIO::Lut3DOp lut(lutData);

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(lut.m_data->isIdentity());
    OIIO_CHECK_ASSERT(false == lut.isNoOp());

    const float step = OCIO::OpData::GetValueStepSize(
        lut.m_data->getInputBitDepth(),
        lut.m_data->getArray().getLength());
    
    float myImage[8] = { 0.0f, 0.0f, 0.0f, 1.0f,
                         0.0f, 0.0f, step, 1.0f };

    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, 2));

        OIIO_CHECK_EQUAL(myImage[0], 0.0f);
        OIIO_CHECK_EQUAL(myImage[1], 0.0f);
        OIIO_CHECK_EQUAL(myImage[2], 0.0f);
        OIIO_CHECK_EQUAL(myImage[3], 1.0f);

        OIIO_CHECK_EQUAL(myImage[4], 0.0f);
        OIIO_CHECK_EQUAL(myImage[5], 0.0f);
        OIIO_CHECK_EQUAL(myImage[6], step);
        OIIO_CHECK_EQUAL(myImage[7], 1.0f);
    }

    // No more an 'identity LUT 3D'
    const float arbitraryVal = 0.123456f;
    lut.m_data->getArray()[5] = arbitraryVal;

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(!lut.m_data->isIdentity());
    OIIO_CHECK_ASSERT(!lut.isNoOp());

    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, 2));

        OIIO_CHECK_EQUAL(myImage[0], 0.0f);
        OIIO_CHECK_EQUAL(myImage[1], 0.0f);
        OIIO_CHECK_EQUAL(myImage[2], 0.0f);
        OIIO_CHECK_EQUAL(myImage[3], 1.0f);

        OIIO_CHECK_EQUAL(myImage[4], 0.0f);
        OIIO_CHECK_EQUAL(myImage[5], 0.0f);
        OIIO_CHECK_EQUAL(myImage[6], arbitraryVal);
        OIIO_CHECK_EQUAL(myImage[7], 1.0f);
    }

    // Change interpolation
    lut.m_data->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(!lut.m_data->isIdentity());
    OIIO_CHECK_ASSERT(!lut.isNoOp());

    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, 2));

        OIIO_CHECK_EQUAL(myImage[0], 0.0f);
        OIIO_CHECK_EQUAL(myImage[1], 0.0f);
        OIIO_CHECK_EQUAL(myImage[2], 0.0f);
        OIIO_CHECK_EQUAL(myImage[3], 1.0f);

        OIIO_CHECK_EQUAL(myImage[4], 0.0f);
        OIIO_CHECK_EQUAL(myImage[5], 0.0f);
        OIIO_CHECK_EQUAL(myImage[6], arbitraryVal);
        OIIO_CHECK_EQUAL(myImage[7], 1.0f);
    }

}

/*BOOST_AUTO_TEST_CASE(CPURendererLUT3D_Blue)
{
    const std::string filename("/lut3d_blue.xml");

    Color::XMLTransformReader reader;

    try
    {
        reader.parseFile(path + filename);
    }
    catch (Color::Exception e)
    {
        BOOST_ERROR(e.getErrorString());
    }

    const boost::shared_ptr<Color::Transform>& pTransform
        = reader.getTransform();
    BOOST_REQUIRE(pTransform.use_count() == 1);

    const Color::Lut3DOp* pL
        = dynamic_cast<const Color::Lut3DOp*>(pTransform.get()->getOps().get(0));

    SYNCOLOR::PixelFormat pf = SYNCOLOR::PF_RGBA_16i;

    const uint16_t step
        = (uint16_t)(Color::Op::getValueStepSize(GET_BIT_DEPTH(pf),
            pL->getArray().getLength()));

    uint16_t myImage[] = { 0,    0,    0,    0,
        step, 0,    0,    0,
        0,    step, 0,    0,
        0,    0,    step, 0,
        step, step, step, 0 };

    pRenderer->finalize(pTransform, pf, pf);

    pRenderer->apply(myImage, myImage,
        SYNCOLOR::ROI(5, 1),
        0, 0);

    BOOST_CHECK_EQUAL(myImage[0], 0);
    BOOST_CHECK_EQUAL(myImage[1], 0);
    BOOST_CHECK_EQUAL(myImage[2], 0);
    BOOST_CHECK_EQUAL(myImage[3], 0);

    BOOST_CHECK_EQUAL(myImage[4], 0);
    BOOST_CHECK_EQUAL(myImage[5], 0);
    BOOST_CHECK_EQUAL(myImage[6], 0);
    BOOST_CHECK_EQUAL(myImage[7], 0);

    BOOST_CHECK_EQUAL(myImage[8], 0);
    BOOST_CHECK_EQUAL(myImage[9], 0);
    BOOST_CHECK_EQUAL(myImage[10], 0);
    BOOST_CHECK_EQUAL(myImage[11], 0);

    BOOST_CHECK_EQUAL(myImage[12], 0);
    BOOST_CHECK_EQUAL(myImage[13], 0);
    BOOST_CHECK_EQUAL(myImage[14], step);
    BOOST_CHECK_EQUAL(myImage[15], 0);

    BOOST_CHECK_EQUAL(myImage[16], 0);
    BOOST_CHECK_EQUAL(myImage[17], 0);
    BOOST_CHECK_EQUAL(myImage[18], step);
    BOOST_CHECK_EQUAL(myImage[19], 0);
}

BOOST_AUTO_TEST_CASE(CPURendererLUT3D_Green)
{
    const std::string filename("/lut3d_green.xml");

    Color::XMLTransformReader reader;

    try
    {
        reader.parseFile(path + filename);
    }
    catch (Color::Exception e)
    {
        BOOST_ERROR(e.getErrorString());
    }

    const boost::shared_ptr<Color::Transform>& pTransform
        = reader.getTransform();
    BOOST_REQUIRE(pTransform.use_count() == 1);

    const Color::Lut3DOp* pL
        = dynamic_cast<const Color::Lut3DOp*>(pTransform.get()->getOps().get(0));

    const SYNCOLOR::BitDepth inBD = SYNCOLOR::BIT_DEPTH_16i;
    const SYNCOLOR::BitDepth outBD = SYNCOLOR::BIT_DEPTH_32f;

    const uint16_t step
        = (uint16_t)(Color::Op::getValueStepSize(inBD,
            pL->getArray().getLength()));

    uint16_t myImage[] = { 0,    0,    0,    0,
        step, 0,    0,    0,
        0,    step, 0,    0,
        0,    0,    step, 0,
        step, step, step, 0 };

    float resImage[] = { -1., -1., -1., -1.,
        -1., -1., -1., -1.,
        -1., -1., -1., -1.,
        -1., -1., -1., -1.,
        -1., -1., -1., -1. };

    pRenderer->finalize(
        pTransform,
        (SYNCOLOR::PixelFormat)(Color::Utils::PXL_LAYOUT_RGBA | inBD),
        (SYNCOLOR::PixelFormat)(Color::Utils::PXL_LAYOUT_RGBA | outBD));

    pRenderer->apply(myImage, resImage,
        SYNCOLOR::ROI(5, 1),
        0, 0);

    const float scaleFactor
        = SYNCOLOR::getBitDepthValueRange(outBD)
        / SYNCOLOR::getBitDepthValueRange(inBD);

    BOOST_CHECK_EQUAL(resImage[0], 0.f);
    BOOST_CHECK_EQUAL(resImage[1], 0.f);
    BOOST_CHECK_EQUAL(resImage[2], 0.f);
    BOOST_CHECK_EQUAL(resImage[3], 0.f);

    BOOST_CHECK_EQUAL(resImage[4], 0.f);
    BOOST_CHECK_EQUAL(resImage[5], 0.f);
    BOOST_CHECK_EQUAL(resImage[6], 0.f);
    BOOST_CHECK_EQUAL(resImage[7], 0.f);

    BOOST_CHECK_EQUAL(resImage[8], 0.f);
    BOOST_CHECK_EQUAL(resImage[9], (float)step * scaleFactor);
    BOOST_CHECK_EQUAL(resImage[10], 0.f);
    BOOST_CHECK_EQUAL(resImage[11], 0.f);

    BOOST_CHECK_EQUAL(resImage[12], 0.f);
    BOOST_CHECK_EQUAL(resImage[13], 0.f);
    BOOST_CHECK_EQUAL(resImage[14], 0.f);
    BOOST_CHECK_EQUAL(resImage[15], 0.f);

    BOOST_CHECK_EQUAL(resImage[16], 0.f);
    BOOST_CHECK_EQUAL(resImage[17], (float)step * scaleFactor);
    BOOST_CHECK_EQUAL(resImage[18], 0.f);
    BOOST_CHECK_EQUAL(resImage[19], 0.f);
}

BOOST_AUTO_TEST_CASE(CPURendererLUT3D_Red)
{
    const std::string filename("/lut3d_red.xml");

    Color::XMLTransformReader reader;

    try
    {
        reader.parseFile(path + filename);
    }
    catch (Color::Exception e)
    {
        BOOST_ERROR(e.getErrorString());
    }

    const boost::shared_ptr<Color::Transform>& pTransform
        = reader.getTransform();
    BOOST_REQUIRE(pTransform.use_count() == 1);

    const Color::Lut3DOp* pL
        = dynamic_cast<const Color::Lut3DOp*>(pTransform.get()->getOps().get(0));

    const SYNCOLOR::BitDepth inBD = SYNCOLOR::BIT_DEPTH_32f;
    const SYNCOLOR::BitDepth outBD = SYNCOLOR::BIT_DEPTH_16i;

    const float step
        = (Color::Op::getValueStepSize(inBD,
            pL->getArray().getLength()));

    float myImage[] = { 0,    0,    0,    0,
        step, 0,    0,    0,
        0,    step, 0,    0,
        0,    0,    step, 0,
        step, step, step, 0 };

    uint16_t resImage[] = { (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    pRenderer->finalize(
        pTransform,
        (SYNCOLOR::PixelFormat)(Color::Utils::PXL_LAYOUT_RGBA | inBD),
        (SYNCOLOR::PixelFormat)(Color::Utils::PXL_LAYOUT_RGBA | outBD));

    pRenderer->apply(myImage, resImage,
        SYNCOLOR::ROI(5, 1),
        0, 0);

    const float scaleFactor
        = SYNCOLOR::getBitDepthValueRange(outBD)
        / SYNCOLOR::getBitDepthValueRange(inBD);

    BOOST_CHECK_EQUAL(resImage[0], 0);
    BOOST_CHECK_EQUAL(resImage[1], 0);
    BOOST_CHECK_EQUAL(resImage[2], 0);
    BOOST_CHECK_EQUAL(resImage[3], 0);

    BOOST_CHECK_EQUAL(resImage[4], roundf(step * scaleFactor));
    BOOST_CHECK_EQUAL(resImage[5], 0);
    BOOST_CHECK_EQUAL(resImage[6], 0);
    BOOST_CHECK_EQUAL(resImage[7], 0);

    BOOST_CHECK_EQUAL(resImage[8], 0);
    BOOST_CHECK_EQUAL(resImage[9], 0);
    BOOST_CHECK_EQUAL(resImage[10], 0);
    BOOST_CHECK_EQUAL(resImage[11], 0);

    BOOST_CHECK_EQUAL(resImage[12], 0);
    BOOST_CHECK_EQUAL(resImage[13], 0);
    BOOST_CHECK_EQUAL(resImage[14], 0);
    BOOST_CHECK_EQUAL(resImage[15], 0);

    BOOST_CHECK_EQUAL(resImage[16], roundf(step * scaleFactor));
    BOOST_CHECK_EQUAL(resImage[17], 0);
    BOOST_CHECK_EQUAL(resImage[18], 0);
    BOOST_CHECK_EQUAL(resImage[19], 0);
}*/

OIIO_ADD_TEST(Lut3DOp, CPURendererInvLut3D)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;
    const std::string fileName("lut3d_example.clf");
    OCIO::OpRcPtrVec ops;

    OIIO_CHECK_NO_THROW(BuildOps(fileName, ops));

    // TODO: BitDepth support: this is currently switching ops to 32F
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops));
    OIIO_CHECK_EQUAL(1, ops.size());

    OCIO::Lut3DOp* fwdLut
        = dynamic_cast<OCIO::Lut3DOp*>(ops[0].get());

    // Inversion is based on tetrahedral interpolation, so need to make sure 
    // the forward evals are also tetrahedral.
    fwdLut->m_data->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    
    OCIO::OpData::OpDataVec opDataVec;
    fwdLut->m_data->inverse(opDataVec);
    OCIO::OpData::OpDataInvLut3DRcPtr invLutData(dynamic_cast<OCIO::OpData::InvLut3D*>(opDataVec.remove(0)));
    OCIO::InvLut3DOp invLut(invLutData);

    // inverse the inverse
    invLutData->inverse(opDataVec);
    OCIO::OpData::OpDataLut3DRcPtr invInvLutData(dynamic_cast<OCIO::OpData::Lut3D*>(opDataVec.remove(0)));
    OCIO::Lut3DOp invInvLut(invInvLutData);

    // Default InvStyle should be 'FAST' but we test the 'EXACT' InvStyle first
    OIIO_CHECK_EQUAL(invLutData->getInvStyle(), OCIO::OpData::InvLut3D::FAST);
    invLutData->setInvStyle(OCIO::OpData::InvLut3D::EXACT);

    const float inImage[] = {
        0.1f, 0.25f, 0.7f, 0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        //0.18f, 0.80f, 0.45f, 1.f }; // this one is easier to get correct
        0.18f, 0.99f, 0.45f, 1.f };   // setting G way up on the s-curve is harder

    float bufferImage[12];
    unsigned i = 0;
    for (i = 0; i < 12; ++i)
    {
        bufferImage[i] = inImage[i];
    }

    float bufferImageClone[12];
    for (i = 0; i < 12; ++i)
    {
        bufferImageClone[i] = bufferImage[i];
    }

    float bufferImageClone2[12];
    for (i = 0; i < 12; ++i)
    {
        bufferImageClone2[i] = bufferImage[i];
    }

    // Apply forward LUT.
    OIIO_CHECK_NO_THROW(fwdLut->finalize());
    OIIO_CHECK_NO_THROW(fwdLut->apply(bufferImage, 3));

    // clone and verify clone is giving same results
    OCIO::OpRcPtr fwdLutCloned = fwdLut->clone();
    OIIO_CHECK_NO_THROW(fwdLutCloned->finalize());
    OIIO_CHECK_NO_THROW(fwdLutCloned->apply(bufferImageClone, 3));
    const float error = 1e-6f;
    for (unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_CLOSE(bufferImageClone[i], bufferImage[i], error);
    }

    // Inverse of inverse is the same as forward
    OIIO_CHECK_NO_THROW(invInvLut.finalize());
    OIIO_CHECK_NO_THROW(invInvLut.apply(bufferImageClone2, 3));
    for (unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_CLOSE(bufferImageClone2[i], bufferImage[i], error);
    }

    float outImage1[12];
    for (i = 0; i < 12; ++i)
    {
        outImage1[i] = bufferImage[i];
    }

    for (i = 0; i < 12; ++i)
    {
        bufferImageClone[i] = bufferImage[i];
    }

    // Apply inverse LUT.
    OIIO_CHECK_NO_THROW(invLut.finalize());
    OIIO_CHECK_NO_THROW(invLut.apply(bufferImage, 3));

    // clone the inverse and do same transform
    OCIO::OpRcPtr invLutCloned = invLut.clone();
    OIIO_CHECK_NO_THROW(invLutCloned->finalize());
    OIIO_CHECK_NO_THROW(invLutCloned->apply(bufferImageClone, 3));

    for (unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_CLOSE(bufferImageClone[i], bufferImage[i], error);
    }

    // Need to do another foward/inverse cycle.  This is due to precision issues.
    // Also, some LUTs have flat or virtually flat areas so the inverse is not unique.
    // The first inverse does not match the source, but the fact that it winds up
    // in the same place after another cycle verifies that it is as good an inverse
    // for this particular LUT as the original input.  In other words, when the
    // forward LUT has a small derivative, precision issues imply that there will
    // be a range of inverses which should be considered valid.
    OIIO_CHECK_NO_THROW(fwdLut->apply(bufferImage, 3));

    for (unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_CLOSE(outImage1[i], bufferImage[i], error);
    }

    // Repeat with style = FAST.
    invLut.m_data->setInvStyle(OCIO::OpData::InvLut3D::FAST);

    for (i = 0; i < 12; ++i)
    {
        bufferImage[i] = outImage1[i];
    }

    // Apply inverse LUT.
    OIIO_CHECK_NO_THROW(invLut.finalize());
    OIIO_CHECK_NO_THROW(invLut.apply(bufferImage, 3));

    OIIO_CHECK_NO_THROW(fwdLut->apply(bufferImage, 3));

    // Note that, even more than with Lut1D, the FAST inv Lut3D renderer is not exact.
    // It is expected that a fairly loose tolerance must be used here.
    const float errorLoose = 0.015f;
    for (unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_CLOSE(outImage1[i], bufferImage[i], errorLoose);
    }

    // Test clamping of large values in EXACT mode.
    // (No need to test FAST mode since the forward LUT eval clamps inputs to the input domain.)
    invLut.m_data->setInvStyle(OCIO::OpData::InvLut3D::EXACT);

    bufferImage[0] = 100.f;

    OIIO_CHECK_NO_THROW(invLut.finalize());
    OIIO_CHECK_NO_THROW(invLut.apply(bufferImage, 1));

    // This tests that extreme large values get inverted.
    // (If no inverse is found, apply() currently returns zeros.)
    OIIO_CHECK_ASSERT(bufferImage[0] > 0.5f);

}



#endif // OCIO_UNIT_TEST
