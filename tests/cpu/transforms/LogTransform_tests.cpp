// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "transforms/LogTransform.cpp"

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(LogTransform, basic)
{
    const OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();

    OCIO_CHECK_EQUAL(log->getBase(), 2.0f);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    log->setBase(10.0f);
    OCIO_CHECK_EQUAL(log->getBase(), 10.0f);
}

