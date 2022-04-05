// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatHDL.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatHDL, read_1d)
{
    std::ostringstream strebuf;
    strebuf << "Version\t\t1" << "\n";
    strebuf << "Format\t\tany" << "\n";
    strebuf << "Type\t\tC" << "\n";
    strebuf << "From\t\t0.1 3.2" << "\n";
    strebuf << "To\t\t0 1" << "\n";
    strebuf << "Black\t\t0" << "\n";
    strebuf << "White\t\t0.99" << "\n";
    strebuf << "Length\t\t9" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << "RGB {" << "\n";
    strebuf << "\t0" << "\n";
    strebuf << "\t0.000977517" << "\n";
    strebuf << "\t0.00195503" << "\n";
    strebuf << "\t0.00293255" << "\n";
    strebuf << "\t0.00391007" << "\n";
    strebuf << "\t0.00488759" << "\n";
    strebuf << "\t0.0058651" << "\n";
    strebuf << "\t0.999022" << "\n";
    strebuf << "\t1.67 }" << "\n";

    //
    float from_min = 0.1f;
    float from_max = 3.2f;
    float to_min = 0.0f;
    float to_max = 1.0f;
    float black = 0.0f;
    float white = 0.99f;
    float lut1d[9] = { 0.0f, 0.000977517f, 0.00195503f, 0.00293255f,
        0.00391007f, 0.00488759f, 0.0058651f, 0.999022f, 1.67f };

    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.read(simple3D1D, emptyString, OCIO::INTERP_DEFAULT);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_REQUIRE_ASSERT(lut->lut1D);

    OCIO_CHECK_EQUAL(lut->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    //
    OCIO_CHECK_EQUAL(to_min, lut->to_min);
    OCIO_CHECK_EQUAL(to_max, lut->to_max);
    OCIO_CHECK_EQUAL(black, lut->hdlblack);
    OCIO_CHECK_EQUAL(white, lut->hdlwhite);

    // Check 1D data
    OCIO_CHECK_EQUAL(from_min, lut->from_min);
    OCIO_CHECK_EQUAL(from_max, lut->from_max);

    const auto & lutArray = lut->lut1D->getArray();
    OCIO_CHECK_EQUAL(9, lutArray.getLength());

    for(unsigned int i = 0; i < lutArray.getLength(); ++i)
    {
        OCIO_CHECK_EQUAL(lut1d[i], lutArray[i * 3 + 0]);
        OCIO_CHECK_EQUAL(lut1d[i], lutArray[i * 3 + 1]);
        OCIO_CHECK_EQUAL(lut1d[i], lutArray[i * 3 + 2]);
    }
}

OCIO_ADD_TEST(FileFormatHDL, bake_1d)
{

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }

    // Add target space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();

        double rgb[3] = {0.1, 0.1, 0.1};
        transform1->setOffset(rgb);

        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    std::string bout =
    "Version\t\t1\n"
    "Format\t\tany\n"
    "Type\t\tRGB\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t10\n"
    "LUT:\n"
    "R {\n"
    "\t0.100000\n"
    "\t0.211111\n"
    "\t0.322222\n"
    "\t0.433333\n"
    "\t0.544444\n"
    "\t0.655556\n"
    "\t0.766667\n"
    "\t0.877778\n"
    "\t0.988889\n"
    "\t1.100000\n"
    " }\n"
    "G {\n"
    "\t0.100000\n"
    "\t0.211111\n"
    "\t0.322222\n"
    "\t0.433333\n"
    "\t0.544444\n"
    "\t0.655556\n"
    "\t0.766667\n"
    "\t0.877778\n"
    "\t0.988889\n"
    "\t1.100000\n"
    " }\n"
    "B {\n"
    "\t0.100000\n"
    "\t0.211111\n"
    "\t0.322222\n"
    "\t0.433333\n"
    "\t0.544444\n"
    "\t0.655556\n"
    "\t0.766667\n"
    "\t0.877778\n"
    "\t0.988889\n"
    "\t1.100000\n"
    " }\n";

    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setCubeSize(10); // FIXME: Misusing the cube size to set the 1D LUT size
    std::ostringstream output;
    baker->bake(output);

    //std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    //std::cerr << "Expected:" << std::endl << bout << std::endl;

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout);
    OCIO_REQUIRE_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
    {
        OCIO_CHECK_EQUAL(StringUtils::Trim(osvec[i]), StringUtils::Trim(resvec[i]));
    }
}


