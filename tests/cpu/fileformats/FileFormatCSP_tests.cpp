// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatCSP.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void compareFloats(const std::string& floats1, const std::string& floats2)
{
    // Number comparison.
    const StringUtils::StringVec strings1 = StringUtils::SplitByWhiteSpaces(StringUtils::Trim(floats1));
    std::vector<float> numbers1;
    OCIO::StringVecToFloatVec(numbers1, strings1);

    const StringUtils::StringVec strings2 = StringUtils::SplitByWhiteSpaces(StringUtils::Trim(floats2));
    std::vector<float> numbers2;
    OCIO::StringVecToFloatVec(numbers2, strings2);

    OCIO_CHECK_EQUAL(numbers1.size(), numbers2.size());
    for(unsigned int j=0; j<numbers1.size(); ++j)
    {
        OCIO_CHECK_CLOSE(numbers1[j], numbers2[j], 1e-5f);
    }
}
}

OCIO_ADD_TEST(FileFormatCSP, simple_1d)
{
    std::ostringstream strebuf;
    strebuf << "CSPLUTV100"              << "\n";
    strebuf << "1D"                      << "\n";
    strebuf << ""                        << "\n";
    strebuf << "BEGIN METADATA"          << "\n";
    strebuf << "foobar"                  << "\n";
    strebuf << "END METADATA"            << "\n";
    strebuf << ""                        << "\n";
    strebuf << "2"                       << "\n";
    strebuf << "0.0 1.0"                 << "\n";
    strebuf << "0.0 2.0"                 << "\n";
    strebuf << "6"                       << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
    strebuf << "0.0 0.4 0.8 1.2 1.6 2.0" << "\n";
    strebuf << "3"                       << "\n";
    strebuf << "0.0 0.1 1.0"             << "\n";
    strebuf << "0.0 0.2 2.0"             << "\n";
    strebuf << ""                        << "\n";
    strebuf << "6"                       << "\n";
    strebuf << "0.0 0.0 0.0"             << "\n";
    strebuf << "0.2 0.3 0.1"             << "\n";
    strebuf << "0.4 0.5 0.2"             << "\n";
    strebuf << "0.5 0.6 0.3"             << "\n";
    strebuf << "0.6 0.8 0.4"             << "\n";
    strebuf << "1.0 0.9 0.5"             << "\n";

    float red[6]   = { 0.0f, 0.2f, 0.4f, 0.5f, 0.6f, 1.0f };
    float green[6] = { 0.0f, 0.3f, 0.5f, 0.6f, 0.8f, 0.9f };
    float blue[6]  = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };

    std::istringstream simple1D;
    simple1D.str(strebuf.str());

    // Read file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.read(simple1D, emptyString, OCIO::INTERP_DEFAULT);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);

    // Check metadata.
    OCIO_CHECK_EQUAL(csplut->metadata, std::string("foobar\n"));

    // Check prelut data.
    OCIO_REQUIRE_ASSERT(csplut->prelut);
    OCIO_CHECK_EQUAL(csplut->prelut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::Array & prelutArray = csplut->prelut->getArray();

    // Check prelut data (note: the spline is resampled into a 1D LUT).
    const unsigned long length = prelutArray.getLength();
    for (unsigned int i = 0; i < length; i += 128)
    {
        float input = float(i) / float(length - 1);
        float output = prelutArray[i * 3];
        OCIO_CHECK_CLOSE(input*2.0f, output, 1e-4);
    }

    // Check 1D data.
    OCIO_REQUIRE_ASSERT(csplut->lut1D);
    OCIO_CHECK_EQUAL(csplut->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::Array & lutArray = csplut->lut1D->getArray();
    const unsigned long lutlength = lutArray.getLength();
    OCIO_REQUIRE_EQUAL(lutlength, 6);
    // Red.
    unsigned int i;
    for (i = 0; i < lutlength; ++i)
    {
        OCIO_CHECK_EQUAL(red[i], lutArray[i * 3]);
    }
    // Green.
    for (i = 0; i < lutlength; ++i)
    {
        OCIO_CHECK_EQUAL(green[i], lutArray[i * 3 + 1]);
    }
    // Blue.
    for (i = 0; i < lutlength; ++i)
    {
        OCIO_CHECK_EQUAL(blue[i], lutArray[i * 3 + 2]);
    }

    // Check 3D data.
    OCIO_CHECK_ASSERT(!csplut->lut3D);
}

OCIO_ADD_TEST(FileFormatCSP, simple_3d)
{
    std::ostringstream strebuf;
    strebuf << "CSPLUTV100"                                  << "\n";
    strebuf << "3D"                                          << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "BEGIN METADATA"                              << "\n";
    strebuf << "foobar"                                      << "\n";
    strebuf << "END METADATA"                                << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "11"                                          << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "6"                                           << "\n";
    strebuf << "0.0 0.2       0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "5"                                           << "\n";
    strebuf << "0.0 0.25       0.5 0.6 0.7"                  << "\n";
    strebuf << "0.0 0.25000001 0.5 0.6 0.7"                  << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "3 3 3"                                       << "\n";
    strebuf << "0.0 0.0 0.0"                                 << "\n";
    strebuf << "0.5 0.0 0.0"                                 << "\n";
    strebuf << "1.0 0.0 0.0"                                 << "\n";
    strebuf << "0.0 0.5 0.0"                                 << "\n";
    strebuf << "0.5 0.5 0.0"                                 << "\n";
    strebuf << "1.0 0.5 0.0"                                 << "\n";
    strebuf << "0.0 1.0 0.0"                                 << "\n";
    strebuf << "0.5 1.0 0.0"                                 << "\n";
    strebuf << "1.0 1.0 0.0"                                 << "\n";
    strebuf << "0.0 0.0 0.5"                                 << "\n";
    strebuf << "0.5 0.0 0.5"                                 << "\n";
    strebuf << "1.0 0.0 0.5"                                 << "\n";
    strebuf << "0.0 0.5 0.5"                                 << "\n";
    strebuf << "0.5 0.5 0.5"                                 << "\n";
    strebuf << "1.0 0.5 0.5"                                 << "\n";
    strebuf << "0.0 1.0 0.5"                                 << "\n";
    strebuf << "0.5 1.0 0.5"                                 << "\n";
    strebuf << "1.0 1.0 0.5"                                 << "\n";
    strebuf << "0.0 0.0 1.0"                                 << "\n";
    strebuf << "0.5 0.0 1.0"                                 << "\n";
    strebuf << "1.0 0.0 1.0"                                 << "\n";
    strebuf << "0.0 0.5 1.0"                                 << "\n";
    strebuf << "0.5 0.5 1.0"                                 << "\n";
    strebuf << "1.0 0.5 1.0"                                 << "\n";
    strebuf << "0.0 1.0 1.0"                                 << "\n";
    strebuf << "0.5 1.0 1.0"                                 << "\n";
    strebuf << "1.0 1.0 1.0"                                 << "\n";

    float cube[3 * 3 * 3 * 3] = { 0.0, 0.0, 0.0,
                                  0.0, 0.0, 0.5,
                                  0.0, 0.0, 1.0,
                                  0.0, 0.5, 0.0,
                                  0.0, 0.5, 0.5,
                                  0.0, 0.5, 1.0,
                                  0.0, 1.0, 0.0,
                                  0.0, 1.0, 0.5,
                                  0.0, 1.0, 1.0,
                                  0.5, 0.0, 0.0,
                                  0.5, 0.0, 0.5,
                                  0.5, 0.0, 1.0,
                                  0.5, 0.5, 0.0,
                                  0.5, 0.5, 0.5,
                                  0.5, 0.5, 1.0,
                                  0.5, 1.0, 0.0,
                                  0.5, 1.0, 0.5,
                                  0.5, 1.0, 1.0,
                                  1.0, 0.0, 0.0,
                                  1.0, 0.0, 0.5,
                                  1.0, 0.0, 1.0,
                                  1.0, 0.5, 0.0,
                                  1.0, 0.5, 0.5,
                                  1.0, 0.5, 1.0,
                                  1.0, 1.0, 0.0,
                                  1.0, 1.0, 0.5,
                                  1.0, 1.0, 1.0 };

    std::istringstream simple3D;
    simple3D.str(strebuf.str());

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.read(simple3D, emptyString, OCIO::INTERP_TETRAHEDRAL);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);

    // Check metadata.
    OCIO_CHECK_EQUAL(csplut->metadata, std::string("foobar\n"));

    // Check prelut data.
    OCIO_CHECK_ASSERT(!csplut->prelut); // As in & out preLut values are the same
                                        // there is nothing to do.

    // Check cube data.
    OCIO_REQUIRE_ASSERT(csplut->lut3D);
    OCIO_CHECK_EQUAL(csplut->lut3D->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    const OCIO::Array & lutArray = csplut->lut3D->getArray();
    const unsigned long lutlength = lutArray.getLength();

    for(unsigned int i = 0; i < lutlength; ++i)
    {
        OCIO_CHECK_EQUAL(cube[i], lutArray[i]);
    }

    // Check 1D data.
    OCIO_CHECK_ASSERT(!csplut->lut1D);
}

OCIO_ADD_TEST(FileFormatCSP, complete_3d)
{
    // Check baker output.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
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

    std::ostringstream bout;
    bout << "CSPLUTV100"                 << "\n";
    bout << "3D"                         << "\n";
    bout << ""                           << "\n";
    bout << "BEGIN METADATA"             << "\n";
    bout << "date: 2011:02:21 15:22:55"  << "\n";
    bout << "Baked by OCIO"              << "\n";
    bout << "END METADATA"               << "\n";
    bout << ""                           << "\n";
    bout << "10"                         << "\n";
    bout << "0.000000 0.003303 0.020028 0.057476 0.121430 0.216916 0.348468 0.520265 0.736213 1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "0.000000 0.003303 0.020028 0.057476 0.121430 0.216916 0.348468 0.520265 0.736213 1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "0.000000 0.003303 0.020028 0.057476 0.121430 0.216916 0.348468 0.520265 0.736213 1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << ""                           << "\n";
    bout << "2 2 2"                      << "\n";
    bout << "0.100000 0.100000 0.100000" << "\n";
    bout << "1.100000 0.100000 0.100000" << "\n";
    bout << "0.100000 1.100000 0.100000" << "\n";
    bout << "1.100000 1.100000 0.100000" << "\n";
    bout << "0.100000 0.100000 1.100000" << "\n";
    bout << "1.100000 0.100000 1.100000" << "\n";
    bout << "0.100000 1.100000 1.100000" << "\n";
    bout << "1.100000 1.100000 1.100000" << "\n";
    bout << ""                           << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "date: 2011:02:21 15:22:55");
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, 
                                               "Baked by OCIO");
    baker->setFormat("cinespace");
    baker->setInputSpace("lnf");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout.str());
    OCIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        if(i>6)
        {
            // Number comparison.
            compareFloats(osvec[i], resvec[i]);
        }
        else
        {
            // text comparison
            OCIO_CHECK_EQUAL(osvec[i], resvec[i]);
        }
    }
}

