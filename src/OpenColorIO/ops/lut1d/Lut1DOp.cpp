// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "HashUtils.h"
#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut1d/Lut1DOpCPU.h"
#include "ops/lut1d/Lut1DOpGPU.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/OpTools.h"
#include "SSE.h"
#include "transforms/Lut1DTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{
class Lut1DOp;
typedef OCIO_SHARED_PTR<Lut1DOp> Lut1DOpRcPtr;
typedef OCIO_SHARED_PTR<const Lut1DOp> ConstLut1DOpRcPtr;

class Lut1DOp : public Op
{
public:
    Lut1DOp() = delete;
    Lut1DOp(const Lut1DOp &) = delete;
    explicit Lut1DOp(Lut1DOpDataRcPtr & lutData);
    virtual ~Lut1DOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;
    bool hasChannelCrosstalk() const override;
    void finalize() override;
    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    bool supportedByLegacyShader() const override { return false; }
    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

    ConstLut1DOpDataRcPtr lut1DData() const { return DynamicPtrCast<const Lut1DOpData>(data()); }
    Lut1DOpDataRcPtr lut1DData() { return DynamicPtrCast<Lut1DOpData>(data()); }
};

Lut1DOp::Lut1DOp(Lut1DOpDataRcPtr & lut1D)
{
    data() = lut1D;
}

Lut1DOp::~Lut1DOp()
{
}

OpRcPtr Lut1DOp::clone() const
{
    Lut1DOpDataRcPtr lut = lut1DData()->clone();
    return std::make_shared<Lut1DOp>(lut);
}

std::string Lut1DOp::getInfo() const
{
    return "<Lut1DOp>";
}

bool Lut1DOp::isSameType(ConstOpRcPtr & op) const
{
    ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(op);
    return (bool)typedRcPtr;
}

bool Lut1DOp::isInverse(ConstOpRcPtr & op) const
{
    ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(op);
    if (typedRcPtr)
    {
        ConstLut1DOpDataRcPtr lutData = typedRcPtr->lut1DData();
        return lut1DData()->isInverse(lutData);
    }

    return false;
}

bool Lut1DOp::canCombineWith(ConstOpRcPtr & op) const
{
    ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(op);
    if (typedRcPtr)
    {
        ConstLut1DOpDataRcPtr lutData = typedRcPtr->lut1DData();
        return lut1DData()->mayCompose(lutData);
    }

    // TODO: drop a clampIdentity Range after an Inverse LUT.
    return false;
}

void Lut1DOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("Lut1DOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
    ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(secondOp);
    auto secondLut = typedRcPtr->lut1DData();

    // We want compose to upsample the LUTs to minimize precision loss.
    const auto compFlag = Lut1DOpData::COMPOSE_RESAMPLE_BIG;
    auto thisLut = lut1DData();
    Lut1DOpDataRcPtr result =  Lut1DOpData::Compose(thisLut, secondLut, compFlag);
    auto composedOp = std::make_shared<Lut1DOp>(result);
    ops.push_back(composedOp);
}

bool Lut1DOp::hasChannelCrosstalk() const
{
    return lut1DData()->hasChannelCrosstalk();
}

void Lut1DOp::finalize()
{
    lut1DData()->finalize();
}

std::string Lut1DOp::getCacheID() const
{
    // Rebuild the cache identifier.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<Lut1D ";
    cacheIDStream << lut1DData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

ConstOpCPURcPtr Lut1DOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstLut1DOpDataRcPtr data = lut1DData();
    return GetLut1DRenderer(data, BIT_DEPTH_F32, BIT_DEPTH_F32);
}

void Lut1DOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstLut1DOpDataRcPtr lutData = lut1DData();
    if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        // TODO: Even if the optim flags specify exact inversion, only fast inversion is supported
        // on the GPU. Add GPU renderer for EXACT mode.

        Lut1DOpDataRcPtr tmp = MakeFastLut1DFromInverse(lutData);
        if (!tmp)
        {
            throw Exception("Cannot apply Lut1DOp, inversion failed.");
        }

        lutData = tmp;
    }

    GetLut1DGPUShaderProgram(shaderCreator, lutData);
}
}

void CreateLut1DOp(OpRcPtrVec & ops,
                    Lut1DOpDataRcPtr & lut,
                    TransformDirection direction)
{
    // TODO: Detect if 1D LUT can be exactly approximated as y = mx + b
    // If so, return a mtx instead.

    Lut1DOpDataRcPtr lutData = lut;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        lutData = lut->inverse();
    }

    ops.push_back(std::make_shared<Lut1DOp>(lutData));
}

void GenerateIdentityLut1D(float* img, int numElements, int numChannels)
{
    if(!img) return;
    int numChannelsToFill = std::min(3, numChannels);

    float scale = 1.0f / ((float) numElements - 1.0f);
    for(int i=0; i<numElements; i++)
    {
        for(int c=0; c<numChannelsToFill; ++c)
        {
            img[numChannels*i+c] = scale * (float)(i);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void CreateLut1DTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto lut = DynamicPtrCast<const Lut1DOp>(op);
    if (!lut)
    {
        throw Exception("CreateLut1DTransform: op has to be a Lut1DOp");
    }
    auto lutData = DynamicPtrCast<const Lut1DOpData>(op->data());
    auto lutTransform = Lut1DTransform::Create();
    auto & data = dynamic_cast<Lut1DTransformImpl *>(lutTransform.get())->data();

    data = *lutData;
    group->appendTransform(lutTransform);
}

void BuildLut1DOp(OpRcPtrVec & ops,
                  const Lut1DTransform & transform,
                  TransformDirection dir)
{
    const auto & data = dynamic_cast<const Lut1DTransformImpl &>(transform).data();
    data.validate();

    auto lut = data.clone();
    CreateLut1DOp(ops, lut, dir);
}

} // namespace OCIO_NAMESPACE

