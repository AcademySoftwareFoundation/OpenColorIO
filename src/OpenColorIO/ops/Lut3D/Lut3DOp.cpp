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
#include <limits>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ops/Lut3D/Lut3DOpCPU.h"
#include "ops/Lut3D/Lut3DOpGPU.h"
#include "ops/Matrix/MatrixOps.h"
#include "OpTools.h"

OCIO_NAMESPACE_ENTER
{

Lut3D::Lut3D()
{
    for (int i = 0; i<3; ++i)
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

    if (lut.empty())
        throw Exception("Cannot compute cacheID of invalid Lut3D");

    if (!m_cacheID.empty())
        return m_cacheID;

    md5_state_t state;
    md5_byte_t digest[16];

    md5_init(&state);

    md5_append(&state, (const md5_byte_t *)from_min, (int)(3 * sizeof(float)));
    md5_append(&state, (const md5_byte_t *)from_max, (int)(3 * sizeof(float)));
    md5_append(&state, (const md5_byte_t *)size,     (int)(3 * sizeof(int)));
    md5_append(&state, (const md5_byte_t *)&lut[0],  (int)(lut.size() * sizeof(float)));

    md5_finish(&state, digest);

    m_cacheID = GetPrintableHash(digest);

    return m_cacheID;
}


namespace
{
    // Linear
    inline float lerp(float a, float b, float z)
    {
        return (b - a) * z + a;
    }

    inline void lerp_rgb(float* out, float* a, float* b, float* z)
    {
        out[0] = (b[0] - a[0]) * z[0] + a[0];
        out[1] = (b[1] - a[1]) * z[1] + a[1];
        out[2] = (b[2] - a[2]) * z[2] + a[2];
    }

    // Bilinear
    inline float lerp(float a, float b, float c, float d, float y, float z)
    {
        return lerp(lerp(a, b, z), lerp(c, d, z), y);
    }

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
    {
        return lerp(lerp(a,b,c,d,y,z), lerp(e,f,g,h,y,z), x);
    }

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
                                                    size_red, size_green, size_blue)
                              + channelIndex];
    }

    inline void lookupNearest_3D_rgb(float* rgb,
                                     int rIndex, int gIndex, int bIndex,
                                     int size_red, int size_green, int size_blue,
                                     const float* simple_rgb_lut)
    {
        int offset = GetLut3DIndex_RedFast(rIndex, gIndex, bIndex, size_red, size_green, size_blue);
        rgb[0] = simple_rgb_lut[offset];
        rgb[1] = simple_rgb_lut[offset + 1];
        rgb[2] = simple_rgb_lut[offset + 2];
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

        for (int i = 0; i<3; ++i)
        {
            maxIndex[i] = (float)(lut.size[i] - 1);
            mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
            b[i] = lut.from_min[i];
            mInv_x_maxIndex[i] = (float)(mInv[i] * maxIndex[i]);

            lutSize[i] = lut.size[i];
        }

        int localIndex[3];

        for (long pixelIndex = 0; pixelIndex<numPixels; ++pixelIndex)
        {
            if (IsNan(rgbaBuffer[0]) || IsNan(rgbaBuffer[1]) || IsNan(rgbaBuffer[2]))
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

        for (int i = 0; i<3; ++i)
        {
            maxIndex[i] = (float)(lut.size[i] - 1);
            mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
            b[i] = lut.from_min[i];
            mInv_x_maxIndex[i] = (float)(mInv[i] * maxIndex[i]);

            lutSize[i] = lut.size[i];
        }

        for (long pixelIndex = 0; pixelIndex<numPixels; ++pixelIndex)
        {

            if (IsNan(rgbaBuffer[0]) || IsNan(rgbaBuffer[1]) || IsNan(rgbaBuffer[2]))
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

                indexLow[0] = static_cast<int>(std::floor(localIndex[0]));
                indexLow[1] = static_cast<int>(std::floor(localIndex[1]));
                indexLow[2] = static_cast<int>(std::floor(localIndex[2]));

                indexHigh[0] = static_cast<int>(std::ceil(localIndex[0]));
                indexHigh[1] = static_cast<int>(std::ceil(localIndex[1]));
                indexHigh[2] = static_cast<int>(std::ceil(localIndex[2]));

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

    for (int i = 0; i<3; ++i)
    {
        maxIndex[i] = (float)(lut.size[i] - 1);
        mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
        b[i] = lut.from_min[i];
        mInv_x_maxIndex[i] = (float)(mInv[i] * maxIndex[i]);

        lutSize[i] = lut.size[i];
    }

    for (long pixelIndex = 0; pixelIndex<numPixels; ++pixelIndex)
    {

        if (IsNan(rgbaBuffer[0]) || IsNan(rgbaBuffer[1]) || IsNan(rgbaBuffer[2]))
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

            indexLow[0] = static_cast<int>(std::floor(localIndex[0]));
            indexLow[1] = static_cast<int>(std::floor(localIndex[1]));
            indexLow[2] = static_cast<int>(std::floor(localIndex[2]));

            indexHigh[0] = static_cast<int>(std::ceil(localIndex[0]));
            indexHigh[1] = static_cast<int>(std::ceil(localIndex[1]));
            indexHigh[2] = static_cast<int>(std::ceil(localIndex[2]));

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
                        (1 - fx)  * startPos[n000] +
                        (fx - fy) * startPos[n100] +
                        (fy - fz) * startPos[n110] +
                        (fz)* startPos[n111];

                    rgbaBuffer[1] =
                        (1 - fx)  * startPos[n000 + 1] +
                        (fx - fy) * startPos[n100 + 1] +
                        (fy - fz) * startPos[n110 + 1] +
                        (fz)* startPos[n111 + 1];

                    rgbaBuffer[2] =
                        (1 - fx)  * startPos[n000 + 2] +
                        (fx - fy) * startPos[n100 + 2] +
                        (fy - fz) * startPos[n110 + 2] +
                        (fz)* startPos[n111 + 2];
                }
                else if (fx > fz)
                {
                    rgbaBuffer[0] =
                        (1 - fx)  * startPos[n000] +
                        (fx - fz) * startPos[n100] +
                        (fz - fy) * startPos[n101] +
                        (fy)* startPos[n111];

                    rgbaBuffer[1] =
                        (1 - fx)  * startPos[n000 + 1] +
                        (fx - fz) * startPos[n100 + 1] +
                        (fz - fy) * startPos[n101 + 1] +
                        (fy)* startPos[n111 + 1];

                    rgbaBuffer[2] =
                        (1 - fx)  * startPos[n000 + 2] +
                        (fx - fz) * startPos[n100 + 2] +
                        (fz - fy) * startPos[n101 + 2] +
                        (fy)* startPos[n111 + 2];
                }
                else
                {
                    rgbaBuffer[0] =
                        (1 - fz)  * startPos[n000] +
                        (fz - fx) * startPos[n001] +
                        (fx - fy) * startPos[n101] +
                        (fy)* startPos[n111];

                    rgbaBuffer[1] =
                        (1 - fz)  * startPos[n000 + 1] +
                        (fz - fx) * startPos[n001 + 1] +
                        (fx - fy) * startPos[n101 + 1] +
                        (fy)* startPos[n111 + 1];

                    rgbaBuffer[2] =
                        (1 - fz)  * startPos[n000 + 2] +
                        (fz - fx) * startPos[n001 + 2] +
                        (fx - fy) * startPos[n101 + 2] +
                        (fy)* startPos[n111 + 2];
                }
            }
            else
            {
                if (fz > fy)
                {
                    rgbaBuffer[0] =
                        (1 - fz)  * startPos[n000] +
                        (fz - fy) * startPos[n001] +
                        (fy - fx) * startPos[n011] +
                        (fx)* startPos[n111];

                    rgbaBuffer[1] =
                        (1 - fz)  * startPos[n000 + 1] +
                        (fz - fy) * startPos[n001 + 1] +
                        (fy - fx) * startPos[n011 + 1] +
                        (fx)* startPos[n111 + 1];

                    rgbaBuffer[2] =
                        (1 - fz)  * startPos[n000 + 2] +
                        (fz - fy) * startPos[n001 + 2] +
                        (fy - fx) * startPos[n011 + 2] +
                        (fx)* startPos[n111 + 2];
                }
                else if (fz > fx)
                {
                    rgbaBuffer[0] =
                        (1 - fy)  * startPos[n000] +
                        (fy - fz) * startPos[n010] +
                        (fz - fx) * startPos[n011] +
                        (fx)* startPos[n111];

                    rgbaBuffer[1] =
                        (1 - fy)  * startPos[n000 + 1] +
                        (fy - fz) * startPos[n010 + 1] +
                        (fz - fx) * startPos[n011 + 1] +
                        (fx)* startPos[n111 + 1];

                    rgbaBuffer[2] =
                        (1 - fy)  * startPos[n000 + 2] +
                        (fy - fz) * startPos[n010 + 2] +
                        (fz - fx) * startPos[n011 + 2] +
                        (fx)* startPos[n111 + 2];
                }
                else
                {
                    rgbaBuffer[0] =
                        (1 - fy)  * startPos[n000] +
                        (fy - fx) * startPos[n010] +
                        (fx - fz) * startPos[n110] +
                        (fz)* startPos[n111];

                    rgbaBuffer[1] =
                        (1 - fy)  * startPos[n000 + 1] +
                        (fy - fx) * startPos[n010 + 1] +
                        (fx - fz) * startPos[n110 + 1] +
                        (fz)* startPos[n111 + 1];

                    rgbaBuffer[2] =
                        (1 - fy)  * startPos[n000 + 2] +
                        (fy - fx) * startPos[n010 + 2] +
                        (fx - fz) * startPos[n110 + 2] +
                        (fz)* startPos[n111 + 2];
                }
            }
        } // !IsNan

        rgbaBuffer += 4;
    }
}
#endif

