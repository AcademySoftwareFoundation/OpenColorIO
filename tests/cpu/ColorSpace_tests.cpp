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
