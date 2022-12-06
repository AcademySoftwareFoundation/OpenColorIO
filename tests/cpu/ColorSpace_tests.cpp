// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include "ColorSpace.cpp"

#include <pystring/pystring.h>
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

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
    OCIO_CHECK_EQUAL(0, cs->getNumAliases());
    OCIO_CHECK_EQUAL(std::string(""), cs->getAlias(0));
    OCIO_CHECK_EQUAL(std::string(""), cs->getFamily());
    OCIO_CHECK_EQUAL(std::string(""), cs->getDescription());
    OCIO_CHECK_EQUAL(std::string(""), cs->getEqualityGroup());
    OCIO_CHECK_EQUAL(std::string(""), cs->getEncoding());
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
    cs->setEncoding("encoding");
    OCIO_CHECK_EQUAL(std::string("encoding"), cs->getEncoding());
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
    OCIO_CHECK_EQUAL(oss.str().size(), 193);
}

OCIO_ADD_TEST(ColorSpace, alias)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
    constexpr char AliasA[]{ "aliasA" };
    constexpr char AliasAAlt[]{ "aLiaSa" };
    constexpr char AliasB[]{ "aliasB" };
    cs->addAlias(AliasA);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
    cs->addAlias(AliasB);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasA);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(1)), AliasB);

    // Alias with same name (different case) already exists, do nothing.

    cs->addAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasA);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(1)), AliasB);

    // Remove alias.

    cs->removeAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);

    // Add with new case.

    cs->addAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(1)), AliasAAlt);

    // Setting the name of the color space to one of its aliases removes the alias.

    cs->setName(AliasA);
    OCIO_CHECK_EQUAL(std::string(cs->getName()), AliasA);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);

    // Alias is not added if it is already the color space name.

    cs->addAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(std::string(cs->getName()), AliasA);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);

    // Remove all aliases.

    cs->addAlias("other");
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    cs->clearAliases();
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
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

