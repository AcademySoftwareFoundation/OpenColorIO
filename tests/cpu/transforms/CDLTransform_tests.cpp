// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstdio>

#include "ops/exponent/ExponentOp.h"
#include "ops/matrix/MatrixOpData.h"
#include "transforms/CDLTransform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestUtils.h"

#include "Platform.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(CDLTransform, equality)
{
    const OCIO::CDLTransformRcPtr cdl1 = OCIO::CDLTransform::Create();
    const OCIO::CDLTransformRcPtr cdl2 = OCIO::CDLTransform::Create();

    OCIO_CHECK_ASSERT(cdl1->equals(*cdl1));
    OCIO_CHECK_ASSERT(cdl1->equals(*cdl2));
    OCIO_CHECK_ASSERT(cdl2->equals(*cdl1));

    const OCIO::CDLTransformRcPtr cdl3 = OCIO::CDLTransform::Create();
    cdl3->setSat(cdl3->getSat()+0.002f);

    OCIO_CHECK_ASSERT(!cdl1->equals(*cdl3));
    OCIO_CHECK_ASSERT(!cdl2->equals(*cdl3));
    OCIO_CHECK_ASSERT(cdl3->equals(*cdl3));
    
    cdl2->setStyle(OCIO::CDL_ASC);
    OCIO_CHECK_ASSERT(!cdl1->equals(*cdl2));
}

OCIO_ADD_TEST(CDLTransform, create_from_cc_file)
{
    const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_test1.cc");

    {
        OCIO::CDLTransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filePath.c_str(),
                                                                           nullptr));
        OCIO_REQUIRE_ASSERT(transform);
        OCIO_CHECK_EQUAL(std::string(transform->getID()), "foo");
        OCIO_CHECK_EQUAL(std::string(transform->getFirstSOPDescription()),
                         "this is a description");
        OCIO_CHECK_EQUAL(transform->getStyle(), OCIO::CDL_NO_CLAMP);
        double slope[3] = {0., 0., 0.};
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(1.1, slope[0]);
        OCIO_CHECK_EQUAL(1.2, slope[1]);
        OCIO_CHECK_EQUAL(1.3, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(2.1, offset[0]);
        OCIO_CHECK_EQUAL(2.2, offset[1]);
        OCIO_CHECK_EQUAL(2.3, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(3.1, power[0]);
        OCIO_CHECK_EQUAL(3.2, power[1]);
        OCIO_CHECK_EQUAL(3.3, power[2]);
        OCIO_CHECK_EQUAL(0.7, transform->getSat());
    }

    {
        OCIO::CDLTransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filePath.c_str(),
                                                                           "foo"));
        OCIO_CHECK_ASSERT(transform);
    }

    {
        OCIO::CDLTransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filePath.c_str(),
                                                                           "0"));
        OCIO_CHECK_ASSERT(transform);
    }

    {
        // The cccid is case sensitive.
        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "FOO"),
                              OCIO::Exception, "The specified CDL Id/Index 'FOO' could not be "
                                               "loaded from the file");
    }

    {
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = OCIO::CDLTransform::CreateGroupFromFile(filePath.c_str()));
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    }
}

