// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "apphelpers/CategoryHelpers.cpp"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


// The configuration file used by the unit tests.
#include "configs.data"

OCIO_ADD_TEST(CategoryHelpers, categories)
{
    {
        constexpr const char * categories = "iNpuT";

        OCIO::Categories cats = OCIO::ExtractItems(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 1);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
    }

    {
        constexpr const char * categories = "    iNpuT     ";

        OCIO::Categories cats = OCIO::ExtractItems(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 1);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
    }

    {
        constexpr const char * categories = ",,iNpuT,    ,,";

        OCIO::Categories cats = OCIO::ExtractItems(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 1);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
    }

    {
        constexpr const char * categories = ",,iNpuT,    ,,lut_input_SPACE";

        OCIO::Categories cats = OCIO::ExtractItems(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 2);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
        OCIO_CHECK_EQUAL(cats[1], std::string("lut_input_space"));
    }
}

OCIO_ADD_TEST(CategoryHelpers, basic)
{
    // This is testing internals that are using vectors of pointers.

    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::ColorSpaceVec css = OCIO::GetColorSpaces(config, true,
                                                       OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                       categories, encodings);
        OCIO_REQUIRE_EQUAL(css.size(), 3);
        OCIO_CHECK_EQUAL(css[0]->getName(), std::string("log_1"));
        OCIO_CHECK_EQUAL(css[1]->getName(), std::string("in_1"));
        OCIO_CHECK_EQUAL(css[2]->getName(), std::string("in_2"));
    }
    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::ColorSpaceVec css = OCIO::GetColorSpaces(config, false,
                                                       OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                       categories, encodings);
        OCIO_REQUIRE_EQUAL(css.size(), 0);
    }
    {
        OCIO::Categories categories{};
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::ColorSpaceVec css = OCIO::GetColorSpaces(config, true,
                                                       OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                       categories, encodings);
        OCIO_CHECK_EQUAL(css.size(), 0);
        css = OCIO::GetColorSpacesFromEncodings(config, true,
                                                OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                encodings);
        OCIO_CHECK_EQUAL(css.size(), 3);
    }
    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{};
        OCIO::ColorSpaceVec css = OCIO::GetColorSpaces(config, true,
                                                       OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                       categories, encodings);
        OCIO_CHECK_EQUAL(css.size(), 0);
        css = OCIO::GetColorSpaces(config, true, OCIO::SEARCH_REFERENCE_SPACE_SCENE, categories);
        OCIO_CHECK_EQUAL(css.size(), 7);
    }
    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::ColorSpaceVec css = OCIO::GetColorSpaces(config, true,
                                                       OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                                       categories, encodings);
        OCIO_REQUIRE_EQUAL(css.size(), 2);
        OCIO_CHECK_EQUAL(css[0]->getName(), std::string("display_lin_2"));
        OCIO_CHECK_EQUAL(css[1]->getName(), std::string("display_log_1"));
    }
    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::ColorSpaceVec css = OCIO::GetColorSpaces(config, true,
                                                       OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                       categories, encodings);
        OCIO_REQUIRE_EQUAL(css.size(), 5);
        OCIO_CHECK_EQUAL(css[0]->getName(), std::string("log_1"));
        OCIO_CHECK_EQUAL(css[1]->getName(), std::string("in_1"));
        OCIO_CHECK_EQUAL(css[2]->getName(), std::string("in_2"));
        OCIO_CHECK_EQUAL(css[3]->getName(), std::string("display_lin_2"));
        OCIO_CHECK_EQUAL(css[4]->getName(), std::string("display_log_1"));
    }
    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::ColorSpaceVec css = OCIO::GetColorSpaces(config, true,
                                                       OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                       categories);
        OCIO_REQUIRE_EQUAL(css.size(), 10);
        OCIO_CHECK_EQUAL(css[0]->getName(), std::string("lin_1"));
        OCIO_CHECK_EQUAL(css[1]->getName(), std::string("lin_2"));
        OCIO_CHECK_EQUAL(css[2]->getName(), std::string("log_1"));
        OCIO_CHECK_EQUAL(css[3]->getName(), std::string("in_1"));
        OCIO_CHECK_EQUAL(css[4]->getName(), std::string("in_2"));
        OCIO_CHECK_EQUAL(css[5]->getName(), std::string("in_3"));
        OCIO_CHECK_EQUAL(css[6]->getName(), std::string("lut_input_3"));
        OCIO_CHECK_EQUAL(css[7]->getName(), std::string("display_lin_1"));
        OCIO_CHECK_EQUAL(css[8]->getName(), std::string("display_lin_2"));
        OCIO_CHECK_EQUAL(css[9]->getName(), std::string("display_log_1"));
    }
    {
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::ColorSpaceVec css = OCIO::GetColorSpacesFromEncodings(config, true,
                                                                    OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                                    encodings);
        OCIO_REQUIRE_EQUAL(css.size(), 5);
        OCIO_CHECK_EQUAL(css[0]->getName(), std::string("log_1"));
        OCIO_CHECK_EQUAL(css[1]->getName(), std::string("in_1"));
        OCIO_CHECK_EQUAL(css[2]->getName(), std::string("in_2"));
        OCIO_CHECK_EQUAL(css[3]->getName(), std::string("display_lin_2"));
        OCIO_CHECK_EQUAL(css[4]->getName(), std::string("display_log_1"));
    }

    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::NamedTransformVec nts = OCIO::GetNamedTransforms(config, true,
                                                               categories, encodings);
        OCIO_REQUIRE_EQUAL(nts.size(), 2);
        OCIO_CHECK_EQUAL(nts[0]->getName(), std::string("nt1"));
        OCIO_CHECK_EQUAL(nts[1]->getName(), std::string("nt3"));
    }
    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::NamedTransformVec nts = OCIO::GetNamedTransforms(config, false,
                                                               categories, encodings);
        OCIO_CHECK_EQUAL(nts.size(), 0);
    }
    {
        OCIO::Categories categories{};
        OCIO::Encodings encodings{ "sdr-video", "log" };
        OCIO::NamedTransformVec nts = OCIO::GetNamedTransforms(config, true,
                                                               categories, encodings);
        OCIO_CHECK_EQUAL(nts.size(), 0);
    }
    {
        OCIO::Categories categories{ "file-io", "working-space" };
        OCIO::Encodings encodings{};
        OCIO::NamedTransformVec nts = OCIO::GetNamedTransforms(config, true,
                                                               categories, encodings);
        OCIO_CHECK_EQUAL(nts.size(), 0);
    }
    {
        OCIO::Categories categories{ "file-io" };
        OCIO::NamedTransformVec nts = OCIO::GetNamedTransforms(config, true, categories);
        OCIO_REQUIRE_EQUAL(nts.size(), 1);
        OCIO_CHECK_EQUAL(nts[0]->getName(), std::string("nt3"));
    }
    {
        OCIO::Encodings encodings{ "log" };
        OCIO::NamedTransformVec nts = OCIO::GetNamedTransformsFromEncodings(config, true,
                                                                            encodings);
        OCIO_REQUIRE_EQUAL(nts.size(), 1);
        OCIO_CHECK_EQUAL(nts[0]->getName(), std::string("nt2"));
    }
}
