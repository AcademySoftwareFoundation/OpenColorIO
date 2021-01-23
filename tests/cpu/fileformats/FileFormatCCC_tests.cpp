// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatCCC.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
OCIO::LocalCachedFileRcPtr LoadCCCFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(fileName,
                                                                            std::ios_base::in);
}
}

OCIO_ADD_TEST(FileFormatCCC, read)
{
    // CCC file
    const std::string fileName("cdl_test1.ccc");

    OCIO::LocalCachedFileRcPtr cccFile;
    OCIO_CHECK_NO_THROW(cccFile = LoadCCCFile(fileName));
    OCIO_REQUIRE_ASSERT(cccFile);

    // Check that Descriptive element children of <ColorCorrectionCollection> are preserved.
    OCIO_REQUIRE_EQUAL(cccFile->m_metadata.getNumChildrenElements(), 4);
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(0).getElementName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(0).getElementValue()),
                     "This is a color correction collection example.");
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(1).getElementName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(1).getElementValue()),
                     "It includes all possible description uses.");
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(2).getElementName()),
                     "InputDescription");
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(2).getElementValue()),
                     "These should be applied in ACESproxy color space.");
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(3).getElementName()),
                     "ViewingDescription");
    OCIO_CHECK_EQUAL(std::string(cccFile->m_metadata.getChildElement(3).getElementValue()),
                     "View using the ACES RRT+ODT transforms.");

    OCIO_CHECK_EQUAL(5, cccFile->m_transformVec.size());
    // Two of the five CDLs in the file don't have an id attribute and are not
    // included in the transformMap since it used the id as the key.
    OCIO_CHECK_EQUAL(3, cccFile->m_transformMap.size());
    {
        std::string idStr(cccFile->m_transformVec[0]->data().getID());
        OCIO_CHECK_EQUAL("cc0001", idStr);

        // Check that Descriptive element children of <ColorCorrection> are preserved.
        auto & formatMetadata = cccFile->m_transformVec[0]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 7);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementName()),
                         "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementValue()),
                         "CC-level description 1a");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementName()),
                         "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementValue()),
                         "CC-level description 1b");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementName()),
                         "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementValue()),
                         "CC-level input description 1");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementName()),
                         "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementValue()),
                         "CC-level viewing description 1");
        // Check that Descriptive element children of SOPNode and SatNode are preserved.
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementValue()),
                         "Example look");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementValue()),
                         "For scenes 1 and 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(6).getElementName()),
                         "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(6).getElementValue()),
                         "boosting sat");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[0]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.0, slope[0]);
        OCIO_CHECK_EQUAL(1.0, slope[1]);
        OCIO_CHECK_EQUAL(0.9, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[0]->getOffset(offset));
        OCIO_CHECK_EQUAL(-0.03, offset[0]);
        OCIO_CHECK_EQUAL(-0.02, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[0]->getPower(power));
        OCIO_CHECK_EQUAL(1.25, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.0, power[2]);
        OCIO_CHECK_EQUAL(1.7, cccFile->m_transformVec[0]->getSat());
    }
    {
        std::string idStr(cccFile->m_transformVec[1]->data().getID());
        OCIO_CHECK_EQUAL("cc0002", idStr);

        auto & formatMetadata = cccFile->m_transformVec[1]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 7);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementName()),
                         "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementValue()),
                         "CC-level description 2a");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementName()),
                         "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementValue()),
                         "CC-level description 2b");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementName()),
                         "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementValue()),
                         "CC-level input description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementName()),
                         "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementValue()),
                         "CC-level viewing description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementValue()),
                         "pastel");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementValue()),
                         "another example");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(6).getElementName()),
                         "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(6).getElementValue()),
                         "dropping sat");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[1]->getSlope(slope));
        OCIO_CHECK_EQUAL(0.9, slope[0]);
        OCIO_CHECK_EQUAL(0.7, slope[1]);
        OCIO_CHECK_EQUAL(0.6, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[1]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.1, offset[0]);
        OCIO_CHECK_EQUAL(0.1, offset[1]);
        OCIO_CHECK_EQUAL(0.1, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[1]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(0.9, power[1]);
        OCIO_CHECK_EQUAL(0.9, power[2]);
        OCIO_CHECK_EQUAL(0.7, cccFile->m_transformVec[1]->getSat());
    }
    {
        std::string idStr(cccFile->m_transformVec[2]->data().getID());
        OCIO_CHECK_EQUAL("cc0003", idStr);

        auto & formatMetadata = cccFile->m_transformVec[2]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 6);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementName()),
                         "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementValue()),
                         "CC-level description 3");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementName()),
                         "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementValue()),
                         "CC-level input description 3");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementName()),
                         "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementValue()),
                         "CC-level viewing description 3");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementValue()),
                         "golden");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementName()),
                         "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementValue()),
                         "no sat change");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementName()),
                         "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementValue()),
                         "sat==1");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[2]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2, slope[0]);
        OCIO_CHECK_EQUAL(1.1, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[2]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[2]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        OCIO_CHECK_EQUAL(1.0, cccFile->m_transformVec[2]->getSat());
    }
    {
        std::string idStr(cccFile->m_transformVec[3]->data().getID());
        OCIO_CHECK_EQUAL("", idStr);

        auto & formatMetadata = cccFile->m_transformVec[3]->getFormatMetadata();
        OCIO_CHECK_EQUAL(formatMetadata.getNumChildrenElements(), 0);

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[3]->getSlope(slope));
        OCIO_CHECK_EQUAL(4.0, slope[0]);
        OCIO_CHECK_EQUAL(5.0, slope[1]);
        OCIO_CHECK_EQUAL(6.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[3]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[3]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        // SatNode missing from XML, uses a default of 1.0.
        OCIO_CHECK_EQUAL(1.0, cccFile->m_transformVec[3]->getSat());
    }
    {
        std::string idStr(cccFile->m_transformVec[4]->data().getID());
        OCIO_CHECK_EQUAL("", idStr);

        auto & formatMetadata = cccFile->m_transformVec[4]->getFormatMetadata();
        OCIO_CHECK_EQUAL(formatMetadata.getNumChildrenElements(), 0);

        // SOPNode missing from XML, uses default values.
        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[4]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.0, slope[0]);
        OCIO_CHECK_EQUAL(1.0, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[4]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cccFile->m_transformVec[4]->getPower(power));
        OCIO_CHECK_EQUAL(1.0, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.0, power[2]);

        OCIO_CHECK_EQUAL(0.0, cccFile->m_transformVec[4]->getSat());
    }

    const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_test1.ccc");

    // Create a FileTransform
    OCIO::FileTransformRcPtr fileTransform = OCIO::FileTransform::Create();
    fileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    fileTransform->setSrc(filePath.c_str());
    fileTransform->setCCCId("cc0002");

    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    auto context = config->getCurrentContext();
    OCIO::LocalFileFormat tester;
    OCIO::OpRcPtrVec ops;

    tester.buildFileOps(ops, *config, context, cccFile, *fileTransform, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op = ops[0];
    // Check that the Descriptive element children of <ColorCorrection> are preserved.
    // Note that Descriptive element children of <ColorCorrectionCollection> are only
    // available in the CachedFile, not in the OpData.
    auto data = op->data();
    auto & metadata = data->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumChildrenElements(), 7);
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(0).getElementName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(0).getElementValue()),
                     "CC-level description 2a");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(1).getElementName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(1).getElementValue()),
                     "CC-level description 2b");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(2).getElementName()),
                     "InputDescription");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(2).getElementValue()),
                     "CC-level input description 2");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(3).getElementName()),
                     "ViewingDescription");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(3).getElementValue()),
                     "CC-level viewing description 2");
    // Check that the Descriptive element children of SOPNode and SatNode are preserved.
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(4).getElementName()),
                     "SOPDescription");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(4).getElementValue()),
                     "pastel");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(5).getElementName()),
                     "SOPDescription");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(5).getElementValue()),
                     "another example");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(6).getElementName()),
                     "SATDescription");
    OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(6).getElementValue()),
                     "dropping sat");

    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::CDLType);
    auto cdlData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(data);
    OCIO_CHECK_EQUAL(cdlData->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    // Test with ASC style.
    fileTransform->setCDLStyle(OCIO::CDL_ASC);

    ops.clear();
    tester.buildFileOps(ops, *config, context, cccFile, *fileTransform, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    op = ops[0];
    data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::CDLType);
    cdlData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(data);
    OCIO_CHECK_EQUAL(cdlData->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
}

// See also test: (CDLTransform, create_from_ccc_file).

OCIO_ADD_TEST(FileFormatCCC, write)
{
    const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_test1.ccc");
    OCIO::GroupTransformRcPtr group;
    OCIO_CHECK_NO_THROW(group = OCIO::CDLTransform::CreateGroupFromFile(filePath.c_str()));
    OCIO_REQUIRE_ASSERT(group);

    auto cfg = OCIO::Config::CreateRaw();
    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(group->write(cfg, OCIO::FILEFORMAT_COLOR_CORRECTION_COLLECTION, oss));
    constexpr char RESULT[] { R"(<ColorCorrectionCollection xmlns="urn:ASC:CDL:v1.01">
    <Description>This is a color correction collection example.</Description>
    <Description>It includes all possible description uses.</Description>
    <InputDescription>These should be applied in ACESproxy color space.</InputDescription>
    <ViewingDescription>View using the ACES RRT+ODT transforms.</ViewingDescription>
    <ColorCorrection id="cc0001">
        <Description>CC-level description 1a</Description>
        <Description>CC-level description 1b</Description>
        <InputDescription>CC-level input description 1</InputDescription>
        <ViewingDescription>CC-level viewing description 1</ViewingDescription>
        <SOPNode>
            <Description>Example look</Description>
            <Description>For scenes 1 and 2</Description>
            <Slope>1 1 0.9</Slope>
            <Offset>-0.03 -0.02 0</Offset>
            <Power>1.25 1 1</Power>
        </SOPNode>
        <SatNode>
            <Description>boosting sat</Description>
            <Saturation>1.7</Saturation>
        </SatNode>
    </ColorCorrection>
    <ColorCorrection id="cc0002">
        <Description>CC-level description 2a</Description>
        <Description>CC-level description 2b</Description>
        <InputDescription>CC-level input description 2</InputDescription>
        <ViewingDescription>CC-level viewing description 2</ViewingDescription>
        <SOPNode>
            <Description>pastel</Description>
            <Description>another example</Description>
            <Slope>0.9 0.7 0.6</Slope>
            <Offset>0.1 0.1 0.1</Offset>
            <Power>0.9 0.9 0.9</Power>
        </SOPNode>
        <SatNode>
            <Description>dropping sat</Description>
            <Saturation>0.7</Saturation>
        </SatNode>
    </ColorCorrection>
    <ColorCorrection id="cc0003">
        <Description>CC-level description 3</Description>
        <InputDescription>CC-level input description 3</InputDescription>
        <ViewingDescription>CC-level viewing description 3</ViewingDescription>
        <SOPNode>
            <Description>golden</Description>
            <Slope>1.2 1.1 1</Slope>
            <Offset>0 0 0</Offset>
            <Power>0.9 1 1.2</Power>
        </SOPNode>
        <SatNode>
            <Description>no sat change</Description>
            <Description>sat==1</Description>
            <Saturation>1</Saturation>
        </SatNode>
    </ColorCorrection>
    <ColorCorrection>
        <SOPNode>
            <Slope>4 5 6</Slope>
            <Offset>0 0 0</Offset>
            <Power>0.9 1 1.2</Power>
        </SOPNode>
        <SatNode>
            <Saturation>1</Saturation>
        </SatNode>
    </ColorCorrection>
    <ColorCorrection>
        <SOPNode>
            <Slope>1 1 1</Slope>
            <Offset>0 0 0</Offset>
            <Power>1 1 1</Power>
        </SOPNode>
        <SatNode>
            <Saturation>0</Saturation>
        </SatNode>
    </ColorCorrection>
</ColorCorrectionCollection>
)" };
    OCIO_CHECK_EQUAL(oss.str(), RESULT);
}
