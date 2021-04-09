// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "MathUtils.h"
#include "ops/gamma/GammaOp.cpp"
#include "ParseUtils.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GammaOp, combining)
{
    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params param1R{ 1.201 };
    const OCIO::GammaOpData::Params param1G{ 1.201 };
    const OCIO::GammaOpData::Params param1B{ 1.201 };
    const OCIO::GammaOpData::Params param1A{ 1. };

    OCIO::GammaOpDataRcPtr gammaData1
        = std::make_shared<OCIO::GammaOpData>(
            OCIO::GammaOpData::BASIC_FWD,
            param1R, param1G, param1B, param1A);

    OCIO::FormatMetadataImpl & info1 = gammaData1->getFormatMetadata();
    info1.addAttribute(OCIO::METADATA_NAME, "gamma1");
    info1.addAttribute(OCIO::METADATA_ID, "ID1");
    info1.addAttribute("Attrib", "1");
    info1.addAttribute("Attrib1", "10");
    info1.addChildElement("Gamma1Child", "Some content");
    auto & child1 = info1.getChildElement(0);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData1, OCIO::TRANSFORM_DIR_FORWARD));

    const OCIO::GammaOpData::Params param2R{ 2.345 };
    const OCIO::GammaOpData::Params param2G{ 2.345 };
    const OCIO::GammaOpData::Params param2B{ 2.345 };
    const OCIO::GammaOpData::Params param2A{ 1. };

    OCIO::GammaOpDataRcPtr gammaData2
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                              param2R, param2G, param2B, param2A);

    OCIO::FormatMetadataImpl & info2 = gammaData2->getFormatMetadata();
    info2.addAttribute(OCIO::METADATA_NAME, "gamma2");
    info2.addAttribute(OCIO::METADATA_ID, "ID2");
    info2.addAttribute("Attrib", "2");
    info2.addAttribute("Attrib2", "20");
    info2.addChildElement("Gamma2Child", "Other content");
    auto & child2 = info2.getChildElement(0);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(ops[0]->canCombineWith(op1));
    OCIO_CHECK_NO_THROW(ops[0]->combineWith(ops, op1));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op2 = ops[2];

    auto combinedData = op2->data();

    // Check metadata of combined op.
    OCIO_CHECK_EQUAL(combinedData->getName(), "gamma1 + gamma2");
    OCIO_CHECK_EQUAL(combinedData->getID(), "ID1 + ID2");
    // 5 attributes: name, id, Attrib, Attrib1 and  Attrib2.
    OCIO_CHECK_EQUAL(combinedData->getFormatMetadata().getNumAttributes(), 5);
    auto & attribs = combinedData->getFormatMetadata().getAttributes();
    OCIO_CHECK_EQUAL(attribs[2].first, "Attrib");
    OCIO_CHECK_EQUAL(attribs[2].second, "1 + 2");
    OCIO_CHECK_EQUAL(attribs[3].first, "Attrib1");
    OCIO_CHECK_EQUAL(attribs[3].second, "10");
    OCIO_CHECK_EQUAL(attribs[4].first, "Attrib2");
    OCIO_CHECK_EQUAL(attribs[4].second, "20");
    auto & children = combinedData->getFormatMetadata().getChildrenElements();
    OCIO_REQUIRE_EQUAL(children.size(), 2);
    OCIO_CHECK_ASSERT(children[0] == child1);
    OCIO_CHECK_ASSERT(children[1] == child2);

    OCIO_REQUIRE_EQUAL(op2->data()->getType(), OCIO::OpData::GammaType);

    OCIO::ConstGammaOpDataRcPtr g = OCIO::DynamicPtrCast<const OCIO::GammaOpData>(op2->data());

    OCIO_CHECK_EQUAL(g->getRedParams()[0], param1R[0] * param2R[0]);
    OCIO_CHECK_EQUAL(g->getGreenParams()[0], param1G[0] * param2G[0]);
    OCIO_CHECK_EQUAL(g->getBlueParams()[0], param1B[0]* param2B[0]);
    OCIO_CHECK_EQUAL(g->getAlphaParams()[0], param1A[0]* param2A[0]);
}

OCIO_ADD_TEST(GammaOp, basic)
{
    const OCIO::GammaOpData::Params redParams = { 1.001 };
    const OCIO::GammaOpData::Params greenParams = { 1. };
    const OCIO::GammaOpData::Params blueParams = { 2. };
    const OCIO::GammaOpData::Params alphaParams = { 1. };

    OCIO::GammaOpDataRcPtr gamma1
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                              redParams,
                                              greenParams,
                                              blueParams,
                                              alphaParams);
    const OCIO::GammaOp op0(gamma1);

    OCIO_CHECK_EQUAL(op0.data()->getType(), OCIO::OpData::GammaType);
    auto gammaData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GammaOpData>(op0.data());
    OCIO_REQUIRE_ASSERT(gammaData);
    OCIO_CHECK_EQUAL(gammaData->getStyle(), OCIO::GammaOpData::BASIC_FWD);
    OCIO_CHECK_ASSERT(redParams   == gammaData->getRedParams());
    OCIO_CHECK_ASSERT(greenParams == gammaData->getGreenParams());
    OCIO_CHECK_ASSERT(blueParams  == gammaData->getBlueParams());
    OCIO_CHECK_ASSERT(alphaParams == gammaData->getAlphaParams());

    // Test isInverse, see also OCIO_ADD_TEST(GammaOpData, is_inverse).
    OCIO::OpRcPtrVec ops;
    auto gamma2 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV,
                                                      redParams,
                                                      greenParams,
                                                      blueParams,
                                                      alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op1 = ops[0];
    OCIO_CHECK_ASSERT(op0.isInverse(op1));
}