OCIO_ADD_TEST(FileFormatHDL, bake_1d_shaper)
{
    OCIO::BakerRcPtr bake;
    std::ostringstream os;

    constexpr auto myProfile = R"(
        ocio_profile_version: 1

        colorspaces:
        - !<ColorSpace>
          name : Raw
          isdata : false

        - !<ColorSpace>
          name: Log2
          isdata: false
          from_reference: !<GroupTransform>
            children:
              - !<MatrixTransform> {matrix: [5.55556, 0, 0, 0, 0, 5.55556, 0, 0, 0, 0, 5.55556, 0, 0, 0, 0, 1]}
              - !<LogTransform> {base: 2}
              - !<MatrixTransform> {offset: [6.5, 6.5, 6.5, 0]}
              - !<MatrixTransform> {matrix: [0.076923, 0, 0, 0, 0, 0.076923, 0, 0, 0, 0, 0.076923, 0, 0, 0, 0, 1]}
    )";

    std::istringstream is(myProfile);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_ASSERT(config);

    {
        // Lin to Log
        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat("houdini");
        baker->setInputSpace("Raw");
        baker->setTargetSpace("Log2");
        baker->setShaperSpace("Log2");
        baker->setCubeSize(10);
        std::ostringstream outputHDL;
        baker->bake(outputHDL);

        const std::string expectedHDL =
        "Version\t\t1\n"
        "Format\t\tany\n"
        "Type\t\tRGB\n"
        "From\t\t0.001989 16.291878\n"
        "To\t\t0.000000 1.000000\n"
        "Black\t\t0.000000\n"
        "White\t\t1.000000\n"
        "Length\t\t10\n"
        "LUT:\n"
        "R {\n"
        "\t0.000000\n"
        "\t0.756268\n"
        "\t0.833130\n"
        "\t0.878107\n"
        "\t0.910023\n"
        "\t0.934780\n"
        "\t0.955010\n"
        "\t0.972114\n"
        "\t0.986931\n"
        "\t1.000000\n"
        "}\n"
        "G {\n"
        "\t0.000000\n"
        "\t0.756268\n"
        "\t0.833130\n"
        "\t0.878107\n"
        "\t0.910023\n"
        "\t0.934780\n"
        "\t0.955010\n"
        "\t0.972114\n"
        "\t0.986931\n"
        "\t1.000000\n"
        "}\n"
        "B {\n"
        "\t0.000000\n"
        "\t0.756268\n"
        "\t0.833130\n"
        "\t0.878107\n"
        "\t0.910023\n"
        "\t0.934780\n"
        "\t0.955010\n"
        "\t0.972114\n"
        "\t0.986931\n"
        "\t1.000000\n"
        "}\n";

        OCIO_CHECK_EQUAL(expectedHDL.size(), outputHDL.str().size());
        OCIO_CHECK_EQUAL(expectedHDL, outputHDL.str());
    }

    {
        // Log to Lin
        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat("houdini");
        baker->setInputSpace("Log2");
        baker->setTargetSpace("Raw");
        baker->setCubeSize(10);
        std::ostringstream outputHDL;
        baker->bake(outputHDL);

        const std::string expectedHDL =
        "Version\t\t1\n"
        "Format\t\tany\n"
        "Type\t\tRGB\n"
        "From\t\t0.000000 1.000000\n"
        "To\t\t0.000000 1.000000\n"
        "Black\t\t0.000000\n"
        "White\t\t1.000000\n"
        "Length\t\t10\n"
        "LUT:\n"
        "R {\n"
        "\t0.001989\n"
        "\t0.005413\n"
        "\t0.014731\n"
        "\t0.040091\n"
        "\t0.109110\n"
        "\t0.296951\n"
        "\t0.808177\n"
        "\t2.199522\n"
        "\t5.986179\n"
        "\t16.291878\n"
        "}\n"
        "G {\n"
        "\t0.001989\n"
        "\t0.005413\n"
        "\t0.014731\n"
        "\t0.040091\n"
        "\t0.109110\n"
        "\t0.296951\n"
        "\t0.808177\n"
        "\t2.199522\n"
        "\t5.986179\n"
        "\t16.291878\n"
        "}\n"
        "B {\n"
        "\t0.001989\n"
        "\t0.005413\n"
        "\t0.014731\n"
        "\t0.040091\n"
        "\t0.109110\n"
        "\t0.296951\n"
        "\t0.808177\n"
        "\t2.199522\n"
        "\t5.986179\n"
        "\t16.291878\n"
        "}\n";

        OCIO_CHECK_EQUAL(expectedHDL.size(), outputHDL.str().size());
        OCIO_CHECK_EQUAL(expectedHDL, outputHDL.str());
    }
}

