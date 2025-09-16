// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <pystring.h>

#include "ColorSpace.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"
#include "UnitTestLogUtils.h"

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
    
    // Check the nullptr assignment hardening.
    // First set the fields to non-empty values.
    cs->setName("NAME");
    cs->setDescription("DESC");
    cs->setFamily("FAMILY");
    cs->setEqualityGroup("EQGRP");
    cs->setEncoding("ENC");
    cs->setInteropID("interop");
    OCIO_CHECK_NO_THROW(cs->setInterchangeAttribute("amf_transform_ids", "AMF"));
    OCIO_CHECK_NO_THROW(cs->setInterchangeAttribute("icc_profile_name", "ICC"));

    // Set to nullptr, this should erase the old values.
    OCIO_CHECK_NO_THROW(cs->setName(nullptr));
    OCIO_CHECK_NO_THROW(cs->setDescription(nullptr));
    OCIO_CHECK_NO_THROW(cs->setFamily(nullptr));
    OCIO_CHECK_NO_THROW(cs->setEqualityGroup(nullptr));
    OCIO_CHECK_NO_THROW(cs->setEncoding(nullptr));
    OCIO_CHECK_NO_THROW(cs->setInteropID(nullptr));
    OCIO_CHECK_NO_THROW(cs->setInterchangeAttribute("amf_transform_ids", nullptr));
    OCIO_CHECK_NO_THROW(cs->setInterchangeAttribute("icc_profile_name", nullptr));

    // Check that the values are empty now.
    OCIO_CHECK_ASSERT(!*cs->getName());
    OCIO_CHECK_ASSERT(!*cs->getDescription());
    OCIO_CHECK_ASSERT(!*cs->getFamily());
    OCIO_CHECK_ASSERT(!*cs->getEqualityGroup());
    OCIO_CHECK_ASSERT(!*cs->getEncoding());
    OCIO_CHECK_ASSERT(!*cs->getInteropID());
    OCIO_CHECK_ASSERT(!*cs->getInterchangeAttribute("amf_transform_ids"));
    OCIO_CHECK_ASSERT(!*cs->getInterchangeAttribute("icc_profile_name"));

    // Test set/get roundtrip.
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
    cs->setInteropID("interop_id");
    OCIO_CHECK_EQUAL(std::string("interop_id"), cs->getInteropID());
    cs->setInterchangeAttribute("amf_transform_ids", "amf_transform_id1\namf_transform_id2");
    OCIO_CHECK_EQUAL(std::string("amf_transform_id1\namf_transform_id2"),
                     cs->getInterchangeAttribute("amf_transform_ids"));
    cs->setInterchangeAttribute("icc_profile_name","icc_profile_name");
    OCIO_CHECK_EQUAL(std::string("icc_profile_name"), cs->getInterchangeAttribute("icc_profile_name"));

    std::ostringstream oss;
    oss << *cs;
    OCIO_CHECK_EQUAL(oss.str().size(), 306);
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
    OCIO_CHECK_ASSERT(cs->hasAlias(AliasA));
    OCIO_CHECK_ASSERT(cs->hasAlias(AliasAAlt));
    OCIO_CHECK_ASSERT(!cs->hasAlias(AliasB));
    cs->addAlias(AliasB);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasA);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(1)), AliasB);
    OCIO_CHECK_ASSERT(cs->hasAlias(AliasB));

    // Alias with same name (different case) already exists, do nothing.

    cs->addAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasA);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(1)), AliasB);

    // Remove alias.

    cs->removeAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);
    OCIO_CHECK_ASSERT(!cs->hasAlias(AliasA));
    OCIO_CHECK_ASSERT(!cs->hasAlias(AliasAAlt));

    // Add with new case.

    cs->addAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(1)), AliasAAlt);
    OCIO_CHECK_ASSERT(cs->hasAlias(AliasA));
    OCIO_CHECK_ASSERT(cs->hasAlias(AliasAAlt));

    // Setting the name of the color space to one of its aliases removes the alias.

    cs->setName(AliasA);
    OCIO_CHECK_EQUAL(std::string(cs->getName()), AliasA);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);
    OCIO_CHECK_ASSERT(!cs->hasAlias(AliasA));
    OCIO_CHECK_ASSERT(!cs->hasAlias(AliasAAlt));

    // Alias is not added if it is already the color space name.

    cs->addAlias(AliasAAlt);
    OCIO_CHECK_EQUAL(std::string(cs->getName()), AliasA);
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
    OCIO_CHECK_EQUAL(std::string(cs->getAlias(0)), AliasB);
    OCIO_CHECK_ASSERT(!cs->hasAlias(AliasAAlt));

    // Remove all aliases.

    cs->addAlias("other");
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
    OCIO_CHECK_ASSERT(cs->hasAlias("other"));
    cs->clearAliases();
    OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
    OCIO_CHECK_ASSERT(!cs->hasAlias(AliasB));
    OCIO_CHECK_ASSERT(!cs->hasAlias("other"));
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

    // Adding a color space that uses all parameters (as of 2.0).
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

    // Test that the interop_id is valid in v2.0 config too.
    {
        constexpr char End[]{R"(colorspaces:
  - !<ColorSpace>
    name: raw
    aliases: [ data ]
    interop_id: data
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: Some text.
    isdata: true
    allocation: uniform
)"};
        std::string cfgString{Start};
        cfgString += End;

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        auto cs = config->getColorSpace("raw");
        OCIO_CHECK_ASSERT(cs);
        OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), "data");

        OCIO_CHECK_NO_THROW(config->validate());
    }

    // Test that the undefined interop_id does not pass validation
    {
        constexpr char End[]{R"(colorspaces:
  - !<ColorSpace>
    name: raw
    interop_id: data
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: Some text.
    isdata: true
    allocation: uniform
)"};
        std::string cfgString{Start};
        cfgString += End;

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        auto cs = config->getColorSpace("raw");
        OCIO_CHECK_ASSERT(cs);
        OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), "data");

        OCIO_CHECK_THROW_WHAT(config->validate(),
            OCIO::Exception,
            "Config failed color space validation. The color space 'raw' refers "
            "to an interop ID, 'data', which is not defined in this config.");
    }

    // Test that the interop id can be found in another color space.
    {
        constexpr char End[]{R"(colorspaces:
  - !<ColorSpace>
    name: raw
    interop_id: data
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: one data color space.
    isdata: true
    allocation: uniform
  - !<ColorSpace>
    name: data
    interop_id: data
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: another data color space.
    isdata: true
    allocation: uniform
)"};
        std::string cfgString{Start};
        cfgString += End;

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));

        OCIO_CHECK_NO_THROW(config->validate());
    }

    // Test that the interchange is NOT valid in v2.0 config.
    {
        constexpr char End[]{R"(colorspaces:
  - !<ColorSpace>
    name: raw
    interchange:
        amf_transform_ids: should NOT be valid in 2.0 config
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: Some text.
    isdata: true
    allocation: uniform
)"};
        std::string cfgString{Start};
        cfgString += End;

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_THROW_WHAT(
            config = OCIO::Config::CreateFromStream(is),
            OCIO::Exception,
            "Config failed validation. The color space 'raw' has non-empty "
            "interchange attributes and config version is less than 2.5.");
    }

    // Interchange tests in 2.5
    constexpr char Start2_5[]{R"(ocio_profile_version: 2.5

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

)"};

    // Test that the interchange is valid in v2.5 config.
    {
        constexpr char End_amf[]{R"(
colorspaces:
  - !<ColorSpace>
    name: raw
    interchange:
        amf_transform_ids: This is valid in 2.5 config
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: Some text.
    isdata: true
    allocation: uniform
)"};

        std::string cfgString{Start2_5};
        cfgString += End_amf;

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_REQUIRE_EQUAL(!config, false);
        auto attrMap = config->getColorSpace("raw")->getInterchangeAttributes();
        OCIO_CHECK_EQUAL(attrMap.size(), 1);
    }

    // Test that the unknown interchange attrib will be ignored in 2.5.
    {
        constexpr char End_unkown[]{R"(
colorspaces:
  - !<ColorSpace>
    name: raw
    interchange:
        my-attrib: will be ignored
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: Some text.
    isdata: true
    allocation: uniform
)"};

        std::string cfgString{Start2_5};
        cfgString += End_unkown;

        std::istringstream is;
        is.str(cfgString);
        OCIO::ConstConfigRcPtr config;
        OCIO::LogGuard logGuard;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_EQUAL(logGuard.output(), "[OpenColorIO Warning]: Unknown key in interchange: 'my-attrib'.\n");
        OCIO_REQUIRE_EQUAL(!config, false);
        auto attrMap = config->getColorSpace("raw")->getInterchangeAttributes();
        OCIO_CHECK_EQUAL(attrMap.size(), 0);
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

OCIO_ADD_TEST(ColorSpace, interop_id)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    
    // Test default value.
    OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), "");

    // Test setting and getting single profile name.
    const char * interop_id = "srgb_p3d65_scene";
    cs->setInteropID(interop_id);
    OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), interop_id);

    // Test setting empty string.
    cs->setInteropID("");
    OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), "");

    // Test setting and getting another value.
    const char * anotherID= "lin_rec2020_scene";
    cs->setInteropID(anotherID);
    OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), anotherID);

    // Test setting null pointer (should be safe).
    cs->setInteropID("something");
    OCIO_CHECK_NO_THROW(cs->setInteropID(nullptr));
    OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), "");

    // Test copy constructor preserves InteropID.
    cs->setInteropID(interop_id);
    OCIO::ColorSpaceRcPtr copy = cs->createEditableCopy();
    OCIO_CHECK_EQUAL(std::string(copy->getInteropID()), interop_id);

    // Test valid InteropID with colon in the middle.
    const char * validColonMiddle = "namespace:colorspace_name";
    OCIO_CHECK_NO_THROW(cs->setInteropID(validColonMiddle));
    OCIO_CHECK_EQUAL(std::string(cs->getInteropID()), validColonMiddle);

    // Test invalid InteropID with multiple colons.
    OCIO_CHECK_THROW_WHAT(cs->setInteropID("name:space:cs_name"), OCIO::Exception,
                          "Only one ':' is allowed to separate the namespace and the color space.");

    // Test invalid InteropID with colon at the end.
    OCIO_CHECK_THROW_WHAT(cs->setInteropID("namespace:"), OCIO::Exception,
                          " If ':' is used, both the namespace and the color space parts must be non-empty.");

    // Test invalid InteropID with empty namespace or color space.
    OCIO_CHECK_THROW_WHAT(cs->setInteropID(":cs_name"), OCIO::Exception,
                          "If ':' is used, both the namespace and the color space parts must be non-empty.");
    
    // Test invalid InteropID with illegal characters.
    OCIO_CHECK_THROW_WHAT(cs->setInteropID("café_scene"), OCIO::Exception,
                          "Only lowercase a-z, 0-9 and . - _ ~ / * # % ^ + ( ) [ ] | are allowed.");

    OCIO_CHECK_THROW_WHAT(cs->setInteropID("UPPERCASE"), OCIO::Exception,
                          "Only lowercase a-z, 0-9 and . - _ ~ / * # % ^ + ( ) [ ] | are allowed.");

    OCIO_CHECK_THROW_WHAT(cs->setInteropID("{curly_bracket}"), OCIO::Exception,
                          "Only lowercase a-z, 0-9 and . - _ ~ / * # % ^ + ( ) [ ] | are allowed.");
    
    OCIO_CHECK_THROW_WHAT(cs->setInteropID("\\backslash"), OCIO::Exception,
                          "Only lowercase a-z, 0-9 and . - _ ~ / * # % ^ + ( ) [ ] | are allowed.");

    OCIO_CHECK_THROW_WHAT(cs->setInteropID(" space "), OCIO::Exception,
                          "Only lowercase a-z, 0-9 and . - _ ~ / * # % ^ + ( ) [ ] | are allowed.");
}