OCIO_ADD_TEST(Config, color_space_serialize)
{
    constexpr char Start[]{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

)" };

    // The raw config.
    {
        constexpr char End[]{ R"(colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform
)" };
        std::string cfgString{ Start };
        cfgString += End;

        // Load config.

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_REQUIRE_ASSERT(config);
        OCIO_CHECK_NO_THROW(config->validate());

        // Check colorspace.

        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 1);
        OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(config->getColorSpaceNameByIndex(0));
        OCIO_REQUIRE_ASSERT(cs);
        OCIO_CHECK_EQUAL(cs->getAllocation(), OCIO::ALLOCATION_UNIFORM);
        OCIO_CHECK_EQUAL(cs->getAllocationNumVars(), 0);
        OCIO_CHECK_EQUAL(cs->getBitDepth(), OCIO::BIT_DEPTH_F32);
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()),
                         "A raw color space. Conversions to and from this space are no-ops.");
        OCIO_CHECK_EQUAL(std::string(cs->getEncoding()), "");
        OCIO_CHECK_EQUAL(std::string(cs->getEqualityGroup()), "");
        OCIO_CHECK_EQUAL(std::string(cs->getFamily()), "raw");
        OCIO_CHECK_EQUAL(std::string(cs->getName()), "raw");
        OCIO_CHECK_EQUAL(cs->getNumCategories(), 0);
        OCIO_CHECK_EQUAL(cs->getReferenceSpaceType(), OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
        OCIO_CHECK_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        OCIO_CHECK_ASSERT(cs->isData());

        // Save and compare output with input.

        std::ostringstream os;
        os << *config;

        OCIO_CHECK_EQUAL(cfgString, os.str());
    }

    // Adding a color space that uses all parameters.
    {
        constexpr char End[]{ R"(colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: colorspace
    aliases: [alias1, alias2]
    family: family
    equalitygroup: group
    bitdepth: 16f
    description: |
      A raw color space.
      Second line.
    isdata: false
    categories: [one, two]
    encoding: scene-linear
    allocation: lg2
    allocationvars: [0.1, 0.9, 0.15]
    to_scene_reference: !<LogTransform> {}
    from_scene_reference: !<LogTransform> {}
)" };
        std::string cfgString{ Start };
        cfgString += End;

        // Load config.

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_REQUIRE_ASSERT(config);
        OCIO_CHECK_NO_THROW(config->validate());

        // Check colorspace.

        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
        OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(config->getColorSpaceNameByIndex(1));
        OCIO_REQUIRE_ASSERT(cs);
        OCIO_CHECK_EQUAL(cs->getAllocation(), OCIO::ALLOCATION_LG2);
        OCIO_REQUIRE_EQUAL(cs->getAllocationNumVars(), 3);
        float vars[3]{ 0.f };
        cs->getAllocationVars(vars);
        OCIO_CHECK_EQUAL(vars[0], 0.1f);
        OCIO_CHECK_EQUAL(vars[1], 0.9f);
        OCIO_CHECK_EQUAL(vars[2], 0.15f);
        OCIO_CHECK_EQUAL(cs->getBitDepth(), OCIO::BIT_DEPTH_F16);
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()),
                         "A raw color space.\nSecond line.");
        OCIO_CHECK_EQUAL(std::string(cs->getEncoding()), "scene-linear");
        OCIO_CHECK_EQUAL(std::string(cs->getEqualityGroup()), "group");
        OCIO_CHECK_EQUAL(std::string(cs->getFamily()), "family");
        OCIO_CHECK_EQUAL(std::string(cs->getName()), "colorspace");
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), "alias1");
        OCIO_CHECK_EQUAL(std::string(cs->getAlias(1)), "alias2");
        OCIO_REQUIRE_EQUAL(cs->getNumCategories(), 2);
        OCIO_CHECK_EQUAL(std::string(cs->getCategory(0)), "one");
        OCIO_CHECK_EQUAL(std::string(cs->getCategory(1)), "two");
        OCIO_CHECK_EQUAL(cs->getReferenceSpaceType(), OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_ASSERT(cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
        OCIO_CHECK_ASSERT(cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        OCIO_CHECK_ASSERT(!cs->isData());

        // Save and compare output with input.

        std::ostringstream os;
        os << *config;

        OCIO_CHECK_EQUAL(cfgString, os.str());
    }

    // Description trailing newlines are removed.
    {
        constexpr char End[]{ R"(colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: Some text.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: raw2
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: |
      One line.

      Other line.
    isdata: true
    allocation: uniform
)" };
        std::string cfgString{ Start };
        cfgString += End;

        // Load config.

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_REQUIRE_ASSERT(config);
        OCIO_CHECK_NO_THROW(config->validate());

        // Check colorspace.

        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
        OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(config->getColorSpaceNameByIndex(0));
        OCIO_REQUIRE_ASSERT(cs);
        // Description has no trailing \n.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "Some text.");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(1));
        OCIO_REQUIRE_ASSERT(cs);
        // Description has no trailing \n.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "One line.\n\nOther line.");

        // Save and compare output with input.

        std::ostringstream os;
        os << *config;

        OCIO_CHECK_EQUAL(cfgString, os.str());

        // Even if some line feeds are added to the end of description they won't be saved.
        auto csEdit = cs->createEditableCopy();

        csEdit->setDescription("One line.\n\nOther line.\n");

        auto configEdit = config->createEditableCopy();
        configEdit->addColorSpace(csEdit);

        os.clear();
        os.str("");
        os << *configEdit;

        OCIO_CHECK_EQUAL(cfgString, os.str());

        // Even if several line feeds are added.

        csEdit->setDescription("One line.\n\nOther line.\n\n\n\n");
        configEdit->addColorSpace(csEdit);

        os.clear();
        os.str("");
        os << *configEdit;

        OCIO_CHECK_EQUAL(cfgString, os.str());

        // Single line descriptions are saved on one line and trailing \n are ignored.

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(0));
        OCIO_REQUIRE_ASSERT(cs);
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "Some text.");

        csEdit = cs->createEditableCopy();
        csEdit->setDescription("Some text.\n\n\n");
        configEdit->addColorSpace(csEdit);

        os.clear();
        os.str("");
        os << *configEdit;

        OCIO_CHECK_EQUAL(cfgString, os.str());
    }

    // Test different way of writing description, some are not written as they would be saved.
    {
        constexpr char End[]{ R"(colorspaces:
  - !<ColorSpace>
    name: raw
    description: |
      "Some text."

  - !<ColorSpace>
    name: raw2
    description: "Multiple lines\n\nOther line.\n\n\n"

  - !<ColorSpace>
    name: raw3
    description: |
      Test \n backslash+n.

  - !<ColorSpace>
    name: raw4
    description: "One"

  - !<ColorSpace>
    name: raw5
    description: More "than" one

  - !<ColorSpace>
    name: raw6
    description: Other \n test.

  - !<ColorSpace>
    name: raw7
    description: Double backslash+n \\n test.

  - !<ColorSpace>
    name: raw8
    description: "Double backslash+n \\n in quotes."
)" };
        std::string cfgString{ Start };
        cfgString += End;

        // Load config.

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_REQUIRE_ASSERT(config);
        OCIO_CHECK_NO_THROW(config->validate());

        // Check colorspace descriptions.

        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 8);
        OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(config->getColorSpaceNameByIndex(0));
        OCIO_REQUIRE_ASSERT(cs);
        // description: |
        //   "Some text."
        // A single line comment can be written using the multi-line syntax. Note that surounding
        // quotes are preserved when multi-line syntax is used.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "\"Some text.\"");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(1));
        OCIO_REQUIRE_ASSERT(cs);
        // description: "Multiple lines\n\nOther line.\n\n\n"
        // Multi-lines comment can be written using the single line syntax when "" are used.
        // Note that trailing newlines are removed.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "Multiple lines\n\nOther line.");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(2));
        OCIO_REQUIRE_ASSERT(cs);
        // description: |
        //     Test \n backslash+n.
        // Without "" \n is just a backslash '\' on a 'n'. Would be written using single line
        // syntax.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "Test \\n backslash+n.");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(3));
        OCIO_REQUIRE_ASSERT(cs);
        // description: "One"
        // Surrounding "" for single line comment are removed.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "One");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(4));
        OCIO_REQUIRE_ASSERT(cs);
        // description: More "than" one
        // In between "" are preserved.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "More \"than\" one");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(5));
        OCIO_REQUIRE_ASSERT(cs);
        // description: Other \n test.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "Other \\n test.");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(6));
        OCIO_REQUIRE_ASSERT(cs);
        // description: Double backslash+n \\n test.
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "Double backslash+n \\\\n test.");

        cs = config->getColorSpace(config->getColorSpaceNameByIndex(7));
        OCIO_REQUIRE_ASSERT(cs);
        // description: "Double backslash+n \\n in quotes."
        OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "Double backslash+n \\n in quotes.");

        std::ostringstream os;
        os << *config;

        constexpr char EndRes[]{ R"(colorspaces:
  - !<ColorSpace>
    name: raw
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: "\"Some text.\""
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: raw2
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: |
      Multiple lines

      Other line.
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: raw3
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: Test \n backslash+n.
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: raw4
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: One
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: raw5
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: More "than" one
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: raw6
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: Other \n test.
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: raw7
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: Double backslash+n \\n test.
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: raw8
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: Double backslash+n \n in quotes.
    isdata: false
    allocation: uniform
)" };
        std::string cfgRes{ Start };
        cfgRes += EndRes;

        OCIO_CHECK_EQUAL(cfgRes, os.str());
    }
}