OCIO_ADD_TEST(CDLTransform, create_from_ccc_file)
{
    const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_test1.ccc");
    {
        // Using ID.
        OCIO::CDLTransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filePath.c_str(),
                                                                           "cc0003"));
        OCIO_REQUIRE_ASSERT(transform);
        OCIO_CHECK_EQUAL(std::string(transform->getID()), "cc0003");
        OCIO_CHECK_EQUAL(transform->getStyle(), OCIO::CDL_NO_CLAMP);

        OCIO_CHECK_EQUAL(std::string(transform->getFirstSOPDescription()), "golden");

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2, slope[0]);
        OCIO_CHECK_EQUAL(1.1, slope[1]);
        OCIO_CHECK_EQUAL(1.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        OCIO_CHECK_EQUAL(1.0, transform->getSat());
    }
    {
        // Using 0 based index.
        auto transform = OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "3");
        OCIO_CHECK_EQUAL(std::string(transform->getID()), "");
        OCIO_CHECK_EQUAL(transform->getStyle(), OCIO::CDL_NO_CLAMP);

        double slope[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(4.0, slope[0]);
        OCIO_CHECK_EQUAL(5.0, slope[1]);
        OCIO_CHECK_EQUAL(6.0, slope[2]);
        double offset[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0, offset[0]);
        OCIO_CHECK_EQUAL(0.0, offset[1]);
        OCIO_CHECK_EQUAL(0.0, offset[2]);
        double power[3] = { 0., 0., 0. };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(0.9, power[0]);
        OCIO_CHECK_EQUAL(1.0, power[1]);
        OCIO_CHECK_EQUAL(1.2, power[2]);
        OCIO_CHECK_EQUAL(1.0f, transform->getSat());
    }
    {
        // No ID: return the first one.
        OCIO::CDLTransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filePath.c_str(), ""));
        OCIO_REQUIRE_ASSERT(transform);
        OCIO_CHECK_EQUAL(std::string(transform->getID()), "cc0001");
    }
    {
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = OCIO::CDLTransform::CreateGroupFromFile(filePath.c_str()));
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 5);
    }
    {
        // Wrong ID.
        OCIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "NotFound"),
            OCIO::Exception, "could not be loaded from the file");
    }
    {
        // Wrong index.
        OCIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "42"),
            OCIO::Exception, "is outside the valid range for this file [0,4]");
    }
}

OCIO_ADD_TEST(CDLTransform, create_from_cdl_file)
{
    // As warning messages are expected, please mute them.
    OCIO::MuteLogging mute;

    // Note: Detailed test is already done, this unit test only validates that
    // this CDL file (i.e. containing a ColorDecisionList) correctly loads
    // using a CDLTransform.

    const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_test1.cdl");
    {
        OCIO::CDLTransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "cc0003"));
        OCIO_REQUIRE_ASSERT(transform);
        OCIO_CHECK_EQUAL(std::string(transform->getID()), "cc0003");
        OCIO_CHECK_EQUAL(transform->getStyle(), OCIO::CDL_NO_CLAMP);
    }
    {
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = OCIO::CDLTransform::CreateGroupFromFile(filePath.c_str()));
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 5);
    }
}

namespace
{

struct FileGuard
{
    explicit FileGuard(unsigned lineNo)
    {
        OCIO_CHECK_NO_THROW_FROM(m_filename = OCIO::Platform::CreateTempFilename(""), lineNo);
    }
    ~FileGuard()
    {
        // Even if not strictly required on most OSes, perform the cleanup.
        std::remove(m_filename.c_str());
        OCIO::ClearAllCaches();
    }

    std::string m_filename;
};

} //anon.


OCIO_ADD_TEST(CDLTransform, escape_xml)
{
    const std::string inputXML(
        "<ColorCorrection id=\"Esc &lt; &amp; &quot; &apos; &gt;\">\n"
        "    <SOPNode>\n"
        "        <Description>These: &lt; &amp; &quot; &apos; &gt; are escape chars</Description>\n"
        "        <Slope>1.1 1.2 1.3</Slope>\n"
        "        <Offset>2.1 2.2 2.3</Offset>\n"
        "        <Power>3.1 3.2 3.3</Power>\n"
        "    </SOPNode>\n"
        "    <SatNode>\n"
        "        <Saturation>0.7</Saturation>\n"
        "    </SatNode>\n"
        "</ColorCorrection>");

    FileGuard guard(__LINE__);

    std::fstream stream(guard.m_filename, std::ios_base::out | std::ios_base::trunc);
    OCIO_REQUIRE_ASSERT(stream.is_open());
    stream << inputXML;
    stream.close();

    OCIO::CDLTransformRcPtr transform;
    OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(guard.m_filename.c_str(), ""));
    OCIO_REQUIRE_ASSERT(transform);

    {
        std::string idStr(transform->getID());
        OCIO_CHECK_EQUAL("Esc < & \" ' >", idStr);
        OCIO_CHECK_EQUAL(transform->getStyle(), OCIO::CDL_NO_CLAMP);

        std::string descStr(transform->getFirstSOPDescription());
        OCIO_CHECK_EQUAL("These: < & \" ' > are escape chars", descStr);
    }
}