OCIO_ADD_TEST(ColorSpace, interop_id_serialization)
{
    // Test YAML serialization and deserialization of InteropID.
    auto cfg = OCIO::Config::Create();
    auto cs = OCIO::ColorSpace::Create();
    cs->setName("test_colorspace");
    
    const std::string interop_id = "lin_rec709_scene";
    
    cs->setInteropID(interop_id.c_str());
    cfg->addColorSpace(cs);

    // Serialize the Config.
    std::stringstream ss;
    cfg->serialize(ss);
    std::string yamlStr = ss.str();

    // Verify interop_id appears in YAML.
    OCIO_CHECK_NE(yamlStr.find("interop_id"), std::string::npos);
    OCIO_CHECK_NE(yamlStr.find(interop_id), std::string::npos);

    // Deserialize and verify.
    std::istringstream iss(yamlStr);
    OCIO::ConstConfigRcPtr deserializedCfg;
    OCIO_CHECK_NO_THROW(deserializedCfg = OCIO::Config::CreateFromStream(iss));

    // Verify interop_id is preserved.
    OCIO::ConstColorSpaceRcPtr deserializedCs = deserializedCfg->getColorSpace("test_colorspace");
    OCIO_CHECK_EQUAL(std::string(deserializedCs->getInteropID()), interop_id);

    // verify that that versions earlier than 2.0 reject interop_id.
    OCIO::ConfigRcPtr cfgCopy = cfg->createEditableCopy();
    cfgCopy->setVersion(2, 0);
    OCIO_CHECK_NO_THROW(cfgCopy->serialize(ss));

    cfgCopy->setVersion(1, 0);
    OCIO_CHECK_THROW_WHAT(
        cfgCopy->serialize(ss),
        OCIO::Exception,
        "Config failed validation. The color space 'test_colorspace' has non-empty "
        "InteropID and config version is less than 2.0.");

    // Test with empty interop_id (should not appear in YAML).
    cs->setInteropID(nullptr);
    cfg->addColorSpace(cs); // Replace the existing CS.
    ss.str("");
    cfg->serialize(ss);
    std::string yamlStr2 = ss.str();

    // Verify empty interop_id does not appear in YAML.
    OCIO_CHECK_EQUAL(yamlStr2.find("interop_id"), std::string::npos);
}

