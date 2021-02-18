/*
Copyright (c) 2014 Cinesite VFX Ltd, et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "fileformats/FileFormatCDL.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
OCIO::LocalCachedFileRcPtr LoadCDLFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}
}

OCIO_ADD_TEST(FileFormatCDL, test_cdl)
{
    // As a warning message is expected, please mute it.
    OCIO::MuteLogging mute;

    // CDL file
    const std::string fileName("cdl_test1.cdl");

    OCIO::LocalCachedFileRcPtr cdlFile;
    OCIO_CHECK_NO_THROW(cdlFile = LoadCDLFile(fileName));
    OCIO_REQUIRE_ASSERT(cdlFile);

    // Check that Descriptive element children of <ColorDecisionList> are preserved.
    OCIO_REQUIRE_EQUAL(cdlFile->m_metadata.getNumChildrenElements(), 4);
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(0).getElementName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(0).getElementValue()),
                     "This is a color decision list example.");
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(1).getElementName()),
                     "InputDescription");
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(1).getElementValue()),
                     "These should be applied in ACESproxy color space.");
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(2).getElementName()),
                     "ViewingDescription");
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(2).getElementValue()),
                     "View using the ACES RRT+ODT transforms.");
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(3).getElementName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(cdlFile->m_metadata.getChildElement(3).getElementValue()),
                     "It includes all possible description uses.");

    OCIO_CHECK_EQUAL(5, cdlFile->m_transformVec.size());
    // Two of the five CDLs in the file don't have an id attribute and are not
    // included in the m_transformMap since it used the id as the key.
    OCIO_CHECK_EQUAL(3, cdlFile->m_transformMap.size());
    {
        // Note: Descriptive elements that are children of <ColorDecision> are not preserved.

        std::string idStr(cdlFile->m_transformVec[0]->data().getID());
        OCIO_CHECK_EQUAL("cc0001", idStr);

        // Check that Descriptive element children of <ColorCorrection> are preserved.
        auto & formatMetadata = cdlFile->m_transformVec[0]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 6);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementName()),
                         "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementValue()),
                         "CC-level description 1");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementName()),
                         "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementValue()),
                         "CC-level input description 1");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementName()),
                         "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementValue()),
                         "CC-level viewing description 1");
        // Check that Descriptive element children of SOPNode and SatNode are preserved.
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementValue()),
                         "Example look");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementValue()),
                         "For scenes 1 and 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementName()),
                         "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementValue()),
                         "boosting sat");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[0]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.0, slope[0]);
        OCIO_CHECK_EQUAL(1.0, slope[1]);
        OCIO_CHECK_EQUAL(0.9, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[0]->getOffset(offset));
        OCIO_CHECK_EQUAL(-0.03, offset[0]);
        OCIO_CHECK_EQUAL(-0.02, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[0]->getPower(power));
        OCIO_CHECK_EQUAL(1.25, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.0, power[2]);
        OCIO_CHECK_EQUAL(1.7, cdlFile->m_transformVec[0]->getSat());
    }
    {
        std::string idStr(cdlFile->m_transformVec[1]->data().getID());
        OCIO_CHECK_EQUAL("cc0002", idStr);

        auto & formatMetadata = cdlFile->m_transformVec[1]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 6);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementName()),
                         "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getElementValue()),
                         "CC-level description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementName()),
                         "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getElementValue()),
                         "CC-level input description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementName()),
                         "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getElementValue()),
                         "CC-level viewing description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getElementValue()),
                         "pastel");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementName()),
                         "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getElementValue()),
                         "another example");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementName()),
                         "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getElementValue()),
                         "dropping sat");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[1]->getSlope(slope));
        OCIO_CHECK_EQUAL(0.9, slope[0]);
        OCIO_CHECK_EQUAL(0.7, slope[1]);
        OCIO_CHECK_EQUAL(0.6, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[1]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.1, offset[0]);
        OCIO_CHECK_EQUAL(0.1, offset[1]);
        OCIO_CHECK_EQUAL(0.1, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[1]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(0.9, power[1]);
        OCIO_CHECK_EQUAL(0.9, power[2]);
        OCIO_CHECK_EQUAL(0.7, cdlFile->m_transformVec[1]->getSat());
    }
    {
        std::string idStr(cdlFile->m_transformVec[2]->data().getID());
        OCIO_CHECK_EQUAL("cc0003", idStr);

        auto & formatMetadata = cdlFile->m_transformVec[2]->getFormatMetadata();
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
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[2]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2, slope[0]);
        OCIO_CHECK_EQUAL(1.1, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[2]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[2]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        OCIO_CHECK_EQUAL(1.0, cdlFile->m_transformVec[2]->getSat());
    }
    {
        std::string idStr(cdlFile->m_transformVec[3]->data().getID());
        OCIO_CHECK_EQUAL("", idStr);

        auto & formatMetadata = cdlFile->m_transformVec[3]->getFormatMetadata();
        OCIO_CHECK_EQUAL(formatMetadata.getNumChildrenElements(), 0);

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[3]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2, slope[0]);
        OCIO_CHECK_EQUAL(1.1, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[3]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[3]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        // SatNode missing from XML, uses a default of 1.0.
        OCIO_CHECK_EQUAL(1.0, cdlFile->m_transformVec[3]->getSat());
    }
    {
        std::string idStr(cdlFile->m_transformVec[4]->data().getID());
        OCIO_CHECK_EQUAL("", idStr);

        auto & formatMetadata = cdlFile->m_transformVec[4]->getFormatMetadata();
        OCIO_CHECK_EQUAL(formatMetadata.getNumChildrenElements(), 0);

        // SOPNode missing from XML, uses default values.
        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[4]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.0, slope[0]);
        OCIO_CHECK_EQUAL(1.0, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[4]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->m_transformVec[4]->getPower(power));
        OCIO_CHECK_EQUAL(1.0, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.0, power[2]);
        OCIO_CHECK_EQUAL(0.0, cdlFile->m_transformVec[4]->getSat());
    }
}

// See also test: (CDLTransform, create_from_cdl_file).

OCIO_ADD_TEST(FileFormatCDL, write)
{
    // As a warning message is expected, please mute it.
    OCIO::MuteLogging mute;

    // Note that metadata in ColorDecisionList and in ColorCorrection are preserved, but not inside
    // ColorDecision.
    const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_test1.cdl");
    OCIO::GroupTransformRcPtr group;
    OCIO_CHECK_NO_THROW(group = OCIO::CDLTransform::CreateGroupFromFile(filePath.c_str()));
    OCIO_REQUIRE_ASSERT(group);

    auto cfg = OCIO::Config::CreateRaw();
    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(group->write(cfg,OCIO::FILEFORMAT_COLOR_DECISION_LIST, oss));
    constexpr char RESULT[]{ R"(<ColorDecisionList xmlns="urn:ASC:CDL:v1.01">
    <Description>This is a color decision list example.</Description>
    <Description>It includes all possible description uses.</Description>
    <InputDescription>These should be applied in ACESproxy color space.</InputDescription>
    <ViewingDescription>View using the ACES RRT+ODT transforms.</ViewingDescription>
    <ColorDecision>
        <ColorCorrection id="cc0001">
            <Description>CC-level description 1</Description>
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
    </ColorDecision>
    <ColorDecision>
        <ColorCorrection id="cc0002">
            <Description>CC-level description 2</Description>
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
    </ColorDecision>
    <ColorDecision>
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
    </ColorDecision>
    <ColorDecision>
        <ColorCorrection>
            <SOPNode>
                <Slope>1.2 1.1 1</Slope>
                <Offset>0 0 0</Offset>
                <Power>0.9 1 1.2</Power>
            </SOPNode>
            <SatNode>
                <Saturation>1</Saturation>
            </SatNode>
        </ColorCorrection>
    </ColorDecision>
    <ColorDecision>
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
    </ColorDecision>
</ColorDecisionList>
)" };
    OCIO_CHECK_EQUAL(oss.str(), RESULT);

    // Write failures.

    oss.clear();
    oss.str("");

    // Empty group.
    group = OCIO::GroupTransform::Create();
    OCIO_CHECK_THROW_WHAT(group->write(cfg, OCIO::FILEFORMAT_COLOR_DECISION_LIST, oss),
                          OCIO::Exception, "there should be at least one CDL");

    // Only CDL.
    group->appendTransform(OCIO::RangeTransform::Create());
    OCIO_CHECK_THROW_WHAT(group->write(cfg, OCIO::FILEFORMAT_COLOR_DECISION_LIST, oss),
                          OCIO::Exception, "only CDL can be written");
}
