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

#include "ClampOp.h"
#include "GpuShaderUtils.h"
#include "MathUtils.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        inline bool emptyRange(float min, float max)
        {
            return min > max;
        }

        inline bool allEmptyRanges(const float *min, const float *max)
        {
            return emptyRange(min[0], max[0]) && emptyRange(min[1], max[1]) &&
                   emptyRange(min[2], max[2]) && emptyRange(min[3], max[3]);
        }

        inline float clamp(float val, float min, float max)
        {
            if (val < min)
                return min;
            else if (val > max)
                return max;
            else
                return val;
        }

        void ApplyClamp(float* rgbaBuffer, long numPixels,
                        const float* min4, const float* max4)
        {
            int index = 0;
            if (emptyRange(min4[0], max4[0]))
                index += 1;
            if (emptyRange(min4[1], max4[1]))
                index += 2;
            if (emptyRange(min4[2], max4[2]))
                index += 4;
            if (emptyRange(min4[3], max4[3]))
                index += 8;

            switch (index)
            {
                case 0:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 1:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 2:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 3:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 4:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 5:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 6:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 7:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[3] = clamp(rgbaBuffer[3], min4[3], max4[3]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 8:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 9:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 10:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 11:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[2] = clamp(rgbaBuffer[2], min4[2], max4[2]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 12:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 13:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[1] = clamp(rgbaBuffer[1], min4[1], max4[1]);
                        rgbaBuffer += 4;
                    }
                    break;
                case 14:
                    for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                    {
                        rgbaBuffer[0] = clamp(rgbaBuffer[0], min4[0], max4[0]);
                        rgbaBuffer += 4;
                    }
                    break;
                default:
                    break;
            }
        }

        const int FLOAT_DECIMALS = 7;
    }

    namespace
    {
        class ClampOp : public Op
        {
        public:
            ClampOp(const float * min4, const float * max4,
                    TransformDirection direction);
            virtual ~ClampOp();

            virtual OpRcPtr clone() const;

            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;

            virtual bool isNoOp() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;

            virtual bool canCombineWith(const OpRcPtr & op) const;
            virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;

            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;

            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
        private:
            float m_min4[4], m_max4[4];

            // Set in finalize
            std::string m_cacheID;
        };

        typedef OCIO_SHARED_PTR<ClampOp> ClampOpRcPtr;


        ClampOp::ClampOp(const float * min4, const float * max4,
                         TransformDirection direction):
                         Op()
        {
            if(direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot create ClampOp with unspecified transform direction.");
            }

            if(direction == TRANSFORM_DIR_INVERSE)
            {
                for(int i=0; i<4; ++i)
                {
                    m_min4[4] = 1.0f;
                    m_max4[4] = 0.0f;
                }
            }
            else
            {
                memcpy(m_min4, min4, 4*sizeof(float));
                memcpy(m_max4, max4, 4*sizeof(float));
            }
        }

        OpRcPtr ClampOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new ClampOp(m_min4, m_max4,
                                             TRANSFORM_DIR_FORWARD));
            return op;
        }

        ClampOp::~ClampOp()
        { }

        std::string ClampOp::getInfo() const
        {
            return "<ClampOp>";
        }

        std::string ClampOp::getCacheID() const
        {
            return m_cacheID;
        }

        bool ClampOp::isNoOp() const
        {
            for (int i = 0; i < 4; ++i)
            {
                if (! emptyRange(m_min4[i], m_max4[i]))
                {
                    return false;
                }
            }
            return true;
        }

        bool ClampOp::isSameType(const OpRcPtr & op) const
        {
            ClampOpRcPtr typedRcPtr = DynamicPtrCast<ClampOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }

        bool ClampOp::isInverse(const OpRcPtr &) const
        {
            return false;
        }

        bool ClampOp::canCombineWith(const OpRcPtr & op) const
        {
            return isSameType(op);
        }

        void ClampOp::combineWith(OpRcPtrVec & ops,
                                  const OpRcPtr & secondOp) const
        {
            ClampOpRcPtr typedRcPtr = DynamicPtrCast<ClampOp>(secondOp);
            if(!typedRcPtr)
            {
                std::ostringstream os;
                os << "ClampOp can only be combined with other ";
                os << "ClampOps.  secondOp:" << secondOp->getInfo();
                throw Exception(os.str().c_str());
            }

            float min4[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            float max4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

            for (int i = 0; i < 4; ++i)
            {
                if (emptyRange(m_min4[i], m_max4[i]))
                {
                    // My range is empty so use the second ops range
                    min4[i] = typedRcPtr->m_min4[i];
                    max4[i] = typedRcPtr->m_max4[i];
                }
                else if (emptyRange(typedRcPtr->m_min4[i], typedRcPtr->m_max4[i]))
                {
                    // The second ops range is empty so use mine
                    min4[i] = m_min4[i];
                    max4[i] = m_max4[i];
                }
                else if (m_max4[i] <= typedRcPtr->m_min4[i])
                {
                    // My range is completely below the second op
                    // so use the second ops min
                    min4[i] = max4[i] = typedRcPtr->m_min4[i];
                }
                else if (m_min4[i] >= typedRcPtr->m_max4[i])
                {
                    // My range is complete above the second op
                    // so use the second ops max
                    min4[i] = max4[i] = typedRcPtr->m_max4[i];
                }
                else
                {
                    // The ranges intersect use the intersection
                    min4[i] = std::max(m_min4[i], typedRcPtr->m_min4[i]);
                    max4[i] = std::min(m_max4[i], typedRcPtr->m_max4[i]);
                }
            }

            if(! allEmptyRanges(min4, max4))
            {
                ops.push_back(
                    ClampOpRcPtr(new ClampOp(min4, max4,
                        TRANSFORM_DIR_FORWARD)));
            }
        }

        bool ClampOp::hasChannelCrosstalk() const
        {
            return false;
        }

        void ClampOp::finalize()
        {
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<ClampOp";
            cacheIDStream.precision(FLOAT_DECIMALS);
            for(int i=0; i<4; ++i)
            {
                cacheIDStream << " " << m_min4[i] << " " << m_max4[i];
            }
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }

        void ClampOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(!rgbaBuffer) return;
            ApplyClamp(rgbaBuffer, numPixels, m_min4, m_max4);
        }

        bool ClampOp::supportsGpuShader() const
        {
            return true;
        }

        void ClampOp::writeGpuShader(std::ostream & shader,
                                     const std::string & pixelName,
                                     const GpuShaderDesc & shaderDesc) const
        {
            GpuLanguage lang = shaderDesc.getLanguage();

            int index = 0;
            if (emptyRange(m_min4[0], m_max4[0]))
                index += 1;
            if (emptyRange(m_min4[1], m_max4[1]))
                index += 2;
            if (emptyRange(m_min4[2], m_max4[2]))
                index += 4;
            if (emptyRange(m_min4[3], m_max4[3]))
                index += 8;

            switch (index)
            {
                case 0: {
                    shader << pixelName << " = clamp("
                           << pixelName << ", "
                           << GpuTextHalf4(m_min4, lang) << ", "
                           << GpuTextHalf4(m_max4, lang) << ");\n";
                    break;
                }
                case 1: {
                    shader << pixelName << ".gba = clamp("
                           << pixelName << ".gba, "
                           << GpuTextHalf3(m_min4 + 1, lang) << ", "
                           << GpuTextHalf3(m_max4 + 1, lang) << ");\n";
                    break;
                }
                case 2: {
                    float min[3] = { m_min4[0], m_min4[2], m_min4[3] };
                    float max[3] = { m_max4[0], m_max4[2], m_max4[3] };
                    shader << pixelName << ".rba = clamp("
                           << pixelName << ".rba, "
                           << GpuTextHalf3(min, lang) << ", "
                           << GpuTextHalf3(max, lang) << ");\n";
                    break;
                }
                case 3: {
                    shader << pixelName << ".ba = clamp("
                           << pixelName << ".ba, "
                           << GpuTextHalf2(m_min4 + 2, lang) << ", "
                           << GpuTextHalf2(m_max4 + 2, lang) << ");\n";
                    break;
                }
                case 4: {
                    float min[3] = { m_min4[0], m_min4[1], m_min4[3] };
                    float max[3] = { m_max4[0], m_max4[1], m_max4[3] };
                    shader << pixelName << ".rga = clamp("
                           << pixelName << ".rga, "
                           << GpuTextHalf3(min, lang) << ", "
                           << GpuTextHalf3(max, lang) << ");\n";
                    break;
                }
                case 5: {
                    float min[2] = { m_min4[1], m_min4[3] };
                    float max[2] = { m_max4[1], m_max4[3] };
                    shader << pixelName << ".ga = clamp("
                           << pixelName << ".ga, "
                           << GpuTextHalf2(min, lang) << ", "
                           << GpuTextHalf2(max, lang) << ");\n";
                    break;
                }
                case 6: {
                    float min[2] = { m_min4[0], m_min4[3] };
                    float max[2] = { m_max4[0], m_max4[3] };
                    shader << pixelName << ".ra = clamp("
                           << pixelName << ".ra, "
                           << GpuTextHalf2(min, lang) << ", "
                           << GpuTextHalf2(max, lang) << ");\n";
                    break;
                }
                case 7: {
                    float min = m_min4[3];
                    float max = m_max4[3];
                    if (lang == GPU_LANGUAGE_CG) {
                        min = ClampToNormHalf(min);
                        max = ClampToNormHalf(max);
                    }
                    shader << pixelName << ".a = clamp("
                           << pixelName << ".a, "
                           << min << ", " << max << ");\n";
                    break;
                }
                case 8: {
                    shader << pixelName << ".rgb = clamp("
                           << pixelName << ".rgb, "
                           << GpuTextHalf3(m_min4, lang) << ", "
                           << GpuTextHalf3(m_max4, lang) << ");\n";
                    break;
                }
                case 9: {
                    shader << pixelName << ".gb = clamp("
                           << pixelName << ".gb, "
                           << GpuTextHalf2(m_min4 + 1, lang) << ", "
                           << GpuTextHalf2(m_max4 + 1, lang) << ");\n";
                    break;
                }
                case 10: {
                    float min[2] = { m_min4[0], m_min4[2] };
                    float max[2] = { m_max4[0], m_max4[2] };
                    shader << pixelName << ".rb = clamp("
                           << pixelName << ".rb, "
                           << GpuTextHalf2(min, lang) << ", "
                           << GpuTextHalf2(max, lang) << ");\n";
                    break;
                }
                case 11: {
                    float min = m_min4[2];
                    float max = m_max4[2];
                    if (lang == GPU_LANGUAGE_CG) {
                        min = ClampToNormHalf(min);
                        max = ClampToNormHalf(max);
                    }
                    shader << pixelName << ".b = clamp("
                           << pixelName << ".b, "
                           << min << ", " << max << ");\n";
                    break;
                }
                case 12: {
                    shader << pixelName << ".rg = clamp("
                           << pixelName << ".rg, "
                           << GpuTextHalf2(m_min4, lang) << ", "
                           << GpuTextHalf2(m_max4, lang) << ");\n";
                    break;
                }
                case 13: {
                    float min = m_min4[1];
                    float max = m_max4[1];
                    if (lang == GPU_LANGUAGE_CG) {
                        min = ClampToNormHalf(min);
                        max = ClampToNormHalf(max);
                    }
                    shader << pixelName << ".g = clamp("
                           << pixelName << ".g, "
                           << min << ", " << max << ");\n";
                    break;
                }
                case 14: {
                    float min = m_min4[0];
                    float max = m_max4[0];
                    if (lang == GPU_LANGUAGE_CG) {
                        min = ClampToNormHalf(min);
                        max = ClampToNormHalf(max);
                    }
                    shader << pixelName << ".r = clamp("
                           << pixelName << ".r, "
                           << min << ", " << max << ");\n";
                    break;
                }
                default:
                    break;
            }
        }
    }  // Anon namespace

    void CreateClampOps(OpRcPtrVec & ops,
                        const float * min4,
                        const float * max4,
                        TransformDirection direction)
    {
        if (allEmptyRanges(min4, max4) || direction == TRANSFORM_DIR_INVERSE)
            return;
        ops.push_back(ClampOpRcPtr(new ClampOp(min4, max4, direction)));
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include "UnitTest.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(ClampOp, Value)
{
    float min4[4] = { 1.0f, 0.1f, 1.0f, 0.1f };
    float max4[4] = { 0.0f, 0.9f, 0.0f, 0.9f };

    OpRcPtrVec ops;
    CreateClampOps(ops, min4, max4, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);

    for(unsigned int i=0; i<ops.size(); ++i)
    {
        ops[i]->finalize();
    }

    const float source[] = { -1.0f, -1.0f, 1.0f, 1.0f };
    const float result[] = { -1.0f,  0.1f, 1.0f, 0.9f };

    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, 1);

    for(unsigned int i=0; i<4; ++i)
    {
        OIIO_CHECK_EQUAL(tmp[i], result[i]);
    }
}