void GenerateIdentityLut3D(float* img, int edgeLen, int numChannels, Lut3DOrder lut3DOrder)
{
    if (!img) return;
    if (numChannels < 3)
    {
        throw Exception("Cannot generate idenitity 3d LUT with less than 3 channels.");
    }

    float c = 1.0f / ((float)edgeLen - 1.0f);

    if (lut3DOrder == LUT3DORDER_FAST_RED)
    {
        for (int i = 0; i<edgeLen*edgeLen*edgeLen; i++)
        {
            img[numChannels*i + 0] = (float)(i%edgeLen) * c;
            img[numChannels*i + 1] = (float)((i / edgeLen) % edgeLen) * c;
            img[numChannels*i + 2] = (float)((i / edgeLen / edgeLen) % edgeLen) * c;
        }
    }
    else if (lut3DOrder == LUT3DORDER_FAST_BLUE)
    {
        for (int i = 0; i<edgeLen*edgeLen*edgeLen; i++)
        {
            img[numChannels*i + 0] = (float)((i / edgeLen / edgeLen) % edgeLen) * c;
            img[numChannels*i + 1] = (float)((i / edgeLen) % edgeLen) * c;
            img[numChannels*i + 2] = (float)(i%edgeLen) * c;
        }
    }
    else
    {
        throw Exception("Unknown Lut3DOrder.");
    }
}


