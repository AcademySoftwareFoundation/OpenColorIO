// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "Baker.cpp"

#include "testutils/UnitTest.h"
#include "ParseUtils.h"
#include "UnitTestUtils.h"
#include "utils/StringUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void CompareFloats(const std::string& floats1, const std::string& floats2)
{
    // Number comparison.
    const StringUtils::StringVec strings1 = StringUtils::SplitByWhiteSpaces(StringUtils::Trim(floats1));
    std::vector<float> numbers1;
    OCIO::StringVecToFloatVec(numbers1, strings1);

    const StringUtils::StringVec strings2 = StringUtils::SplitByWhiteSpaces(StringUtils::Trim(floats2));
    std::vector<float> numbers2;
    OCIO::StringVecToFloatVec(numbers2, strings2);

    OCIO_CHECK_EQUAL(numbers1.size(), numbers2.size());
    for (unsigned int j = 0; j<numbers1.size(); ++j)
    {
        OCIO_CHECK_CLOSE(numbers1[j], numbers2[j], 1e-5f);
    }
}
}

OCIO_ADD_TEST(Baker, bake_3dlut)
{
    OCIO::BakerRcPtr bake = OCIO::Baker::Create();

    static const std::string myProfile =
        "ocio_profile_version: 1\n"
        "\n"
        "strictparsing: false\n"
        "\n"
        "colorspaces :\n"
        "  - !<ColorSpace>\n"
        "    name : lnh\n"
        "    bitdepth : 16f\n"
        "    isdata : false\n"
        "    allocation : lg2\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name : test\n"
        "    bitdepth : 8ui\n"
        "    isdata : false\n"
        "    allocation : uniform\n"
        "    to_reference : !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}\n";

    static const std::string expectedLut =
        "CSPLUTV100\n"
        "3D\n"
        "\n"
        "BEGIN METADATA\n"
        "this is some metadata!\n"
        "END METADATA\n"
        "\n"
        "4\n"
        "0.000977 0.039373 1.587401 64.000000\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587401 64.000000\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587401 64.000000\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "\n"
        "2 2 2\n"
        "0.042823 0.042823 0.042823\n"
        "6.622026 0.042823 0.042823\n"
        "0.042823 6.622026 0.042823\n"
        "6.622026 6.622026 0.042823\n"
        "0.042823 0.042823 6.622026\n"
        "6.622026 0.042823 6.622026\n"
        "0.042823 6.622026 6.622026\n"
        "6.622026 6.622026 6.622026\n"
        "\n";
    std::istringstream is(myProfile);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 2);
    bake->setConfig(config);
    auto cfg2 = bake->getConfig();
    OCIO_REQUIRE_EQUAL(cfg2->getNumColorSpaces(), 2);

    const std::string testString{ "this is some metadata!" };
    bake->getFormatMetadata().addChildElement("Desc", testString.c_str());
    const auto & data = bake->getFormatMetadata();
    OCIO_CHECK_EQUAL(data.getNumChildrenElements(), 1);
    OCIO_CHECK_EQUAL(testString, data.getChildElement(0).getElementValue());

    bake->setFormat("cinespace");
    OCIO_CHECK_EQUAL("cinespace", std::string(bake->getFormat()));
    bake->setInputSpace("lnh");
    OCIO_CHECK_EQUAL("lnh", std::string(bake->getInputSpace()));
    bake->setLooks("foo, +bar");
    OCIO_CHECK_EQUAL("foo, +bar", std::string(bake->getLooks()));
    bake->setLooks("");
    bake->setTargetSpace("test");
    OCIO_CHECK_EQUAL("test", std::string(bake->getTargetSpace()));
    bake->setShaperSize(4);
    OCIO_CHECK_EQUAL(4, bake->getShaperSize());
    bake->setCubeSize(2);
    OCIO_CHECK_EQUAL(2, bake->getCubeSize());
    std::ostringstream os;
    OCIO_CHECK_NO_THROW(bake->bake(os));
    const StringUtils::StringVec osvec = StringUtils::SplitByLines(expectedLut);
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(os.str());
    OCIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for (unsigned int i = 0; i < resvec.size(); ++i)
    {
        if (i>6)
        {
            // Number comparison.
            CompareFloats(osvec[i], resvec[i]);
        }
        else
        {
            // text comparison
            OCIO_CHECK_EQUAL(osvec[i], resvec[i]);
        }
    }

    OCIO_CHECK_EQUAL(12, bake->getNumFormats());
    OCIO_CHECK_EQUAL("cinespace", std::string(bake->getFormatNameByIndex(4)));
    OCIO_CHECK_EQUAL("3dl", std::string(bake->getFormatExtensionByIndex(1)));
}

