// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/ColorSpaceTransform.cpp"

#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "ops/gamma/GammaOpData.h"
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

    OCIO_CHECK_EQUAL(true, cst->getDataBypass());
    cst->setDataBypass(false);
    OCIO_CHECK_EQUAL(false, cst->getDataBypass());
    cst->setDataBypass(true);
    OCIO_CHECK_EQUAL(true, cst->getDataBypass());

    OCIO_CHECK_NO_THROW(cst->validate());

    cst->setSrc("");
    OCIO_CHECK_THROW_WHAT(cst->validate(), OCIO::Exception,
                          "ColorSpaceTransform: empty source color space name");
    cst->setSrc(src.c_str());

    cst->setDst("");
    OCIO_CHECK_THROW_WHAT(cst->validate(), OCIO::Exception,
                          "ColorSpaceTransform: empty destination color space name");
    cst->setDst(dst.c_str());
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

    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    auto csSceneToRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    csSceneToRef->setName(src.c_str());
    auto mat = OCIO::MatrixTransform::Create();
    const double offset[4]{ 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    csSceneToRef->setTransform(mat, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(csSceneToRef);

    auto csSceneFromRef = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    csSceneFromRef->setName(dst.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    csSceneFromRef->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(csSceneFromRef);

    config->addDisplayView("display", "view", dst.c_str(), "");

    OCIO_CHECK_NO_THROW(config->validate());

    {
        // Test from source to destination.
        // Source has the to_scene_reference transform defined.
        // Destination has the from_scene_reference transform defined.
        // Expecting source to_scene_reference + destination from_scene_reference.
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
        // Add alias names to color spaces.

        csSceneToRef->addAlias("aliasToRef");
        config->addColorSpace(csSceneToRef);

        csSceneFromRef->addAlias("aliasFromRef");
        config->addColorSpace(csSceneFromRef);

        // Use aliases in transform.

        OCIO::ColorSpaceTransformRcPtr cstAlias = OCIO::ColorSpaceTransform::Create();
        cstAlias->setSrc("aliasToRef");
        cstAlias->setDst("aliasFromRef");

        // Same result as previous block.

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cstAlias, OCIO::TRANSFORM_DIR_FORWARD));
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

        // Reference to dst CS.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);

        // Allocation no-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    {
        // From a color space to the same one, identified by its name and an alias.

        OCIO::ColorSpaceTransformRcPtr cstAlias = OCIO::ColorSpaceTransform::Create();
        cstAlias->setSrc(src.c_str());
        cstAlias->setDst("aliasToRef");

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cstAlias, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }

    {
        // Test that data color space will not create a color space conversion unless the
        // color space transform forces data to be processed.

        csSceneToRef->setIsData(true);
        config->addColorSpace(csSceneToRef);

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cst, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_CHECK_EQUAL(ops.size(), 0);
        ops.clear();

        OCIO::ConstProcessorRcPtr proc;
        config->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_OFF); // Cache disabled
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(cst));
        OCIO_CHECK_EQUAL(proc->getNumTransforms(), 0);

        cst->setDataBypass(false);
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cst, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_CHECK_EQUAL(ops.size(), 4);
        ops.clear();

        // Some of the ops are no-ops, processor has in fact 2 transforms.
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(cst));
        // Remove the no-ops, since they are useless here.
        OCIO_CHECK_NO_THROW(proc = proc->getOptimizedProcessor(OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(proc->getNumTransforms(), 2);

        // Similar test with color space, data by-pass can't be controlled.
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(src.c_str(), dst.c_str()));
        OCIO_CHECK_EQUAL(proc->getNumTransforms(), 0);

        // Restore default data flags.
        csSceneToRef->setIsData(false);
        config->addColorSpace(csSceneToRef);
        cst->setDataBypass(true);

        // Same with destination color space.
        csSceneFromRef->setIsData(true);
        config->addColorSpace(csSceneFromRef);

        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cst, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_CHECK_EQUAL(ops.size(), 0);
        ops.clear();

        OCIO_CHECK_NO_THROW(proc = config->getProcessor(cst));
        OCIO_CHECK_EQUAL(proc->getNumTransforms(), 0);

        cst->setDataBypass(false);
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceOps(ops, *config, config->getCurrentContext(),
                                                     *cst, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_CHECK_EQUAL(ops.size(), 4);
        ops.clear();

        OCIO_CHECK_NO_THROW(proc = config->getProcessor(cst));
        // Remove the no-ops, since they are useless here.
        OCIO_CHECK_NO_THROW(proc = proc->getOptimizedProcessor(OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(proc->getNumTransforms(), 2);

        // Similar test with color space, data bypass can't be controlled.
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(src.c_str(), dst.c_str()));
        OCIO_CHECK_EQUAL(proc->getNumTransforms(), 0);

        // Restore default data flags.
        csSceneFromRef->setIsData(false);
        config->addColorSpace(csSceneFromRef);
        cst->setDataBypass(true);
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
        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));
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
        // inverse of that transform. Don't bypass data color spaces.

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceToReferenceOps(ops, *config,
                                                                config->getCurrentContext(),
                                                                csSceneFromRef, false));
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
                                                                  csSceneFromRef, true));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);
    }

    {
        // Color space with both to_scene_reference and from_scene_reference transform defined. No
        // inversion is made.

        auto csSceneBoth = csSceneFromRef->createEditableCopy();
        ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
        csSceneBoth->setTransform(ff, OCIO::COLORSPACE_DIR_TO_REFERENCE);

        // Make it a data colorspace.
        csSceneBoth->setIsData(true);

        // If data is bypassed, nothing is created.
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceFromReferenceOps(ops, *config,
                                                                  config->getCurrentContext(),
                                                                  csSceneBoth, true));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 0);

        // If data is not bypassed ops are created.
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceFromReferenceOps(ops, *config,
                                                                  config->getCurrentContext(),
                                                                  csSceneBoth, false));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        // Remove the data property and try in the other direction.
        csSceneBoth->setIsData(false);
        ops.clear();
        OCIO_CHECK_NO_THROW(OCIO::BuildColorSpaceToReferenceOps(ops, *config,
                                                                config->getCurrentContext(),
                                                                csSceneBoth, true));
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

    // 3 color spaces including "raw".
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);
    OCIO_CHECK_NO_THROW(config->validate());

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
                              "Color space 'source_missing' could not be found");
    
        cst = OCIO::ColorSpaceTransform::Create();
        cst->setSrc(src.c_str());
        cst->setDst("destination_missing");
    
        OCIO_CHECK_THROW_WHAT(OCIO::BuildColorSpaceOps(ops, *config,
                                                       config->getCurrentContext(), *cst,
                                                       OCIO::TRANSFORM_DIR_FORWARD), OCIO::Exception,
                              "Color space 'destination_missing' could not be found");
    }
}