OCIO_ADD_TEST(ColorSpace, amf_transform_ids)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    
    // Test default value.
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("amf_transform_ids")), "");

    // Test setting and getting single ID.
    const char * singleID = "urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScc_to_ACES.a1.0.3";
    cs->setInterchangeAttribute("amf_transform_ids", singleID);
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("amf_transform_ids")), singleID);

    // Test setting to empty string.
    cs->setInterchangeAttribute("amf_transform_ids", "");
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("amf_transform_ids")), "");    
    
    // Test setting and getting multiple IDs.
    const char * multipleIDs = 
        "urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScc_to_ACES.a1.0.3\n"
        "urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_ACEScc.a1.0.3";
    cs->setInterchangeAttribute("amf_transform_ids", multipleIDs);
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("amf_transform_ids")), multipleIDs);

    // Test setting to null pointer (should be safe).
    cs->setInterchangeAttribute("amf_transform_ids", "something");
    cs->setInterchangeAttribute("amf_transform_ids", nullptr);
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("amf_transform_ids")), "");

    // Test copy constructor preserves AMF transform IDs.
    cs->setInterchangeAttribute("amf_transform_ids", singleID);
    OCIO::ColorSpaceRcPtr copy = cs->createEditableCopy();
    OCIO_CHECK_EQUAL(std::string(copy->getInterchangeAttribute("amf_transform_ids")), singleID);
}

OCIO_ADD_TEST(ColorSpace, amf_transform_ids_serialization)
{
    // Test YAML serialization and deserialization of AmfTransformIDs.
    auto cfg = OCIO::Config::Create();
    auto cs = OCIO::ColorSpace::Create();
    cs->setName("test_colorspace");
    
    const std::string amfIDs = 
        "urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScc_to_ACES.a1.0.3\n"
        "urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_ACEScc.a1.0.3";
    
    cs->setInterchangeAttribute("amf_transform_ids", amfIDs.c_str());
    cfg->addColorSpace(cs);

    // Serialize the Config.
    std::stringstream ss;
    cfg->serialize(ss);
    std::string yamlStr = ss.str();

    // Verify AmfTransformIDs appears in YAML.
    OCIO_CHECK_NE(yamlStr.find("amf_transform_ids"), std::string::npos);
    OCIO_CHECK_NE(yamlStr.find("ACEScsc.Academy.ACEScc_to_ACES"), std::string::npos);

    // Deserialize and verify.
    std::istringstream iss(yamlStr);
    OCIO::ConstConfigRcPtr deserializedCfg;
    OCIO_CHECK_NO_THROW(deserializedCfg = OCIO::Config::CreateFromStream(iss));

    // Verify AmfTransformIDs is preserved.
    OCIO::ConstColorSpaceRcPtr deserializedCs = deserializedCfg->getColorSpace("test_colorspace");
    auto deserializedIDs = deserializedCs->getInterchangeAttribute("amf_transform_ids");
    OCIO_CHECK_EQUAL(deserializedIDs, amfIDs);

    // Verify that that earlier versions reject amf_transform_ids.
    OCIO::ConfigRcPtr cfgCopy = cfg->createEditableCopy();
    cfgCopy->setVersion(2,4);
    OCIO_CHECK_THROW_WHAT(cfgCopy->serialize(ss),
        OCIO::Exception,
        "has non-empty interchange attributes and config version is less than 2.5.");

    // Test with empty AmfTransformIDs (should not appear in YAML).
    cs->setInterchangeAttribute("amf_transform_ids", nullptr);
    cfg->addColorSpace(cs); // Replace the existing CS.
    ss.str("");
    cfg->serialize(ss);
    std::string yamlStr2 = ss.str();

    // Verify empty AmfTransformIDs does not appear in YAML.
    OCIO_CHECK_EQUAL(yamlStr2.find("amf_transform_ids"), std::string::npos);
}

