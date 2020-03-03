// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ColorSpace.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ColorSpace, category)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    OCIO_CHECK_EQUAL(cs->getNumCategories(), 0);

    OCIO_CHECK_ASSERT(!cs->hasCategory("linear"));
    OCIO_CHECK_ASSERT(!cs->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!cs->hasCategory("log"));

    OCIO_CHECK_NO_THROW(cs->addCategory("linear"));
    OCIO_CHECK_NO_THROW(cs->addCategory("rendering"));
    OCIO_CHECK_EQUAL(cs->getNumCategories(), 2);

    OCIO_CHECK_ASSERT(cs->hasCategory("linear"));
    OCIO_CHECK_ASSERT(cs->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!cs->hasCategory("log"));

    OCIO_CHECK_EQUAL(std::string(cs->getCategory(0)), std::string("linear"));
    OCIO_CHECK_EQUAL(std::string(cs->getCategory(1)), std::string("rendering"));
    // Check with an invalid index.
    OCIO_CHECK_NO_THROW(cs->getCategory(2));
    OCIO_CHECK_ASSERT(cs->getCategory(2) == nullptr);

    OCIO_CHECK_NO_THROW(cs->removeCategory("linear"));
    OCIO_CHECK_EQUAL(cs->getNumCategories(), 1);
    OCIO_CHECK_ASSERT(!cs->hasCategory("linear"));
    OCIO_CHECK_ASSERT(cs->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!cs->hasCategory("log"));

    // Remove a category not in the color space.
    OCIO_CHECK_NO_THROW(cs->removeCategory("log"));
    OCIO_CHECK_EQUAL(cs->getNumCategories(), 1);
    OCIO_CHECK_ASSERT(cs->hasCategory("rendering"));

    OCIO_CHECK_NO_THROW(cs->clearCategories());
    OCIO_CHECK_EQUAL(cs->getNumCategories(), 0);
}