OCIO_ADD_TEST(FileFormatCSP, shaper_hdr)
{
    // Check baker output.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf_tweak");
        cs->setFamily("lnf_tweak");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        double rgb[3] = {2.0, -2.0, 0.9};
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
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

    std::ostringstream bout;
    bout << "CSPLUTV100"                 << "\n";
    bout << "3D"                         << "\n";
    bout << ""                           << "\n";
    bout << "BEGIN METADATA"             << "\n";
    bout << "date: 2011:02:21 15:22:55"  << "\n";
    bout << "END METADATA"               << "\n";
    bout << ""                           << "\n";
    bout << "10"                         << "\n";
    bout << "2.000000 2.111111 2.222222 2.333333 2.444444 2.555556 2.666667 2.777778 2.888889 3.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "-2.000000 -1.888889 -1.777778 -1.666667 -1.555556 -1.444444 -1.333333 -1.222222 -1.111111 -1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "0.900000 1.011111 1.122222 1.233333 1.344444 1.455556 1.566667 1.677778 1.788889 1.900000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << ""                           << "\n";
    bout << "2 2 2"                      << "\n";
    bout << "0.100000 0.100000 0.100000" << "\n";
    bout << "1.100000 0.100000 0.100000" << "\n";
    bout << "0.100000 1.100000 0.100000" << "\n";
    bout << "1.100000 1.100000 0.100000" << "\n";
    bout << "0.100000 0.100000 1.100000" << "\n";
    bout << "1.100000 0.100000 1.100000" << "\n";
    bout << "0.100000 1.100000 1.100000" << "\n";
    bout << "1.100000 1.100000 1.100000" << "\n";
    bout << ""                           << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "date: 2011:02:21 15:22:55");
    baker->setFormat("cinespace");
    baker->setInputSpace("lnf_tweak");
    baker->setShaperSpace("lnf");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout.str());
    OCIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        if(i>6)
        {
            // Number comparison.
            compareFloats(osvec[i], resvec[i]);
        }
        else
        {
            // text comparison
            OCIO_CHECK_EQUAL(osvec[i], resvec[i]);
        }
    }
}