OCIO_ADD_TEST(ColorSpace, icc_profile_name)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    
    // Test default value.
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("icc_profile_name")), "");

    // Test setting and getting single profile name.
    const char * profileName = "sRGB IEC61966-2.1";
    cs->setInterchangeAttribute("icc_profile_name",profileName);
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("icc_profile_name")), profileName);

    // Test setting and getting another profile name.
    const char * anotherProfile = "Adobe RGB (1998)";
    cs->setInterchangeAttribute("icc_profile_name", anotherProfile);
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("icc_profile_name")), anotherProfile);

    // Test setting empty string.
    cs->setInterchangeAttribute("icc_profile_name", "");
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("icc_profile_name")), "");

    // Test setting null pointer (should be safe).
    cs->setInterchangeAttribute("icc_profile_name", "something");
    OCIO_CHECK_NO_THROW(cs->setInterchangeAttribute("icc_profile_name", nullptr));
    OCIO_CHECK_EQUAL(std::string(cs->getInterchangeAttribute("icc_profile_name")), "");

    // Test copy constructor preserves ICC profile name.
    cs->setInterchangeAttribute("icc_profile_name", profileName);
    OCIO::ColorSpaceRcPtr copy = cs->createEditableCopy();
    OCIO_CHECK_EQUAL(std::string(copy->getInterchangeAttribute("icc_profile_name")), profileName);
}

OCIO_ADD_TEST(ColorSpace, icc_profile_name_serialization)
{
    // Test YAML serialization and deserialization of IccProfileName.
    auto cfg = OCIO::Config::Create();
    auto cs = OCIO::ColorSpace::Create();
    cs->setName("test_colorspace");
    
    const std::string profileName = "sRGB IEC61966-2.1";
    
    cs->setInterchangeAttribute("icc_profile_name", profileName.c_str());
    cfg->addColorSpace(cs);

    // Serialize the Config.
    std::stringstream ss;
    cfg->serialize(ss);
    std::string yamlStr = ss.str();

    // Verify IccProfileName appears in YAML.
    OCIO_CHECK_NE(yamlStr.find("icc_profile_name"), std::string::npos);
    OCIO_CHECK_NE(yamlStr.find(profileName), std::string::npos);

    // Deserialize and verify.
    std::istringstream iss(yamlStr);
    OCIO::ConstConfigRcPtr deserializedCfg;
    OCIO_CHECK_NO_THROW(deserializedCfg = OCIO::Config::CreateFromStream(iss));

    // Verify IccProfileName is preserved.
    OCIO::ConstColorSpaceRcPtr deserializedCs = deserializedCfg->getColorSpace("test_colorspace");
    OCIO_CHECK_EQUAL(std::string(deserializedCs->getInterchangeAttribute("icc_profile_name")), profileName);

    // verify that that earlier versions reject icc_profile_name.
    OCIO::ConfigRcPtr cfgCopy = cfg->createEditableCopy();
    cfgCopy->setVersion(2, 4);
    OCIO_CHECK_THROW_WHAT(cfgCopy->serialize(ss),
        OCIO::Exception,
        "has non-empty interchange attributes and config version is less than 2.5.");

    // Test with empty IccProfileName (should not appear in YAML, and so won't invalidate a 2.4 config).
    cs->setInterchangeAttribute("icc_profile_name", nullptr);
    cfg->addColorSpace(cs); // replace the existing CS
    ss.str("");
    cfg->serialize(ss);
    std::string yamlStr2 = ss.str();

    // Verify empty IccProfileName does not appear in YAML.
    OCIO_CHECK_EQUAL(yamlStr2.find("icc_profile_name"), std::string::npos);
}

