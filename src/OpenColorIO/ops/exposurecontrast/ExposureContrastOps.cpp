// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpCPU.h"
#include "ops/exposurecontrast/ExposureContrastOpGPU.h"
#include "ops/exposurecontrast/ExposureContrastOps.h"

OCIO_NAMESPACE_ENTER
{

namespace
{

class ExposureContrastOp;
typedef OCIO_SHARED_PTR<ExposureContrastOp> ExposureContrastOpRcPtr;
typedef OCIO_SHARED_PTR<const ExposureContrastOp> ConstExposureContrastOpRcPtr;

class ExposureContrastOp : public Op
{
public:
    ExposureContrastOp() = delete;
    ExposureContrastOp(const ExposureContrastOp &) = delete;
    explicit ExposureContrastOp(ExposureContrastOpDataRcPtr & ec);

    virtual ~ExposureContrastOp();

    TransformDirection getDirection() const noexcept override { return TRANSFORM_DIR_FORWARD; }

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isIdentity() const override;
    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize(FinalizationFlags fFlags) override;

    bool isDynamic() const override;
    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;
    void replaceDynamicProperty(DynamicPropertyType type,
                                DynamicPropertyImplRcPtr prop) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

protected:
    ConstExposureContrastOpDataRcPtr ecData() const
    { 
        return DynamicPtrCast<const ExposureContrastOpData>(data());
    }
    ExposureContrastOpDataRcPtr ecData()
    { 
        return DynamicPtrCast<ExposureContrastOpData>(data());
    }
};


ExposureContrastOp::ExposureContrastOp(ExposureContrastOpDataRcPtr & ec)
    : Op()
{
    data() = ec;
}

OpRcPtr ExposureContrastOp::clone() const
{
    ExposureContrastOpDataRcPtr f = ecData()->clone();
    return std::make_shared<ExposureContrastOp>(f);
}

ExposureContrastOp::~ExposureContrastOp()
{
}

std::string ExposureContrastOp::getInfo() const
{
    return "<ExposureContrastOp>";
}

bool ExposureContrastOp::isIdentity() const
{
    return ecData()->isIdentity();
}

bool ExposureContrastOp::isSameType(ConstOpRcPtr & op) const
{
    ConstExposureContrastOpRcPtr typedRcPtr = DynamicPtrCast<const ExposureContrastOp>(op);
    return (bool)typedRcPtr;
}

bool ExposureContrastOp::isInverse(ConstOpRcPtr & op) const
{
    ConstExposureContrastOpRcPtr typedRcPtr = DynamicPtrCast<const ExposureContrastOp>(op);
    if (!typedRcPtr) return false;

    ConstExposureContrastOpDataRcPtr ecOpData = typedRcPtr->ecData();
    return ecData()->isInverse(ecOpData);
}

bool ExposureContrastOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void ExposureContrastOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("ExposureContrast can't be combined.");
    }
}

void ExposureContrastOp::finalize(FinalizationFlags /*fFlags*/)
{
    ecData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<ExposureContrastOp ";
    cacheIDStream << ecData()->getCacheID() << " ";
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr ExposureContrastOp::getCPUOp() const
{
    ConstExposureContrastOpDataRcPtr ecOpData = ecData();
    return GetExposureContrastCPURenderer(ecOpData);
}

void ExposureContrastOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    if (getInputBitDepth() != BIT_DEPTH_F32 || getOutputBitDepth() != BIT_DEPTH_F32)
    {
        throw Exception("Only 32F bit depth is supported for the GPU shader");
    }

    ConstExposureContrastOpDataRcPtr ecOpData = ecData();
    GetExposureContrastGPUShaderProgram(shaderDesc, ecOpData);
}

bool ExposureContrastOp::isDynamic() const
{
    return ecData()->isDynamic();
}

bool ExposureContrastOp::hasDynamicProperty(DynamicPropertyType type) const
{
    return ecData()->hasDynamicProperty(type);
}

