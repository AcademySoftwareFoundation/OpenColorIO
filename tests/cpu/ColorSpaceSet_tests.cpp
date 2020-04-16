// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ColorSpaceSet.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ColorSpaceSet, basic)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstColorSpaceSetRcPtr css1;
    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 0);

    // No category.

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OCIO_CHECK_ASSERT(!cs1->hasCategory("linear"));
    OCIO_CHECK_ASSERT(!cs1->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!cs1->hasCategory("log"));

    // Having categories to filter with.

    OCIO::ColorSpaceRcPtr cs2 = OCIO::ColorSpace::Create();
    cs2->setName("cs2");
    cs2->addCategory("linear");
    cs2->addCategory("rendering");
    OCIO_CHECK_ASSERT(cs2->hasCategory("linear"));
    OCIO_CHECK_ASSERT(cs2->hasCategory("rendering"));
    OCIO_CHECK_ASSERT(!cs2->hasCategory("log"));

    OCIO_CHECK_NO_THROW(cs2->addCategory("log"));
    OCIO_CHECK_ASSERT(cs2->hasCategory("log"));
    OCIO_CHECK_NO_THROW(cs2->removeCategory("log"));
    OCIO_CHECK_ASSERT(!cs2->hasCategory("log"));

    // Update config.

    OCIO_CHECK_NO_THROW(config->addColorSpace(cs1));
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs2));

    // Search some color spaces based on criteria.

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces(""));
    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 2);

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("log"));
    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 0);

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs2"));

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("LinEar"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs2"));

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces(" LinEar "));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs2"));
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceByIndex(0)->getName()), std::string("cs2"));

    // Test some faulty requests.

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("lin ear"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("[linear]"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear log"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linearlog"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    // Empty the config.

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);

    OCIO_CHECK_NO_THROW(config->clearColorSpaces());
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 0);
    // But existing sets are preserved.
    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 1);

    OCIO::ConstColorSpaceSetRcPtr css2;
    OCIO_CHECK_NO_THROW(css2 = config->getColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(css2->getNumColorSpaces(), 0);
}

OCIO_ADD_TEST(ColorSpaceSet, decoupled_sets)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OCIO_CHECK_NO_THROW(cs1->addCategory("linear"));
    OCIO_CHECK_ASSERT(cs1->hasCategory("linear"));
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs1));

    OCIO::ConstColorSpaceSetRcPtr css1;
    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OCIO::ConstColorSpaceSetRcPtr css2;
    OCIO_CHECK_NO_THROW(css2 = config->getColorSpaces("linear"));
    OCIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs1"));

    // Change the original color space.

    cs1->setName("new_cs1");

    // Check that color spaces in existing sets are not changed.
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OCIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs1"));

    // Change the color space from the config instance.

    OCIO_CHECK_ASSERT(!cs1->isData());
    config->clearColorSpaces();
    config->addColorSpace(cs1);
    cs1->setIsData(true);

    OCIO_CHECK_EQUAL(std::string(cs1->getName()), std::string("new_cs1"));
    OCIO_CHECK_ASSERT(cs1->isData());
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("new_cs1"));
    // NB: ColorSpace would need to be re-added to the config to reflect the change to isData.
    OCIO_CHECK_ASSERT(!config->getColorSpace("new_cs1")->isData());

    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OCIO_CHECK_ASSERT(!css1->getColorSpace("cs1")->isData());

    OCIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OCIO_CHECK_ASSERT(!css2->getColorSpace("cs1")->isData());
}

OCIO_ADD_TEST(ColorSpaceSet, order_validation)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstColorSpaceSetRcPtr css1;
    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 0);

    // Create some color spaces.

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    cs1->addCategory("linear");
    cs1->addCategory("rendering");

    OCIO::ColorSpaceRcPtr cs2 = OCIO::ColorSpace::Create();
    cs2->setName("cs2");
    cs2->addCategory("rendering");
    cs2->addCategory("linear");

    OCIO::ColorSpaceRcPtr cs3 = OCIO::ColorSpace::Create();
    cs3->setName("cs3");
    cs3->addCategory("rendering");

    // Add the color spaces.

    OCIO_CHECK_NO_THROW(config->addColorSpace(cs1));
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs2));
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs3));

    // Check the color space order for the category "linear".

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 2);

    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(1)), std::string("cs2"));

    // Check the color space order for the category "rendering".

    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces("rendering"));
    OCIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 3);

    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(1)), std::string("cs2"));
    OCIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(2)), std::string("cs3"));
}