OCIO_ADD_TEST(Config, use_alias)
{
    std::istringstream is{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  testAlias: aces
  default: raw

file_rules:
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: aces}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    aliases: [ colorspaceAlias ]
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: colorspace
    aliases: [ aces, aces2065-1, ACES - ACES2065-1, "ACES AP0, scene-linear" ]
    family: family
    equalitygroup: group
    bitdepth: 16f
    description: |
      A raw color space.
      Second line.
    isdata: false
    categories: [one, two]
    encoding: scene-linear
    allocation: lg2
    allocationvars: [0.1, 0.9, 0.15]
    to_reference: !<LogTransform> {}
    from_reference: !<LogTransform> {}
)" };

    // Load config.

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_ASSERT(config);
    OCIO_CHECK_NO_THROW(config->validate());

    // Get a color space from alias.

    auto cs = config->getColorSpace("aces2065-1");
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "colorspace");

    cs = config->getColorSpace("ACES - ACES2065-1");
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "colorspace");

    cs = config->getColorSpace("alias no valid");
    OCIO_REQUIRE_ASSERT(!cs);

    // Get the canonical name.

    OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("aces")), "colorspace");
    OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("ACES AP0, scene-linear")), "colorspace");
    OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("colorspace")), "colorspace");
    OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("default")), "raw");
    OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("DEFault")), "raw");
    OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("not an alias")), "");
    OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("")), "");

    // Get the index.

    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("AceS"), 1); // Case insensitve
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("aces2065-1"), 1);
    OCIO_CHECK_EQUAL(config->getIndexForColorSpace("not an alias"), -1);

    // Get color space referenced by alias in role.

    cs = config->getColorSpace("testAlias");
    OCIO_REQUIRE_ASSERT(cs);
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "colorspace");

    // Color space from string.

    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("test_aces_test")),
                     "colorspace");
    // "colorspace" is present but "ColorspaceAlias" is longer (and at the same position).
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("skdj_ColorspaceAlias_dfjdk")),
                     "raw");

    // With inactive color spaces.

    auto cfg = config->createEditableCopy();
    cfg->setInactiveColorSpaces("colorspace");

    OCIO_CHECK_EQUAL(std::string(cfg->getColorSpaceFromFilepath("test_aces_test")),
                     "colorspace");
}

