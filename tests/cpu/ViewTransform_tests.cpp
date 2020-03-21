// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"
#include "ViewTransform.cpp"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ViewTransform, basic)
{
    OCIO::ViewTransformRcPtr vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    OCIO_REQUIRE_ASSERT(vt);
    OCIO_CHECK_EQUAL(OCIO::REFERENCE_SPACE_SCENE, vt->getReferenceSpaceType());
    OCIO_CHECK_EQUAL(std::string(""), vt->getName());
    OCIO_CHECK_EQUAL(std::string(""), vt->getFamily());
    OCIO_CHECK_EQUAL(std::string(""), vt->getDescription());
    OCIO_CHECK_EQUAL(0, vt->getNumCategories());

    vt->setName("name");
    OCIO_CHECK_EQUAL(std::string("name"), vt->getName());
    vt->setFamily("family");
    OCIO_CHECK_EQUAL(std::string("family"), vt->getFamily());
    vt->setDescription("description");
    OCIO_CHECK_EQUAL(std::string("description"), vt->getDescription());

    OCIO_CHECK_EQUAL(vt->getNumCategories(), 0);

    OCIO_CHECK_ASSERT(!vt->hasCategory("linear"));
    OCIO_CHECK_ASSERT(!vt->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!vt->hasCategory("log"));

    OCIO_CHECK_NO_THROW(vt->addCategory("linear"));
    OCIO_CHECK_NO_THROW(vt->addCategory("rendering"));
    OCIO_CHECK_EQUAL(vt->getNumCategories(), 2);

    OCIO_CHECK_ASSERT(vt->hasCategory("linear"));
    OCIO_CHECK_ASSERT(vt->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!vt->hasCategory("log"));

    OCIO_CHECK_EQUAL(std::string(vt->getCategory(0)), std::string("linear"));
    OCIO_CHECK_EQUAL(std::string(vt->getCategory(1)), std::string("rendering"));
    // Check with an invalid index.
    OCIO_CHECK_NO_THROW(vt->getCategory(2));
    OCIO_CHECK_ASSERT(vt->getCategory(2) == nullptr);

    OCIO_CHECK_NO_THROW(vt->removeCategory("linear"));
    OCIO_CHECK_EQUAL(vt->getNumCategories(), 1);
    OCIO_CHECK_ASSERT(!vt->hasCategory("linear"));
    OCIO_CHECK_ASSERT(vt->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!vt->hasCategory("log"));

    // Remove a category not in the view transform.
    OCIO_CHECK_NO_THROW(vt->removeCategory("log"));
    OCIO_CHECK_EQUAL(vt->getNumCategories(), 1);
    OCIO_CHECK_ASSERT(vt->hasCategory("rendering"));

    OCIO_CHECK_NO_THROW(vt->clearCategories());
    OCIO_CHECK_EQUAL(vt->getNumCategories(), 0);

    OCIO::ViewTransformRcPtr vtd = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    OCIO_REQUIRE_ASSERT(vtd);
    OCIO_CHECK_EQUAL(OCIO::REFERENCE_SPACE_DISPLAY, vtd->getReferenceSpaceType());
}
