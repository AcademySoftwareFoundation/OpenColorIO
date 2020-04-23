// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatCC.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
OCIO::LocalCachedFileRcPtr LoadCCFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}
}

OCIO_ADD_TEST(FileFormatCC, test_ccc1)
{
    // CC file
    const std::string fileName("cdl_test1.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    auto & formatMetadata = ccFile->transform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 1);
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getName()), "SOPDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getValue()), "this is a description");

    std::string idStr(ccFile->transform->getID());
    OCIO_CHECK_EQUAL("foo", idStr);
    std::string descStr(ccFile->transform->getDescription());
    OCIO_CHECK_EQUAL("this is a description", descStr);
    double slope[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.1, slope[0]);
    OCIO_CHECK_EQUAL(1.2, slope[1]);
    OCIO_CHECK_EQUAL(1.3, slope[2]);
    double offset[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getOffset(offset));
    OCIO_CHECK_EQUAL(2.1, offset[0]);
    OCIO_CHECK_EQUAL(2.2, offset[1]);
    OCIO_CHECK_EQUAL(2.3, offset[2]);
    double power[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getPower(power));
    OCIO_CHECK_EQUAL(3.1, power[0]);
    OCIO_CHECK_EQUAL(3.2, power[1]);
    OCIO_CHECK_EQUAL(3.3, power[2]);
    OCIO_CHECK_EQUAL(0.7, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, test_ccc2)
{
    // CC file using windows eol.
    const std::string fileName("cdl_test2.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    auto & formatMetadata = ccFile->transform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 2);
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getName()), "SOPDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getValue()), "Example look");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getName()), "SATDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getValue()), "boosting sat");

    std::string idStr(ccFile->transform->getID());
    OCIO_CHECK_EQUAL("cc0001", idStr);
    // OCIO keeps only the first SOPNode description
    std::string descStr(ccFile->transform->getDescription());
    OCIO_CHECK_EQUAL("Example look", descStr);
    double slope[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.0, slope[0]);
    OCIO_CHECK_EQUAL(1.0, slope[1]);
    OCIO_CHECK_EQUAL(0.9, slope[2]);
    double offset[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getOffset(offset));
    OCIO_CHECK_EQUAL(-0.03, offset[0]);
    OCIO_CHECK_EQUAL(-0.02, offset[1]);
    OCIO_CHECK_EQUAL(0.0, offset[2]);
    double power[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getPower(power));
    OCIO_CHECK_EQUAL(1.25, power[0]);
    OCIO_CHECK_EQUAL(1.0, power[1]);
    OCIO_CHECK_EQUAL(1.0, power[2]);
    OCIO_CHECK_EQUAL(1.7, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, test_cc_sat_node)
{
    // CC file
    const std::string fileName("cdl_test_SATNode.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // "SATNode" is recognized.
    OCIO_CHECK_EQUAL(0.42, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, test_cc_asc_sat)
{
    // As a warning message is expected, please mute it.
    OCIO::MuteLogging mute;

    // CC file
    const std::string fileName("cdl_test_ASC_SAT.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // "ASC_SAT" is not recognized. Default value is returned.
    OCIO_CHECK_EQUAL(1.0, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, test_cc_asc_sop)
{
    // As a warning message is expected, please mute it.
    OCIO::MuteLogging mute;

    // CC file
    const std::string fileName("cdl_test_ASC_SOP.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // "ASC_SOP" is not recognized. Default values are used.
    auto & formatMetadata = ccFile->transform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 0);

    std::string descStr(ccFile->transform->getDescription());
    OCIO_CHECK_EQUAL("", descStr);
    double slope[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.0, slope[0]);
    double offset[3] = { 1., 1., 1. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getOffset(offset));
    OCIO_CHECK_EQUAL(0.0, offset[0]);
    double power[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->transform->getPower(power));
    OCIO_CHECK_EQUAL(1.0, power[0]);
}