OCIO_ADD_TEST(FileFormatCSP, no_shaper)
{
    // Check baker output.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
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

    std::ostringstream bout;
    bout << "CSPLUTV100"                 << "\n";
    bout << "3D"                         << "\n";
    bout << ""                           << "\n";
    bout << "BEGIN METADATA"             << "\n";
    bout << "date: 2011:02:21 15:22:55"  << "\n";
    bout << "END METADATA"               << "\n";
    bout << ""                           << "\n";
    bout << "2"                          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "2"                          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "2"                          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << ""                           << "\n";
    bout << "2 2 2"                      << "\n";
    bout << "0.100000 0.100000 0.100000" << "\n";
    bout << "1.100000 0.100000 0.100000" << "\n";
    bout << "0.100000 1.100000 0.100000" << "\n";
    bout << "1.100000 1.100000 0.100000" << "\n";
    bout << "0.100000 0.100000 1.100000" << "\n";
    bout << "1.100000 0.100000 1.100000" << "\n";
    bout << "0.100000 1.100000 1.100000" << "\n";
    bout << "1.100000 1.100000 1.100000" << "\n";
    bout << ""                           << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "date: 2011:02:21 15:22:55");
    baker->setFormat("cinespace");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout.str());

    OCIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        OCIO_CHECK_EQUAL(osvec[i], resvec[i]);
    }
}