DynamicPropertyRcPtr ExposureContrastOp::getDynamicProperty(DynamicPropertyType type) const
{
    return ecData()->getDynamicProperty(type);
}

void ExposureContrastOp::replaceDynamicProperty(DynamicPropertyType type,
                                                DynamicPropertyImplRcPtr prop)
{
    ecData()->replaceDynamicProperty(type, prop);
}

}  // Anon namespace


///////////////////////////////////////////////////////////////////////////

void CreateExposureContrastOp(OpRcPtrVec & ops,
                              ExposureContrastOpDataRcPtr & data,
                              TransformDirection direction)
{
    if (direction == TRANSFORM_DIR_FORWARD)
    {
        ops.push_back(std::make_shared<ExposureContrastOp>(data));
    }
    else if(direction == TRANSFORM_DIR_INVERSE)
    {
        ExposureContrastOpDataRcPtr dataInv = data->inverse();
        ops.push_back(std::make_shared<ExposureContrastOp>(dataInv));
    }
    else
    {
        throw Exception("Cannot apply ExposureContrast op, "
                        "unspecified transform direction.");
    }
}

///////////////////////////////////////////////////////////////////////////

void CreateExposureContrastTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto ec = DynamicPtrCast<const ExposureContrastOp>(op);
    if (!ec)
    {
        throw Exception("CreateExposureContrastTransform: op has to be a ExposureContrastOp");
    }
    auto ecData = DynamicPtrCast<const ExposureContrastOpData>(op->data());
    auto ecTransform = ExposureContrastTransform::Create();

    const auto style = ecData->getStyle();

    if (style == ExposureContrastOpData::STYLE_LINEAR_REV ||
        style == ExposureContrastOpData::STYLE_VIDEO_REV ||
        style == ExposureContrastOpData::STYLE_LOGARITHMIC_REV)
    {
        ecTransform->setDirection(TRANSFORM_DIR_INVERSE);
    }
    const auto transformStyle = ConvertStyle(style);
    ecTransform->setStyle(transformStyle);

    auto & formatMetadata = ecTransform->getFormatMetadata();
    auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
    metadata = ecData->getFormatMetadata();

    const double exposure = ecData->getExposure();
    const double contrast = ecData->getContrast();
    const double gamma = ecData->getGamma();
    const double logExposureStep = ecData->getLogExposureStep();
    const double logMidGray = ecData->getLogMidGray();
    const double pivot = ecData->getPivot();

    ecTransform->setExposure(exposure);
    ecTransform->setContrast(contrast);
    ecTransform->setGamma(gamma);
    ecTransform->setLogExposureStep(logExposureStep);
    ecTransform->setLogMidGray(logMidGray);
    ecTransform->setPivot(pivot);

    if (ecData->hasDynamicProperty(DYNAMIC_PROPERTY_EXPOSURE))
    {
        ecTransform->makeExposureDynamic();
    }
    if (ecData->hasDynamicProperty(DYNAMIC_PROPERTY_CONTRAST))
    {
        ecTransform->makeContrastDynamic();
    }
    if (ecData->hasDynamicProperty(DYNAMIC_PROPERTY_GAMMA))
    {
        ecTransform->makeGammaDynamic();
    }

    group->push_back(ecTransform);
}

