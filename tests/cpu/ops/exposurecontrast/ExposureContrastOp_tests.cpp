// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/exposurecontrast/ExposureContrastOp.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


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
    auto dp3 = data->getExposureProperty();
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO_REQUIRE_ASSERT(ops[3]);
    OCIO::ConstOpRcPtr op3 = ops[3];

    data = data->clone();
    auto dp4 = data->getExposureProperty();

    direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO_REQUIRE_ASSERT(ops[4]);

    // Exposure dynamic, same value, opposite direction.
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op3));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op1));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op0));

    // When dynamic property is enabled they are never equal.
    dp4->setValue(-1.);
    OCIO_CHECK_ASSERT(dp3->getValue() != dp4->getValue());
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op3));
    dp3->setValue(-1.);
    OCIO_CHECK_ASSERT(dp3->getValue() == dp4->getValue());
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op3));
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
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
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