int Get3DLutEdgeLenFromNumPixels(int numPixels)
{
    int dim = static_cast<int>(roundf(powf((float)numPixels, 1.0f / 3.0f)));

    if (dim*dim*dim != numPixels)
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
        Lut3DOp(Lut3DOpDataRcPtr & data);
        virtual ~Lut3DOp();

        TransformDirection getDirection() const noexcept override { return lut3DData()->getDirection(); }

        OpRcPtr clone() const override;

        std::string getInfo() const override;

        bool isSameType(ConstOpRcPtr & op) const override;
        bool isInverse(ConstOpRcPtr & op) const override;
        bool hasChannelCrosstalk() const override;
        void finalize(FinalizationFlags fFlags) override;

        ConstOpCPURcPtr getCPUOp() const override;

        bool supportedByLegacyShader() const override { return false; }
        void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

#ifdef OCIO_UNIT_TEST
        Array & getArray()
        {
            return lut3DData()->getArray();
        }
#endif

    protected:
        ConstLut3DOpDataRcPtr lut3DData() const
        {
            return DynamicPtrCast<const Lut3DOpData>(data());
        }

        Lut3DOpDataRcPtr lut3DData()
        { 
            return DynamicPtrCast<Lut3DOpData>(data());
        }

    private:
        Lut3DOp() = delete;
    };

    typedef OCIO_SHARED_PTR<Lut3DOp> Lut3DOpRcPtr;
    typedef OCIO_SHARED_PTR<const Lut3DOp> ConstLut3DOpRcPtr;

    Lut3DOp::Lut3DOp(Lut3DOpDataRcPtr & lut3D)
    {
        data() = lut3D;
    }

    Lut3DOp::~Lut3DOp()
    {
    }

    OpRcPtr Lut3DOp::clone() const
    {
        Lut3DOpDataRcPtr lut = lut3DData()->clone();
        return std::make_shared<Lut3DOp>(lut);
    }

    std::string Lut3DOp::getInfo() const
    {
        return "<Lut3DOp>";
    }

    bool Lut3DOp::isSameType(ConstOpRcPtr & op) const
    {
        ConstLut3DOpRcPtr lutRcPtr = DynamicPtrCast<const Lut3DOp>(op);
        return (bool)lutRcPtr;
    }

    bool Lut3DOp::isInverse(ConstOpRcPtr & op) const
    {
        ConstLut3DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut3DOp>(op);
        if (typedRcPtr)
        {
            ConstLut3DOpDataRcPtr lutData = typedRcPtr->lut3DData();
            return lut3DData()->isInverse(lutData);
        }

        return false;
    }

    bool Lut3DOp::hasChannelCrosstalk() const
    {
        return lut3DData()->hasChannelCrosstalk();
    }

    void Lut3DOp::finalize(FinalizationFlags fFlags)
    {
        Lut3DOpDataRcPtr lutData = lut3DData();

        lutData->setInversionQuality(
            fFlags==FINALIZATION_FAST ? LUT_INVERSION_FAST: LUT_INVERSION_EXACT);

        // Only 32f processing is natively supported.
        lutData->setInputBitDepth(BIT_DEPTH_F32);
        lutData->setOutputBitDepth(BIT_DEPTH_F32);

        lutData->finalize();

        std::ostringstream cacheIDStream;
        cacheIDStream << "<Lut3D ";
        cacheIDStream << lutData->getCacheID() << " ";
        cacheIDStream << ">";

        m_cacheID = cacheIDStream.str();
    }

    ConstOpCPURcPtr Lut3DOp::getCPUOp() const
    {
        ConstLut3DOpDataRcPtr data = lut3DData();
        return GetLut3DRenderer(data);
    }

    void Lut3DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
    {
        if (getInputBitDepth() != BIT_DEPTH_F32 || getOutputBitDepth() != BIT_DEPTH_F32)
        {
            throw Exception("Only 32F bit depth is supported for the GPU shader");
        }

        ConstLut3DOpDataRcPtr lutData = lut3DData();
        if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
        {
            // TODO: Add GPU renderer for EXACT mode.

            Lut3DOpDataRcPtr tmp = MakeFastLut3DFromInverse(lutData);

            tmp->finalize();

            lutData = tmp;
        }

        GetLut3DGPUShaderProgram(shaderDesc, lutData);
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

    const unsigned long lutSize = lut->size[0];
    if (lut->lut.size() != lutSize*lutSize*lutSize * 3)
    {
        throw Exception("Cannot apply Lut3DOp op, "
            "specified size does not match data.");
    }

    // Convert Lut3D struct into Lut3DOpData.
    // Code assumes that LUT coming in are LUT3DORDER_FAST_RED.
    // Need to transform to Blue fast for OpData::Lut3D.
    Lut3DOpDataRcPtr lutBF(new Lut3DOpData(lutSize));
    lutBF->setInterpolation(interpolation);

    Array & lutArray = lutBF->getArray();

    for (unsigned long b = 0; b < lutSize; ++b)
    {
        for (unsigned long g = 0; g < lutSize; ++g)
        {
            for (unsigned long r = 0; r < lutSize; ++r)
            {
                // OpData::Lut3D Array index. b changes fastest.
                const unsigned long arrayIdx = 3 * ((r*lutSize + g)*lutSize + b);

                // Lut3D struct index. r changes fastest.
                const unsigned long ocioIdx = 3 * ((b*lutSize + g)*lutSize + r);

                lutArray[arrayIdx    ] = lut->lut[ocioIdx    ];
                lutArray[arrayIdx + 1] = lut->lut[ocioIdx + 1];
                lutArray[arrayIdx + 2] = lut->lut[ocioIdx + 2];
            }
        }
    }

    if (direction == TRANSFORM_DIR_FORWARD)
    {
        // NB: CreateMinMaxOp will not add the matrix if from_min & from_max
        //     are at their defaults.
        CreateMinMaxOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_FORWARD);
        CreateLut3DOp(ops, lutBF, TRANSFORM_DIR_FORWARD);
    }
    else
    {
        CreateLut3DOp(ops, lutBF, TRANSFORM_DIR_INVERSE);
        CreateMinMaxOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_INVERSE);
    }
}