void BuildExposureContrastOps(OpRcPtrVec & ops,
                              const Config& config,
                              const ExposureContrastTransform & transform,
                              TransformDirection dir)
{
    const TransformDirection combinedDir
        = CombineTransformDirections(dir, transform.getDirection());

    auto data = std::make_shared<ExposureContrastOpData>(
        BIT_DEPTH_F32, BIT_DEPTH_F32,
        ExposureContrastOpData::ConvertStyle(transform.getStyle(), combinedDir));

    data->setExposure(transform.getExposure());
    bool dyn = transform.isExposureDynamic();
    if (dyn) data->getExposureProperty()->makeDynamic();
    data->setContrast(transform.getContrast());
    dyn = transform.isContrastDynamic();
    if (dyn) data->getContrastProperty()->makeDynamic();
    data->setGamma(transform.getGamma());
    dyn = transform.isGammaDynamic();
    if (dyn) data->getGammaProperty()->makeDynamic();
    data->setPivot(transform.getPivot());
    data->setLogExposureStep(transform.getLogExposureStep());
    data->setLogMidGray(transform.getLogMidGray());
    // NB: Always use Forward here since direction is handled with the style above.
    CreateExposureContrastOp(ops, data, TRANSFORM_DIR_FORWARD);
}

}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(ExposureContrastOp, create)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();
    OCIO::OpRcPtrVec ops;

    // Make it dynamic so that it is not a no op.
    data->getExposureProperty()->makeDynamic();
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<ExposureContrastOp>");

    data = data->clone();
    data->getContrastProperty()->makeDynamic();
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
}

OCIO_ADD_TEST(ExposureContrastOp, inverse)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();

    data->setExposure(1.2);
    data->setPivot(0.5);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    data = data->clone();
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));

    // Create op2 similar to op1 with different exposure.
    data = data->clone();
    data->setExposure(1.3);
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_REQUIRE_ASSERT(ops[2]);

    OCIO::ConstOpRcPtr op2 = ops[2];
    // As exposure from E/C is not dynamic and exposure is different,
    // op1 and op2 are different.
    OCIO_CHECK_ASSERT(op1 != op2);
    OCIO_CHECK_ASSERT(!op0->isInverse(op2));

    // With dynamic property.
    data = data->clone();
    data->getExposureProperty()->makeDynamic();
    OCIO::DynamicPropertyRcPtr dp3 = data->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO_REQUIRE_ASSERT(ops[3]);
    OCIO::ConstOpRcPtr op3 = ops[3];

    data = data->clone();
    OCIO::DynamicPropertyRcPtr dp4 = data->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);

    direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO_REQUIRE_ASSERT(ops[4]);

    // Exposure dynamic, same value, opposite direction.
    OCIO_CHECK_ASSERT(ops[4]->isInverse(op3));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op1));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op0));

    // Value is not taken into account when dynamic property is enabled.
    dp4->setValue(-1.);
    OCIO_CHECK_ASSERT(dp3->getDoubleValue() != dp4->getDoubleValue());
    OCIO_CHECK_ASSERT(ops[4]->isInverse(op3));
}

OCIO_ADD_TEST(ExposureContrastOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();

    data->getContrastProperty()->makeDynamic();
    data->setExposure(1.2);
    data->setPivot(0.5);
    data->setLogExposureStep(0.09);
    data->setLogMidGray(0.7);
    data->getFormatMetadata().addAttribute("name", "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateExposureContrastTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto ecTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExposureContrastTransform>(transform);
    OCIO_REQUIRE_ASSERT(ecTransform);

    const auto & metadata = ecTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(ecTransform->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ecTransform->getExposure(), data->getExposure());
    OCIO_CHECK_EQUAL(ecTransform->isExposureDynamic(), false);
    OCIO_CHECK_EQUAL(ecTransform->getContrast(), data->getContrast());
    OCIO_CHECK_EQUAL(ecTransform->isContrastDynamic(), true);
    OCIO_CHECK_EQUAL(ecTransform->getGamma(), data->getGamma());
    OCIO_CHECK_EQUAL(ecTransform->isGammaDynamic(), false);
    OCIO_CHECK_EQUAL(ecTransform->getPivot(), data->getPivot());
    OCIO_CHECK_EQUAL(ecTransform->getLogExposureStep(), data->getLogExposureStep());
    OCIO_CHECK_EQUAL(ecTransform->getLogMidGray(), data->getLogMidGray());
}

#endif