OCIO_ADD_TEST(FileFormatHDL, read_3d)
{
    std::ostringstream strebuf;
    strebuf << "Version         2" << "\n";
    strebuf << "Format      any" << "\n";
    strebuf << "Type        3D" << "\n";
    strebuf << "From        0.2 0.9" << "\n";
    strebuf << "To      0.001 0.999" << "\n";
    strebuf << "Black       0.002" << "\n";
    strebuf << "White       0.98" << "\n";
    strebuf << "Length      2" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << " {" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0.390735 2.68116e-28" << "\n";
    strebuf << " 0 0.390735 0" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0 0.599397" << "\n";
    strebuf << " 0 0.601016 0" << "\n";
    strebuf << " 0 0.601016 0.917034" << "\n";
    strebuf << " }" << "\n";

    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());
    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.read(simple3D1D, emptyString, OCIO::INTERP_DEFAULT);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_REQUIRE_ASSERT(lut->lut3D);

    // from_min & from_max are only stored when there is a 1D LUT.
    float to_min = 0.001f;
    float to_max = 0.999f;
    float black = 0.002f;
    float white = 0.98f;
    float cube[2 * 2 * 2 * 3 ] = {
        0.f, 0.f, 0.f,
        0.f, 0.f, 0.f,
        0.f, 0.390735f, 2.68116e-28f,
        0.f, 0.390735f, 0.f,
        0.f, 0.f, 0.f,
        0.f, 0.f, 0.599397f,
        0.f, 0.601016f, 0.f,
        0.f, 0.601016f, 0.917034f };

    //
    OCIO_CHECK_EQUAL(to_min, lut->to_min);
    OCIO_CHECK_EQUAL(to_max, lut->to_max);
    OCIO_CHECK_EQUAL(black, lut->hdlblack);
    OCIO_CHECK_EQUAL(white, lut->hdlwhite);

    // Check cube data
    const auto & lutArray = lut->lut3D->getArray();
    const unsigned long lutSize = lutArray.getLength();
    OCIO_CHECK_EQUAL(2, lutSize);

    for (unsigned long b = 0; b < lutSize; ++b)
    {
        for (unsigned long g = 0; g < lutSize; ++g)
        {
            for (unsigned long r = 0; r < lutSize; ++r)
            {
                // OpData::Lut3D Array index: blue changes fastest.
                const unsigned long arrayIdx = 3 * ((r*lutSize + g)*lutSize + b);

                // Houdini order, red changes fastest.
                const unsigned long ocioIdx = 3 * ((b*lutSize + g)*lutSize + r);

                OCIO_CHECK_EQUAL(lutArray[arrayIdx], cube[ocioIdx]);
                OCIO_CHECK_EQUAL(lutArray[arrayIdx + 1], cube[ocioIdx + 1]);
                OCIO_CHECK_EQUAL(lutArray[arrayIdx + 2], cube[ocioIdx + 2]);
            }
        }
    }
}