OCIO_ADD_TEST(ColorSpace, unknown_interchange_attrib)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    
    // Getting should throw.
    OCIO_CHECK_THROW_WHAT(std::string(cs->getInterchangeAttribute("unknown_attrib")),
        OCIO::Exception, 
        "Unknown attribute name");
    
    // Empty name is not legal.
    OCIO_CHECK_THROW_WHAT(std::string(cs->getInterchangeAttribute("")), OCIO::Exception, 
        "Unknown attribute name");
    OCIO_CHECK_THROW_WHAT(std::string(cs->getInterchangeAttribute(nullptr)), OCIO::Exception, 
        "Unknown attribute name");

    // Setting should throw too.
    OCIO_CHECK_THROW_WHAT(cs->setInterchangeAttribute("unknown_attribute1", "unknown"),
        OCIO::Exception, 
        "Unknown attribute name");
    OCIO_CHECK_THROW_WHAT(cs->setInterchangeAttribute("unknown_attribute2", ""),
        OCIO::Exception, 
        "Unknown attribute name");
    OCIO_CHECK_THROW_WHAT(cs->setInterchangeAttribute("unknown_attribute3", nullptr),
        OCIO::Exception, 
        "Unknown attribute name");

    // Make sure none of the above was stored.
    auto attrMap = cs->getInterchangeAttributes();
    OCIO_CHECK_EQUAL(attrMap.size(), 0);
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

  - !<ColorSpace>
    name: linear_mtx_from_file
    description: This is an identity matrix and therefore linear, but is in an external file.
    isdata: false
    to_scene_reference: !<GroupTransform>
      children:
        - !<FileTransform> {src: clf/matrix_windows.clf, interpolation: linear}

  - !<ColorSpace>
    name: linear_lut3d_from_file
    description: |
      This is a Lut3D which is linear across it's unbounded range but, like all LUTs, clamps 
      outside it's [0,1] domain.  Therefore, when the algorithm inputs [4,4,4], it is no different
      than inputing [1,1,1] and so it is not determined to be linear.
    isdata: false
    to_scene_reference: !<GroupTransform>
      children:
        - !<FileTransform> {src: clf/lut3d_as_matrix.clf, interpolation: linear}
)" };

    // Load config.
    std::istringstream is;
    is.str(TEST_CONFIG);
    OCIO::ConstConfigRcPtr src_config;
    
    OCIO_CHECK_NO_THROW(src_config = OCIO::Config::CreateFromStream(is));

    OCIO::ConfigRcPtr config = src_config->createEditableCopy();
    config->setSearchPath(OCIO::GetTestFilesDir().c_str());

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

    // Test undefined color spaces.
    {
      OCIO_CHECK_THROW_WHAT(
        config->isColorSpaceLinear("colorspace_abc", OCIO::REFERENCE_SPACE_SCENE), OCIO::Exception,
        "Could not test colorspace linearity. Colorspace colorspace_abc does not exist"
      );

      OCIO_CHECK_THROW_WHAT(
        config->isColorSpaceLinear("colorspace_abc", OCIO::REFERENCE_SPACE_DISPLAY), OCIO::Exception,
        "Could not test colorspace linearity. Colorspace colorspace_abc does not exist"
      );
    }

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
        testSceneReferred("linear_mtx_from_file", true, __LINE__);
        testSceneReferred("linear_lut3d_from_file", false, __LINE__);
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
        testDisplayReferred("linear_mtx_from_file", false, __LINE__);
        testDisplayReferred("linear_lut3d_from_file", false, __LINE__);
    }
}