OCIO_ADD_TEST(GammaOp, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { 1.001 };
    OCIO::GammaOpData::Params greenParams       = { 1. };
    const OCIO::GammaOpData::Params blueParams  = { 1. };
    const OCIO::GammaOpData::Params alphaParams = { 1. };

    auto gamma1 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                      redParams,
                                                      greenParams,
                                                      blueParams,
                                                      alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma1, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_EQUAL(ops.size(), 1);

    greenParams[0] = 1.001;
    auto gamma2 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                      redParams,
                                                      greenParams,
                                                      blueParams,
                                                      alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(ops.validate());

    std::string id0, id1;
    OCIO_CHECK_NO_THROW(id0 = ops[0]->getCacheID());
    OCIO_CHECK_NO_THROW(id1 = ops[1]->getCacheID());
    OCIO_CHECK_ASSERT(id0 != id1);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_EQUAL(ops.size(), 3);

    OCIO_CHECK_NO_THROW(ops.validate());

    std::string id2;
    OCIO_CHECK_NO_THROW(id2 = ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(id0 != id2);
    OCIO_CHECK_ASSERT(id1 == id2);

    auto gamma3 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV,
                                                      redParams,
                                                      greenParams,
                                                      blueParams,
                                                      alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma3, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO_CHECK_NO_THROW(ops.validate());

    std::string id3;
    OCIO_CHECK_NO_THROW(id3 = ops[3]->getCacheID());
    OCIO_CHECK_ASSERT(id0 != id3);
    OCIO_CHECK_ASSERT(id1 != id3);
    OCIO_CHECK_ASSERT(id2 != id3);
}

OCIO_ADD_TEST(GammaOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    const OCIO::GammaOpData::Params red{2., 0.2};
    const OCIO::GammaOpData::Params green{3., 0.3};
    const OCIO::GammaOpData::Params blue{4., 0.4};
    const OCIO::GammaOpData::Params alpha{2.5, 0.25};

    OCIO::GammaOpDataRcPtr gamma
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_FWD,
                                              red, green, blue, alpha);

    gamma->getFormatMetadata().addAttribute("name", "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateGammaTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto gTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentWithLinearTransform>(transform);
    OCIO_REQUIRE_ASSERT(gTransform);

    OCIO_CHECK_EQUAL(gTransform->getNegativeStyle(), OCIO::NEGATIVE_LINEAR);

    const auto & metadata = gTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(gTransform->getDirection(), direction);
    double gval[4];
    gTransform->getGamma(gval);
    OCIO_CHECK_EQUAL(gval[0], red[0]);
    OCIO_CHECK_EQUAL(gval[1], green[0]);
    OCIO_CHECK_EQUAL(gval[2], blue[0]);
    OCIO_CHECK_EQUAL(gval[3], alpha[0]);

    double oval[4];
    gTransform->getOffset(oval);
    OCIO_CHECK_EQUAL(oval[0], red[1]);
    OCIO_CHECK_EQUAL(oval[1], green[1]);
    OCIO_CHECK_EQUAL(oval[2], blue[1]);
    OCIO_CHECK_EQUAL(oval[3], alpha[1]);

    const OCIO::GammaOpData::Params red0{ 2. };
    const OCIO::GammaOpData::Params green0{ 3. };
    const OCIO::GammaOpData::Params blue0{ 4. };
    const OCIO::GammaOpData::Params alpha0{ 2.5 };

    OCIO::GammaOpDataRcPtr gamma0
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV,
                                              red0, green0, blue0, alpha0);

    gamma0->getFormatMetadata().addAttribute("name", "test");

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma0, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);

    OCIO::ConstOpRcPtr op0(ops[1]);

    OCIO::CreateGammaTransform(group, op0);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 2);
    auto transform0 = group->getTransform(1);
    OCIO_REQUIRE_ASSERT(transform0);
    auto eTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentTransform>(transform0);
    OCIO_REQUIRE_ASSERT(eTransform);

    const auto & metadata0 = eTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata0.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata0.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata0.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(eTransform->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    double expVal[4]{ 0.0 };
    eTransform->getValue(expVal);
    OCIO_CHECK_EQUAL(expVal[0], red0[0]);
    OCIO_CHECK_EQUAL(expVal[1], green0[0]);
    OCIO_CHECK_EQUAL(expVal[2], blue0[0]);
    OCIO_CHECK_EQUAL(expVal[3], alpha0[0]);

    OCIO::GammaOpDataRcPtr gamma1
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_MIRROR_FWD,
                                              red, green, blue, alpha);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma1, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_REQUIRE_ASSERT(ops[2]);

    OCIO::ConstOpRcPtr op1(ops[2]);

    OCIO::CreateGammaTransform(group, op1);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 3);
    auto transform1 = group->getTransform(2);
    OCIO_REQUIRE_ASSERT(transform1);
    auto elTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentWithLinearTransform>(transform1);
    OCIO_REQUIRE_ASSERT(elTransform);
    OCIO_CHECK_EQUAL(elTransform->getNegativeStyle(), OCIO::NEGATIVE_MIRROR);
}