void CreateLut3DOp(OpRcPtrVec & ops,
                   Lut3DOpDataRcPtr & lut,
                   TransformDirection direction)
{
    if (lut->isNoOp()) return;

    if (direction == TRANSFORM_DIR_FORWARD)
    {
        ops.push_back(std::make_shared<Lut3DOp>(lut));
    }
    else if (direction == TRANSFORM_DIR_INVERSE)
    {
        Lut3DOpDataRcPtr data = lut->inverse();
        ops.push_back(std::make_shared<Lut3DOp>(data));
    }
    else
    {
        throw Exception("Cannot apply Lut3DOp op, unspecified transform direction.");
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

#include "BitDepthUtils.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(Lut3DOpStruct, nan_inf_value_check)
{
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    
    lut->size[0] = 3;
    lut->size[1] = 3;
    lut->size[2] = 3;
    
    lut->lut.resize(lut->size[0]*lut->size[1]*lut->size[2]*3);
    
    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));
    for(size_t i=0; i<lut->lut.size(); ++i)
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


OCIO_ADD_TEST(Lut3DOpStruct, value_check)
{
    const float error = 1e-5f;

    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();

    for (int i = 0; i < 3; ++i)
    {
        OCIO_CHECK_CLOSE(lut->from_min[i], 0.0f, error);
        OCIO_CHECK_CLOSE(lut->from_max[i], 1.0f, error);
    }

    lut->size[0] = 32;
    lut->size[1] = 32;
    lut->size[2] = 32;

    lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);
    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));
    for (size_t i = 0; i < lut->lut.size(); ++i)
    {
        lut->lut[i] = powf(lut->lut[i], 2.0f);
    }

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::INTERP_NEAREST,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::INTERP_TETRAHEDRAL,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    ops[0]->finalize(OCIO::FINALIZATION_EXACT);
    ops[1]->finalize(OCIO::FINALIZATION_EXACT);
    ops[2]->finalize(OCIO::FINALIZATION_EXACT);

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
    const float tetrahedral[] = { 0.0f, 0.0401664972f, 0.0902185217f, 1.0f,
                                  0.0153775234f, 0.208713099f, 0.975600004f, 1.0f,
                                  1.0f, 0.0f, 0.251260102f, 1.0f
    };

    float color[12];
    float color2[12];

    // Check nearest
    memcpy(color, reference, 12*sizeof(float));
    OCIO::Lut3D_Nearest(color, 3, *lut);
    memcpy(color2, reference, 12 * sizeof(float));
    ops[0]->apply(color2, 3);
    for(long i=0; i<12; ++i)
    {
        OCIO_CHECK_CLOSE(color[i], nearest[i], 1e-8);
        // NB: In OCIO v2, INTERP_NEAREST is implemented as trilinear.
        OCIO_CHECK_CLOSE(color2[i], linear[i], 1e-8);
    }

    // Check linear.
    memcpy(color, reference, 12*sizeof(float));
    OCIO::Lut3D_Linear(color, 3, *lut);
    memcpy(color2, reference, 12 * sizeof(float));
    ops[1]->apply(color2, 3);
    for(long i=0; i<12; ++i)
    {
        OCIO_CHECK_CLOSE(color[i], linear[i], 1e-8);
        OCIO_CHECK_CLOSE(color2[i], linear[i], 1e-8);
    }

    // Check tetrahedral
    memcpy(color, reference, 12*sizeof(float));
    OCIO::Lut3D_Tetrahedral(color, 3, *lut);
    memcpy(color2, reference, 12 * sizeof(float));
    ops[2]->apply(color2, 3);
    for(long i=0; i<12; ++i)
    {
        OCIO_CHECK_CLOSE(color[i], tetrahedral[i], 1e-8);
        OCIO_CHECK_CLOSE(color2[i], tetrahedral[i], 1e-7);
    }
}

