// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/ColorSpaceTransform.cpp"

#include "ops/exponent/ExponentOp.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "ops/log/LogOpData.h"
#include "ops/matrix/MatrixOpData.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ColorSpaceTransform, basic)
{
    OCIO::ColorSpaceTransformRcPtr cst = OCIO::ColorSpaceTransform::Create();
    OCIO_CHECK_EQUAL(cst->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    cst->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(cst->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    const std::string empty;

    OCIO_CHECK_EQUAL(empty, cst->getSrc());
    const std::string src{ "source" };
    cst->setSrc(src.c_str());
    OCIO_CHECK_EQUAL(src, cst->getSrc());

    OCIO_CHECK_EQUAL(empty, cst->getDst());
    const std::string dst{ "destination" };
    cst->setDst(dst.c_str());
    OCIO_CHECK_EQUAL(dst, cst->getDst());

    OCIO_CHECK_NO_THROW(cst->validate());

    cst->setSrc("");
    OCIO_CHECK_THROW_WHAT(cst->validate(), OCIO::Exception,
                          "ColorSpaceTransform: empty source color space name");
    cst->setSrc(src.c_str());

    cst->setDst("");
    OCIO_CHECK_THROW_WHAT(cst->validate(), OCIO::Exception,
                          "ColorSpaceTransform: empty destination color space name");
    cst->setDst(dst.c_str());

    cst->setDirection(OCIO::TRANSFORM_DIR_UNKNOWN);
    OCIO_CHECK_THROW_WHAT(cst->validate(), OCIO::Exception, "invalid direction");
}

OCIO_ADD_TEST(ColorSpaceTransform, build_colorspace_ops)
{
    //
    // Prepare.
    //

    OCIO::ColorSpaceTransformRcPtr cst = OCIO::ColorSpaceTransform::Create();
    const std::string src{ "source" };
    cst->setSrc(src.c_str());
    const std::string dst{ "destination" };
    cst->setDst(dst.c_str());

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    auto csSceneToRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    csSceneToRef->setName(src.c_str());
    auto mat = OCIO::MatrixTransform::Create();
    const double offset[4]{ 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    csSceneToRef->setTransform(mat, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(csSceneToRef);

    auto csSceneFromRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    csSceneFromRef->setName(dst.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    csSceneFromRef->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(csSceneFromRef);

    config->addDisplay("display", "view", dst.c_str(), "");

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    {
        // Test from source to destination.
        // Source has the to_reference transform defined.
        // Destination has the from_reference transform defined.
        // Expecting source to_reference + destination from_reference.
        // (The no-ops are the Allocation transforms.)

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cst, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 4);

        // Allocation no-op.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // Src CS to reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));

        // Reference to dst CS.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        // Allocation no-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    {
        // Test in other direction.
        // Expecting destination-from reference inverted + source-to reference inverted.

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cst, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 4);

        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // Dst CS to reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_INV);

        // Reference to src CS.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        // Not finalized, opData is still inverse.
        OCIO_CHECK_EQUAL(matData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // Finalize converts invert matrix into forward matrix.
        OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));
        // No-ops are gone.
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        // Matrix is now forward.
        OCIO_CHECK_EQUAL(matData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        // Offset is inverted.
        OCIO_CHECK_EQUAL(-offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(-offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(-offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(-offset[3], matData->getOffsetValue(3));
    }

    {
        // Color space to reference ops: color space only defines from_reference, expecting the
        // inverse of that transform.

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceToReferenceOps(ops, *config,
                                                                config->getCurrentContext(),
                                                                csSceneFromRef));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_INV);
    }

    {
        // Color space from reference ops: color space defines from_reference, expecting that
        // transform.

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceFromReferenceOps(ops, *config,
                                                                  config->getCurrentContext(),
                                                                  csSceneFromRef));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);
    }

    {
        // Color space with both to_reference and from_reference transform defined. No inversion
        // is made.

        auto csSceneBoth = csSceneFromRef->createEditableCopy();
        ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
        csSceneBoth->setTransform(ff, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceFromReferenceOps(ops, *config,
                                                                  config->getCurrentContext(),
                                                                  csSceneBoth));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);
        ops.clear();
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceToReferenceOps(ops, *config,
                                                                config->getCurrentContext(),
                                                                csSceneBoth));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_10_FWD);
    }

    // Replace the 2 color spaces by display-referred color spaces.
    auto csDisplayToRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    csDisplayToRef->setName(src.c_str());
    csDisplayToRef->setTransform(mat, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(csDisplayToRef);

    auto csDisplayFromRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    csDisplayFromRef->setName(dst.c_str());
    csDisplayFromRef->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(csDisplayFromRef);

    // Because there are display-referred color spaces, there need to be a view transform.
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("view_transform");
    vt->setTransform(mat, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    // cst is now from csDisplayToRef to csDisplayFromRef.
    {
        // Still getting 4 ops. View transform is not used.

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cst, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 4);

        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::MatrixType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    {
        // Errors.

        cst = OCIO::ColorSpaceTransform::Create();
        cst->setSrc("source_missing");
        cst->setDst(dst.c_str());
    
        OCIO::OpRcPtrVec ops;
    
        OCIO_CHECK_THROW_WHAT(OCIO::BuildColorSpaceOps(ops, *config,
                                                       config->getCurrentContext(), *cst,
                                                       OCIO::TRANSFORM_DIR_FORWARD), OCIO::Exception,
                              "source color space 'source_missing' could not be found");
    
        cst = OCIO::ColorSpaceTransform::Create();
        cst->setSrc(src.c_str());
        cst->setDst("destination_missing");
    
        OCIO_CHECK_THROW_WHAT(OCIO::BuildColorSpaceOps(ops, *config,
                                                       config->getCurrentContext(), *cst,
                                                       OCIO::TRANSFORM_DIR_FORWARD), OCIO::Exception,
                              "destination color space 'destination_missing' could not be found");
    }
}

OCIO_ADD_TEST(ColorSpaceTransform, build_reference_conversion_ops)
{
    const std::string scn{ "scene" };

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    cs->setName(scn.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    config->addDisplay("display", "view", scn.c_str(), "");

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    //
    // No view transform.
    //
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildReferenceConversionOps(ops, *config,
                                                              config->getCurrentContext(), 
                                                              OCIO::REFERENCE_SPACE_SCENE,
                                                              OCIO::REFERENCE_SPACE_SCENE));
        OCIO_CHECK_EQUAL(ops.size(), 0);
    
        OCIO_CHECK_NO_THROW(OCIO::BuildReferenceConversionOps(ops, *config,
                                                              config->getCurrentContext(), 
                                                              OCIO::REFERENCE_SPACE_DISPLAY,
                                                              OCIO::REFERENCE_SPACE_DISPLAY));
        OCIO_CHECK_EQUAL(ops.size(), 0);
    
        OCIO_CHECK_THROW_WHAT(OCIO::BuildReferenceConversionOps(ops, *config,
                                                                config->getCurrentContext(),
                                                                OCIO::REFERENCE_SPACE_SCENE,
                                                                OCIO::REFERENCE_SPACE_DISPLAY),
                              OCIO::Exception,
                              "no view transform between the main scene-referred space and "
                              "the display-referred space");
        OCIO_CHECK_THROW_WHAT(OCIO::BuildReferenceConversionOps(ops, *config,
                                                                config->getCurrentContext(),
                                                                OCIO::REFERENCE_SPACE_DISPLAY,
                                                                OCIO::REFERENCE_SPACE_SCENE),
                              OCIO::Exception,
                              "no view transform between the main scene-referred space and "
                              "the display-referred space");
    }

    //
    // Add scene-referred view transform.
    //

    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("view_transform");
    auto mat = OCIO::MatrixTransform::Create();
    constexpr double offset[4] = { 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    vt->setTransform(mat, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildReferenceConversionOps(ops, *config,
                                                              config->getCurrentContext(),
                                                              OCIO::REFERENCE_SPACE_SCENE,
                                                              OCIO::REFERENCE_SPACE_DISPLAY));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 1);

        // Scene reference to display reference.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildReferenceConversionOps(ops, *config,
                                                              config->getCurrentContext(),
                                                              OCIO::REFERENCE_SPACE_DISPLAY,
                                                              OCIO::REFERENCE_SPACE_SCENE));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
    
        // Dispaly reference to scene reference.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(matData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));
    }
}