namespace
{
static const std::string kContentsA = {
    "<ColorCorrectionCollection>\n"
    "    <ColorCorrection id=\"cc03343\">\n"
    "        <SOPNode>\n"
    "            <Slope>0.1 0.2 0.3 </Slope>\n"
    "            <Offset>0.8 0.1 0.3 </Offset>\n"
    "            <Power>0.5 0.5 0.5 </Power>\n"
    "        </SOPNode>\n"
    "        <SATNode>\n"
    "            <Saturation>1</Saturation>\n"
    "        </SATNode>\n"
    "    </ColorCorrection>\n"
    "    <ColorCorrection id=\"cc03344\">\n"
    "        <SOPNode>\n"
    "            <Slope>1.2 1.3 1.4 </Slope>\n"
    "            <Offset>0.3 0 0 </Offset>\n"
    "            <Power>0.75 0.75 0.75 </Power>\n"
    "        </SOPNode>\n"
    "        <SATNode>\n"
    "            <Saturation>1</Saturation>\n"
    "        </SATNode>\n"
    "    </ColorCorrection>\n"
    "</ColorCorrectionCollection>\n"
    };

static const std::string kContentsB = {
    "<ColorCorrectionCollection>\n"
    "    <ColorCorrection id=\"cc03343\">\n"
    "        <SOPNode>\n"
    "            <Slope>1.1 2.2 3.3 </Slope>\n"
    "            <Offset>0.8 0.1 0.3 </Offset>\n"
    "            <Power>0.5 0.5 0.5 </Power>\n"
    "        </SOPNode>\n"
    "        <SATNode>\n"
    "            <Saturation>1</Saturation>\n"
    "        </SATNode>\n"
    "    </ColorCorrection>\n"
    "    <ColorCorrection id=\"cc03344\">\n"
    "        <SOPNode>\n"
    "            <Slope>1.2 1.3 1.4 </Slope>\n"
    "            <Offset>0.3 0 0 </Offset>\n"
    "            <Power>0.75 0.75 0.75 </Power>\n"
    "        </SOPNode>\n"
    "        <SATNode>\n"
    "            <Saturation>1</Saturation>\n"
    "        </SATNode>\n"
    "    </ColorCorrection>\n"
    "</ColorCorrectionCollection>\n"
    };

}

OCIO_ADD_TEST(CDLTransform, clear_caches)
{
    FileGuard guard(__LINE__);

    std::fstream stream(guard.m_filename, std::ios_base::out|std::ios_base::trunc);
    OCIO_REQUIRE_ASSERT(stream.is_open());
    stream << kContentsA;
    stream.close();

    OCIO::CDLTransformRcPtr transform;
    OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(guard.m_filename.c_str(),
                                                                       "cc03343"));
    OCIO_REQUIRE_ASSERT(transform);
    double slope[3]{};

    OCIO_CHECK_NO_THROW(transform->getSlope(slope));
    OCIO_CHECK_EQUAL(slope[0], 0.1);
    OCIO_CHECK_EQUAL(slope[1], 0.2);
    OCIO_CHECK_EQUAL(slope[2], 0.3);

    stream.open(guard.m_filename, std::ios_base::out|std::ios_base::trunc);
    OCIO_REQUIRE_ASSERT(stream.is_open());
    stream << kContentsB;
    stream.close();

    OCIO_CHECK_NO_THROW(OCIO::ClearAllCaches());

    OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(guard.m_filename.c_str(),
                                                                       "cc03343"));
    OCIO_REQUIRE_ASSERT(transform);
    OCIO_CHECK_NO_THROW(transform->getSlope(slope));

    OCIO_CHECK_EQUAL(slope[0], 1.1);
    OCIO_CHECK_EQUAL(slope[1], 2.2);
    OCIO_CHECK_EQUAL(slope[2], 3.3);
}

