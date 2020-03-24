// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/GroupTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GroupTransform, basic)
{
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO_CHECK_EQUAL(group->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    group->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(group->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_EQUAL(group->getNumTransforms(), 0);

    auto & groupData = group->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(groupData.getName()), OCIO::METADATA_ROOT);
    OCIO_CHECK_EQUAL(groupData.getNumAttributes(), 0);
    OCIO_CHECK_EQUAL(groupData.getNumChildrenElements(), 0);

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    group->appendTransform(matrix);
    OCIO::FixedFunctionTransformRcPtr ff =  OCIO::FixedFunctionTransform::Create();
    group->appendTransform(ff);

    OCIO_CHECK_EQUAL(group->getNumTransforms(), 2);

    auto t0 = group->getTransform(0);
    auto m0 = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(t0);
    OCIO_CHECK_ASSERT(m0);

    auto t1 = group->getTransform(1);
    auto ff1 = OCIO_DYNAMIC_POINTER_CAST<OCIO::FixedFunctionTransform>(t1);
    OCIO_CHECK_ASSERT(ff1);

    auto & metadata = group->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(metadata.getName()), OCIO::METADATA_ROOT);
    OCIO_CHECK_EQUAL(std::string(metadata.getValue()), "");
    OCIO_CHECK_EQUAL(metadata.getNumAttributes(), 0);
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 0);
    metadata.addAttribute("att1", "val1");
    metadata.addChildElement("child1", "content1");
}

