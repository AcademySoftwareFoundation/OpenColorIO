// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "MathUtils.h"
#include "ops/gamma/GammaOp.cpp"
#include "ParseUtils.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
void ApplyGamma(const OCIO::OpRcPtr & op, 
                float * image, const float * result,
                long numPixels, 
                float errorThreshold)
{
    OCIO_CHECK_NO_THROW(op->apply(image, numPixels));

    for(long idx=0; idx<(numPixels*4); ++idx)
    {
        // Using rel error with a large minExpected value of 1 will transition
        // from absolute error for expected values < 1 and
        // relative error for values > 1.
        const bool equalRel
            = OCIO::EqualWithSafeRelError(image[idx], result[idx],
                                          errorThreshold, 1.0f);
        if (!equalRel)
        {
            // As most of the error thresholds are 1e-7f, the output 
            // value precision should then be bigger than 7 digits 
            // to highlight small differences. 
            std::ostringstream message;
            message.precision(9);
            message << "Index: " << idx
                    << " - Values: " << image[idx]
                    << " and: " << result[idx]
                    << " - Threshold: " << errorThreshold;
            OCIO_CHECK_ASSERT_MESSAGE(0, message.str());
        }
    }
}

};

OCIO_ADD_TEST(GammaOp, apply_basic_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

    const std::vector<double> gammaVals = { 1.2, 2.12, 1.123, 1.05 };

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00001478f, 0.48297336f,
        0.00010933f, 0.00001323f, 0.03458935f, 0.73928129f,
        0.18946611f, 0.23005184f, 0.72391760f, 1.00001204f,
        0.76507961f, 0.89695119f, 1.00001264f, 1.53070319f,
        1.00601125f, 1.10895324f, 1.57668686f, 0.0f  };
#else
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00001478f, 0.48296818f,
        0.00010933f, 0.00001323f, 0.03458942f, 0.73928916f,
        powf(input_32f[12], (float)gammaVals[0]),
        powf(input_32f[13], (float)gammaVals[1]),
        powf(input_32f[14], (float)gammaVals[2]),
        powf(input_32f[15], (float)gammaVals[3]),
        powf(input_32f[16], (float)gammaVals[0]),
        powf(input_32f[17], (float)gammaVals[1]),
        powf(input_32f[18], (float)gammaVals[2]),
        powf(input_32f[19], (float)gammaVals[3]),
        1.00600302f, 1.10897374f, 1.57670521f, 0.0f  };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams  = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

OCIO_ADD_TEST(GammaOp, apply_basic_style_rev)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

    const std::vector<double> gammaVals = { 1.2, 2.12, 1.123, 1.05 };

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00014792f, 0.51678240f,
        0.00177476f, 0.08215060f, 0.06941742f, 0.76033723f, 
        0.31498342f, 0.72111737f, 0.77400052f, 1.00001109f, 
        0.83031141f, 0.97609287f, 1.00001061f, 1.47130167f, 
        1.00417137f, 1.02327621f, 1.43483067f, 0.0f };
#else
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00014792f, 0.51677888f,
        0.00177476f, 0.08215017f, 0.06941755f, 0.76034504f,
        powf(input_32f[12], (float)(1./gammaVals[0])),
        powf(input_32f[13], (float)(1./gammaVals[1])),
        powf(input_32f[14], (float)(1./gammaVals[2])),
        powf(input_32f[15], (float)(1./gammaVals[3])),
        powf(input_32f[16], (float)(1./gammaVals[0])),
        powf(input_32f[17], (float)(1./gammaVals[1])),
        powf(input_32f[18], (float)(1./gammaVals[2])),
        powf(input_32f[19], (float)(1./gammaVals[3])),
        1.00416493f, 1.02328109f, 1.43484282f, 0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams  = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

