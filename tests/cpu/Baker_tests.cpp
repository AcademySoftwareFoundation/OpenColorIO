// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "Baker.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


/*
OCIO_ADD_TEST(Baker_Unit_Tests, test_listlutwriters)
{

    std::vector<std::string> current_writers;
    current_writers.push_back("cinespace");
    current_writers.push_back("houdini");

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();

    OCIO_CHECK_EQUAL(baker->getNumFormats(), (int)current_writers.size());

    std::vector<std::string> test;
    for(int i = 0; i < baker->getNumFormats(); ++i)
        test.push_back(baker->getFormatNameByIndex(i));

    for(unsigned int i = 0; i < current_writers.size(); ++i)
        OCIO_CHECK_EQUAL(current_writers[i], test[i]);

}
*/

OCIO_ADD_TEST(Baker, bake)
{
    // SSE aware test, similar to python test.
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
#ifdef USE_SSE
        "4\n"
        "0.000977 0.039373 1.587398 64.000168\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587398 64.000168\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587398 64.000168\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "\n"
        "2 2 2\n"
        "0.042823 0.042823 0.042823\n"
        "6.622035 0.042823 0.042823\n"
        "0.042823 6.622035 0.042823\n"
        "6.622035 6.622035 0.042823\n"
        "0.042823 0.042823 6.622035\n"
        "6.622035 0.042823 6.622035\n"
        "0.042823 6.622035 6.622035\n"
        "6.622035 6.622035 6.622035\n"
#else
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
#endif // USE_SSE
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
    OCIO_CHECK_EQUAL(testString, data.getChildElement(0).getValue());

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
    OCIO_CHECK_EQUAL(expectedLut, os.str());
    OCIO_CHECK_EQUAL(10, bake->getNumFormats());
    OCIO_CHECK_EQUAL("cinespace", std::string(bake->getFormatNameByIndex(4)));
    OCIO_CHECK_EQUAL("3dl", std::string(bake->getFormatExtensionByIndex(1)));
}

OCIO_ADD_TEST(Baker, empty_config)
{
    // Verify that running bake with an empty configuration
    // throws an exception and does not segfault.
    OCIO::BakerRcPtr bake = OCIO::Baker::Create();
    bake->setFormat("cinespace");
    std::ostringstream os;
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "No OCIO config has been set");
}