OCIO_ADD_TEST(Config, is_colorspace_linear)
{

    constexpr const char * TEST_CONFIG { R"(ocio_profile_version: 2

description: Test config for the isColorSpaceLinear method.

environment:
  {}
search_path: "non_existing_path"
roles:
  aces_interchange: scene_linear-trans
  cie_xyz_d65_interchange: display_linear-enc
  color_timing: scene_linear-trans
  compositing_log: scene_log-enc
  default: display_data
  scene_linear: scene_linear-trans

displays:
  generic display:
    - !<View> {name: Raw, colorspace: scene_data}

# Make a few of the color spaces inactive, this should not affect the result.
inactive_colorspaces: [display_linear-trans, scene_linear-trans]

view_transforms:
  - !<ViewTransform>
    name: view_transform
    from_scene_reference: !<MatrixTransform> {}

# Display-referred color spaces.

display_colorspaces:
  - !<ColorSpace>
    name: display_data
    description: |
      Data space.
      Has a linear transform, which should never happen, but this will be ignored since 
      isdata is true.
    isdata: true
    encoding: data
    from_display_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: display_linear-enc
    description: |
      Encoding set to display-linear.
      Has a non-existent transform, but this should be ignored since the encoding takes precedence.
    isdata: false
    encoding: display-linear
    from_display_reference: !<FileTransform> {src: does-not-exist.lut}

  - !<ColorSpace>
    name: display_wrong-linear-enc
    description: |
      Encoding set to scene-linear.  This should never happen for a display space, but test it.
    isdata: false
    encoding: scene-linear

  - !<ColorSpace>
    name: display_video-enc
    description: |
      Encoding set to sdr-video.
      Has a linear transform, but this should be ignored since the encoding takes precedence.
    isdata: false
    encoding: sdr-video
    from_display_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: display_linear-trans
    description: |
      No encoding.  Transform is linear.
    isdata: false
    from_display_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}
        - !<CDLTransform> {slope: [0.1, 2, 3], style: noclamp}

  - !<ColorSpace>
    name: display_video-trans
    description: |
      No encoding.  Transform is non-linear.
    isdata: false
    from_display_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_sRGB}

