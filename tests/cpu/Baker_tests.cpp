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
    constexpr const char * myProfile =
        "ocio_profile_version: 2\n"
        "\n"
        "file_rules:\n"
        "  - !<Rule> {name: Default, colorspace: lnh}\n"
        "\n"
        "displays:\n"
        "  display1:\n"
        "    - !<View> {name: view1, colorspace: gamma22}\n"
        "    - !<View> {name: view2, looks: satlook, colorspace: gamma22}\n"
        "\n"
        "looks:\n"
        "  - !<Look>\n"
        "    name : contrastlook\n"
        "    process_space : lnh\n"
        "    transform : !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}\n"
        "  - !<Look>\n"
        "    name : satlook\n"
        "    process_space : lnh\n"
        "    transform : !<CDLTransform> {sat: 2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name : lnh\n"
        "    bitdepth : 16f\n"
        "    isdata : false\n"
        "    allocation : lg2\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name : gamma22\n"
        "    bitdepth : 8ui\n"
        "    isdata : false\n"
        "    allocation : uniform\n"
        "    to_reference : !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}\n"
        "\n"
        "named_transforms:\n"
        "- !<NamedTransform>\n"
        "  name: logcnt\n"
        "  transform: !<LogCameraTransform>\n"
        "    log_side_slope:  0.247189638318671\n"
        "    log_side_offset: 0.385536998692443\n"
        "    lin_side_slope:  5.55555555555556\n"
        "    lin_side_offset: 0.0522722750251688\n"
        "    lin_side_break:  0.0105909904954696\n"
        "    base: 10\n"
        "    direction: inverse\n"
        "\n";

    constexpr const char * expectedLut =
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
    OCIO_CHECK_NO_THROW(config->validate());
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 2);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(), 1);

    {
        OCIO::BakerRcPtr bake = OCIO::Baker::Create();
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
        bake->setTargetSpace("gamma22");
        OCIO_CHECK_EQUAL("gamma22", std::string(bake->getTargetSpace()));
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

    {
        OCIO::BakerRcPtr bake = OCIO::Baker::Create();
        bake->setConfig(config);
        bake->setFormat("resolve_cube");
        bake->setInputSpace("lnh");
        bake->setLooks("contrastlook");
        bake->setDisplayView("display1", "view1");
        bake->setCubeSize(10);
        std::ostringstream os;
        OCIO_CHECK_NO_THROW(bake->bake(os));

        const std::string expectedCube{
R"(LUT_1D_SIZE 10
0.000000 0.000000 0.000000
0.111111 0.111111 0.111111
0.222222 0.222222 0.222222
0.333333 0.333333 0.333333
0.444444 0.444444 0.444444
0.555556 0.555556 0.555556
0.666667 0.666667 0.666667
0.777778 0.777778 0.777778
0.888889 0.888889 0.888889
1.000000 1.000000 1.000000
)"};

        OCIO_CHECK_EQUAL(expectedCube.size(), os.str().size());
        OCIO_CHECK_EQUAL(expectedCube, os.str());
    }

    {
        OCIO::BakerRcPtr bake = OCIO::Baker::Create();
        bake->setConfig(config);
        bake->setFormat("resolve_cube");
        bake->setInputSpace("lnh");
        bake->setShaperSpace("logcnt");
        bake->setDisplayView("display1", "view2");
        bake->setShaperSize(10);
        bake->setCubeSize(2);
        std::ostringstream os;
        OCIO_CHECK_NO_THROW(bake->bake(os));

        const std::string expectedCube{
R"(LUT_1D_SIZE 10
LUT_1D_INPUT_RANGE -0.017290 55.080036
LUT_3D_SIZE 2
0.000000 0.000000 0.000000
0.763998 0.763998 0.763998
0.838479 0.838479 0.838479
0.882030 0.882030 0.882030
0.912925 0.912925 0.912925
0.936887 0.936887 0.936887
0.956464 0.956464 0.956464
0.973016 0.973016 0.973016
0.987354 0.987354 0.987354
1.000000 1.000000 1.000000
0.000000 0.000000 0.000000
8.054426 0.000000 0.000000
0.000000 6.931791 0.000000
6.384501 6.384501 0.000000
0.000000 0.000000 8.336130
7.904850 0.000000 7.904850
0.000000 6.751890 6.751890
6.185304 6.185304 6.185304
)"};

        OCIO_CHECK_EQUAL(expectedCube.size(), os.str().size());
        OCIO_CHECK_EQUAL(expectedCube, os.str());
    }
}

OCIO_ADD_TEST(Baker, baking_validation)
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

        looks:
        - !<Look>
          name : foo
          process_space : Raw
          transform : !<CDLTransform> {sat: 2}

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
          name : Saturation
          isdata : false
          to_reference: !<CDLTransform> {sat: 0.5}

        - !<ColorSpace>
          name : Log2sRGB
          isdata : false
          to_reference: !<GroupTransform>
            children:
              - !<LogTransform> {base: 2, direction: inverse}
              - !<MatrixTransform> {matrix: [3.2409, -1.5373, -0.4986, 0, -0.9692, 1.8759, 0.0415, 0, 0.0556, -0.2039, 1.0569, 0, 0, 0, 0, 1 ], direction: inverse}

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

        named_transforms:

        - !<NamedTransform>
          name: Log2NT
          transform: !<LogTransform> {base: 2}

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
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "No display / view or target colorspace has been set.");

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
    bake->setLooks("foo");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Invalid input space.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Invalid");
    bake->setDisplayView("sRGB", "Film");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "Could not find input colorspace 'Invalid'.");

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
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "Could not find target colorspace 'Invalid'.");

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
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "The format 'spi1d' does not support transformations with channel crosstalk.");

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
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "A shaper space 'Log' has been specified, so the shaper size must be 2 or larger.");

    // Using shaper with unsupported format.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("sRGB");
    bake->setShaperSpace("Log");
    bake->setFormat("iridas_itx");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "The format 'iridas_itx' does not support shaper space.");

    // Using shaper space with Crosstalk.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Raw");
    bake->setTargetSpace("sRGB");
    bake->setShaperSpace("Saturation");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "The specified shaper space, 'Saturation' has channel crosstalk, which "
        "is not appropriate for shapers. Please select an alternate shaper "
        "space or omit this option.");

    // Using shaper space without Crosstalk (after optimization).
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("sRGB");
    bake->setTargetSpace("Raw");
    bake->setShaperSpace("Log2sRGB");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Using NamedTransform as shaper space.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("sRGB");
    bake->setTargetSpace("Raw");
    bake->setShaperSpace("Log2NT");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_NO_THROW(bake->bake(os));

    // Using NamedTransform as input space is not supported.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("Log2NT");
    bake->setTargetSpace("sRGB");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "Could not find input colorspace 'Log2NT'.");

    // Using NamedTransform as target space is not supported.
    bake = OCIO::Baker::Create();
    bake->setConfig(config);
    bake->setInputSpace("sRGB");
    bake->setTargetSpace("Log2NT");
    bake->setFormat("cinespace");

    os.str("");
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception,
        "Could not find target colorspace 'Log2NT'.");
}