OCIO_ADD_TEST(ColorSpaceSet, operations_on_set)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // No category.

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs1));

    // Having categories to filter with.

    OCIO::ColorSpaceRcPtr cs2 = OCIO::ColorSpace::Create();
    cs2->setName("cs2");
    cs2->addCategory("linear");
    cs2->addCategory("rendering");
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs2));

    OCIO::ColorSpaceRcPtr cs3 = OCIO::ColorSpace::Create();
    cs3->setName("cs3");
    cs3->addCategory("log");
    cs3->addCategory("rendering");
    OCIO_CHECK_NO_THROW(config->addColorSpace(cs3));


    // Recap. of the existing color spaces:
    // cs1  -> name="cs1" i.e. no category
    // cs2  -> name="cs2", categories=[rendering, linear]
    // cs3  -> name="cs3", categories=[rendering, log]


    OCIO::ConstColorSpaceSetRcPtr css1;
    OCIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(css1->getNumColorSpaces(), 3);

    OCIO::ConstColorSpaceSetRcPtr css2;
    OCIO_CHECK_NO_THROW(css2 = config->getColorSpaces("linear"));
    OCIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs2"));

    OCIO::ConstColorSpaceSetRcPtr css3;
    OCIO_CHECK_NO_THROW(css3 = config->getColorSpaces("log"));
    OCIO_CHECK_EQUAL(css3->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css3->getColorSpaceNameByIndex(0)), std::string("cs3"));


    // Recap. of the existing color space sets:
    // ccs1 -> {cs1, cs2, cs3}
    // ccs2 -> {cs2}
    // css3 -> {cs3}


    // Test the union.

    OCIO::ConstColorSpaceSetRcPtr css4 = css2 || css3;
    OCIO_CHECK_EQUAL(css4->getNumColorSpaces(), 2); // {cs2, cs3}

    css4 = css1 || css2;
    OCIO_CHECK_EQUAL(css4->getNumColorSpaces(), 3); // no duplication i.e. all color spaces

    // Test the intersection.

    css4 = css2 && css3;
    OCIO_CHECK_EQUAL(css4->getNumColorSpaces(), 0);

    css4 = css2 && css1;
    OCIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 1); // {cs2}
    OCIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs2"));
    OCIO_CHECK_EQUAL(std::string(css4->getColorSpaceByIndex(0)->getName()), std::string("cs2"));

    // Test the difference.

    css4 = css1 - css3;
    OCIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 2); // {cs1, cs2}
    OCIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OCIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(1)), std::string("cs2"));

    css4 = css1 - css2;
    OCIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 2); // {cs1, cs3}
    OCIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OCIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(1)), std::string("cs3"));

    // Test with several embedded operations.

    css4 = css1 - (css2 || css3);
    OCIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 1); // {cs1}
    OCIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OCIO::ColorSpaceSetRcPtr css5;
    OCIO_CHECK_NO_THROW(css5 = config->getColorSpaces("rendering"));
    OCIO_CHECK_EQUAL(css5->getNumColorSpaces(), 2); // {cs2, cs3}
    // Manipulate the result with few tests.
    OCIO_CHECK_NO_THROW(css5->addColorSpace(cs1));
    OCIO_CHECK_EQUAL(css5->getNumColorSpaces(), 3); // {cs1, cs2, cs3}
    OCIO_CHECK_NO_THROW(css5->removeColorSpace("cs2"));
    OCIO_CHECK_NO_THROW(css5->removeColorSpace("cs1"));
    OCIO_CHECK_EQUAL(css5->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(css5->getColorSpaceNameByIndex(0)), std::string("cs3"));
    OCIO_CHECK_NO_THROW(css5->clearColorSpaces());
    OCIO_CHECK_EQUAL(css5->getNumColorSpaces(), 0);

    OCIO_CHECK_NO_THROW(css5 = config->getColorSpaces("rendering"));
    OCIO_REQUIRE_EQUAL(css5->getNumColorSpaces(), 2); // {cs2, cs3}
    OCIO_CHECK_EQUAL(std::string(css5->getColorSpaceNameByIndex(0)), std::string("cs2"));
    OCIO_CHECK_EQUAL(std::string(css5->getColorSpaceNameByIndex(1)), std::string("cs3"));

    css4 = (css1  - css5)  // ( {cs1, cs2, cs3} - {cs2, cs3} ) --> {cs1}
           && 
           (css2 || css3); // ( {cs2} || {cs3} )               --> {cs2, cs3}

    OCIO_CHECK_EQUAL(css4->getNumColorSpaces(), 0);
}