# Scene-referred color spaces.

colorspaces:
  - !<ColorSpace>
    name: scene_data
    description: |
      Data space.
      Has a linear transform, which should never happen, but this will be ignored 
      since isdata is true.
    isdata: true
    encoding: data
    from_scene_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene_linear-enc
    description: |
      Encoding set to scene-linear.
      Has a non-linear transform, but this will be ignored since the encoding takes precedence.
    isdata: false
    encoding: scene-linear
    from_scene_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_sRGB}

  - !<ColorSpace>
    name: scene_wrong-linear-enc
    description: |
      Encoding set to display-linear.  This should never happen for a scene space, but test it.
    isdata: false
    encoding: display-linear

  - !<ColorSpace>
    name: scene_log-enc
    description: |
      Encoding set to log.
      Has a linear transform, but this will be ignored since the encoding takes precedence.
    isdata: false
    encoding: log
    from_scene_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene_linear-trans
    aliases: [scene_linear-trans-alias]
    description: |
      No encoding.  Transform is linear.
    isdata: false
    to_scene_reference: !<GroupTransform>
      children:
        - !<BuiltinTransform> {style: UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD}
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene_nonlin-trans
    description: |
      No encoding.  Transform is non-linear because it clamps values outside [0,1].
    isdata: false
    to_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}
        - !<RangeTransform> {min_in_value: 0., min_out_value: 0., max_in_value: 1., max_out_value: 1.}

  - !<ColorSpace>
    name: scene_ref
    description: |
      No encoding.  Considered linear since it is equivalent to the reference space.
    isdata: false
)" };

    // Load config.
    std::istringstream is;
    is.str(TEST_CONFIG);
    OCIO::ConstConfigRcPtr config;
    
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_ASSERT(config);
    OCIO_CHECK_NO_THROW(config->validate());

    auto testSceneReferred = [&config](const char * csName, bool bSceneExpected, int line)
    {
        auto cs = config->getColorSpace(csName);
        OCIO_REQUIRE_ASSERT(cs);

        bool isLinearToSceneReference = config->isColorSpaceLinear(csName, OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL_FROM(isLinearToSceneReference, bSceneExpected, line);
    };

    auto testDisplayReferred = [&config](const char * csName, bool bDisplayExpected, int line)
    {
        auto cs = config->getColorSpace(csName);
        OCIO_REQUIRE_ASSERT(cs);

        bool isLinearToDisplayReference = config->isColorSpaceLinear(csName, OCIO::REFERENCE_SPACE_DISPLAY);
        OCIO_CHECK_EQUAL_FROM(isLinearToDisplayReference, bDisplayExpected, line);
    };

    {
        testSceneReferred("display_data", false, __LINE__);
        testSceneReferred("display_linear-enc", false, __LINE__);
        testSceneReferred("display_wrong-linear-enc", false, __LINE__);
        testSceneReferred("display_video-enc", false, __LINE__);
        testSceneReferred("display_linear-trans", false, __LINE__);
        testSceneReferred("display_video-trans", false, __LINE__);

        testSceneReferred("scene_data", false, __LINE__);
        testSceneReferred("scene_linear-enc", true, __LINE__);
        testSceneReferred("scene_wrong-linear-enc", false, __LINE__);
        testSceneReferred("scene_log-enc", false, __LINE__);
        testSceneReferred("scene_linear-trans", true, __LINE__);
        testSceneReferred("scene_nonlin-trans", false, __LINE__);
        testSceneReferred("scene_linear-trans-alias", true, __LINE__);
        testSceneReferred("scene_ref", true, __LINE__);
    }
    
    {
        testDisplayReferred("display_data", false, __LINE__);
        testDisplayReferred("display_linear-enc", true, __LINE__);
        testDisplayReferred("display_wrong-linear-enc", false, __LINE__);
        testDisplayReferred("display_video-enc", false, __LINE__);
        testDisplayReferred("display_linear-trans", true, __LINE__);
        testDisplayReferred("display_video-trans", false, __LINE__);

        testDisplayReferred("scene_data", false, __LINE__);
        testDisplayReferred("scene_linear-enc", false, __LINE__);
        testDisplayReferred("scene_wrong-linear-enc", false, __LINE__);
        testDisplayReferred("scene_log-enc", false, __LINE__);
        testDisplayReferred("scene_linear-trans", false, __LINE__);
        testDisplayReferred("scene_nonlin-trans", false, __LINE__);
        testDisplayReferred("scene_linear-trans-alias", false, __LINE__);
        testDisplayReferred("scene_ref", false, __LINE__);
    }
}