OCIO_ADD_TEST(CDLTransform, faulty_file_content)
{
    FileGuard guard(__LINE__);

    {
        std::fstream stream(guard.m_filename, std::ios_base::out|std::ios_base::trunc);
        OCIO_REQUIRE_ASSERT(stream.is_open());
        stream << kContentsA << "Some Extra faulty information";
        stream.close();

        // Detailed parsing error is part of the debug log.
        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(guard.m_filename.c_str(), "cc03343"),
                              OCIO::Exception, "All formats have been tried");
    }
    OCIO::ClearAllCaches();
    {
        // Duplicated identifier.

        std::string faultyContent = kContentsA;
        const std::size_t found = faultyContent.find("cc03344");
        OCIO_CHECK_ASSERT(found!=std::string::npos);
        faultyContent.replace(found, strlen("cc03344"), "cc03343");

        std::fstream stream(guard.m_filename, std::ios_base::out|std::ios_base::trunc);
        OCIO_REQUIRE_ASSERT(stream.is_open());
        stream << faultyContent;
        stream.close();

        // Detailed parsing error is part of the debug log.
        OCIO::LogGuard logGuard;
        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(guard.m_filename.c_str(), "cc03343"),
                              OCIO::Exception,
                              "All formats have been tried");
        OCIO_CHECK_NE(logGuard.output().find("Error loading ccc xml. Duplicate elements with "
                                             "'cc03343' found"),
                      std::string::npos);
    }

    {
        const std::string filePath(OCIO::GetTestFilesDir() + "/cdl_various.ctf");
        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "0"),
                              OCIO::Exception, "Not a CDL file format");
    }
}

OCIO_ADD_TEST(CDLTransform, buildops)
{
    auto cdl = OCIO::CDLTransform::Create();

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    // For a v1 config, a CDL uses an exponent and two matrix ops rather than the CDL op
    // that was introduced in v2.
    config->setMajorVersion(1);

    OCIO::OpRcPtrVec ops;
    OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    ops.clear();
    const double power[]{1.1, 1.0, 1.0};
    cdl->setPower(power);
    OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
    auto expData = OCIO::DynamicPtrCast<const OCIO::ExponentOpData>(op->data());
    OCIO_REQUIRE_ASSERT(expData);

    ops.clear();
    cdl->setSat(1.5);
    OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
    expData = OCIO::DynamicPtrCast<const OCIO::ExponentOpData>(op->data());
    OCIO_REQUIRE_ASSERT(expData);
    op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[1]);
    auto matData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(matData);

    ops.clear();
    const double offset[]{ 0.0, 0.1, 0.0 };
    cdl->setOffset(offset);
    OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
    matData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(matData);
    op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[1]);
    expData = OCIO::DynamicPtrCast<const OCIO::ExponentOpData>(op->data());
    OCIO_REQUIRE_ASSERT(expData);
    op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[2]);
    matData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(matData);

    // Testing v2 onward behavior.
    config->setMajorVersion(2);
    ops.clear();
    OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
    auto cdlData = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
    OCIO_REQUIRE_ASSERT(cdlData);
}

OCIO_ADD_TEST(CDLTransform, description)
{
    auto cdl = OCIO::CDLTransform::Create();

    const std::string id("TestCDL");
    cdl->setID(id.c_str());

    const std::string initialDesc(cdl->getFirstSOPDescription());
    OCIO_CHECK_ASSERT(initialDesc.empty());

    auto & metadata = cdl->getFormatMetadata();
    metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "Desc");
    metadata.addChildElement(OCIO::METADATA_INPUT_DESCRIPTION, "Input Desc");
    const std::string sopDesc("SOP Desc");
    metadata.addChildElement(OCIO::METADATA_SOP_DESCRIPTION, sopDesc.c_str());
    metadata.addChildElement(OCIO::METADATA_SAT_DESCRIPTION, "Sat Desc");
    const std::string sopOther("Additional SOP");
    metadata.addChildElement(OCIO::METADATA_SOP_DESCRIPTION, sopOther.c_str());

    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 5);
    OCIO_CHECK_EQUAL(cdl->getFirstSOPDescription(), sopDesc);

    const std::string newSopDesc("SOP Desc New");
    cdl->setFirstSOPDescription(newSopDesc.c_str());

    OCIO_CHECK_EQUAL(cdl->getFirstSOPDescription(), newSopDesc);
    // The first SOP_DESCRIPTION has been replaced.
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 5);

    // Null description is removing the first SOP_DESCRIPTION.
    cdl->setFirstSOPDescription(nullptr);
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 4);
    // There is still a SOP description because there were 2.
    OCIO_CHECK_EQUAL(cdl->getFirstSOPDescription(), sopOther);
    // Removing the second one.
    cdl->setFirstSOPDescription(nullptr);
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 3);
    // SOP description is now gone.
    OCIO_CHECK_EQUAL(std::string(cdl->getFirstSOPDescription()), "");
}

