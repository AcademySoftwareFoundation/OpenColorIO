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

        OCIO::Categories cats = OCIO::ExtractCategories(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 1);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
    }

    {
        constexpr const char * categories = "    iNpuT     ";

        OCIO::Categories cats = OCIO::ExtractCategories(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 1);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
    }

    {
        constexpr const char * categories = ",,iNpuT,    ,,";

        OCIO::Categories cats = OCIO::ExtractCategories(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 1);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
    }

    {
        constexpr const char * categories = ",,iNpuT,    ,,lut_input_SPACE";

        OCIO::Categories cats = OCIO::ExtractCategories(categories);
        OCIO_REQUIRE_EQUAL(cats.size(), 2);
        OCIO_CHECK_EQUAL(cats[0], std::string("input"));
        OCIO_CHECK_EQUAL(cats[1], std::string("lut_input_space"));
    }
}

OCIO_ADD_TEST(CategoryHelpers, basic)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceNames names;

    {
        OCIO::Categories categories { OCIO::Category::INPUT };

        OCIO_CHECK_NO_THROW(names = OCIO::FindColorSpaceNames(config, categories));

        OCIO_CHECK_EQUAL(names.size(), 4);
        OCIO_CHECK_EQUAL(names[0], std::string("in_1"));
        OCIO_CHECK_EQUAL(names[1], std::string("in_2"));
        OCIO_CHECK_EQUAL(names[2], std::string("in_3"));
        OCIO_CHECK_EQUAL(names[3], std::string("lut_input_3"));
    }

    {
        OCIO::Categories categories {};

        OCIO_CHECK_NO_THROW(names = OCIO::FindColorSpaceNames(config, categories));
        OCIO_CHECK_EQUAL(names.size(), 0);

        OCIO::Infos infos;
        OCIO_CHECK_NO_THROW(infos = OCIO::FindColorSpaceInfos(config, categories));
        OCIO_REQUIRE_EQUAL(infos.size(), 0);
    }

    {
        OCIO::Categories categories { OCIO::Category::SCENE_LINEAR_WORKING_SPACE,
                                      OCIO::Category::LOG_WORKING_SPACE };

        OCIO_CHECK_NO_THROW(names = OCIO::FindColorSpaceNames(config, categories));

        OCIO_CHECK_EQUAL(names.size(), 4);
        OCIO_CHECK_EQUAL(names[0], std::string("lin_1"));
        OCIO_CHECK_EQUAL(names[1], std::string("lin_2"));
        OCIO_CHECK_EQUAL(names[2], std::string("log_1"));
        OCIO_CHECK_EQUAL(names[3], std::string("in_3"));

        categories.push_back(OCIO::Category::LUT_INPUT_SPACE);

        OCIO_CHECK_NO_THROW(names = OCIO::FindColorSpaceNames(config, categories));

        OCIO_CHECK_EQUAL(names.size(), 7);
        OCIO_CHECK_EQUAL(names[0], std::string("lin_1"));
        OCIO_CHECK_EQUAL(names[1], std::string("lin_2"));
        OCIO_CHECK_EQUAL(names[2], std::string("log_1"));
        OCIO_CHECK_EQUAL(names[3], std::string("in_3"));
        OCIO_CHECK_EQUAL(names[4], std::string("lut_input_1"));
        OCIO_CHECK_EQUAL(names[5], std::string("lut_input_2"));
        OCIO_CHECK_EQUAL(names[6], std::string("lut_input_3"));
    }

    {
        OCIO::Categories categories { OCIO::Category::SCENE_LINEAR_WORKING_SPACE,
                                      OCIO::Category::LOG_WORKING_SPACE };

        OCIO_CHECK_NO_THROW(names = OCIO::FindColorSpaceNames(config, categories));
        OCIO_REQUIRE_EQUAL(names.size(), 4);

        OCIO::Infos infos;
        OCIO_CHECK_NO_THROW(infos = OCIO::FindColorSpaceInfos(config, categories));
        OCIO_REQUIRE_EQUAL(infos.size(), 4);

        for (size_t idx = 0; idx < names.size(); ++idx)
        {
            OCIO_CHECK_EQUAL(std::string(infos[idx]->getName()), names[idx]);
        }
    }

    {
        OCIO::Infos infos;
        OCIO_CHECK_NO_THROW(infos = OCIO::FindAllColorSpaceInfos(config));
        OCIO_REQUIRE_EQUAL(infos.size(), 12);
    }

    {
        OCIO::ConstColorSpaceInfoRcPtr info;
        OCIO_CHECK_NO_THROW(info = OCIO::ColorSpaceInfo::CreateFromRole(config, "reference", nullptr));
        OCIO_CHECK_ASSERT(info);
        OCIO_CHECK_EQUAL(std::string(info->getName()), std::string("reference"));
        OCIO_CHECK_EQUAL(std::string(info->getUIName()), std::string("reference (lin_1)"));

        OCIO_CHECK_NO_THROW(info = OCIO::ColorSpaceInfo::CreateFromRole(config, "unknown_role", nullptr));
        OCIO_CHECK_ASSERT(!info);
    }
}
