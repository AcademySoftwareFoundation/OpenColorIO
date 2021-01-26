// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/lut3d/Lut3DOpCPU.h"
#include "ops/lut3d/Lut3DOpGPU.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/OpTools.h"
#include "transforms/Lut3DTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{
// Linear
inline float lerp(float a, float b, float z)
{
    return (b - a) * z + a;
}

inline void lerp_rgb(float * out, float * a, float * b, float * z)
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

inline void lerp_rgb(float * out, float * a, float * b, float * c,
                     float * d, float * y, float * z)
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

inline void lerp_rgb(float * out, float * a, float * b, float * c, float * d,
                     float * e, float * f, float * g, float * h,
                     float * x, float * y, float * z)
{
    float v1[3];
    float v2[3];

    lerp_rgb(v1, a,b,c,d,y,z);
    lerp_rgb(v2, e,f,g,h,y,z);
    lerp_rgb(out, v1, v2, x);
}

inline float lookupNearest_3D(int rIndex, int gIndex, int bIndex,
                              int size_red, int size_green, int size_blue,
                              const float * simple_rgb_lut, int channelIndex)
{
    return simple_rgb_lut[GetLut3DIndex_RedFast(rIndex, gIndex, bIndex,
                                                size_red, size_green, size_blue)
                          + channelIndex];
}

inline void lookupNearest_3D_rgb(float * rgb,
                                 int rIndex, int gIndex, int bIndex,
                                 int size_red, int size_green, int size_blue,
                                 const float * simple_rgb_lut)
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

}

void GenerateIdentityLut3D(float * img, int edgeLen, int numChannels, Lut3DOrder lut3DOrder)
{
    if (!img) return;
    if (numChannels < 3)
    {
        throw Exception("Cannot generate idenitity 3d LUT with less than 3 channels.");
    }

    float c = 1.0f / ((float)edgeLen - 1.0f);

    if (lut3DOrder == LUT3DORDER_FAST_RED)
    {
        for (int i = 0; i < edgeLen*edgeLen*edgeLen; i++)
        {
            img[numChannels*i + 0] = (float)(i%edgeLen) * c;
            img[numChannels*i + 1] = (float)((i / edgeLen) % edgeLen) * c;
            img[numChannels*i + 2] = (float)((i / edgeLen / edgeLen) % edgeLen) * c;
        }
    }
    else if (lut3DOrder == LUT3DORDER_FAST_BLUE)
    {
        for (int i = 0; i < edgeLen*edgeLen*edgeLen; i++)
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
    Lut3DOp() = delete;
    Lut3DOp(const Lut3DOp &) = delete;
    explicit Lut3DOp(Lut3DOpDataRcPtr & data);
    virtual ~Lut3DOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;
    bool hasChannelCrosstalk() const override;
    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    bool supportedByLegacyShader() const override { return false; }
    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstLut3DOpDataRcPtr lut3DData() const
    {
        return DynamicPtrCast<const Lut3DOpData>(data());
    }

    Lut3DOpDataRcPtr lut3DData()
    {
        return DynamicPtrCast<Lut3DOpData>(data());
    }
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
    return op->data()->getType() == OpData::Lut3DType;
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

bool Lut3DOp::canCombineWith(ConstOpRcPtr & op) const
{
    if (isSameType(op))
    {
        return true;
    }
    return false;

    // TODO: Anything following the LUT 3D could be combined into it.  Just need to be careful
    // about causing the interpolation to become worse.
    // TODO: drop a clampIdentity Range after an Inverse LUT.
}

void Lut3DOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("Lut3DOp: canCombineWith must be checked before calling combineWith.");
    }
    ConstLut3DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut3DOp>(secondOp);
    auto secondLut = typedRcPtr->lut3DData();
    auto thisLut = lut3DData();
    auto composed = Lut3DOpData::Compose(thisLut, secondLut);
    auto composedOp = std::make_shared<Lut3DOp>(composed);
    ops.push_back(composedOp);
}

bool Lut3DOp::hasChannelCrosstalk() const
{
    return lut3DData()->hasChannelCrosstalk();
}

std::string Lut3DOp::getCacheID() const
{
    std::ostringstream cacheIDStream;
    cacheIDStream << "<Lut3D ";
    cacheIDStream << lut3DData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

ConstOpCPURcPtr Lut3DOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstLut3DOpDataRcPtr data = lut3DData();
    return GetLut3DRenderer(data);
}

void Lut3DOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstLut3DOpDataRcPtr lutData = lut3DData();
    if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        // TODO: Add GPU renderer for EXACT mode.

        Lut3DOpDataRcPtr tmp = MakeFastLut3DFromInverse(lutData);
        if (!tmp)
        {
            throw Exception("Cannot apply Lut3DOp, inversion failed.");
        }

        lutData = tmp;
    }

    GetLut3DGPUShaderProgram(shaderCreator, lutData);
}
}

void CreateLut3DOp(OpRcPtrVec & ops, Lut3DOpDataRcPtr & lut, TransformDirection direction)
{
    Lut3DOpDataRcPtr lutData = lut;

    if (direction == TRANSFORM_DIR_INVERSE)
    {
        lutData = lut->inverse();
    }

    ops.push_back(std::make_shared<Lut3DOp>(lutData));
}

void CreateLut3DTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto lut = DynamicPtrCast<const Lut3DOp>(op);
    if (!lut)
    {
        throw Exception("CreateLut3DTransform: op has to be a Lut3DOp");
    }
    auto lutData = DynamicPtrCast<const Lut3DOpData>(op->data());
    auto lutTransform = Lut3DTransform::Create();
    Lut3DOpData & data = dynamic_cast<Lut3DTransformImpl*>(lutTransform.get())->data();

    data = *lutData;

    group->appendTransform(lutTransform);
}

void BuildLut3DOp(OpRcPtrVec & ops,
                  const Lut3DTransform & transform,
                  TransformDirection dir)
{
    const auto & data = dynamic_cast<const Lut3DTransformImpl &>(transform).data();
    data.validate();

    auto lut = data.clone();
    CreateLut3DOp(ops, lut, dir);
}

} // namespace OCIO_NAMESPACE