OCIO_ADD_TEST(FileFormatHDL, bake_3d)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // Set luma coef's to simple values
    {
        double lumaCoef[3] = {0.333, 0.333, 0.333};
        config->setDefaultLumaCoefs(lumaCoef);
    }

    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }

    // Add target space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();

        // Set saturation to cause channel crosstalk, making a 3D LUT
        transform1->setSat(0.5f);

        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    std::string bout =
        "Version\t\t2\n"
        "Format\t\tany\n"
        "Type\t\t3D\n"
        "From\t\t0.000000 1.000000\n"
        "To\t\t0.000000 1.000000\n"
        "Black\t\t0.000000\n"
        "White\t\t1.000000\n"
        "Length\t\t2\n"
        "LUT:\n"
        " {\n"
        "\t0.000000 0.000000 0.000000\n"
        "\t0.606300 0.106300 0.106300\n"
        "\t0.357600 0.857600 0.357600\n"
        "\t0.963900 0.963900 0.463900\n"
        "\t0.036100 0.036100 0.536100\n"
        "\t0.642400 0.142400 0.642400\n"
        "\t0.393700 0.893700 0.893700\n"
        "\t1.000000 1.000000 1.000000\n"
        " }\n";

    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    //std::cerr << "Expected:" << std::endl << bout << std::endl;

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout);
    OCIO_REQUIRE_EQUAL(osvec.size(), resvec.size());
    for (unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
    {
        OCIO_CHECK_EQUAL(StringUtils::Trim(osvec[i]), StringUtils::Trim(resvec[i]));
    }
}