OCIO_ADD_TEST(ConfigUtils, processor_to_known_colorspace)
{
    constexpr const char * CONFIG { R"(
ocio_profile_version: 2

roles:
  default: raw data
  scene_linear: ref_cs

display_colorspaces:
  - !<ColorSpace>
    name: CIE-XYZ-D65
    description: The CIE XYZ (D65) display connection colorspace.
    isdata: false

  - !<ColorSpace>
    name: sRGB - Display CS
    description: Convert CIE XYZ (D65 white) to sRGB (piecewise EOTF)
    isdata: false
    from_display_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_sRGB}

colorspaces:
  # Put a couple of test color space first in the config since the heuristics stop upon success.

  - !<ColorSpace>
    name: File color space AP0 to linear rec.709
    description: Would be useable by the heuristics except for the clamping and the fact that it's a clf.
    isdata: false
    from_scene_reference: !<GroupTransform>
      children:
        - !<GroupTransform>
          children:
            - !<FileTransform> {src: clf/lut3d_as_matrix.clf}

  - !<ColorSpace>
    name: CS Transform color space
    description: Verify that that ColorSpaceTransforms load correctly when running the heuristics.
    isdata: false
    from_scene_reference: !<GroupTransform>
      children:
        - !<ColorSpaceTransform> {src: ref_cs, dst: not sRGB}

  - !<ColorSpace>
    name: raw data
    description: A data colorspace (should not be used).
    isdata: true

  - !<ColorSpace>
    name: ref_cs
    description: The reference colorspace, ACES2065-1.
    isdata: false

  - !<ColorSpace>
    name: not sRGB
    description: A color space that misleadingly has sRGB in the name, even though it's not.
    isdata: false
    to_scene_reference: !<BuiltinTransform> {style: ACEScct_to_ACES2065-1}

  - !<ColorSpace>
    name: sRGB - curve
    description: Just the sRGB gamma curve.  Note - None of the heuristics should be able to use this.
    isdata: false
    from_scene_reference: !<GroupTransform>
      children:
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: sRGB - curve 2
    description: Another sRGB curve the heuristics shouldn't use.  NB - the transform uses a matrix.
    isdata: false
    to_scene_reference: !<GroupTransform>
      children:
        - !<FileTransform> {src: sRGB_to_linear.spi1d, interpolation: linear}

  - !<ColorSpace>
    name: pseudo sRGB
    description: Ensure that a gamma 2.2 Rec.709 space is not mistaken for an sRGB space.
    isdata: false
    from_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}
        - !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1], direction: inverse}

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
    name: sRGB Encoded AP1 - Texture
    description: Another space with "sRGB" in the name that is not actually an sRGB texture space.
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Encoded AP1 - Texture
      children:
        - !<MatrixTransform> {matrix: [1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: OCIO v1 -- sRGB
    description: The sRGB texture space from the legacy v1 ACES config. Usable by the heuristics despite the file.
    isdata: false
    from_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [0.952552, 0, 9.36786e-05, 0, 0.343966, 0.728166, -0.0721325, 0, 0, 0, 1.00883, 0, 0, 0, 0, 1]}
        - !<MatrixTransform> {matrix: [3.2096, -1.55743, -0.495805, 0, -0.970989, 1.88517, 0.0394894, 0, 0.0597193, -0.210104, 1.14312, 0, 0, 0, 0, 1]}
        - !<FileTransform> {src: sRGB_to_linear.spi1d, interpolation: linear, direction: inverse}

  - !<ColorSpace>
    name: Texture -- sRGB
    description: An sRGB Texture space, spelled differently than in the built-in config.
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Rec.709
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}
)" };

    std::istringstream is;
    is.str(CONFIG);
    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));

    OCIO::ConfigRcPtr editableCfg = cfg->createEditableCopy();

    editableCfg->setSearchPath(OCIO::GetTestFilesDir().c_str());

    OCIO::ConstConfigRcPtr builtinConfig = OCIO::Config::CreateFromFile("ocio://default");

    // Make all color spaces suitable for the heuristics inactive.
    // The heuristics don't look at inactive color spaces.
    editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, Texture -- sRGB, OCIO v1 -- sRGB");

    std::string srcColorSpaceName = "not sRGB";
    std::string builtinColorSpaceName  = "Gamma 2.2 AP1 - Texture";

    // Test throw if no suitable spaces are present.
    {
        OCIO_CHECK_THROW_WHAT(
            auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(editableCfg,
                                                                      srcColorSpaceName.c_str(),
                                                                      builtinColorSpaceName.c_str()),
            OCIO::Exception,
            "Heuristics were not able to find a known color space in the provided config. "
            "Please set the interchange roles."
        );
    }

    // Generate a reference Processor for the correct result.
    OCIO::ConstProcessorRcPtr refProc = OCIO::Config::GetProcessorFromConfigs(
        editableCfg,
        srcColorSpaceName.c_str(),
        "ref_cs",
        builtinConfig,
        builtinColorSpaceName.c_str(),
        "ACES2065-1");

    // Now make various spaces active and test that they enable the heuristics to find
    // the appropriate interchange spaces.

    editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, OCIO v1 -- sRGB");
    {
        // Uses "sRGB Texture" to find the reference.
        auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  builtinColorSpaceName.c_str());
        OCIO_CHECK_EQUAL(std::string(refProc->getCacheID()), std::string(proc->getCacheID()));
    }

    // Test using linear color space from_ref direction.
    editableCfg->setInactiveColorSpaces("ACES cg, Texture -- sRGB, OCIO v1 -- sRGB");
    {
        // Uses "Linear Rec.709 (sRGB)" to find the reference.
        auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  builtinColorSpaceName.c_str());
        OCIO_CHECK_EQUAL(std::string(refProc->getCacheID()), std::string(proc->getCacheID()));
    }

    // Test linear color space to_ref direction.
    editableCfg->setInactiveColorSpaces("Linear ITU-R BT.709, Texture -- sRGB, OCIO v1 -- sRGB");
    {
        // Uses "ACEScg" to find the reference.
        auto proc = OCIO::Config::GetProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  builtinColorSpaceName.c_str());
        OCIO_CHECK_EQUAL(std::string(refProc->getCacheID()), std::string(proc->getCacheID()));
    }

    // Generate a reference Processor for the correct result.
    OCIO::ConstProcessorRcPtr invRefProc = OCIO::Config::GetProcessorFromConfigs(
        builtinConfig,
        builtinColorSpaceName.c_str(),
        "ACES2065-1",
        editableCfg,
        srcColorSpaceName.c_str(),
        "ref_cs");

    // Make the reference space inactive too.
    editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, ref_cs, OCIO v1 -- sRGB");
    {
        // Uses "sRGB Texture" to find the reference.
        auto proc = OCIO::Config::GetProcessorFromBuiltinColorSpace(builtinColorSpaceName.c_str(),
                                                                    editableCfg,
                                                                    srcColorSpaceName.c_str());
        OCIO_CHECK_EQUAL(std::string(invRefProc->getCacheID()), std::string(proc->getCacheID()));
    }

    // Test using linear color space from_ref direction.
    editableCfg->setInactiveColorSpaces("ACES cg, Texture -- sRGB, ref_cs, OCIO v1 -- sRGB");
    {
        // Uses "Linear Rec.709 (sRGB)" to find the reference.
        auto proc = OCIO::Config::GetProcessorFromBuiltinColorSpace(builtinColorSpaceName.c_str(),
                                                                    editableCfg,
                                                                    srcColorSpaceName.c_str());
        OCIO_CHECK_EQUAL(std::string(invRefProc->getCacheID()), std::string(proc->getCacheID()));
    }

    // Test linear color space to_ref direction.
    editableCfg->setInactiveColorSpaces("Linear ITU-R BT.709, Texture -- sRGB, ref_cs, OCIO v1 -- sRGB");
    {
        // Uses "ACEScg" to find the reference.
        auto proc = OCIO::Config::GetProcessorFromBuiltinColorSpace(builtinColorSpaceName.c_str(),
                                                                    editableCfg,
                                                                    srcColorSpaceName.c_str());
        OCIO_CHECK_EQUAL(std::string(invRefProc->getCacheID()), std::string(proc->getCacheID()));
    }

    //
    // Test IdentifyInterchangeSpace.
    //

    {
        // Uses "ACEScg" to find the reference.
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "Linear ITU-R BT.709", 
                                               builtinConfig, "lin_rec709_srgb");
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("ref_cs"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("ACES2065-1"));
    }

    editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, OCIO v1 -- sRGB");
    {
        // Uses "sRGB Texture" to find the reference.
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "Linear ITU-R BT.709", 
                                               builtinConfig, "lin_rec709_srgb");
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("ref_cs"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("ACES2065-1"));
    }

    editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, Texture -- sRGB");
    {
        // Uses "OCIO v1 -- sRGB" to find the reference.
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "Linear ITU-R BT.709", 
                                               builtinConfig, "lin_rec709_srgb");
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("ref_cs"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("ACES2065-1"));
    }

    // Set the interchange role.  In order to prove that it is being used rather than
    // the heuristics, set it to something wrong and check that it gets returned anyway.
    editableCfg->setRole("aces_interchange", "Texture -- sRGB");
    {
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "Linear ITU-R BT.709", 
                                               builtinConfig, "lin_rec709_srgb");
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("Texture -- sRGB"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("ACES2065-1"));
    }

    // Unset the interchange role, so the heuristics will be used for the other tests.
    editableCfg->setRole("aces_interchange", "");

    // Check what happens if a totally bogus config is passed for the built-in config.
    OCIO::ConstConfigRcPtr rawCfg = OCIO::Config::CreateRaw();
    {
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                                   editableCfg, "raw data", 
                                                   rawCfg, "raw"),
            OCIO::Exception,
            // (It fails in the first heuristic that tries to use one of the known 
            //  built-in spaces, hence the surprising error message.)
            "Could not find destination color space 'sRGB - Texture'"
        );
    }

    // Check what happens if the source color space doesn't exist.
    {
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                                   editableCfg, "Foo", 
                                                   rawCfg, "raw"),
            OCIO::Exception,
            "Could not find source color space 'Foo'."
        );
    }

    // Check what happens if the destination color space doesn't exist.
    {
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                                   editableCfg, "Foo", 
                                                   rawCfg, ""),
            OCIO::Exception,
            "Could not find destination color space ''."
        );
    }

    //
    // Test IdentifyBuiltinColorSpace.
    //

    editableCfg->setInactiveColorSpaces("OCIO v1 -- sRGB");

    {
        // Uses "Texture -- sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACEScg");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("ACES cg"));
    }

    {
        // Uses "Texture -- sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Texture");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("Texture -- sRGB"));
    }

    {
        // Uses "Texture -- sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACES2065-1");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("ref_cs"));
    }

    editableCfg->setInactiveColorSpaces("Texture -- sRGB");

    {
        // Uses "OCIO v1 -- sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Texture");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("OCIO v1 -- sRGB"));
    }

    editableCfg->setInactiveColorSpaces("Texture -- sRGB, ref_cs, OCIO v1 -- sRGB");

    {
        // Uses "ACEScg" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Linear Rec.709 (sRGB)");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("Linear ITU-R BT.709"));
    }

    {
        // Uses "ACEScg" to find the reference.
        // If ColorSpaceTransforms were allowed, this would find "CS Transform color space" instead.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACEScct");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("not sRGB"));
    }

    // Aliases for built-in color spaces must work.
    {
        // Uses "ACEScg" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "lin_ap1");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("ACES cg"));
    }

    // Test that a data space is handled properly.
    {
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Raw");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("raw data"));
    }

    // Display-referred spaces are not supported unless the display-referred interchange
    // role is present.
    {
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display"),
            OCIO::Exception,
            "The heuristics currently only support scene-referred color spaces. Please set the interchange roles."
        );
    }

    // The next three cases directly use the interchange roles rather than heuristics.

    // With the required role, it then works.
    editableCfg->setRole("cie_xyz_d65_interchange", "CIE-XYZ-D65");
    {
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("sRGB - Display CS"));
    }

    // Must continue to work if the color space for the interchange role is inactive.
    editableCfg->setInactiveColorSpaces("CIE-XYZ-D65");
    {
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("sRGB - Display CS"));
    }

    // Test the scene-referred interchange role (and even make it inactive).
    editableCfg->setRole("aces_interchange", "ref_cs");
    editableCfg->setInactiveColorSpaces("ref_cs");
    {
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACEScg");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("ACES cg"));
    }
}