OCIO_ADD_TEST(Baker, baking_config)
{
    OCIO::BakerRcPtr bake;
    std::ostringstream os;

    constexpr auto myProfile = R"(
        ocio_profile_version: 2

        strictparsing: false

        roles:
          scene_linear: Raw

        file_rules:
          - !<Rule> {name: Default, colorspace: Raw}

        shared_views:
          - !<View> {name: Raw, colorspace: Raw}
          - !<View> {name: RawInactive, colorspace: Raw}

        displays:
          sRGB:
            - !<Views> [Raw, RawInactive]
            - !<View> {name: Film, colorspace: sRGB}
            - !<View> {name: FilmInactive, colorspace: sRGB}
          sRGBInactive:
            - !<Views> [Raw, RawInactive]
            - !<View> {name: Film, colorspace: sRGB}
            - !<View> {name: FilmInactive, colorspace: sRGB}

        active_displays: [sRGB]
        active_views: [Film, Raw]

        colorspaces:
        - !<ColorSpace>
          name : Raw
          isdata : false

        - !<ColorSpace>
          name : RawInactive
          isdata : false

        - !<ColorSpace>
          name : Log
          isdata : false
          to_reference: !<LogTransform> {}

        - !<ColorSpace>
          name : Crosstalk
          isdata : false
          to_reference: !<CDLTransform> {sat: 0.5}

        - !<ColorSpace>
          name : sRGB
          isdata : false
          from_reference: !<GroupTransform>
            children:
              - !<MatrixTransform> {matrix: [3.2409, -1.5373, -0.4986, 0, -0.9692, 1.8759, 0.0415, 0, 0.0556, -0.2039, 1.0569, 0, 0, 0, 0, 1 ]}
              - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

        - !<ColorSpace>
          name : Gamma22
          isdata : false
          from_reference : !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1], direction: inverse}

        inactive_colorspaces: [RawInactive]
    )";

    std::istringstream is(myProfile);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_ASSERT(config);
    OCIO_CHECK_NO_THROW(config->validate());

    // Missing configuration.
    bake = OCIO::Baker::Create();
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "No OCIO config has been set.");

    // Missing input space
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setTargetSpace("Gamma22");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "No input space has been set.");

    // Missing target space and display / view.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "No display / view or target colorspace has been set.");

    // Setting both target space and display / view.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("Gamma22");
    bake->setDisplayView("sRGB", "Film");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "Cannot use both display / view and target colorspace.");

    // Setting looks with display / view.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("sRGB", "Film");
    bake->setLooks("foo, +bar");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "Cannot use looks with display / view.");

    // Invalid input space.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Invalid");
    bake->setDisplayView("sRGB", "Film");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "Could not find input colorspace 'Invalid'.");

    // Inactive input space.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("RawInactive");
    bake->setDisplayView("sRGB", "Film");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Invalid target space.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("Invalid");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "Could not find target colorspace 'Invalid'.");

    // Invalid display.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("Invalid", "Film");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "Could not find display 'Invalid'.");

    // Invalid view.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("sRGB", "Invalid");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "Could not find view 'Invalid'.");

    // Inactive display.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("sRGBInactive", "Film");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Shared view.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("sRGB", "Raw");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Inactive view.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("sRGB", "FilmInactive");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Inactive shared view.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("sRGBInactive", "RawInactive");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Baking 1D LUT with Crosstalk.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setDisplayView("sRGB", "Film");
    bake->setFormat("spi1d");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "The format 'spi1d' does not support transformations with channel crosstalk.");

    // Cube Size < 2.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("sRGB");
    bake->setCubeSize(1);
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "Cube size must be at least 2 if set.");

    // Shaper Size < 2.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("sRGB");
    bake->setShaperSpace("Log");
    bake->setShaperSize(1);
    bake->setFormat("resolve_cube");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "A shaper space 'Log' has been specified, so the shaper size must be 2 or larger.");

    // Using shaper with unsupported format.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("sRGB");
    bake->setShaperSpace("Log");
    bake->setFormat("iridas_itx");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "The format 'iridas_itx' does not support shaper space.");

    // Using shaper space with Crosstalk.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("sRGB");
    bake->setShaperSpace("Crosstalk");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "The specified shaper space, 'Crosstalk' has channel crosstalk, which is not appropriate for shapers. Please select an alternate shaper space or omit this option.");
}