OCIO_ADD_TEST(FileFormatHDL, read_3d_1d)
{
    std::ostringstream strebuf;
    strebuf << "Version         3" << "\n";
    strebuf << "Format      any" << "\n";
    strebuf << "Type        3D+1D" << "\n";
    strebuf << "From        0.005478 14.080103" << "\n";
    strebuf << "To      0 1" << "\n";
    strebuf << "Black       0" << "\n";
    strebuf << "White       1" << "\n";
    strebuf << "Length      2 10" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << "Pre {" << "\n";
    strebuf << "    0.994922" << "\n";
    strebuf << "    0.995052" << "\n";
    strebuf << "    0.995181" << "\n";
    strebuf << "    0.995310" << "\n";
    strebuf << "    0.995439" << "\n";
    strebuf << "    0.995568" << "\n";
    strebuf << "    0.995697" << "\n";
    strebuf << "    0.995826" << "\n";
    strebuf << "    0.995954" << "\n";
    strebuf << "    0.996082" << "\n";
    strebuf << "}" << "\n";
    strebuf << "3D {" << "\n";
    strebuf << "    0.093776 0.093776 0.093776" << "\n";
    strebuf << "    0.105219 0.093776 0.093776" << "\n";
    strebuf << "    0.118058 0.093776 0.093776" << "\n";
    strebuf << "    0.132463 0.093776 0.093776" << "\n";
    strebuf << "    0.148626 0.093776 0.093776" << "\n";
    strebuf << "    0.166761 0.093776 0.093776" << "\n";
    strebuf << "    0.187109 0.093776 0.093776" << "\n";
    strebuf << "    0.209939 0.093776 0.093776" << "\n";
    strebuf << "}" << "\n";

    //
    float from_min = 0.005478f;
    float from_max = 14.080103f;
    float to_min = 0.0f;
    float to_max = 1.0f;
    float black = 0.0f;
    float white = 1.0f;
    float prelut[10] = { 0.994922f, 0.995052f, 0.995181f, 0.995310f, 0.995439f,
        0.995568f, 0.995697f, 0.995826f, 0.995954f, 0.996082f };
    float cube[2 * 2 * 2 * 3 ] = {
        0.093776f, 0.093776f, 0.093776f,
        0.105219f, 0.093776f, 0.093776f,
        0.118058f, 0.093776f, 0.093776f,
        0.132463f, 0.093776f, 0.093776f,
        0.148626f, 0.093776f, 0.093776f,
        0.166761f, 0.093776f, 0.093776f,
        0.187109f, 0.093776f, 0.093776f,
        0.209939f, 0.093776f, 0.093776f };

    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.read(simple3D1D, emptyString, OCIO::INTERP_DEFAULT);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_REQUIRE_ASSERT(lut->lut1D);
    OCIO_REQUIRE_ASSERT(lut->lut3D);

    OCIO_CHECK_EQUAL(lut->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    //
    OCIO_CHECK_EQUAL(to_min, lut->to_min);
    OCIO_CHECK_EQUAL(to_max, lut->to_max);
    OCIO_CHECK_EQUAL(black, lut->hdlblack);
    OCIO_CHECK_EQUAL(white, lut->hdlwhite);

    // Check prelut data
    OCIO_CHECK_EQUAL(from_min, lut->from_min);
    OCIO_CHECK_EQUAL(from_max, lut->from_max);
    const auto & preLutArray = lut->lut1D->getArray();
    OCIO_CHECK_EQUAL(10, preLutArray.getLength());

    for(unsigned int i = 0; i < preLutArray.getLength(); ++i)
    {
        OCIO_CHECK_EQUAL(prelut[i], preLutArray[3 * i + 0]);
        OCIO_CHECK_EQUAL(prelut[i], preLutArray[3 * i + 1]);
        OCIO_CHECK_EQUAL(prelut[i], preLutArray[3 * i + 2]);
    }

    // check cube data
    const auto & lutArray = lut->lut3D->getArray();
    const unsigned long lutSize = lutArray.getLength();
    OCIO_CHECK_EQUAL(2, lutSize);

    for (unsigned long b = 0; b < lutSize; ++b)
    {
        for (unsigned long g = 0; g < lutSize; ++g)
        {
            for (unsigned long r = 0; r < lutSize; ++r)
            {
                // OpData::Lut3D Array index: blue changes fastest.
                const unsigned long arrayIdx = 3 * ((r*lutSize + g)*lutSize + b);

                // Houdini order, red changes fastest.
                const unsigned long ocioIdx = 3 * ((b*lutSize + g)*lutSize + r);

                OCIO_CHECK_EQUAL(lutArray[arrayIdx], cube[ocioIdx]);
                OCIO_CHECK_EQUAL(lutArray[arrayIdx + 1], cube[ocioIdx + 1]);
                OCIO_CHECK_EQUAL(lutArray[arrayIdx + 2], cube[ocioIdx + 2]);
            }
        }
    }
}

OCIO_ADD_TEST(FileFormatHDL, bake_3d_1d)
{
    // check baker output
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // Set luma coef's to simple values
    {
        double lumaCoef[3] = {0.333, 0.333, 0.333};
        config->setDefaultLumaCoefs(lumaCoef);
    }

    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }

    // Add shaper space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("shaper");
        cs->setFamily("shaper");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        double test[4] = {2.6, 2.6, 2.6, 1.0};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }

    // Add target space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();

        // Set saturation to cause channel crosstalk, making a 3D LUT
        transform1->setSat(0.5);

        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    std::string bout = 
    "Version\t\t3\n"
    "Format\t\tany\n"
    "Type\t\t3D+1D\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t2 10\n"
    "LUT:\n"
    "Pre {\n"
    "\t0.000000\n"
    "\t0.429520\n"
    "\t0.560744\n"
    "\t0.655378\n"
    "\t0.732057\n"
    "\t0.797661\n"
    "\t0.855604\n"
    "\t0.907865\n"
    "\t0.955710\n"
    "\t1.000000\n"
    "}\n"
    "3D {\n"
    "\t0.000000 0.000000 0.000000\n"
    "\t0.606300 0.106300 0.106300\n"
    "\t0.357600 0.857600 0.357600\n"
    "\t0.963900 0.963900 0.463900\n"
    "\t0.036100 0.036100 0.536100\n"
    "\t0.642400 0.142400 0.642400\n"
    "\t0.393700 0.893700 0.893700\n"
    "\t1.000000 1.000000 1.000000\n"
    "}\n";

    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    //std::cerr << "Expected:" << std::endl << bout << std::endl;

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout);
    OCIO_REQUIRE_EQUAL(osvec.size(), resvec.size());

    // TODO: Get this working on osx
    /*
    for(unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
        OCIO_CHECK_EQUAL(StringUtils::Trim(osvec[i]), StringUtils::Trim(resvec[i]));
    */
}