OCIO_ADD_TEST(Lut3DOp, lut3d_tetrahedral)
{
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    lut->size[0] = 3;
    lut->size[1] = 3;
    lut->size[2] = 3;

    // Similar to lut3d_bizarre.spi3d.
    lut->lut = { -0.0586510263f,  0.00488758599f,  0.0733137801f,
                  0.0977517143f, -0.00488758599f, -0.0733137801f,
                  1.62267840f,   -0.0488758534f,  -0.0977517143f,
                 -0.0391006842f,  0.488758564f,   -0.0293255132f,
                  0.0391006842f,  0.586510241f,    0.0293255132f,
                  1.21212125f,    0.293255121f,   -0.0488758534f,
                 -0.0293255132f,  1.46627569f,    -0.0195503421f,
                  0.518084049f,   1.46627569f,    -0.0488758534f,
                  1.00684261f,   -0.0977517143f,  -0.0488758534f,
                 -0.00977517106f, 0.0488758534f,   0.391006857f,
                  0.195503414f,  -0.0488758534f,   0.586510241f,
                  1.47605085f,    0.146627560f,    0.195503414f,
                 -0.0782013685f,  0.391006857f,    0.488758564f,
                  0.371456504f,   0.391006857f,    0.391006857f,
                  0.957966745f,   0.488758564f,    0.586510241f,
                 -0.0488758534f,  1.00000000f,     0.0977517143f,
                  0.439882696f,   1.20234609f,     0.488758564f,
                  1.02639294f,    0.804496586f,    0.195503414f,
                  0.000000000f,   0.0977517143f,   1.17302048f,
                  0.293255121f,   0.0977517143f,   1.17302048f,
                  0.879765391f,  -0.0391006842f,   0.879765391f,
                 -0.0195503421f,  0.293255121f,    0.879765391f,
                  0.0195503421f,  0.195503414f,    1.07526886f,
                  0.801564038f,   0.391006857f,    0.782013714f,
                 -0.00977517106f, 0.782013714f,    0.293255121f,
                  0.596285462f,   1.75953078f,     0.293255121f,
                  1.08504403f,    0.879765391f,    1.17302048f };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::INTERP_TETRAHEDRAL,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    ops[0]->finalize(OCIO::FINALIZATION_EXACT);
    ops[1]->finalize(OCIO::FINALIZATION_EXACT);


    const float reference[] = { 0.778210117f, 0.823987182f, 0.88502327f, 0.5f,
                                0.044556344f, 0.014404517f, 0.88158999f, 1.0f };

    const float linear[] = { 0.7271368f,  0.8984065f,  0.7267998457f, 0.5f,
                             0.02071510f, 0.09061456f, 0.9863739f,    1.0f };

    const float tetrahedral[] = { 0.7461370f,   0.76439046f, 0.9007148f,  0.5f,
                                  0.015932627f, 0.08899306f, 0.98500788f, 1.0f };
    
    float color[8];
    float color2[8];

    // Check linear
    memcpy(color, reference, 8 * sizeof(float));
    OCIO::Lut3D_Linear(color, 2, *lut);
    memcpy(color2, reference, 8 * sizeof(float));
    ops[0]->apply(color2, 2);
    for (long i = 0; i < 8; ++i)
    {
        OCIO_CHECK_CLOSE(color[i], linear[i], 1e-7);
        OCIO_CHECK_CLOSE(color2[i], linear[i], 1e-7);
    }

    // Check tetrahedral.
    memcpy(color, reference, 8 * sizeof(float));
    OCIO::Lut3D_Tetrahedral(color, 2, *lut);
    memcpy(color2, reference, 8 * sizeof(float));
    ops[1]->apply(color2, 2);
    for (long i = 0; i < 8; ++i)
    {
        OCIO_CHECK_CLOSE(color[i], tetrahedral[i], 1e-7);
        OCIO_CHECK_CLOSE(color2[i], tetrahedral[i], 1e-7);
    }

}


OCIO_ADD_TEST(Lut3DOpStruct, inverse_comparison_check)
{
    OCIO::Lut3DRcPtr lut_a = OCIO::Lut3D::Create();
    lut_a->size[0] = 32;
    lut_a->size[1] = 32;
    lut_a->size[2] = 32;
    lut_a->lut.resize(lut_a->size[0]*lut_a->size[1]*lut_a->size[2]*3);
    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
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
    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_b->lut[0], lut_b->size[0], 3, OCIO::LUT3DORDER_FAST_RED));
    
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_a, OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_a, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    // Add Matrix and LUT.
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    // Add LUT and Matrix.
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    
    OCIO_REQUIRE_EQUAL(ops.size(), 6);

    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];
    OCIO::ConstOpRcPtr op4Cloned = op4->clone();

    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op3));
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op4Cloned));

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op3));
    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op4));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op4));
}

/*
OCIO_ADD_TEST(Lut3DOp, PerformanceCheck)
{
    OCIO::Lut3D lut;

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
}
*/

OCIO_ADD_TEST(Lut3DOpStruct, throw_lut)
{
    // std::string Lut3D::getCacheID() const when LUT is empty.
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    OCIO_CHECK_THROW_WHAT(lut->getCacheID(), OCIO::Exception, "invalid Lut3D");

    // GenerateIdentityLut3D with less than 3 channels
    lut->size[0] = 3;
    lut->size[1] = 3;
    lut->size[2] = 3;

    lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);

    OCIO_CHECK_THROW_WHAT(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 2, OCIO::LUT3DORDER_FAST_RED),
        OCIO::Exception, "less than 3 channels");

    // GenerateIdentityLut3D with unknown order.
    OCIO_CHECK_THROW_WHAT(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, (OCIO::Lut3DOrder)42),
        OCIO::Exception, "Unknown Lut3DOrder");

    // Get3DLutEdgeLenFromNumPixels with not cubic size.
    OCIO_CHECK_THROW_WHAT(OCIO::Get3DLutEdgeLenFromNumPixels(10),
        OCIO::Exception, "Cannot infer 3D LUT size");
}

