// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/DisplayTransform.cpp"

#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "ops/gamma/GammaOpData.h"
#include "ops/log/LogOpData.h"
#include "ops/matrix/MatrixOpData.h"
#include "ops/range/RangeOpData.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(DisplayTransform, basic)
{
    const std::string empty;
    auto dt = OCIO::DisplayTransform::Create();

    OCIO_CHECK_EQUAL(dt->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(empty, dt->getInputColorSpaceName());
    OCIO_CHECK_ASSERT(!dt->getLinearCC());
    OCIO_CHECK_ASSERT(!dt->getColorTimingCC());
    OCIO_CHECK_ASSERT(!dt->getChannelView());
    OCIO_CHECK_EQUAL(empty, dt->getDisplay());
    OCIO_CHECK_EQUAL(empty, dt->getView());
    OCIO_CHECK_ASSERT(!dt->getDisplayCC());
    OCIO_CHECK_EQUAL(empty, dt->getLooksOverride());
    OCIO_CHECK_ASSERT(!dt->getLooksOverrideEnabled());

    const std::string inputCS{ "inputCS" };
    dt->setInputColorSpaceName(inputCS.c_str());
    OCIO_CHECK_EQUAL(inputCS, dt->getInputColorSpaceName());

    const std::string display{ "display" };
    dt->setDisplay(display.c_str());
    OCIO_CHECK_EQUAL(display, dt->getDisplay());

    const std::string view{ "view" };
    dt->setView(view.c_str());
    OCIO_CHECK_EQUAL(view, dt->getView());

    OCIO_CHECK_NO_THROW(dt->validate());

    dt->setDirection(OCIO::TRANSFORM_DIR_UNKNOWN);
    OCIO_CHECK_THROW_WHAT(dt->validate(), OCIO::Exception, "invalid direction");
    dt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(dt->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    dt->setInputColorSpaceName("");
    OCIO_CHECK_THROW_WHAT(dt->validate(), OCIO::Exception,
                          "DisplayTransform: empty input color space name");
    dt->setInputColorSpaceName(inputCS.c_str());

    dt->setDisplay("");
    OCIO_CHECK_THROW_WHAT(dt->validate(), OCIO::Exception, "DisplayTransform: empty display name");
    dt->setDisplay(display.c_str());

    dt->setView("");
    OCIO_CHECK_THROW_WHAT(dt->validate(), OCIO::Exception, "DisplayTransform: empty view name");
    dt->setView(view.c_str());

    OCIO_CHECK_NO_THROW(dt->validate());

    OCIO::ConstTransformRcPtr linearCC = OCIO::MatrixTransform::Create();
    dt->setLinearCC(linearCC);
    auto linearCCBack = dt->getLinearCC();
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixTransform>(linearCCBack));

    OCIO::ConstTransformRcPtr timimgCC = OCIO::ExponentTransform::Create();
    dt->setColorTimingCC(timimgCC);
    auto timingCCBack = dt->getColorTimingCC();
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExponentTransform>(timingCCBack));

    OCIO::ConstTransformRcPtr cvTrans = OCIO::MatrixTransform::Create();
    dt->setChannelView(cvTrans);
    auto cvTransBack = dt->getChannelView();
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixTransform>(cvTransBack));

    OCIO::ConstTransformRcPtr displayCC = OCIO::RangeTransform::Create();
    dt->setDisplayCC(displayCC);
    auto displayCCBack = dt->getDisplayCC();
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::RangeTransform>(displayCCBack));

    const std::string looksOveride{ "looks_override" };
    dt->setLooksOverride(looksOveride.c_str());
    OCIO_CHECK_EQUAL(looksOveride, dt->getLooksOverride());

    dt->setLooksOverrideEnabled(true);
    OCIO_CHECK_ASSERT(dt->getLooksOverrideEnabled());
}