OCIO_ADD_TEST(ConfigUtils, processor_to_known_colorspace_alt_config)
{
    // This test uses a config that has a different reference space than the built-in config.

    constexpr const char * CONFIG { R"(
ocio_profile_version: 2

environment: {}

roles:
  default: sRGB
  scene_linear: scene-linear Rec.709-sRGB
  rendering: scene-linear Rec.709-sRGB

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: Un-tone-mapped, view_transform: Un-tone-mapped, display_colorspace: <USE_DISPLAY_NAME>}
  - !<View> {name: Raw, colorspace: Raw}

displays:
  sRGB:
    - !<Views> [Un-tone-mapped, Raw]
  Gamma 2.2 / Rec.709:
    - !<Views> [ Un-tone-mapped, Raw]

view_transforms:
  - !<ViewTransform>
    name: Un-tone-mapped
    from_scene_reference: !<MatrixTransform> {matrix: [ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 ]}

inactive_colorspaces: [scene-linear Rec.709-sRGB, ACES2065-1]

display_colorspaces:
  - !<ColorSpace>
    name: CIE-XYZ D65
    encoding: display-linear
    isdata: false
    to_display_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: display-linear Rec.709-sRGB
    description: |
      Display reference space
    isdata: false
    encoding: display-linear

  - !<ColorSpace>
    name: sRGB
    isdata: false
    categories: [ file-io ]
    encoding: sdr-video
    from_display_reference: !<GroupTransform>
      children:
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}
        - !<RangeTransform> {min_in_value: 0., min_out_value: 0., max_in_value: 1., max_out_value: 1.}