OCIO_ADD_TEST(Lut3DOpStruct, throw_lut_op)
{
    OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
    lut->size[0] = 3;
    lut->size[1] = 3;
    lut->size[2] = 3;
    lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);

    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops,
        lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    // Inverse is fine
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    ops.clear();

    // Only valid interpolation.
    OCIO_CHECK_THROW_WHAT(CreateLut3DOp(ops,
                                        lut,
                                        OCIO::INTERP_UNKNOWN,
                                        OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception,
                          "invalid interpolation");
    ops.clear();

    // Change size so that it does not match.
    lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2]);
    OCIO_CHECK_THROW_WHAT(CreateLut3DOp(ops,
                                        lut,
                                        OCIO::INTERP_LINEAR,
                                        OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception,
                          "size does not match");

}

OCIO_ADD_TEST(Lut3DOpStruct, cache_id)
{
    OCIO::OpRcPtrVec ops;
    for (int i = 0; i < 2; ++i)
    {
        OCIO::Lut3DRcPtr lut = OCIO::Lut3D::Create();
        lut->size[0] = 3;
        lut->size[1] = 3;
        lut->size[2] = 3;
        lut->lut.resize(lut->size[0] * lut->size[1] * lut->size[2] * 3);

        OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
            &lut->lut[0], lut->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

        OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::INTERP_LINEAR,
                                          OCIO::TRANSFORM_DIR_FORWARD));
    }

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    const std::string cacheID = ops[0]->getCacheID();
    OCIO_CHECK_ASSERT(!cacheID.empty());
    // Identical LUTs have the same cacheID.
    OCIO_CHECK_EQUAL(cacheID, ops[1]->getCacheID());
}

OCIO_ADD_TEST(Lut3DOp, edge_len_from_num_pixels)
{
    OCIO_CHECK_THROW_WHAT(OCIO::Get3DLutEdgeLenFromNumPixels(10),
                          OCIO::Exception, "Cannot infer 3D LUT size");
    int expectedRes = 33;
    int res = 0;
    OCIO_CHECK_NO_THROW(res = OCIO::Get3DLutEdgeLenFromNumPixels(
        expectedRes*expectedRes*expectedRes));
    OCIO_CHECK_EQUAL(res, expectedRes);

    expectedRes = 1290; // Maximum value such that v^3 is still an int.
    OCIO_CHECK_NO_THROW(res = OCIO::Get3DLutEdgeLenFromNumPixels(
        expectedRes*expectedRes*expectedRes));
    OCIO_CHECK_EQUAL(res, expectedRes);
}

OCIO_ADD_TEST(Lut3DOpStruct, lut3d_order)
{
    OCIO::Lut3DRcPtr lut_r = OCIO::Lut3D::Create();

    lut_r->size[0] = 3;
    lut_r->size[1] = 3;
    lut_r->size[2] = 3;

    lut_r->lut.resize(lut_r->size[0] * lut_r->size[1] * lut_r->size[2] * 3);

    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_r->lut[0], lut_r->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

    // First 3 values have red changing.
    OCIO_CHECK_EQUAL(lut_r->lut[0], 0.0f);
    OCIO_CHECK_EQUAL(lut_r->lut[3], 0.5f);
    OCIO_CHECK_EQUAL(lut_r->lut[6], 1.0f);
    // Blue is all 0.
    OCIO_CHECK_EQUAL(lut_r->lut[2], 0.0f);
    OCIO_CHECK_EQUAL(lut_r->lut[5], 0.0f);
    OCIO_CHECK_EQUAL(lut_r->lut[8], 0.0f);
    // Last 3 values have red changing.
    OCIO_CHECK_EQUAL(lut_r->lut[72], 0.0f);
    OCIO_CHECK_EQUAL(lut_r->lut[75], 0.5f);
    OCIO_CHECK_EQUAL(lut_r->lut[78], 1.0f);
    // Blue is all 1.
    OCIO_CHECK_EQUAL(lut_r->lut[74], 1.0f);
    OCIO_CHECK_EQUAL(lut_r->lut[77], 1.0f);
    OCIO_CHECK_EQUAL(lut_r->lut[80], 1.0f);

    OCIO::Lut3DRcPtr lut_b = OCIO::Lut3D::Create();

    lut_b->size[0] = 3;
    lut_b->size[1] = 3;
    lut_b->size[2] = 3;

    lut_b->lut.resize(lut_b->size[0] * lut_b->size[1] * lut_b->size[2] * 3);

    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_b->lut[0], lut_b->size[0], 3, OCIO::LUT3DORDER_FAST_BLUE));

    // First 3 values have blue changing.
    OCIO_CHECK_EQUAL(lut_b->lut[2], 0.0f);
    OCIO_CHECK_EQUAL(lut_b->lut[5], 0.5f);
    OCIO_CHECK_EQUAL(lut_b->lut[8], 1.0f);
    // Red is all 0.
    OCIO_CHECK_EQUAL(lut_b->lut[0], 0.0f);
    OCIO_CHECK_EQUAL(lut_b->lut[3], 0.0f);
    OCIO_CHECK_EQUAL(lut_b->lut[6], 0.0f);
    // Last 3 values have blue changing.
    OCIO_CHECK_EQUAL(lut_b->lut[74], 0.0f);
    OCIO_CHECK_EQUAL(lut_b->lut[77], 0.5f);
    OCIO_CHECK_EQUAL(lut_b->lut[80], 1.0f);
    // Red is all 1.
    OCIO_CHECK_EQUAL(lut_b->lut[72], 1.0f);
    OCIO_CHECK_EQUAL(lut_b->lut[75], 1.0f);
    OCIO_CHECK_EQUAL(lut_b->lut[78], 1.0f);

}

