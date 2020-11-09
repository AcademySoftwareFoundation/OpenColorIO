// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>
#include <cstring>
#include <sstream>
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "ops/exponent/ExponentOp.h"
#include "GpuShaderUtils.h"
#include "MathUtils.h"

namespace OCIO_NAMESPACE
{
namespace DefaultValues
{
const int FLOAT_DECIMALS = 7;
}

ExponentOpData::ExponentOpData()
    :   OpData()
{
    for (unsigned i = 0; i < 4; ++i)
    {
        m_exp4[i] = 1.0;
    }
}

ExponentOpData::ExponentOpData(const ExponentOpData & rhs)
    :   OpData()
{
    if (this != &rhs)
    {
        *this = rhs;
    }
}

ExponentOpData::ExponentOpData(const double * exp4)
    :   OpData()
{
    memcpy(m_exp4, exp4, 4*sizeof(double));
}

ExponentOpData & ExponentOpData::operator = (const ExponentOpData & rhs)
{
    if (this != &rhs)
    {
        OpData::operator=(rhs);
        memcpy(m_exp4, rhs.m_exp4, sizeof(double)*4);
    }

    return *this;
}

bool ExponentOpData::isNoOp() const
{
    return isIdentity();
}

bool ExponentOpData::isIdentity() const
{
    return IsVecEqualToOne(m_exp4, 4);
}

std::string ExponentOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    // Create the cacheID.
    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);
    for (int i = 0; i < 4; ++i)
    {
        cacheIDStream << m_exp4[i] << " ";
    }

    return cacheIDStream.str();
}

void ExponentOpData::validate() const
{
}

namespace
{
class ExponentOpCPU : public OpCPU
{
public:
    ExponentOpCPU(ConstExponentOpDataRcPtr exp) : OpCPU(), m_data(exp) {}
    virtual ~ExponentOpCPU() {}

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    ConstExponentOpDataRcPtr m_data;
};

void ExponentOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const float exp[4] = { float(m_data->m_exp4[0]),
                            float(m_data->m_exp4[1]),
                            float(m_data->m_exp4[2]),
                            float(m_data->m_exp4[3]) };

    for (long pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex)
    {
        out[0] = powf( std::max(0.0f, in[0]), exp[0]);
        out[1] = powf( std::max(0.0f, in[1]), exp[1]);
        out[2] = powf( std::max(0.0f, in[2]), exp[2]);
        out[3] = powf( std::max(0.0f, in[3]), exp[3]);

        in  += 4;
        out += 4;
    }
}

class ExponentOp : public Op
{
public:
    ExponentOp() = delete;
    ExponentOp(ExponentOp & exp) = delete;

    explicit ExponentOp(const double * exp4);
    explicit ExponentOp(ExponentOpDataRcPtr & exp);

    virtual ~ExponentOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;

    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstExponentOpDataRcPtr expData() const { return DynamicPtrCast<const ExponentOpData>(data()); }
    ExponentOpDataRcPtr expData() { return DynamicPtrCast<ExponentOpData>(data()); }

};

typedef OCIO_SHARED_PTR<ExponentOp> ExponentOpRcPtr;
typedef OCIO_SHARED_PTR<const ExponentOp> ConstExponentOpRcPtr;

ExponentOp::ExponentOp(const double * exp4)
    : Op()
{
    data().reset(new ExponentOpData(exp4));
}

ExponentOp::ExponentOp(ExponentOpDataRcPtr & exp)
    : Op()
{
    data() = exp;
}

OpRcPtr ExponentOp::clone() const
{
    return std::make_shared<ExponentOp>(expData()->m_exp4);
}

ExponentOp::~ExponentOp()
{
}

std::string ExponentOp::getInfo() const
{
    return "<ExponentOp>";
}

bool ExponentOp::isSameType(ConstOpRcPtr & op) const
{
    ConstExponentOpRcPtr typedRcPtr = DynamicPtrCast<const ExponentOp>(op);
    if (!typedRcPtr) return false;
    return true;
}