OCIO_ADD_TEST(FileFormatCSP, less_strict_parse)
{
    std::ostringstream strebuf;
    strebuf << " CspluTV100 malformed"                       << "\n";
    strebuf << "3D"                                          << "\n";
    strebuf << ""                                            << "\n";
    strebuf << " BegIN MEtadATA malformed malformed malfo"   << "\n";
    strebuf << "foobar"                                      << "\n";
    strebuf << "   end metadata malformed malformed m a l"   << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "11"                                          << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "6"                                           << "\n";
    strebuf << "0.0 0.2       0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "5"                                           << "\n";
    strebuf << "0.0 0.25       0.5 0.6 0.7"                  << "\n";
    strebuf << "0.0 0.25000001 0.5 0.6 0.7"                  << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "2 2 2"                                       << "\n";
    strebuf << "0.100000 0.100000 0.100000"                  << "\n";
    strebuf << "1.100000 0.100000 0.100000"                  << "\n";
    strebuf << "0.100000 1.100000 0.100000"                  << "\n";
    strebuf << "1.100000 1.100000 0.100000"                  << "\n";
    strebuf << "0.100000 0.100000 1.100000"                  << "\n";
    strebuf << "1.100000 0.100000 1.100000"                  << "\n";
    strebuf << "0.100000 1.100000 1.100000"                  << "\n";
    strebuf << "1.100000 1.100000 1.100000"                  << "\n";

    std::istringstream simple3D;
    simple3D.str(strebuf.str());

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = tester.read(simple3D, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);   

    // Check metadata.
    OCIO_CHECK_EQUAL(csplut->metadata, std::string("foobar\n"));

    // Check prelut data.
    OCIO_CHECK_ASSERT(!csplut->prelut); // As in & out from the preLut are the same,
                                        // there is nothing to do.
}

