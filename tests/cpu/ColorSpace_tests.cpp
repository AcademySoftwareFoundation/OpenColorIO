// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include "ColorSpace.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ColorSpace, basic)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_REQUIRE_EQUAL(cs->getReferenceSpaceType(), OCIO::REFERENCE_SPACE_SCENE);

    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_REQUIRE_EQUAL(cs->getReferenceSpaceType(), OCIO::REFERENCE_SPACE_DISPLAY);

    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_REQUIRE_EQUAL(cs->getReferenceSpaceType(), OCIO::REFERENCE_SPACE_SCENE);

    OCIO_CHECK_EQUAL(std::string(""), cs->getName());
    OCIO_CHECK_EQUAL(std::string(""), cs->getFamily());
    OCIO_CHECK_EQUAL(std::string(""), cs->getDescription());
    OCIO_CHECK_EQUAL(std::string(""), cs->getEqualityGroup());
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UNKNOWN, cs->getBitDepth());
    OCIO_CHECK_ASSERT(!cs->isData());
    OCIO_CHECK_EQUAL(OCIO::ALLOCATION_UNIFORM, cs->getAllocation());
    OCIO_CHECK_EQUAL(0, cs->getAllocationNumVars());

    cs->setName("name");
    OCIO_CHECK_EQUAL(std::string("name"), cs->getName());
    cs->setFamily("family");
    OCIO_CHECK_EQUAL(std::string("family"), cs->getFamily());
    cs->setDescription("description");
    OCIO_CHECK_EQUAL(std::string("description"), cs->getDescription());
    cs->setEqualityGroup("equalitygroup");
    OCIO_CHECK_EQUAL(std::string("equalitygroup"), cs->getEqualityGroup());
    cs->setBitDepth(OCIO::BIT_DEPTH_F16);
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F16, cs->getBitDepth());
    cs->setIsData(true);
    OCIO_CHECK_ASSERT(cs->isData());
    cs->setAllocation(OCIO::ALLOCATION_UNKNOWN);
    OCIO_CHECK_EQUAL(OCIO::ALLOCATION_UNKNOWN, cs->getAllocation());
    const float vars[2]{ 1.f, 2.f };
    cs->setAllocationVars(2, vars);
    OCIO_CHECK_EQUAL(2, cs->getAllocationNumVars());
    float readVars[2]{};
    cs->getAllocationVars(readVars);
    OCIO_CHECK_EQUAL(1.f, readVars[0]);
    OCIO_CHECK_EQUAL(2.f, readVars[1]);

    std::ostringstream oss;
    oss << *cs;
    OCIO_CHECK_EQUAL(oss.str().size(), 149);
}

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