bool ExponentOp::isInverse(ConstOpRcPtr & /*op*/) const
{
    // It is simpler to handle a pair of inverses by combining them and then removing
    // the identity.  So we just return false here.

    return false;
}

bool ExponentOp::canCombineWith(ConstOpRcPtr & op) const
{
    return isSameType(op);
}

void ExponentOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("ExponentOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
    ConstExponentOpRcPtr typedRcPtr = DynamicPtrCast<const ExponentOp>(secondOp);

    const double combined[4]
        = { expData()->m_exp4[0]*typedRcPtr->expData()->m_exp4[0],
            expData()->m_exp4[1]*typedRcPtr->expData()->m_exp4[1],
            expData()->m_exp4[2]*typedRcPtr->expData()->m_exp4[2],
            expData()->m_exp4[3]*typedRcPtr->expData()->m_exp4[3] };

    if (!IsVecEqualToOne(combined, 4))
    {
        auto combinedOp = std::make_shared<ExponentOp>(combined);

        // Combine metadata.
        // TODO: May want to revisit how the metadata is set.
        FormatMetadataImpl newDesc = expData()->getFormatMetadata();
        newDesc.combine(typedRcPtr->expData()->getFormatMetadata());
        combinedOp->expData()->getFormatMetadata() = newDesc;

        ops.push_back(combinedOp);
    }
}

std::string ExponentOp::getCacheID() const
{
    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<ExponentOp ";
    cacheIDStream << expData()->getCacheID();
    cacheIDStream << ">";
    return cacheIDStream.str();
}

ConstOpCPURcPtr ExponentOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    return std::make_shared<ExponentOpCPU>(expData());
}

void ExponentOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add an Exponent processing";
    ss.newLine() << "";

    // outColor = pow(max(outColor, 0.), exp);

    ss.newLine()
        << shaderCreator->getPixelName()
        << " = pow( "
        << "max( " << shaderCreator->getPixelName()
        << ", " << ss.float4Const(0.0f) << " )"
        << ", " << ss.float4Const(expData()->m_exp4[0], expData()->m_exp4[1],
                                    expData()->m_exp4[2], expData()->m_exp4[3]) << " );";

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

}  // Anon namespace



void CreateExponentOp(OpRcPtrVec & ops,
                      const double(&vec4)[4],
                      TransformDirection direction)
{
    ExponentOpDataRcPtr expData = std::make_shared<ExponentOpData>(vec4);
    CreateExponentOp(ops, expData, direction);
}

void CreateExponentOp(OpRcPtrVec & ops,
                      ExponentOpDataRcPtr & expData,
                      TransformDirection direction)
{
    switch (direction)
    {
    case TRANSFORM_DIR_FORWARD:
    {
        ops.push_back(std::make_shared<ExponentOp>(expData));
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        double values[4];
        for (int i = 0; i<4; ++i)
        {
            if (!IsScalarEqualToZero(expData->m_exp4[i]))
            {
                values[i] = 1.0 / expData->m_exp4[i];
            }
            else
            {
                throw Exception("Cannot apply ExponentOp op, Cannot apply 0.0 exponent in the inverse.");
            }
        }
        ExponentOpDataRcPtr expInv = std::make_shared<ExponentOpData>(values);
        ops.push_back(std::make_shared<ExponentOp>(expInv));
        break;
    }
    }
}

void CreateExponentTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto exp = DynamicPtrCast<const ExponentOp>(op);
    if (!exp)
    {
        throw Exception("CreateExponentTransform: op has to be a ExponentOp");
    }
    auto expTransform = ExponentTransform::Create();

    auto expData = DynamicPtrCast<const ExponentOpData>(op->data());
    auto & formatMetadata = expTransform->getFormatMetadata();
    auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
    metadata = expData->getFormatMetadata();

    expTransform->setValue(expData->m_exp4);

    group->appendTransform(expTransform);
}


} // namespace OCIO_NAMESPACE