OCIO_ADD_TEST(DisplayTransform, build_ops)
{
    //
    // Validate BuildDisplayOps where the display/view is a simple color space
    // (i.e., no ViewTransform).
    //

    const std::string src{ "source" };
    const std::string dst{ "destination" };
    const std::string linearCS{ "linear_cs" };
    const std::string timingCS{ "color_timing_cs" };

    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    auto csSource = OCIO::ColorSpace::Create();
    csSource->setName(src.c_str());
    auto mat = OCIO::MatrixTransform::Create();
    constexpr double offset[4] = { 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    csSource->setTransform(mat, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(csSource);

    auto cs = OCIO::ColorSpace::Create();
    cs->setName(dst.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    cs = OCIO::ColorSpace::Create();
    cs->setName(linearCS.c_str());
    ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(cs);
    config->setRole(OCIO::ROLE_SCENE_LINEAR, linearCS.c_str());

    cs = OCIO::ColorSpace::Create();
    cs->setName(timingCS.c_str());
    ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(cs);
    config->setRole(OCIO::ROLE_COLOR_TIMING, timingCS.c_str());

    const std::string display{ "display" };
    const std::string view{ "view" };
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), view.c_str(), dst.c_str(), ""));

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    auto dt = OCIO::DisplayTransform::Create();
    dt->setInputColorSpaceName(src.c_str());

    dt->setDisplay(display.c_str());
    dt->setView(view.c_str());

    auto linearCC = OCIO::MatrixTransform::Create();
    constexpr double offsetLinearCC[4] = { 0.2, 0.3, 0.4, 0. };
    linearCC->setOffset(offsetLinearCC);
    dt->setLinearCC(linearCC);
    auto timimgCC = OCIO::ExponentTransform::Create();
    constexpr double valueTimingCC[4] = { 2.2, 2.3, 2.4, 1. };
    timimgCC->setValue(valueTimingCC);
    dt->setColorTimingCC(timimgCC);
    auto cvTrans = OCIO::MatrixTransform::Create();
    const std::string cv{ "channel view transform" };
    cvTrans->getFormatMetadata().setValue(cv.c_str());
    dt->setChannelView(cvTrans);
    auto displayCC = OCIO::ExposureContrastTransform::Create();
    dt->setDisplayCC(displayCC);

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_EQUAL(ops.size(), 16);
        OCIO_CHECK_NO_THROW(ops.validate());

        // 0-3. InputCS -> scene linear role:
        //     0. GPU Allocation No-op.
        //     1. Input to reference.
        //     2. Scene linear role from reference.
        //     3. GPU Allocation No-op.
        // 4. LinearCC.
        // 5-8. Scene linear -> color timing role:
        //     5. GPU Allocation No-op.
        //     6. Scene linear role to reference.
        //     7. ColorTiming from reference. 
        //     8. GPU Allocation No-op.
        // 9. ColorTimingCC.
        // * No look.
        // 10. ChannelView.
        // 11-14. Color timing role -> display/view color space:
        //     11. GPU Allocation No-op.
        //     12. ColorTiming to reference.
        //     13. DisplayCS from reference.
        //     14. GPU Allocation No-op.
        // 15. DisplayCC.

        // 0. GPU Allocation No-op.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 1. Input to reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));

        // 2. Scene linear role from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_10_FWD);

        // 3. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 4. LinearCC.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(offsetLinearCC[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offsetLinearCC[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offsetLinearCC[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offsetLinearCC[3], matData->getOffsetValue(3));

        // 5. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[5]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 6. Scene linear role to reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);

        // 7. ColorTiming from reference. 
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[7]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::RGB_TO_HSV);

        // 8. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[8]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 9. ColorTimingCC.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[9]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::GammaType);
        auto gammaData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GammaOpData>(data);
        OCIO_CHECK_EQUAL(valueTimingCC[0], gammaData->getRedParams()[0]);
        OCIO_CHECK_EQUAL(valueTimingCC[1], gammaData->getGreenParams()[0]);
        OCIO_CHECK_EQUAL(valueTimingCC[2], gammaData->getBlueParams()[0]);
        OCIO_CHECK_EQUAL(valueTimingCC[3], gammaData->getAlphaParams()[0]);

        // 10. ChannelView.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[10]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        OCIO_CHECK_EQUAL(cv, data->getFormatMetadata().getValue());

        // 11. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[11]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 12. ColorTiming to reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[12]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD);

        // 13. DisplayCS from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[13]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        // 14. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[14]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 15. DisplayCC.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[15]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);
    }

    //
    // Using a scene-referred ViewTransform.
    //

    const std::string dsp{ "display" };
    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    cs->setName(dsp.c_str());
    auto ec = OCIO::ExposureContrastTransform::Create();
    cs->setTransform(ec, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    const std::string scenevt{ "scene_vt" };
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName(scenevt.c_str());
    auto log = OCIO::LogTransform::Create();
    log->setBase(4.2);
    vt->setTransform(log, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    config->addViewTransform(vt);

    const std::string viewt{ "viewt" };
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), viewt.c_str(), scenevt.c_str(), dsp.c_str(), ""));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    dt->setView(viewt.c_str());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        // Getting an additional op for the reference space change.
        OCIO_CHECK_EQUAL(ops.size(), 17);
        OCIO_CHECK_NO_THROW(ops.validate());

        // Same as previous up to colorTiming to reference.
        //  0. GPU Allocation No-op.
        //  1. Input to reference.
        //  2. Scene linear role from reference.
        //  3. GPU Allocation No-op.
        //  4. LinearCC.
        //  5. GPU Allocation No-op.
        //  6. Scene linear role to reference.
        //  7. ColorTiming from reference. 
        //  8. GPU Allocation No-op.
        //  9. ColorTimingCC.
        // 10. ChannelView.
        // 11. GPU Allocation No-op.
        // 12. ColorTiming to reference.

        // 13. Changing from scene-referred space to display-referred space done
        // with the specified view transform.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[13]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(4.2, logData->getBase());

        // 14. DisplayCS from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[14]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);

        // 15. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[15]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 16. DisplayCC.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[16]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);

    }

    //
    // Adding a display-referred ViewTransform.
    //

    const std::string displayvt{ "display_vt" };
    vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    vt->setName(displayvt.c_str());
    log = OCIO::LogTransform::Create();
    log->setBase(2.1);
    vt->setTransform(log, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    config->addViewTransform(vt);

    // Replace view display.
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), viewt.c_str(), displayvt.c_str(), dsp.c_str(), ""));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        // Getting an additional op for the display to display view transform.
        OCIO_CHECK_EQUAL(ops.size(), 18);
        OCIO_CHECK_NO_THROW(ops.validate());

        // Same as previous up to scene-referred to display referred.
        // 0. GPU Allocation No-op.
        //  1. Input to reference.
        //  2. Scene linear role from reference.
        //  3. GPU Allocation No-op.
        //  4. LinearCC.
        //  5. GPU Allocation No-op.
        //  6. Scene linear role to reference.
        //  7. ColorTiming from reference. 
        //  8. GPU Allocation No-op.
        //  9. ColorTimingCC.
        // 10. ChannelView.
        // 11. GPU Allocation No-op.
        // 12. ColorTiming to reference.
        // 13. Changing from scene-referred space to display-referred space using the
        //     default view transform.

        // 14. Display-referred reference to display-referred reference using the specified view transform.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[14]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(2.1, logData->getBase());

        // 15. DisplayCS from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[15]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);

        // 16. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[16]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 17. DisplayCC.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[17]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);
    }

    csSource->setIsData(true);
    config->addColorSpace(csSource);
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        // Color space conversion is skipped.
        OCIO_CHECK_EQUAL(ops.size(), 4);
        OCIO_CHECK_NO_THROW(ops.validate());

        // With isData true, the view/display transform is not applied.  The CC and channelView
        // are applied, but without converting to their usual process spaces.
        // 0. LinearCC.
        // 1. ColorTimingCC.
        // 2. ChannelView.
        // 3. DisplayCC.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::GammaType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);

        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);
    }
}