OCIO_ADD_TEST(ColorSpaceTransform, build_colorspace_ops_with_reference_conversion)
{
    const std::string scn{ "scene" };

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    cs->setName(scn.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    config->addDisplay("display", "view", scn.c_str(), "");

    // Add scene-referred view transform.
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("view_transform");
    auto mat = OCIO::MatrixTransform::Create();
    constexpr double offset[4] = { 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    vt->setTransform(mat, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    //
    // Add display-referred color space.
    //

    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    std::string dsp{ "display" };
    cs->setName(dsp.c_str());
    auto log = OCIO::LogTransform::Create();
    cs->setTransform(log, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO::ColorSpaceTransformRcPtr cst = OCIO::ColorSpaceTransform::Create();
    cst->setSrc(scn.c_str());
    cst->setDst(dsp.c_str());

    //
    // Color space to color space with reference space conversion.
    //

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config,
                                                     config->getCurrentContext(),
                                                     *cst,
                                                     OCIO::TRANSFORM_DIR_FORWARD));

        // Expecting 5 transforms (including 2 no-ops).
        OCIO_REQUIRE_EQUAL(ops.size(), 5);

        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // CS to scene reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_INV);

        // Scene reference to display reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));

        // Display reference to CS.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(logData->getBase(), 2.0);
        OCIO_CHECK_EQUAL(logData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    //
    // Same test in inverse direction.
    //

    {
        OCIO::OpRcPtrVec ops;

        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config,
                                                     config->getCurrentContext(),
                                                     *cst,
                                                     OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(ops.validate());

        // Expecting 5 transforms (including 2 no-ops).
        OCIO_REQUIRE_EQUAL(ops.size(), 5);

        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(logData->getBase(), 2.0);
        OCIO_CHECK_EQUAL(logData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

        // Display reference to scene reference. The view transform only defines the other
        // direction. Expecting the inverse of that transform.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(matData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    //
    // Add a to_reference transform to the view transform.
    //

    auto exp = OCIO::ExponentTransform::Create();
    vt->setTransform(exp, OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE);
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));
    {
        OCIO::OpRcPtrVec ops;

        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config,
                                                     config->getCurrentContext(),
                                                     *cst,
                                                     OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(ops.validate());

        // Expecting 5 transforms (including 2 no-ops).
        OCIO_REQUIRE_EQUAL(ops.size(), 5);

        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(logData->getBase(), 2.0);
        OCIO_CHECK_EQUAL(logData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

        // Display reference to scene reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExponentType);
        auto expData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExponentOpData>(data);
        OCIO_CHECK_EQUAL(expData->m_exp4[0], 1.0);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }
}