OIIO_ADD_TEST(ClampOps, Combining)
{
    float min1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float max1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float min2[4] = { -2.0f, -2.0f, 0.5f, 1.5f };
    float max2[4] = { -1.0f, 0.5f,  2.0f, 2.0f };

    const float source[] = { -3.0f, -3.0f, -3.0f, -3.0f,
                              3.0f,  3.0f,  3.0f,  3.0f };
    const float result[] = { -1.0f,  0.0f,  0.5f,  1.5f,
                             -1.0f,  0.5f,  1.0f,  1.5f };

    OpRcPtrVec ops;
    CreateClampOps(ops, min1, max1, TRANSFORM_DIR_FORWARD);
    CreateClampOps(ops, min2, max2, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 2);

    float tmp[8];
    memcpy(tmp, source, 8*sizeof(float));
    ops[0]->apply(tmp, 2);
    ops[1]->apply(tmp, 2);
    for(int i=0; i<8; ++i)
    {
        OIIO_CHECK_EQUAL(tmp[i], result[i]);
    }

    OpRcPtrVec combined;
    ops[0]->combineWith(combined, ops[1]);
    OIIO_CHECK_EQUAL(combined.size(), 1);

    memcpy(tmp, source, 8*sizeof(float));
    combined[0]->apply(tmp, 2);
    for(int i=0; i<8; ++i)
    {
        OIIO_CHECK_EQUAL(tmp[i], result[i]);
    }
}