OCIO_ADD_TEST(GammaOp, apply_moncurve_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        -0.07738016f, -0.33144456f, -0.20408163f,  0.0f,
        -0.00019345f,  0.0f,         0.00004081f,  0.49101364f, 
         0.00003869f,  0.00220963f,  0.04081632f,  0.73652046f,
         0.05087645f,  0.30550804f,  0.67475068f,  1.00001871f,
         0.60383129f,  0.91060406f,  1.00002050f,  1.63147723f,
         1.01142657f,  1.09394502f,  1.84183871f, -0.24550682f };
#else
    const float expected_32f[numPixels*4] = {
        -0.07738015f, -0.33144456f, -0.20408163f,  0.0f,
        -0.00019345f,  0.0f,         0.00004081f,  0.49101364f, 
         0.00003869f,  0.00220963f,  0.04081632f,  0.73652046f,
         0.05087607f,  0.30550399f,  0.67474484f,  1.0f,
         0.60382729f,  0.91061854f,  1.0f,         1.63146877f,
         1.01141202f,  1.09396457f,  1.84183657f, -0.24550682f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { 2.4, 0.055 };
    const OCIO::GammaOpData::Params greenParams = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params blueParams  = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params alphaParams = { 1.8, 0.6 };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_FWD,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

OCIO_ADD_TEST(GammaOp, apply_moncurve_style_rev)
{
    const float errorThreshold = 1e-6f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        -6.18606853f, -1.69711625f, -0.30625000f,  0.0f,
        -0.01546517f,  0.0f,         0.00006125f,  0.50915080f,
         0.00309303f,  0.01131410f,  0.06125000f,  0.76366448f,
         0.51735591f,  0.67569005f,  0.81243133f,  1.00001215f,
         0.90233862f,  0.97234255f,  1.00000989f,  1.40423023f,
         1.00229334f,  1.02690458f,  1.31464004f, -0.25457540f };
#else
    const float expected_32f[numPixels*4] = {
        -6.18606853f, -1.69711625f, -0.30625000f,  0.0f,
        -0.01546517f,  0.0f,         0.00006125f,  0.50915080f,
         0.00309303f,  0.01131410f,  0.06125000f,  0.76367092f,
         0.51735413f,  0.67568808f,  0.81243550f,  1.0f,

#ifdef _WIN32
         0.90233647f,  0.97234553f,  1.0f,         1.40423405f,
#else
         0.90233647f,  0.97234553f,  1.0f,         1.40423429f,
#endif

         1.00228834f,  1.02691006f,  1.31464290f, -0.25457540f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { 2.4, 0.1 };
    const OCIO::GammaOpData::Params greenParams = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params blueParams  = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params alphaParams = { 1.8, 0.6 };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_REV,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

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
    auto & child1 = info1.addChildElement("Gamma1Child", "Some content");

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
    auto & child2 = info2.addChildElement("Gamma2Child", "Other content");

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

OCIO_ADD_TEST(GammaOp, is_inverse)
{
    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { 1.001 };
    const OCIO::GammaOpData::Params greenParams = { 1. };
    const OCIO::GammaOpData::Params blueParams  = { 1. };
    const OCIO::GammaOpData::Params alphaParams = { 1. };

    auto gamma1 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                      redParams,
                                                      greenParams,
                                                      blueParams,
                                                      alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma1, OCIO::TRANSFORM_DIR_FORWARD));

    auto gamma2 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV,
                                                      redParams,
                                                      greenParams,
                                                      blueParams,
                                                      alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));
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

    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[1]->getCacheID());

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_EQUAL(ops.size(), 3);

    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[1]->getCacheID() == ops[2]->getCacheID());

    auto gamma3 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV,
                                                      redParams,
                                                      greenParams,
                                                      blueParams,
                                                      alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma3, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[3]->getCacheID());
    OCIO_CHECK_ASSERT(ops[1]->getCacheID() != ops[3]->getCacheID());
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[3]->getCacheID());
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
}

// Still need bit-depth coverage from these tests:
//      gammaOp_basicfwd
//      gammaOp_basicrev
//      gammaOp_moncurvefwd
//      gammaOp_moncurverev