OCIO_ADD_TEST(Processor, processor_to_known_colorspace)
{
    constexpr const char * CONFIG { R"(
ocio_profile_version: 2

roles:
  default: raw
  scene_linear: ref_cs

colorspaces:
  - !<ColorSpace>
    name: raw
    description: A data colorspace (should not be used).
    isdata: true

  - !<ColorSpace>
    name: ref_cs
    description: The reference colorspace.
    isdata: false

  - !<ColorSpace>
    name: not sRGB
    description: A color space that misleadingly has sRGB in the name, even though it's not.
    isdata: false
    to_scene_reference: !<BuiltinTransform> {style: ACEScct_to_ACES2065-1}

  - !<ColorSpace>
    name: ACES cg
    description: An ACEScg space with an unusual spelling.
    isdata: false
    to_scene_reference: !<BuiltinTransform> {style: ACEScg_to_ACES2065-1}

  - !<ColorSpace>
    name: Linear ITU-R BT.709
    description: A linear Rec.709 space with an unusual spelling.
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to Linear Rec.709 (sRGB)
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}

  - !<ColorSpace>
    name: Texture -- sRGB
    description: An sRGB Texture space, spelled differently than in the built-in config.
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Rec.709
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: sRGB Encoded AP1 - Texture
    description: Another space with "sRGB" in the name that is not actually an sRGB texture space.
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Encoded AP1 - Texture
      children:
        - !<MatrixTransform> {matrix: [1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

)" };

    auto checkProcessor = [](OCIO::ConstProcessorRcPtr & proc)
    {
        OCIO::GroupTransformRcPtr gt = proc->createGroupTransform();
        OCIO_CHECK_EQUAL(gt->getNumTransforms(), 4);

        {
            OCIO::ConstLogCameraTransformRcPtr logAfTf0 = 
            OCIO::DynamicPtrCast<const OCIO::LogCameraTransform>(gt->getTransform(0));

            double values[3];
            logAfTf0->getLogSideSlopeValue(values);
            OCIO_CHECK_CLOSE(values[0], 0.0570776, 0.000001);
            OCIO_CHECK_EQUAL(logAfTf0->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        }

        {
            auto mt1 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gt->getTransform(1));
            double mat[16] = { 0.0, 0.0, 0.0, 0.0,
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0 };
            mt1->getMatrix(mat);
            OCIO_CHECK_CLOSE(mat[0], 0.6954522413574519, 0.000001);
            OCIO_CHECK_EQUAL(mt1->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        }

        {
            auto mt2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gt->getTransform(2));
            double mat[16] = { 0.0, 0.0, 0.0, 0.0,
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0 };
            mt2->getMatrix(mat);
            OCIO_CHECK_CLOSE(mat[0], 1.45143931607166, 0.000001);
            OCIO_CHECK_EQUAL(mt2->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        }

        {
            double vals[4];
            auto mt3 = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(gt->getTransform(3));
            mt3->getValue(vals);
            OCIO_CHECK_CLOSE(vals[0], 2.2, 0.000001);
            OCIO_CHECK_EQUAL(mt3->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        }
    };

    auto checkProcessorInverse = [](OCIO::ConstProcessorRcPtr & proc)
    {
        OCIO::GroupTransformRcPtr gt = proc->createGroupTransform();
        OCIO_CHECK_EQUAL(gt->getNumTransforms(), 4);

        {
            double vals[4];
            auto mt0 = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(gt->getTransform(0));
            mt0->getValue(vals);
            OCIO_CHECK_CLOSE(vals[0], 2.2, 0.000001);
            OCIO_CHECK_EQUAL(mt0->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        }

        {
            auto mt1 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gt->getTransform(1));
            double mat[16] = { 0.0, 0.0, 0.0, 0.0,
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0 };
            mt1->getMatrix(mat);
            OCIO_CHECK_CLOSE(mat[0], 0.6954522413574519, 0.000001);
            OCIO_CHECK_EQUAL(mt1->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        }

        {
            auto mt2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gt->getTransform(2));
            double mat[16] = { 0.0, 0.0, 0.0, 0.0,
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 0.0 };
            mt2->getMatrix(mat);
            OCIO_CHECK_CLOSE(mat[0], 1.45143931607166, 0.000001);
            OCIO_CHECK_EQUAL(mt2->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        }

        {
            OCIO::ConstLogCameraTransformRcPtr logAfTf0 = 
            OCIO::DynamicPtrCast<const OCIO::LogCameraTransform>(gt->getTransform(3));

            double values[3];
            logAfTf0->getLogSideSlopeValue(values);
            OCIO_CHECK_CLOSE(values[0], 0.0570776, 0.000001);
            OCIO_CHECK_EQUAL(logAfTf0->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        }
    };

    std::istringstream is;
    is.str(CONFIG);
    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));

    OCIO::ConfigRcPtr editableCfg = cfg->createEditableCopy();

    // Make all color spaces suitable for the heuristics inactive.
    // The heuristics don't look at inactive color spaces.
    editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, Texture -- sRGB");

    std::string srcColorSpaceName = "not sRGB";
    std::string builtinColorSpaceName  = "Gamma 2.2 AP1 - Texture";

    {
        // Test throw if no suitable spaces are present.

        OCIO_CHECK_THROW(auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(
            editableCfg,
            srcColorSpaceName.c_str(),
            builtinColorSpaceName.c_str()),

            OCIO::Exception
        );
    }

    {
        // Test sRGB Texture space.

        editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709");
        auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  builtinColorSpaceName.c_str());
        checkProcessor(proc);
    }

    {
        // Test linear color space from_ref direction.

        editableCfg->setInactiveColorSpaces("ACES cg, Texture -- sRGB");
        auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  builtinColorSpaceName.c_str());
        checkProcessor(proc);
    }

    {
        // Test linear color space to_ref direction.

        editableCfg->setInactiveColorSpaces("Linear ITU-R BT.709, Texture -- sRGB");
        auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  builtinColorSpaceName.c_str());
        checkProcessor(proc);
    }

    {
        // Test linear color space to_ref direction.

        editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709");
        auto proc = OCIO::Config::GetProcessorFromBuiltinColorSpace(builtinColorSpaceName.c_str(),
                                                                    editableCfg,
                                                                    srcColorSpaceName.c_str());
        checkProcessorInverse(proc);
    }

    {
        // Test linear color space from_ref direction.

        editableCfg->setInactiveColorSpaces("ACES cg, Texture -- sRGB");
        auto proc = OCIO::Config::GetProcessorFromBuiltinColorSpace(builtinColorSpaceName.c_str(),
                                                                    editableCfg,
                                                                    srcColorSpaceName.c_str());
        checkProcessorInverse(proc);        
    }

    {
        // Test linear color space to_ref direction.

        editableCfg->setInactiveColorSpaces("Linear ITU-R BT.709, Texture -- sRGB");
        auto proc = OCIO::Config::GetProcessorFromBuiltinColorSpace(builtinColorSpaceName.c_str(),
                                                                    editableCfg,
                                                                    srcColorSpaceName.c_str());
        checkProcessorInverse(proc);
    }
}