colorspaces:
  - !<ColorSpace>
    name: Raw
    isdata: true
    categories: [ file-io ]
    encoding: data

  - !<ColorSpace>
    name: ACES2065-1
    isdata: false
    encoding: scene-linear
    to_scene_reference: !<MatrixTransform> {matrix: [ 2.521686186744, -1.134130988240, -0.387555198504, 0, -0.276479914230, 1.372719087668, -0.096239173438, 0, -0.015378064966, -0.152975335867, 1.168353400833, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene-linear Rec.709-sRGB
    description: |
      Scene-linear Rec.709 or sRGB primaries -- ** This is the scene reference space **
    isdata: false
    categories: [ file-io, working-space ]
    encoding: scene-linear

  - !<ColorSpace>
    name: scene-linear P3-D65
    aliases: [lin_p3d65, Utility - Linear - P3-D65]
    isdata: false
    encoding: scene-linear
    to_scene_reference: !<MatrixTransform> {matrix: [ 1.224940176281e+00, -2.249401762806e-01, 0, 0, -4.205695470969e-02,  1.042056954710e+00, 0, 0, -1.963755459033e-02, -7.863604555063e-02,  1.098273600141e+00, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene-linear Rec.2020
    isdata: false
    encoding: scene-linear
    from_scene_reference: !<MatrixTransform> {matrix: [ 0.627403895935, 0.329283038378, 0.043313065687, 0 , 0.069097289358, 0.919540395075, 0.011362315566, 0, 0.016391438875, 0.088013307877, 0.895595253248, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: texture sRGB
    isdata: false
    categories: [ file-io ]
    encoding: sdr-video
    from_scene_reference: !<GroupTransform>
      children:
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}
        - !<RangeTransform> {min_in_value: 0., min_out_value: 0., max_in_value: 1., max_out_value: 1.}
)" };

    std::istringstream is;
    is.str(CONFIG);
    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));

    OCIO::ConfigRcPtr editableCfg = cfg->createEditableCopy();

    OCIO::ConfigRcPtr builtinConfig = OCIO::Config::CreateFromFile("ocio://default")->createEditableCopy();

    // Make all of the supported linear color spaces in the built-in config inactive.
    // These spaces are referenced by name in the heuristics, so it should all still work.
    // This is different than the inactive color spaces in the user's config, which the
    // heuristics generally ignore (unless it's the reference space).  (This should not
    // affect the tests below, it simply confirms that inactive spaces are working as
    // expected.)
    builtinConfig->setInactiveColorSpaces(
        "ACES2065-1, ACEScg, Linear Rec.709 (sRGB), Linear P3-D65, Linear Rec.2020, CIE XYZ-D65 - Display-referred, sRGB - Display"
    );

    //
    // Test IdentifyBuiltinColorSpace.
    //

    // The initial editableCfg inactive color spaces are: "scene-linear Rec.709-sRGB, ACES2065-1".

    {
        // Uses "texture sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("scene-linear Rec.2020"));
    }

    {
        // Uses "texture sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Linear P3-D65");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("scene-linear P3-D65"));
    }

    editableCfg->setInactiveColorSpaces("ACES2065-1");
    {
        // Uses "texture sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Linear Rec.709 (sRGB)");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("scene-linear Rec.709-sRGB"));
    }

    editableCfg->setInactiveColorSpaces("ACES2065-1, texture sRGB");
    {
        // Uses "Linear P3-D65" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Linear Rec.709 (sRGB)");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("scene-linear Rec.709-sRGB"));
    }

    editableCfg->setInactiveColorSpaces("ACES2065-1");
    {
        // Uses "texture sRGB" to find the reference.
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Texture");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("texture sRGB"));
    }

    // The color space is present in editableCfg but it's inactive, so not found.
    {
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACES2065-1"),
            OCIO::Exception,
            "Heuristics were not able to find an equivalent to the requested color space: ACES2065-1."
        );
    }

    // Use interchange role rather than heuristics.

    editableCfg->setRole("aces_interchange", "ACES2065-1");
    {
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Texture");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("texture sRGB"));
    }

    {
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Linear Rec.709 (sRGB)");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("scene-linear Rec.709-sRGB"));
    }

    editableCfg->setInactiveColorSpaces("");
    {
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "lin_ap0");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("ACES2065-1"));
    }
    editableCfg->setRole("aces_interchange", "");

    // Display-referred spaces won't work via heuristics, need the interchange role.
    {
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display"),
            OCIO::Exception,
            "The heuristics currently only support scene-referred color spaces. Please set the interchange roles."
        );
    }
    editableCfg->setRole("cie_xyz_d65_interchange", "CIE-XYZ D65");
    {
        // Note that it still works even if the built-in color space is inactive (though not for the source).
        const std::string inactives{builtinConfig->getInactiveColorSpaces()};
        OCIO_CHECK_EQUAL(StringUtils::Find(inactives, "sRGB - Display"), 107);
        const char * csname = OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display");
        OCIO_CHECK_EQUAL(std::string(csname), std::string("sRGB"));
    }
    editableCfg->setRole("cie_xyz_d65_interchange", "");

    // Check handling of color space that isn't in the built-in config.
    {
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "does not exist"),
            OCIO::Exception,
            "Built-in config does not contain the requested color space: does not exist."
        );
    }

    //
    // Test IdentifyInterchangeSpace.
    //

    editableCfg->setInactiveColorSpaces("scene-linear Rec.709-sRGB, ACES2065-1");
    {
        // Uses "texture sRGB" to find the reference.
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "scene-linear Rec.709-sRGB", 
                                               builtinConfig, "lin_rec709_srgb");   // Aliases work.
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("scene-linear Rec.709-sRGB"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("Linear Rec.709 (sRGB)"));
    }

    editableCfg->setInactiveColorSpaces("texture sRGB, scene-linear Rec.709-sRGB, ACES2065-1");
    {
        // Uses "Linear P3-D65" to find the reference.
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "lin_p3d65", 
                                               builtinConfig, "lin_rec709_srgb");
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("scene-linear Rec.709-sRGB"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("Linear Rec.709 (sRGB)"));
    }

    editableCfg->setInactiveColorSpaces("scene-linear P3-D65, texture sRGB, scene-linear Rec.709-sRGB, ACES2065-1");
    {
        // Uses "Linear Rec.2020" to find the reference.
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "Raw",   // A data space, but it works.
                                               builtinConfig, "lin_rec709_srgb");
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("scene-linear Rec.709-sRGB"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("Linear Rec.709 (sRGB)"));
    }

    editableCfg->setInactiveColorSpaces("scene-linear P3-D65, texture sRGB, scene-linear Rec.709-sRGB");
    {
        // Uses "ACES2065-1" to find the reference.
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                               editableCfg, "Raw", 
                                               builtinConfig, "Raw");
        OCIO_CHECK_EQUAL(std::string(srcInterchange), std::string("scene-linear Rec.709-sRGB"));
        OCIO_CHECK_EQUAL(std::string(builtinInterchange), std::string("Linear Rec.709 (sRGB)"));
    }

    {
        const char * srcInterchange = nullptr;
        const char * builtinInterchange = nullptr;
        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::IdentifyInterchangeSpace(&srcInterchange, &builtinInterchange,
                                                   editableCfg, "CIE-XYZ D65", 
                                                   builtinConfig, "CIE XYZ-D65 - Display-referred"),
            OCIO::Exception,
            "The heuristics currently only support scene-referred color spaces. Please set the interchange roles."
        );
    }
}

