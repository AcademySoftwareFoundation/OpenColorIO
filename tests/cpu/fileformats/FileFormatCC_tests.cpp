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
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(fileName,
                                                                            std::ios_base::in);
}
}

OCIO_ADD_TEST(FileFormatCC, test_cc1)
{
    // CC file
    const std::string fileName("cdl_test1.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    OCIO_CHECK_EQUAL(std::string(ccFile->m_transform->getID()), "foo");
    OCIO_CHECK_EQUAL(std::string(ccFile->m_transform->getFirstSOPDescription()),
                     "this is a description");
    double slope[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.1, slope[0]);
    OCIO_CHECK_EQUAL(1.2, slope[1]);
    OCIO_CHECK_EQUAL(1.3, slope[2]);
    double offset[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getOffset(offset));
    OCIO_CHECK_EQUAL(2.1, offset[0]);
    OCIO_CHECK_EQUAL(2.2, offset[1]);
    OCIO_CHECK_EQUAL(2.3, offset[2]);
    double power[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getPower(power));
    OCIO_CHECK_EQUAL(3.1, power[0]);
    OCIO_CHECK_EQUAL(3.2, power[1]);
    OCIO_CHECK_EQUAL(3.3, power[2]);
    OCIO_CHECK_EQUAL(0.7, ccFile->m_transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, test_cc2)
{
    // CC file using windows eol.
    const std::string fileName("cdl_test2.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // Access all using metadata.
    auto & formatMetadata = ccFile->m_transform->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getID()), "cc0001");
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 2);
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementName()),
                     "SOPDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementValue()),
                     "Example look");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementName()),
                     "SATDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementValue()),
                     "boosting sat");
    // Access using CDL transform helper functions (note that only the first SOP description is
    // available that way).
    OCIO_CHECK_EQUAL(std::string(ccFile->m_transform->getID()), "cc0001");
    OCIO_CHECK_EQUAL(std::string(ccFile->m_transform->getFirstSOPDescription()),
                     "Example look");

    double slope[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.0, slope[0]);
    OCIO_CHECK_EQUAL(1.0, slope[1]);
    OCIO_CHECK_EQUAL(0.9, slope[2]);
    double offset[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getOffset(offset));
    OCIO_CHECK_EQUAL(-0.03, offset[0]);
    OCIO_CHECK_EQUAL(-0.02, offset[1]);
    OCIO_CHECK_EQUAL(0.0, offset[2]);
    double power[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getPower(power));
    OCIO_CHECK_EQUAL(1.25, power[0]);
    OCIO_CHECK_EQUAL(1.0, power[1]);
    OCIO_CHECK_EQUAL(1.0, power[2]);
    OCIO_CHECK_EQUAL(1.7, ccFile->m_transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, test_cc_sat_node)
{
    // CC file
    const std::string fileName("cdl_test_SATNode.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // "SATNode" is recognized.
    OCIO_CHECK_EQUAL(0.42, ccFile->m_transform->getSat());
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
    OCIO_CHECK_EQUAL(1.0, ccFile->m_transform->getSat());
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
    auto & formatMetadata = ccFile->m_transform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 0);

    OCIO_CHECK_EQUAL(std::string(ccFile->m_transform->getID()), "foo");
    OCIO_CHECK_EQUAL(std::string(ccFile->m_transform->getFirstSOPDescription()), "");

    double slope[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.0, slope[0]);
    double offset[3] = { 1., 1., 1. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getOffset(offset));
    OCIO_CHECK_EQUAL(0.0, offset[0]);
    double power[3] = { 0., 0., 0. };
    OCIO_CHECK_NO_THROW(ccFile->m_transform->getPower(power));
    OCIO_CHECK_EQUAL(1.0, power[0]);
}

OCIO_ADD_TEST(FileFormatCC, test_cc2_load_save)
{
    const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_test2.cc");

    OCIO::GroupTransformRcPtr group;
    OCIO_CHECK_NO_THROW(group = OCIO::CDLTransform::CreateGroupFromFile(filePath.c_str()));
    OCIO_REQUIRE_ASSERT(group);

    std::ostringstream outputTransform;
    OCIO::ConstConfigRcPtr cfg = OCIO::Config::CreateRaw();
    OCIO_CHECK_NO_THROW(group->write(cfg, OCIO::FILEFORMAT_COLOR_CORRECTION, outputTransform));
    const std::string expected{ R"(<ColorCorrection id="cc0001">
    <SOPNode>
        <Description>Example look</Description>
        <Slope>1 1 0.9</Slope>
        <Offset>-0.03 -0.02 0</Offset>
        <Power>1.25 1 1</Power>
    </SOPNode>
    <SatNode>
        <Description>boosting sat</Description>
        <Saturation>1.7</Saturation>
    </SatNode>
</ColorCorrection>
)" };
    OCIO_CHECK_EQUAL(outputTransform.str(), expected);
}