OIIO_ADD_TEST(ClampOps, CombineEmpty)
{
    float min1[4] = { 1.0f, 1.0f, 0.0f, 2.0f };
    float max1[4] = { 0.0f, 0.0f, 1.0f, 3.0f };
    float min2[4] = { 4.0f, 6.0f, 1.0f, 1.0f };
    float max2[4] = { 5.0f, 7.0f, 0.0f, 0.0f };

    const float source[] = { -10.0f, 10.0f, -10.0f, 10.0f };
    const float result[] = {   4.0f,  7.0f,   0.0f,  3.0f };

    OpRcPtrVec ops;
    CreateClampOps(ops, min1, max1, TRANSFORM_DIR_FORWARD);
    CreateClampOps(ops, min2, max2, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 2);

    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, 1);
    ops[1]->apply(tmp, 1);
    for(int i=0; i<4; ++i)
    {
        OIIO_CHECK_EQUAL(tmp[i], result[i]);
    }

    OpRcPtrVec combined;
    ops[0]->combineWith(combined, ops[1]);
    OIIO_CHECK_EQUAL(combined.size(), 1);

    memcpy(tmp, source, 4*sizeof(float));
    combined[0]->apply(tmp, 1);
    for(int i=0; i<4; ++i)
    {
        OIIO_CHECK_EQUAL(tmp[i], result[i]);
    }
}

#endif // OCIO_UNIT_TEST