OCIO_ADD_TEST(ColorSpaceTransform, build_reference_conversion_ops)
{
    const std::string scn{ "scene" };

    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    cs->setName(scn.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    config->addDisplayView("display", "view", scn.c_str(), "");

    OCIO_CHECK_NO_THROW(config->validate());

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

    OCIO_CHECK_NO_THROW(config->validate());

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

    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_SCENE);
    cs->setName(scn.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    config->addDisplayView("display", "view", scn.c_str(), "");

    // Add scene-referred view transform.
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("view_transform");
    auto mat = OCIO::MatrixTransform::Create();
    constexpr double offset[4] = { 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    vt->setTransform(mat, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));

    OCIO_CHECK_NO_THROW(config->validate());

    //
    // Add display-referred color space.
    //

    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    std::string dsp{ "display" };
    cs->setName(dsp.c_str());
    auto log = OCIO::LogTransform::Create();
    cs->setTransform(log, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);
    OCIO_CHECK_NO_THROW(config->validate());

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
    // Add a to_scene_reference transform to the view transform.
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

        // Display reference to scene reference. (ExponentTransform is implemented using a
        // GammaOpData from v2.)
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::GammaType);
        auto gammaData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GammaOpData>(data);
        OCIO_CHECK_ASSERT(gammaData->isIdentity());

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

OCIO_ADD_TEST(ColorSpaceTransform, context_variables)
{
    OCIO::ContextRcPtr usedContextVars = OCIO::Context::Create();

    OCIO::ConfigRcPtr cfg = OCIO::Config::CreateRaw()->createEditableCopy();
    cfg->setSearchPath(OCIO::GetTestFilesDir().c_str());
    OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    static constexpr double offset4[4] { 0.1, 0.2, 0.3, 0. };
    matrix->setOffset(offset4);

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    cs1->setTransform(matrix, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    cfg->addColorSpace(cs1);

    OCIO::ColorSpaceRcPtr cs2 = OCIO::ColorSpace::Create();
    cs2->setName("cs2");
    cs2->setTransform(matrix, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    cfg->addColorSpace(cs2);

    OCIO::ColorSpaceTransformRcPtr cst = OCIO::ColorSpaceTransform::Create();
    cst->setSrc("cs1");
    cst->setDst("cs2");

    OCIO::ColorSpaceRcPtr cs3 = OCIO::ColorSpace::Create();
    cs3->setName("cs3");
    cs3->setTransform(cst, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    cfg->addColorSpace(cs3);

    OCIO_CHECK_NO_THROW(cfg->validate());


    // Case 1 - No context variables.

    OCIO_CHECK_ASSERT(!OCIO::CollectContextVariables(*cfg.get(), *ctx, *cst, usedContextVars));
    OCIO_CHECK_EQUAL(0, usedContextVars->getNumStringVars());


    // Case 2 - The source color space name is now a context variable.

    OCIO_CHECK_NO_THROW(ctx->setStringVar("ENV1", "cs1"));
    cst->setSrc("$ENV1");
    usedContextVars = OCIO::Context::Create(); // New & empty instance.
    OCIO_CHECK_ASSERT(OCIO::CollectContextVariables(*cfg, *ctx, *cst, usedContextVars));
    OCIO_CHECK_EQUAL(1, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(std::string("ENV1"), usedContextVars->getStringVarNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("cs1"), usedContextVars->getStringVarByIndex(0));


    // Case 3 - A context variable exists but is not used.

    cst->setSrc("cs1");
    usedContextVars = OCIO::Context::Create(); // New & empty instance.
    OCIO_CHECK_ASSERT(!OCIO::CollectContextVariables(*cfg, *ctx, *cst, usedContextVars));
    OCIO_CHECK_EQUAL(0, usedContextVars->getNumStringVars());


    // Case 4 - Context variable indirectly used.

    OCIO_CHECK_NO_THROW(ctx->setStringVar("ENV1", "exposure_contrast_linear.ctf"));
    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc("$ENV1");
    OCIO::ColorSpaceRcPtr cs4 = OCIO::ColorSpace::Create();
    cs4->setName("cs4");
    cs4->setTransform(file, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    cfg->addColorSpace(cs4);

    // 'cst' now uses 'cs4' which is a FileTransform where the file name is a context variable. 
    cst->setSrc("cs4");
    usedContextVars = OCIO::Context::Create(); // New & empty instance.
    OCIO_CHECK_ASSERT(OCIO::CollectContextVariables(*cfg, *ctx, *cst, usedContextVars));
    OCIO_CHECK_EQUAL(1, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(std::string("ENV1"), usedContextVars->getStringVarNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("exposure_contrast_linear.ctf"),
                     usedContextVars->getStringVarByIndex(0));

    // Case 5 - Context variable indirectly used via a NamedTransform.

    OCIO::NamedTransformRcPtr namedTransform = OCIO::NamedTransform::Create();
    namedTransform->setName("nt");
    OCIO::FileTransformRcPtr file2 = OCIO::FileTransform::Create();
    file2->setSrc("$ENV1");
    namedTransform->setTransform(file2, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_NO_THROW(cfg->addNamedTransform(namedTransform));

    // 'cst' now uses 'nt' which is a NamedTransform whose transform uses a context variable.
    cst->setSrc("nt");
    usedContextVars = OCIO::Context::Create(); // New & empty instance.
    OCIO_CHECK_ASSERT(OCIO::CollectContextVariables(*cfg, *ctx, *cst, usedContextVars));
    OCIO_CHECK_EQUAL(1, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(std::string("ENV1"), usedContextVars->getStringVarNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("exposure_contrast_linear.ctf"),
                     usedContextVars->getStringVarByIndex(0));
}

// Please see (Config, named_transform_processor) in NamedTransform_tests.cpp for coverage of
// ColorSpaceTransform where the arguments are NamedTransforms.