OCIO_ADD_TEST(CDLTransform, style)
{
    auto cdl = OCIO::CDLTransform::Create();
    OCIO_CHECK_EQUAL(cdl->getStyle(), OCIO::CDL_TRANSFORM_DEFAULT);
    OCIO_CHECK_EQUAL(cdl->getStyle(), OCIO::CDL_NO_CLAMP);

    cdl->setStyle(OCIO::CDL_ASC);
    OCIO_CHECK_EQUAL(cdl->getStyle(), OCIO::CDL_ASC);
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    OCIO_CHECK_EQUAL(cdl->getStyle(), OCIO::CDL_NO_CLAMP);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO::ConstOpRcPtr op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto cdldata = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
        OCIO_REQUIRE_ASSERT(cdldata);
        OCIO_CHECK_EQUAL(cdldata->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO::ConstOpRcPtr op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto cdldata = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
        OCIO_REQUIRE_ASSERT(cdldata);
        OCIO_CHECK_EQUAL(cdldata->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
    }

    cdl->setStyle(OCIO::CDL_ASC);
    OCIO_CHECK_EQUAL(cdl->getStyle(), OCIO::CDL_ASC);

    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO::ConstOpRcPtr op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto cdldata = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
        OCIO_REQUIRE_ASSERT(cdldata);
        OCIO_CHECK_EQUAL(cdldata->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildCDLOp(ops, *config, *cdl, OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO::ConstOpRcPtr op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto cdldata = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
        OCIO_REQUIRE_ASSERT(cdldata);
        OCIO_CHECK_EQUAL(cdldata->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);
    }
}

OCIO_ADD_TEST(CDLTransform, apply_optimize_simplify)
{
    auto cdl = OCIO::CDLTransform::Create();
    static constexpr double slope[]{ 0.8, 0.9, 1.1 };
    cdl->setSlope(slope);
    static constexpr double offset[]{ 0.1, 0.05, -0.2 };
    cdl->setOffset(offset);
    cdl->setSat(1.23);
    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();
    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(cdl));
    OCIO_REQUIRE_ASSERT(proc);

    // Verify that non-simplified and simplified cpu processors are equivalent.

    OCIO::ConstCPUProcessorRcPtr cpu;
    const auto noSimplify = (OCIO::OptimizationFlags)(OCIO::OPTIMIZATION_DEFAULT &
                                                      ~OCIO::OPTIMIZATION_SIMPLIFY_OPS);
    OCIO_CHECK_NO_THROW(cpu = proc->getOptimizedCPUProcessor(noSimplify));
    OCIO_REQUIRE_ASSERT(cpu);
    static constexpr float source[]{ -0.1f, 0.5f, 1.5f };
    float pixNoSimplify[]{ source[0], source[1], source[2] };
    cpu->applyRGB(pixNoSimplify);

    OCIO_CHECK_NO_THROW(cpu = proc->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_ASSERT(cpu);
    float pixSimplify[]{ source[0], source[1], source[2] };
    cpu->applyRGB(pixSimplify);

    static constexpr float error = 2.e-5f;
    OCIO_CHECK_CLOSE(pixNoSimplify[0], pixSimplify[0], error);
    OCIO_CHECK_CLOSE(pixNoSimplify[1], pixSimplify[1], error);
    OCIO_CHECK_CLOSE(pixNoSimplify[2], pixSimplify[2], error);

    // Same in inverse direction.

    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(cdl));
    OCIO_REQUIRE_ASSERT(proc);
    OCIO_CHECK_NO_THROW(cpu = proc->getOptimizedCPUProcessor(noSimplify));
    OCIO_REQUIRE_ASSERT(cpu);
    pixNoSimplify[0] = source[0];
    pixNoSimplify[1] = source[1];
    pixNoSimplify[2] = source[2];
    cpu->applyRGB(pixNoSimplify);

    OCIO_CHECK_NO_THROW(cpu = proc->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_ASSERT(cpu);
    pixSimplify[0] = source[0];
    pixSimplify[1] = source[1];
    pixSimplify[2] = source[2];
    cpu->applyRGB(pixSimplify);

    OCIO_CHECK_CLOSE(pixNoSimplify[0], pixSimplify[0], error);
    OCIO_CHECK_CLOSE(pixNoSimplify[1], pixSimplify[1], error);
    OCIO_CHECK_CLOSE(pixNoSimplify[2], pixSimplify[2], error);
}