OCIO_ADD_TEST(Lut3DOp, convert_lut)
{
    OCIO::Lut3DRcPtr lut_r = OCIO::Lut3D::Create();

    lut_r->size[0] = 3;
    lut_r->size[1] = 3;
    lut_r->size[2] = 3;

    lut_r->lut.resize(lut_r->size[0] * lut_r->size[1] * lut_r->size[2] * 3);

    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        &lut_r->lut[0], lut_r->size[0], 3, OCIO::LUT3DORDER_FAST_RED));

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut_r, OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    const OCIO::Lut3DOpRcPtr pLB = OCIO::DynamicPtrCast<OCIO::Lut3DOp>(ops[0]);

    // First 3 values have blue changing.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[2], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[5], 0.5f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[8], 1.0f);
    // Red is all 0.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[0], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[3], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[6], 0.0f);
    // Last 3 values have blue changing.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[74], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[77], 0.5f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[80], 1.0f);
    // Red is all 1.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[72], 1.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[75], 1.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[78], 1.0f);
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_lut3d)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut3DOpDataRcPtr
        lutData = std::make_shared<OCIO::Lut3DOpData>(
            bitDepth, bitDepth,
            "", OCIO::OpData::Descriptions(),
            OCIO::INTERP_LINEAR, 33);

    OCIO::Lut3DOp lut(lutData);

    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());

    // Use an input value exactly at a grid point so the output value is 
    // just the grid value, regardless of interpolation.
    const float step =
        OCIO::GetBitDepthMaxValue(lutData->getInputBitDepth())
        / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = { 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, step, 1.0f };

    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_EQUAL(myImage[0], 0.0f);
        OCIO_CHECK_EQUAL(myImage[1], 0.0f);
        OCIO_CHECK_EQUAL(myImage[2], 0.0f);
        OCIO_CHECK_EQUAL(myImage[3], 0.0f);

        OCIO_CHECK_EQUAL(myImage[4], 0.0f);
        OCIO_CHECK_EQUAL(myImage[5], 0.0f);
        OCIO_CHECK_EQUAL(myImage[6], step);
        OCIO_CHECK_EQUAL(myImage[7], 1.0f);
    }

    // No more an 'identity LUT 3D'.
    const float arbitraryVal = 0.123456f;
    lutData->getArray()[5] = arbitraryVal;

    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());

    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_EQUAL(myImage[0], 0.0f);
        OCIO_CHECK_EQUAL(myImage[1], 0.0f);
        OCIO_CHECK_EQUAL(myImage[2], 0.0f);
        OCIO_CHECK_EQUAL(myImage[3], 0.0f);

        OCIO_CHECK_EQUAL(myImage[4], 0.0f);
        OCIO_CHECK_EQUAL(myImage[5], 0.0f);
        OCIO_CHECK_EQUAL(myImage[6], arbitraryVal);
        OCIO_CHECK_EQUAL(myImage[7], 1.0f);
    }

    // Change interpolation.
    lutData->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());
    myImage[6] = step;
    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_EQUAL(myImage[0], 0.0f);
        OCIO_CHECK_EQUAL(myImage[1], 0.0f);
        OCIO_CHECK_EQUAL(myImage[2], 0.0f);
        OCIO_CHECK_EQUAL(myImage[3], 0.0f);

        OCIO_CHECK_EQUAL(myImage[4], 0.0f);
        OCIO_CHECK_EQUAL(myImage[5], 0.0f);
        OCIO_CHECK_EQUAL(myImage[6], arbitraryVal);
        OCIO_CHECK_EQUAL(myImage[7], 1.0f);
    }
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_cloned)
{
    // The unit test validates the processing of cloned ops.

    const std::string fileName("lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops.size());
    OCIO_CHECK_EQUAL(ops[1]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(ops[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_FAST));

    OCIO_REQUIRE_EQUAL(1, ops.size());
    OCIO_CHECK_EQUAL(ops[0]->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    auto op0 = OCIO::DynamicPtrCast<const OCIO::Lut3DOp>(ops[0]);
    OCIO_REQUIRE_ASSERT(op0);
    auto fwdLutData = OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(op0->data());

    OCIO::Lut3DOp * fwdLut = const_cast<OCIO::Lut3DOp *>(op0.get());

    auto fwdLutDataCloned = fwdLutData->clone();
    OCIO::Lut3DOp fwdLutCloned(fwdLutDataCloned);

    OCIO::OpRcPtr fwdLutClonedCloned = fwdLutCloned.clone();

    const float inImage[] = {
        0.1f, 0.25f, 0.7f, 0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        //0.18f, 0.80f, 0.45f, 1.f }; // This one is easier to get correct.
        0.18f, 0.99f, 0.45f, 1.f };   // Setting G way up on the s-curve is harder.

    float bufferImage[12];
    memcpy(bufferImage, inImage, 12 * sizeof(float));

    float bufferImageClone[12];
    memcpy(bufferImageClone, inImage, 12 * sizeof(float));

    float bufferImageClone2[12];
    memcpy(bufferImageClone2, inImage, 12 * sizeof(float));

    // Apply the forward LUT.
    OCIO_CHECK_NO_THROW(fwdLut->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(fwdLut->apply(bufferImage, 3));

    // Apply the cloned forward LUT.
    OCIO_CHECK_NO_THROW(fwdLutCloned.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(fwdLutCloned.apply(bufferImageClone, 3));

    // Apply the cloned cloned forward LUT.
    OCIO_CHECK_NO_THROW(fwdLutClonedCloned->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(fwdLutClonedCloned->apply(bufferImageClone2, 3));

    // Validate that the cloned op produces the exact same results.
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_EQUAL(bufferImageClone[i], bufferImage[i]);
        OCIO_CHECK_EQUAL(bufferImageClone2[i], bufferImage[i]);
    }
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_inverse)
{
    // The unit test validates the processing of inversed ops.

    const std::string fileName("lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_FAST));

    auto op0 = OCIO::DynamicPtrCast<const OCIO::Lut3DOp>(ops[0]);
    OCIO_REQUIRE_ASSERT(op0);
    auto fwdLutData = OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(op0->data());
    auto fwdLutDataCloned = fwdLutData->clone();
    // Inversion is based on tetrahedral interpolation, so need to make sure 
    // the forward evals are also tetrahedral.
    fwdLutDataCloned->setInterpolation(OCIO::INTERP_TETRAHEDRAL);

    OCIO::Lut3DOp fwdLut(fwdLutDataCloned);

    OCIO::Lut3DOpDataRcPtr invLutData = fwdLutDataCloned->inverse();
    OCIO::Lut3DOp invLut(invLutData);

    // Default Inversion quality should be 'FAST' but we test the 'EXACT' first.
    OCIO_CHECK_EQUAL(invLutData->getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    invLutData->setInversionQuality(OCIO::LUT_INVERSION_EXACT);

    const float inImage[] = {
        0.1f, 0.25f, 0.7f, 0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        //0.18f, 0.80f, 0.45f, 1.f }; // This one is easier to get correct.
        0.18f, 0.99f, 0.45f, 1.f };   // Setting G way up on the s-curve is harder.

    float bufferImage[12];
    memcpy(bufferImage, inImage, 12 * sizeof(float));

    // Apply forward LUT.
    OCIO_CHECK_NO_THROW(fwdLut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(fwdLut.apply(bufferImage, 3));

    //
    // Step 1: Test that forward and inverse ops are producing
    //         the right results in FAST mode.
    //

    float outImage1[12];
    memcpy(outImage1, bufferImage, 12 * sizeof(float));

    // Apply inverse LUT.
    OCIO_CHECK_NO_THROW(invLut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(invLut.apply(bufferImage, 3));

    // Need to do another foward apply.  This is due to precision issues.
    // Also, some LUTs have flat or virtually flat areas so the inverse is not unique.
    // The first inverse does not match the source, but the fact that it winds up
    // in the same place after another cycle verifies that it is as good an inverse
    // for this particular LUT as the original input.  In other words, when the
    // forward LUT has a small derivative, precision issues imply that there will
    // be a range of inverses which should be considered valid.
    OCIO_CHECK_NO_THROW(fwdLut.apply(bufferImage, 3));

    const float errorThreshold = 1e-6f;
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_CLOSE(outImage1[i], bufferImage[i], errorThreshold);
    }

    memcpy(bufferImage, outImage1, 12 * sizeof(float));

    //
    // Step 2: Repeat with inversion quality FAST, apply inverse LUT.
    //

    OCIO_CHECK_NO_THROW(invLut.finalize(OCIO::FINALIZATION_FAST));
    OCIO_CHECK_NO_THROW(invLut.apply(bufferImage, 3));

    OCIO_CHECK_NO_THROW(fwdLut.apply(bufferImage, 3));

    // Note that, even more than with Lut1D, the FAST inv Lut3D renderer is not exact.
    // It is expected that a fairly loose tolerance must be used here.
    const float errorLoose = 0.015f;
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_CLOSE(outImage1[i], bufferImage[i], errorLoose);
    }

    //
    // Step 3: Test clamping of large values in EXACT mode.
    // 
    // Note: No need to test FAST mode since the forward LUT eval clamps inputs
    //       to the input domain.
    //

    bufferImage[0] = 100.f;

    OCIO_CHECK_NO_THROW(invLut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(invLut.apply(bufferImage, 1));

    // This tests that extreme large values get inverted.
    // (If no inverse is found, apply() currently returns zeros.)
    OCIO_CHECK_ASSERT(bufferImage[0] > 0.5f);
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_lut3d_with_nan)
{
    const std::string fileName("lut3d_2x2x2_32f_32f.clf");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(1, ops.size());
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO_CHECK_EQUAL(op0->data()->getType(), OCIO::OpData::Lut3DType);

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    float myImage[]
        = { qnan, 0.25f, 0.25f, 0.0f,
            0.25f, qnan, 0.25f, 0.0f,
            0.25f, 0.25f, qnan, 0.0f,
            0.25f, 0.25f, 0.0f, qnan,
            0.5f, 0.5f,  0.5f,  0.0f };

    OCIO_CHECK_NO_THROW(op0->apply(myImage, 5));

    OCIO_CHECK_EQUAL(myImage[0], 0.0f);
    OCIO_CHECK_EQUAL(myImage[1], 0.25f);
    OCIO_CHECK_EQUAL(myImage[2], 0.25f);
    OCIO_CHECK_EQUAL(myImage[3], 0.0f);

    OCIO_CHECK_EQUAL(myImage[5], 0.0f);
    OCIO_CHECK_EQUAL(myImage[10], 0.0f);

    OCIO_CHECK_ASSERT(OCIO::IsNan(myImage[15]));

    OCIO_CHECK_EQUAL(myImage[16], 0.5f);
    OCIO_CHECK_EQUAL(myImage[17], 0.5f);
    OCIO_CHECK_EQUAL(myImage[18], 0.5f);
    OCIO_CHECK_EQUAL(myImage[19], 0.0f);
}



// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Blue
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Green
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Red
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Example
// TODO: Port syncolor test: renderer\test\transformTools_test.cpp   - lut3d_composition

#endif // OCIO_UNIT_TEST