OCIO_ADD_TEST(FileFormatCSP, failures_1d)
{
    {
        // Empty.
        std::istringstream lutStream;

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "file stream empty");
    }
    {
        // Wrong first line.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV2000" << "\n"; // Wrong.
        strebuf << "1D" << "\n";
        strebuf << "" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "expected 'CSPLUTV100'");
    }
    {
        // Missing LUT.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "Require 1D or 3D");
    }
    {
        // Can't read prelut size.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "1D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "A" << "\n";    // <------------ Wrong.
        strebuf << "0.0 1.0" << "\n";
        strebuf << "0.0 2.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.4 0.8 1.2 1.6 2.0" << "\n";
        strebuf << "3" << "\n";
        strebuf << "0.0 0.1 1.0" << "\n";
        strebuf << "0.0 0.2 2.0" << "\n";
        strebuf << "" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.2 0.3 0.1" << "\n";
        strebuf << "0.4 0.5 0.2" << "\n";
        strebuf << "0.5 0.6 0.3" << "\n";
        strebuf << "0.6 0.8 0.4" << "\n";
        strebuf << "1.0 0.9 0.5" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception,
                              "Prelut does not specify valid dimension size");
    }
    {
        // Prelut has too many points.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "1D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "2" << "\n";
        strebuf << "0.0 1.0 1.0" << "\n"; // <-------- Wrong.
        strebuf << "0.0 2.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.4 0.8 1.2 1.6 2.0" << "\n";
        strebuf << "3" << "\n";
        strebuf << "0.0 0.1 1.0" << "\n";
        strebuf << "0.0 0.2 2.0" << "\n";
        strebuf << "" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.2 0.3 0.1" << "\n";
        strebuf << "0.4 0.5 0.2" << "\n";
        strebuf << "0.5 0.6 0.3" << "\n";
        strebuf << "0.6 0.8 0.4" << "\n";
        strebuf << "1.0 0.9 0.5" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("File.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception,
                              "expected number of data points");
    }
    {
        // Can't read a float in prelut.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "1D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "2" << "\n";
        strebuf << "0.0 notFloat" << "\n";
        strebuf << "0.0 2.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.4 0.8 1.2 1.6 2.0" << "\n";
        strebuf << "3" << "\n";
        strebuf << "0.0 0.1 1.0" << "\n";
        strebuf << "0.0 0.2 2.0" << "\n";
        strebuf << "" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.2 0.3 0.1" << "\n";
        strebuf << "0.4 0.5 0.2" << "\n";
        strebuf << "0.5 0.6 0.3" << "\n";
        strebuf << "0.6 0.8 0.4" << "\n";
        strebuf << "1.0 0.9 0.5" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "Prelut data is malformed");
    }
    {
        // Bad number of LUT entries.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "1D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "2" << "\n";
        strebuf << "0.0 1.0" << "\n";
        strebuf << "0.0 2.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.4 0.8 1.2 1.6 2.0" << "\n";
        strebuf << "3" << "\n";
        strebuf << "0.0 0.1 1.0" << "\n";
        strebuf << "0.0 0.2 2.0" << "\n";
        strebuf << "" << "\n";
        strebuf << "-6" << "\n";       // <------------ Wrong.
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.2 0.3 0.1" << "\n";
        strebuf << "0.4 0.5 0.2" << "\n";
        strebuf << "0.5 0.6 0.3" << "\n";
        strebuf << "0.6 0.8 0.4" << "\n";
        strebuf << "1.0 0.9 0.5" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception,
                              "1D LUT with invalid number of entries");
    }
    {
        // Too many components on LUT entry.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "1D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "2" << "\n";
        strebuf << "0.0 1.0" << "\n";
        strebuf << "0.0 2.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.4 0.8 1.2 1.6 2.0" << "\n";
        strebuf << "3" << "\n";
        strebuf << "0.0 0.1 1.0" << "\n";
        strebuf << "0.0 0.2 2.0" << "\n";
        strebuf << "" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.0 0.0 0.0" << "\n"; // <------------ Wrong.
        strebuf << "0.2 0.3 0.1" << "\n";
        strebuf << "0.4 0.5 0.2" << "\n";
        strebuf << "0.5 0.6 0.3" << "\n";
        strebuf << "0.6 0.8 0.4" << "\n";
        strebuf << "1.0 0.9 0.5" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "must contain three numbers");
    }
}