OCIO_ADD_TEST(FileFormatHDL, look_test)
{
    // Note this sets up a Look with the same parameters as the Bake3D1D test
    // however it uses a different shaper space, to ensure we catch that case.
    // Also ensure we detect the effects of the desaturation by using a 3 cubed
    // LUT, which will thus test colour values other than the corner points of
    // the cube

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }

    // Add shaper space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("shaper");
        cs->setFamily("shaper");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        double test[4] = {2.2, 2.2, 2.2, 1.0};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }

    // Add Look process space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("look_process");
        cs->setFamily("look_process");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        double test[4] = {2.6, 2.6, 2.6, 1.0};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }

    // Add Look process space
    {
        OCIO::LookRcPtr look = OCIO::Look::Create();
        look->setName("look");
        look->setProcessSpace("look_process");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();

        // Set saturation to cause channel crosstalk, making a 3D LUT
        transform1->setSat(0.5f);

        look->setTransform(transform1);
        config->addLook(look);
    }


    std::string bout =
        "Version\t\t3\n"
        "Format\t\tany\n"
        "Type\t\t3D+1D\n"
        "From\t\t0.000000 1.000000\n"
        "To\t\t0.000000 1.000000\n"
        "Black\t\t0.000000\n"
        "White\t\t1.000000\n"
        "Length\t\t3 10\n"
        "LUT:\n"
        "Pre {\n"
        "\t0.000000\n"
        "\t0.368344\n"
        "\t0.504760\n"
        "\t0.606913\n"
        "\t0.691699\n"
        "\t0.765539\n"
        "\t0.831684\n"
        "\t0.892049\n"
        "\t0.947870\n"
        "\t1.000000\n"
        "}\n"
        "3D {\n"
        "\t0.000000 0.000000 0.000000\n"
        "\t0.276787 0.035360 0.035360\n"
        "\t0.553575 0.070720 0.070720\n"
        "\t0.148309 0.416989 0.148309\n"
        "\t0.478739 0.478739 0.201718\n"
        "\t0.774120 0.528900 0.245984\n"
        "\t0.296618 0.833978 0.296618\n"
        "\t0.650361 0.902354 0.355417\n"
        "\t0.957478 0.957478 0.403436\n"
        "\t0.009867 0.009867 0.239325\n"
        "\t0.296368 0.049954 0.296368\n"
        "\t0.575308 0.086766 0.343137\n"
        "\t0.166161 0.437812 0.437812\n"
        "\t0.500000 0.500000 0.500000\n"
        "\t0.796987 0.550484 0.550484\n"
        "\t0.316402 0.857106 0.607391\n"
        "\t0.672631 0.925760 0.672631\n"
        "\t0.981096 0.981096 0.725386\n"
        "\t0.019735 0.019735 0.478650\n"
        "\t0.312132 0.062101 0.541651\n"
        "\t0.592736 0.099909 0.592736\n"
        "\t0.180618 0.454533 0.695009\n"
        "\t0.517061 0.517061 0.761560\n"
        "\t0.815301 0.567796 0.815301\n"
        "\t0.332322 0.875624 0.875624\n"
        "\t0.690478 0.944497 0.944497\n"
        "\t1.000000 1.000000 1.000000\n"
        "}\n";

    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("shaper");
    baker->setLooks("look");
    baker->setShaperSize(10);
    baker->setCubeSize(3);
    std::ostringstream output;
    baker->bake(output);

    //std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    //std::cerr << "Expected:" << std::endl << bout << std::endl;

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout);
    OCIO_REQUIRE_EQUAL(osvec.size(), resvec.size());

    for (unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
    {
        OCIO_CHECK_EQUAL(StringUtils::Trim(osvec[i]), StringUtils::Trim(resvec[i]));
    }
}
