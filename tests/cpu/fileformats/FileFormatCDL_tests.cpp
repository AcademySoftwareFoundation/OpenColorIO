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
    OCIO_REQUIRE_EQUAL(cdlFile->metadata.getNumChildrenElements(), 4);
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(0).getName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(0).getValue()),
                     "This is a color decision list example.");
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(1).getName()),
                     "InputDescription");
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(1).getValue()),
                     "These should be applied in ACESproxy color space.");
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(2).getName()),
                     "ViewingDescription");
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(2).getValue()),
                     "View using the ACES RRT+ODT transforms.");
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(3).getName()),
                     "Description");
    OCIO_CHECK_EQUAL(std::string(cdlFile->metadata.getChildElement(3).getValue()),
                     "It includes all possible description uses.");

    OCIO_CHECK_EQUAL(5, cdlFile->transformVec.size());
    // Two of the five CDLs in the file don't have an id attribute and are not
    // included in the transformMap since it used the id as the key.
    OCIO_CHECK_EQUAL(3, cdlFile->transformMap.size());
    {
        // Note: Descriptive elements that are children of <ColorDecision> are not preserved.

        std::string idStr(cdlFile->transformVec[0]->getID());
        OCIO_CHECK_EQUAL("cc0001", idStr);

        // Check that Descriptive element children of <ColorCorrection> are preserved.
        auto & formatMetadata = cdlFile->transformVec[0]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 6);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getName()), "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getValue()), "CC-level description 1");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getName()), "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getValue()), "CC-level input description 1");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getName()), "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getValue()), "CC-level viewing description 1");
        // Check that Descriptive element children of SOPNode and SatNode are preserved.
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getName()), "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getValue()), "Example look");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getName()), "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getValue()), "For scenes 1 and 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getName()), "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getValue()), "boosting sat");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[0]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.0, slope[0]);
        OCIO_CHECK_EQUAL(1.0, slope[1]);
        OCIO_CHECK_EQUAL(0.9, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[0]->getOffset(offset));
        OCIO_CHECK_EQUAL(-0.03, offset[0]);
        OCIO_CHECK_EQUAL(-0.02, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[0]->getPower(power));
        OCIO_CHECK_EQUAL(1.25, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.0, power[2]);
        OCIO_CHECK_EQUAL(1.7, cdlFile->transformVec[0]->getSat());
    }
    {
        std::string idStr(cdlFile->transformVec[1]->getID());
        OCIO_CHECK_EQUAL("cc0002", idStr);

        auto & formatMetadata = cdlFile->transformVec[1]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 6);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getName()), "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getValue()), "CC-level description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getName()), "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getValue()), "CC-level input description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getName()), "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getValue()), "CC-level viewing description 2");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getName()), "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getValue()), "pastel");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getName()), "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getValue()), "another example");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getName()), "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getValue()), "dropping sat");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[1]->getSlope(slope));
        OCIO_CHECK_EQUAL(0.9, slope[0]);
        OCIO_CHECK_EQUAL(0.7, slope[1]);
        OCIO_CHECK_EQUAL(0.6, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[1]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.1, offset[0]);
        OCIO_CHECK_EQUAL(0.1, offset[1]);
        OCIO_CHECK_EQUAL(0.1, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[1]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(0.9, power[1]);
        OCIO_CHECK_EQUAL(0.9, power[2]);
        OCIO_CHECK_EQUAL(0.7, cdlFile->transformVec[1]->getSat());
    }
    {
        std::string idStr(cdlFile->transformVec[2]->getID());
        OCIO_CHECK_EQUAL("cc0003", idStr);

        auto & formatMetadata = cdlFile->transformVec[2]->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 6);
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getName()), "Description");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getValue()), "CC-level description 3");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getName()), "InputDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getValue()), "CC-level input description 3");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getName()), "ViewingDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(2).getValue()), "CC-level viewing description 3");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getName()), "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(3).getValue()), "golden");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getName()), "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(4).getValue()), "no sat change");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getName()), "SATDescription");
        OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(5).getValue()), "sat==1");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[2]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2, slope[0]);
        OCIO_CHECK_EQUAL(1.1, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[2]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[2]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        OCIO_CHECK_EQUAL(1.0, cdlFile->transformVec[2]->getSat());
    }
    {
        std::string idStr(cdlFile->transformVec[3]->getID());
        OCIO_CHECK_EQUAL("", idStr);

        auto & formatMetadata = cdlFile->transformVec[3]->getFormatMetadata();
        OCIO_CHECK_EQUAL(formatMetadata.getNumChildrenElements(), 0);

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[3]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2, slope[0]);
        OCIO_CHECK_EQUAL(1.1, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[3]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[3]->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        // SatNode missing from XML, uses a default of 1.0.
        OCIO_CHECK_EQUAL(1.0, cdlFile->transformVec[3]->getSat());
    }
    {
        std::string idStr(cdlFile->transformVec[4]->getID());
        OCIO_CHECK_EQUAL("", idStr);

        auto & formatMetadata = cdlFile->transformVec[4]->getFormatMetadata();
        OCIO_CHECK_EQUAL(formatMetadata.getNumChildrenElements(), 0);

        // SOPNode missing from XML, uses default values.
        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[4]->getSlope(slope));
        OCIO_CHECK_EQUAL(1.0, slope[0]);
        OCIO_CHECK_EQUAL(1.0, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[4]->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(cdlFile->transformVec[4]->getPower(power));
        OCIO_CHECK_EQUAL(1.0, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.0, power[2]);
        OCIO_CHECK_EQUAL(0.0, cdlFile->transformVec[4]->getSat());
    }
}