OCIO_ADD_TEST(FileFormatCSP, failures_3d)
{
    {
        // Cube size has only 2 entries.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "3D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "11" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2       0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "5" << "\n";
        strebuf << "0.0 0.25       0.5 0.6 0.7" << "\n";
        strebuf << "0.0 0.25000001 0.5 0.6 0.7" << "\n";
        strebuf << "" << "\n";
        strebuf << "3 3" << "\n";   // <------------ Wrong.
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.5 0.0 0.0" << "\n";
        strebuf << "1.0 0.0 0.0" << "\n";
        strebuf << "0.0 0.5 0.0" << "\n";
        strebuf << "0.5 0.5 0.0" << "\n";
        strebuf << "1.0 0.5 0.0" << "\n";
        strebuf << "0.0 1.0 0.0" << "\n";
        strebuf << "0.5 1.0 0.0" << "\n";
        strebuf << "1.0 1.0 0.0" << "\n";
        strebuf << "0.0 0.0 0.5" << "\n";
        strebuf << "0.5 0.0 0.5" << "\n";
        strebuf << "1.0 0.0 0.5" << "\n";
        strebuf << "0.0 0.5 0.5" << "\n";
        strebuf << "0.5 0.5 0.5" << "\n";
        strebuf << "1.0 0.5 0.5" << "\n";
        strebuf << "0.0 1.0 0.5" << "\n";
        strebuf << "0.5 1.0 0.5" << "\n";
        strebuf << "1.0 1.0 0.5" << "\n";
        strebuf << "0.0 0.0 1.0" << "\n";
        strebuf << "0.5 0.0 1.0" << "\n";
        strebuf << "1.0 0.0 1.0" << "\n";
        strebuf << "0.0 0.5 1.0" << "\n";
        strebuf << "0.5 0.5 1.0" << "\n";
        strebuf << "1.0 0.5 1.0" << "\n";
        strebuf << "0.0 1.0 1.0" << "\n";
        strebuf << "0.5 1.0 1.0" << "\n";
        strebuf << "1.0 1.0 1.0" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "couldn't read cube size");
    }
    {
        // Cube sizes are not equal.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "3D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "11" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2       0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "5" << "\n";
        strebuf << "0.0 0.25       0.5 0.6 0.7" << "\n";
        strebuf << "0.0 0.25000001 0.5 0.6 0.7" << "\n";
        strebuf << "" << "\n";
        strebuf << "3 3 4" << "\n";  // <------------ Wrong.
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.5 0.0 0.0" << "\n";
        strebuf << "1.0 0.0 0.0" << "\n";
        strebuf << "0.0 0.5 0.0" << "\n";
        strebuf << "0.5 0.5 0.0" << "\n";
        strebuf << "1.0 0.5 0.0" << "\n";
        strebuf << "0.0 1.0 0.0" << "\n";
        strebuf << "0.5 1.0 0.0" << "\n";
        strebuf << "1.0 1.0 0.0" << "\n";
        strebuf << "0.0 0.0 0.5" << "\n";
        strebuf << "0.5 0.0 0.5" << "\n";
        strebuf << "1.0 0.0 0.5" << "\n";
        strebuf << "0.0 0.5 0.5" << "\n";
        strebuf << "0.5 0.5 0.5" << "\n";
        strebuf << "1.0 0.5 0.5" << "\n";
        strebuf << "0.0 1.0 0.5" << "\n";
        strebuf << "0.5 1.0 0.5" << "\n";
        strebuf << "1.0 1.0 0.5" << "\n";
        strebuf << "0.0 0.0 1.0" << "\n";
        strebuf << "0.5 0.0 1.0" << "\n";
        strebuf << "1.0 0.0 1.0" << "\n";
        strebuf << "0.0 0.5 1.0" << "\n";
        strebuf << "0.5 0.5 1.0" << "\n";
        strebuf << "1.0 0.5 1.0" << "\n";
        strebuf << "0.0 1.0 1.0" << "\n";
        strebuf << "0.5 1.0 1.0" << "\n";
        strebuf << "1.0 1.0 1.0" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "nonuniform cube sizes");
    }
    {
        // Cube size is not >0.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "3D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "11" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2       0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "5" << "\n";
        strebuf << "0.0 0.25       0.5 0.6 0.7" << "\n";
        strebuf << "0.0 0.25000001 0.5 0.6 0.7" << "\n";
        strebuf << "" << "\n";
        strebuf << "-3 -3 -3" << "\n"; // <------------ Wrong.
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.5 0.0 0.0" << "\n";
        strebuf << "1.0 0.0 0.0" << "\n";
        strebuf << "0.0 0.5 0.0" << "\n";
        strebuf << "0.5 0.5 0.0" << "\n";
        strebuf << "1.0 0.5 0.0" << "\n";
        strebuf << "0.0 1.0 0.0" << "\n";
        strebuf << "0.5 1.0 0.0" << "\n";
        strebuf << "1.0 1.0 0.0" << "\n";
        strebuf << "0.0 0.0 0.5" << "\n";
        strebuf << "0.5 0.0 0.5" << "\n";
        strebuf << "1.0 0.0 0.5" << "\n";
        strebuf << "0.0 0.5 0.5" << "\n";
        strebuf << "0.5 0.5 0.5" << "\n";
        strebuf << "1.0 0.5 0.5" << "\n";
        strebuf << "0.0 1.0 0.5" << "\n";
        strebuf << "0.5 1.0 0.5" << "\n";
        strebuf << "1.0 1.0 0.5" << "\n";
        strebuf << "0.0 0.0 1.0" << "\n";
        strebuf << "0.5 0.0 1.0" << "\n";
        strebuf << "1.0 0.0 1.0" << "\n";
        strebuf << "0.0 0.5 1.0" << "\n";
        strebuf << "0.5 0.5 1.0" << "\n";
        strebuf << "1.0 0.5 1.0" << "\n";
        strebuf << "0.0 1.0 1.0" << "\n";
        strebuf << "0.5 1.0 1.0" << "\n";
        strebuf << "1.0 1.0 1.0" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "invalid cube size");
    }
    {
        // One LUT entry has 4 components.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "3D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "11" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2       0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "5" << "\n";
        strebuf << "0.0 0.25       0.5 0.6 0.7" << "\n";
        strebuf << "0.0 0.25000001 0.5 0.6 0.7" << "\n";
        strebuf << "" << "\n";
        strebuf << "3 3 3" << "\n";
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.5 0.0 0.0" << "\n";
        strebuf << "1.0 0.0 0.0" << "\n";
        strebuf << "0.0 0.5 0.0" << "\n";
        strebuf << "0.5 0.5 0.0 1.0" << "\n"; // <------------ Wrong.
        strebuf << "1.0 0.5 0.0" << "\n";
        strebuf << "0.0 1.0 0.0" << "\n";
        strebuf << "0.5 1.0 0.0" << "\n";
        strebuf << "1.0 1.0 0.0" << "\n";
        strebuf << "0.0 0.0 0.5" << "\n";
        strebuf << "0.5 0.0 0.5" << "\n";
        strebuf << "1.0 0.0 0.5" << "\n";
        strebuf << "0.0 0.5 0.5" << "\n";
        strebuf << "0.5 0.5 0.5" << "\n";
        strebuf << "1.0 0.5 0.5" << "\n";
        strebuf << "0.0 1.0 0.5" << "\n";
        strebuf << "0.5 1.0 0.5" << "\n";
        strebuf << "1.0 1.0 0.5" << "\n";
        strebuf << "0.0 0.0 1.0" << "\n";
        strebuf << "0.5 0.0 1.0" << "\n";
        strebuf << "1.0 0.0 1.0" << "\n";
        strebuf << "0.0 0.5 1.0" << "\n";
        strebuf << "0.5 0.5 1.0" << "\n";
        strebuf << "1.0 0.5 1.0" << "\n";
        strebuf << "0.0 1.0 1.0" << "\n";
        strebuf << "0.5 1.0 1.0" << "\n";
        strebuf << "1.0 1.0 1.0" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "couldn't read cube row");
    }
    {
        // One LUT entry has 2 components.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "3D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "11" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2       0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "5" << "\n";
        strebuf << "0.0 0.25       0.5 0.6 0.7" << "\n";
        strebuf << "0.0 0.25000001 0.5 0.6 0.7" << "\n";
        strebuf << "" << "\n";
        strebuf << "3 3 3" << "\n";
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.5 0.0 0.0" << "\n";
        strebuf << "1.0 0.0 0.0" << "\n";
        strebuf << "0.0 0.5 0.0" << "\n";
        strebuf << "0.5 0.5 0.0" << "\n";
        strebuf << "1.0 0.5 0.0" << "\n";
        strebuf << "0.0 1.0 0.0" << "\n";
        strebuf << "0.5 1.0 0.0" << "\n";
        strebuf << "1.0 1.0" << "\n";     // <------------ Wrong.
        strebuf << "0.0 0.0 0.5" << "\n";
        strebuf << "0.5 0.0 0.5" << "\n";
        strebuf << "1.0 0.0 0.5" << "\n";
        strebuf << "0.0 0.5 0.5" << "\n";
        strebuf << "0.5 0.5 0.5" << "\n";
        strebuf << "1.0 0.5 0.5" << "\n";
        strebuf << "0.0 1.0 0.5" << "\n";
        strebuf << "0.5 1.0 0.5" << "\n";
        strebuf << "1.0 1.0 0.5" << "\n";
        strebuf << "0.0 0.0 1.0" << "\n";
        strebuf << "0.5 0.0 1.0" << "\n";
        strebuf << "1.0 0.0 1.0" << "\n";
        strebuf << "0.0 0.5 1.0" << "\n";
        strebuf << "0.5 0.5 1.0" << "\n";
        strebuf << "1.0 0.5 1.0" << "\n";
        strebuf << "0.0 1.0 1.0" << "\n";
        strebuf << "0.5 1.0 1.0" << "\n";
        strebuf << "1.0 1.0 1.0" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "couldn't read cube row");
    }
    {
        // One LUT entry can't be converted to 3 floats.
        std::ostringstream strebuf;
        strebuf << "CSPLUTV100" << "\n";
        strebuf << "3D" << "\n";
        strebuf << "" << "\n";
        strebuf << "BEGIN METADATA" << "\n";
        strebuf << "foobar" << "\n";
        strebuf << "END METADATA" << "\n";
        strebuf << "" << "\n";
        strebuf << "11" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
        strebuf << "6" << "\n";
        strebuf << "0.0 0.2       0.4 0.6 0.8 1.0" << "\n";
        strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0" << "\n";
        strebuf << "5" << "\n";
        strebuf << "0.0 0.25       0.5 0.6 0.7" << "\n";
        strebuf << "0.0 0.25000001 0.5 0.6 0.7" << "\n";
        strebuf << "" << "\n";
        strebuf << "3 3 3" << "\n";
        strebuf << "0.0 0.0 0.0" << "\n";
        strebuf << "0.5 0.0 0.0" << "\n";
        strebuf << "1.0 0.0 0.0" << "\n";
        strebuf << "0.0 0.5 0.0" << "\n";
        strebuf << "0.5 0.5 0.0" << "\n";
        strebuf << "1.0 0.5 One" << "\n";  // <------------ Wrong.
        strebuf << "0.0 1.0 0.0" << "\n";
        strebuf << "0.5 1.0 0.0" << "\n";
        strebuf << "1.0 1.0 0.0" << "\n";
        strebuf << "0.0 0.0 0.5" << "\n";
        strebuf << "0.5 0.0 0.5" << "\n";
        strebuf << "1.0 0.0 0.5" << "\n";
        strebuf << "0.0 0.5 0.5" << "\n";
        strebuf << "0.5 0.5 0.5" << "\n";
        strebuf << "1.0 0.5 0.5" << "\n";
        strebuf << "0.0 1.0 0.5" << "\n";
        strebuf << "0.5 1.0 0.5" << "\n";
        strebuf << "1.0 1.0 0.5" << "\n";
        strebuf << "0.0 0.0 1.0" << "\n";
        strebuf << "0.5 0.0 1.0" << "\n";
        strebuf << "1.0 0.0 1.0" << "\n";
        strebuf << "0.0 0.5 1.0" << "\n";
        strebuf << "0.5 0.5 1.0" << "\n";
        strebuf << "1.0 0.5 1.0" << "\n";
        strebuf << "0.0 1.0 1.0" << "\n";
        strebuf << "0.5 1.0 1.0" << "\n";
        strebuf << "1.0 1.0 1.0" << "\n";

        std::istringstream lutStream;
        lutStream.str(strebuf.str());

        // Read file.
        std::string fileName("file.name");
        OCIO::LocalFileFormat tester;
        OCIO_CHECK_THROW_WHAT(tester.read(lutStream, fileName, OCIO::INTERP_DEFAULT),
                              OCIO::Exception, "couldn't read cube row");
    }
}

// TODO: More strenuous tests of prelut resampling (non-noop preluts)