OCIO_ADD_TEST(DisplayTransform, build_ops_with_looks)
{
    //
    // Validate BuildDisplayOps using a display-referred ViewTransform and a Look with a
    // display-referred ProcessSpace.
    //

    const std::string in{ "displayCSIn" };
    const std::string out{ "displayCSOut" };

    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    auto csIn = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    csIn->setName(in.c_str());
    auto mat = OCIO::MatrixTransform::Create();
    constexpr double offset[4] = { 0., 0.1, 0.2, 0. };
    mat->setOffset(offset);
    csIn->setTransform(mat, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(csIn);

    auto csOut = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    csOut->setName(out.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    csOut->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(csOut);

    const std::string display{ "display" };
    const std::string view{ "view" };
    const std::string look{ "look" };
    const std::string displayCSProcess{ "displayCSProcess" };

    auto csProcess = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    csProcess->setName(displayCSProcess.c_str());
    auto expProcess = OCIO::ExponentTransform::Create();
    constexpr double valueProcess[4] = { 2.2, 2.3, 2.4, 1. };
    expProcess->setValue(valueProcess);
    csProcess->setTransform(expProcess, OCIO::COLORSPACE_DIR_FROM_REFERENCE);

    config->addColorSpace(csProcess);

    // Create look.
    auto lk = OCIO::Look::Create();
    lk->setName(look.c_str());
    lk->setProcessSpace(displayCSProcess.c_str());
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setSat(1.5);
    lk->setTransform(cdl);
    config->addLook(lk);

    auto defaultVT = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    defaultVT->setName("default_vt");
    mat = OCIO::MatrixTransform::Create();
    defaultVT->setTransform(mat, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    config->addViewTransform(defaultVT);

    // Add display-referred view transform.
    const std::string displayvt{ "display_vt" };
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    vt->setName(displayvt.c_str());
    auto log = OCIO::LogTransform::Create();
    log->setBase(2.1);
    vt->setTransform(log, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    config->addViewTransform(vt);

    // Add view display.
    OCIO_CHECK_NO_THROW(config->addDisplay(display.c_str(), view.c_str(), displayvt.c_str(),
                                           out.c_str(), look.c_str()));

    auto dt = OCIO::DisplayTransform::Create();
    dt->setInputColorSpaceName(in.c_str());
    dt->setDisplay(display.c_str());
    dt->setView(view.c_str());

    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 11);
    OCIO_CHECK_NO_THROW(ops.validate());

    // 0-3. DisplayCSIn->displayCSProcess:
    //     0. GPU Allocation No-op.
    //     1. In to reference.
    //     2. Look process space from reference.
    //     3. GPU Allocation No-op.
    // 4-5. Look-noop + look transform.
    // 6-7. displayCSProcess->display reference:
    //     6. GPU Allocation No-op.
    //     7. DisplayCSProcess to display reference.
    // 8. Display-referred VT.
    // 9-10. Reference->DisplayCSOutput:
    //     9. DisplayCSOutput from display reference.
    //    10. GPU Allocation No-op.

    // 0. GPU Allocation No-op.
    auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    auto data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 1. In to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
    auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
    OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
    OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
    OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
    OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));

    // 2. Look process space from reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::GammaType);

    // 3. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 4. Look No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 5. Look transform.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[5]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::CDLType);

    // 6. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 7. DisplayCSProcess to display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[7]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::GammaType);

    // 8. Display-referred VT.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[8]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::LogType);

    // 9. DisplayCSOutput from display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[9]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);

    // 10. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[10]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);
}
