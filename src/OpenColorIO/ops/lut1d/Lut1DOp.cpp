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
    void finalize(OptimizationFlags oFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

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
    auto thisLut = lut1DData()->clone();
    auto secondLut = typedRcPtr->lut1DData();

    // We want compose to upsample the LUTs to minimize precision loss.
    const auto compFlag = Lut1DOpData::COMPOSE_RESAMPLE_BIG;
    Lut1DOpData::Compose(thisLut, secondLut, compFlag);
    auto composedOp = std::make_shared<Lut1DOp>(thisLut);
    ops.push_back(composedOp);
}

bool Lut1DOp::hasChannelCrosstalk() const
{
    return lut1DData()->hasChannelCrosstalk();
}

void Lut1DOp::finalize(OptimizationFlags oFlags)
{
    Lut1DOpDataRcPtr lutData = lut1DData();

    const bool invLutFast = (oFlags & OPTIMIZATION_LUT_INV_FAST) == OPTIMIZATION_LUT_INV_FAST;
    lutData->setInversionQuality(invLutFast ? LUT_INVERSION_FAST: LUT_INVERSION_EXACT);

    lutData->finalize();

    // Rebuild the cache identifier.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<Lut1D ";
    cacheIDStream << lutData->getCacheID();
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr Lut1DOp::getCPUOp() const
{
    ConstLut1DOpDataRcPtr data = lut1DData();
    return GetLut1DRenderer(data, BIT_DEPTH_F32, BIT_DEPTH_F32);
}

void Lut1DOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstLut1DOpDataRcPtr lutData = lut1DData();
    if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        // TODO: Add GPU renderer for EXACT mode.

        Lut1DOpDataRcPtr tmp = Lut1DOpData::MakeFastLut1DFromInverse(lutData, true);
        if (!tmp)
        {
            throw Exception("Cannot apply Lut1DOp, inversion failed.");
        }

        tmp->finalize();

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

    if (direction != TRANSFORM_DIR_FORWARD
        && direction != TRANSFORM_DIR_INVERSE)
    {
        throw Exception("Cannot apply Lut1DOp op, "
                        "unspecified transform direction.");
    }

    if (direction == TRANSFORM_DIR_FORWARD)
    {
        ops.push_back(std::make_shared<Lut1DOp>(lut));
    }
    else
    {
        Lut1DOpDataRcPtr data = lut->inverse();
        ops.push_back(std::make_shared<Lut1DOp>(data));
    }
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
                  const Config & config,
                  const Lut1DTransform & transform,
                  TransformDirection dir)
{
    const auto & data = dynamic_cast<const Lut1DTransformImpl &>(transform).data();
    data.validate();

    auto lut = data.clone();
    CreateLut1DOp(ops, lut, dir);
}

} // namespace OCIO_NAMESPACE

