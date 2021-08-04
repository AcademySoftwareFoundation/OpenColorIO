// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "apphelpers/LegacyViewingPipeline.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"


namespace OCIO = OCIO_NAMESPACE;


#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#define STR(x) FIELD_STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));

// The configuration file used by the unit tests.
#include "configs.data"


OCIO_ADD_TEST(LegacyViewingPipeline, basic)
{
    // Validate default values.
    OCIO::LegacyViewingPipelineRcPtr vp = OCIO::LegacyViewingPipeline::Create();
    auto dt = vp->getDisplayViewTransform();
    OCIO_CHECK_ASSERT(!dt);
    auto cv = vp->getChannelView();
    OCIO_CHECK_ASSERT(!cv);
    auto ctcc = vp->getColorTimingCC();
    OCIO_CHECK_ASSERT(!ctcc);
    auto dcc = vp->getDisplayCC();
    OCIO_CHECK_ASSERT(!dcc);
    auto lcc = vp->getLinearCC();
    OCIO_CHECK_ASSERT(!lcc);
    OCIO_CHECK_ASSERT(!vp->getLooksOverrideEnabled());
    OCIO_CHECK_EQUAL(std::string(vp->getLooksOverride()), "");

    // An empty viewing pipeline transform is not valid.
    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    OCIO_CHECK_THROW_WHAT(vp->getProcessor(config, config->getCurrentContext()),
                          OCIO::Exception,
                          "can't create a processor without a display transform");

    // Validate setters.

    OCIO::DisplayViewTransformRcPtr dte = OCIO::DisplayViewTransform::Create();
    vp->setDisplayViewTransform(dte);
    dt = vp->getDisplayViewTransform();
    OCIO_CHECK_ASSERT(dt);

    // Display transform member has to be valid.
    OCIO_CHECK_THROW_WHAT(vp->getProcessor(config, config->getCurrentContext()),
                          OCIO::Exception,
                          "LegacyViewingPipeline is not valid: "
                          "DisplayViewTransform: empty source color space name");

    dte->setSrc("colorspace1");
    vp->setDisplayViewTransform(dte);

    // Display transform still invalid: missing display/view.
    OCIO_CHECK_THROW_WHAT(vp->getProcessor(config, config->getCurrentContext()),
                          OCIO::Exception,
                          "LegacyViewingPipeline is not valid: "
                          "DisplayViewTransform: empty display name");

    dte->setDisplay("sRGB");
    dte->setView("view1");
    vp->setDisplayViewTransform(dte);

    // Validation is fine but missing elements in config.
    OCIO_CHECK_THROW_WHAT(vp->getProcessor(config, config->getCurrentContext()),
                          OCIO::Exception,
                          "LegacyViewingPipeline error: Cannot find inputColorSpace, "
                          "named 'colorspace1'");

    auto cs = OCIO::ColorSpace::Create();
    cs->setName("colorspace1");
    cs->setTransform(OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03),
                     OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    config->addDisplayView("sRGB", "view1", "colorspace1", "");

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(config, config->getCurrentContext()));
    OCIO_CHECK_ASSERT(proc);

    OCIO::TransformRcPtr empty;
    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    vp->setChannelView(ff);
    cv = vp->getChannelView();
    OCIO_CHECK_ASSERT(cv);
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(config, config->getCurrentContext()));
    OCIO_CHECK_ASSERT(proc);
    vp->setChannelView(empty);
    cv = vp->getChannelView();
    OCIO_CHECK_ASSERT(!cv);

    vp->setColorTimingCC(ff);
    ctcc = vp->getColorTimingCC();
    OCIO_CHECK_ASSERT(ctcc);
    // Missing element: color_timing role.
    OCIO_CHECK_THROW_WHAT(vp->getProcessor(config, config->getCurrentContext()),
                          OCIO::Exception,
                          "ColorTimingCC requires 'color_timing' role to be defined");
    vp->setColorTimingCC(empty);
    ctcc = vp->getColorTimingCC();
    OCIO_CHECK_ASSERT(!ctcc);

    vp->setLinearCC(ff);
    lcc = vp->getLinearCC();
    OCIO_CHECK_ASSERT(lcc);
    // Missing element: scene_linear role.
    OCIO_CHECK_THROW_WHAT(vp->getProcessor(config, config->getCurrentContext()),
                          OCIO::Exception,
                          "LinearCC requires 'scene_linear' role to be defined");
    vp->setLinearCC(empty);
    lcc = vp->getLinearCC();
    OCIO_CHECK_ASSERT(!lcc);

    vp->setDisplayCC(ff);
    dcc = vp->getDisplayCC();
    OCIO_CHECK_ASSERT(dcc);
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(config, config->getCurrentContext()));
    OCIO_CHECK_ASSERT(proc);
    vp->setDisplayCC(empty);
    dcc = vp->getDisplayCC();
    OCIO_CHECK_ASSERT(!dcc);

    vp->setLooksOverride("missingLook");
    OCIO_CHECK_EQUAL(std::string(vp->getLooksOverride()), "missingLook");

    // Look is missing but looks override is not enabled.
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(config, config->getCurrentContext()));
    OCIO_CHECK_ASSERT(proc);

    vp->setLooksOverrideEnabled(true);
    OCIO_CHECK_ASSERT(vp->getLooksOverrideEnabled());

    // Missing look error.
    OCIO_CHECK_THROW_WHAT(vp->getProcessor(config, config->getCurrentContext()),
                          OCIO::Exception,
                          "The specified look, 'missingLook', cannot be found");
}

OCIO_ADD_TEST(LegacyViewingPipeline, processorWithLooks)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(cfg->validate());

    OCIO::DisplayViewTransformRcPtr dt = OCIO::DisplayViewTransform::Create();
    dt->setDisplay("DISP_2");
    dt->setView("VIEW_2");
    dt->setSrc("in_1");
    OCIO::LegacyViewingPipelineRcPtr vp = OCIO::LegacyViewingPipeline::Create();
    vp->setDisplayViewTransform(dt);

    auto mat = OCIO::MatrixTransform::Create();
    double m[16] = { 1.1, 0.,  0.,  0.,
                     0.,  1.2, 0.,  0.,
                     0.,  0.,  1.1, 0.,
                     0.,  0.,  0.,  1. };
    mat->setMatrix(m);
    vp->setChannelView(mat);

    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    vp->setLinearCC(ff);

    // Processor in forward direction.

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
    OCIO_REQUIRE_ASSERT(proc);

    OCIO::GroupTransformRcPtr groupTransform;
    OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

    OCIO_REQUIRE_ASSERT(groupTransform);
    OCIO_CHECK_NO_THROW(groupTransform->validate());
    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 8);
    // LinearCC creates a color space conversion and a transform.
    {
        // Color space conversion from in_1 to scene_linear role (lin_1 color space).
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));

        OCIO::ConstExponentTransformRcPtr exp
            = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);

        OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        double values[4] = { -1. };
        exp->getValue(values);

        OCIO_CHECK_EQUAL(values[0], 2.6);
        OCIO_CHECK_EQUAL(values[1], 2.6);
        OCIO_CHECK_EQUAL(values[2], 2.6);
        OCIO_CHECK_EQUAL(values[3], 1.);

        // LinearCC transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));

        OCIO::ConstFixedFunctionTransformRcPtr fft
            = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(fft);

        OCIO_CHECK_EQUAL(fft->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    }
    // Apply the looks, channel view, and view transform.
    {
        OCIO::ConstTransformRcPtr tr;
        // Lin_1 to look3 process space (log_1).
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(2));
        OCIO::ConstLogTransformRcPtr log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);
        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(log->getBase(), 2.);

        // Look_3 transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(3));
        OCIO::ConstCDLTransformRcPtr cdl = OCIO::DynamicPtrCast<const OCIO::CDLTransform>(tr);
        OCIO_REQUIRE_ASSERT(cdl);
        OCIO_CHECK_EQUAL(cdl->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        double rgb[3] = { -1. };
        OCIO_CHECK_NO_THROW(cdl->getSlope(rgb));
        OCIO_CHECK_EQUAL(rgb[0], 1.);
        OCIO_CHECK_EQUAL(rgb[1], 2.);
        OCIO_CHECK_EQUAL(rgb[2], 1.);

        // Look_3 & look_4 have the same process space, no color space conversion.

        // Look_4 transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(4));
        cdl = OCIO::DynamicPtrCast<const OCIO::CDLTransform>(tr);
        OCIO_REQUIRE_ASSERT(cdl);
        OCIO_CHECK_EQUAL(cdl->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_NO_THROW(cdl->getSlope(rgb));
        OCIO_CHECK_EQUAL(rgb[0], 1.2);
        OCIO_CHECK_EQUAL(rgb[1], 2.2);
        OCIO_CHECK_EQUAL(rgb[2], 1.2);

        // Channel View transform (no color space conversion).
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(5));
        OCIO::ConstMatrixTransformRcPtr mattr = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mattr);
        OCIO_CHECK_EQUAL(mattr->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        mattr->getMatrix(m);
        OCIO_CHECK_EQUAL(m[0],  1.1);
        OCIO_CHECK_EQUAL(m[1],  0);
        OCIO_CHECK_EQUAL(m[2],  0);
        OCIO_CHECK_EQUAL(m[3],  0);
        OCIO_CHECK_EQUAL(m[5],  1.2);
        OCIO_CHECK_EQUAL(m[10], 1.1);

        // Look_4 process color space (log_1) to reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(6));
        log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);
        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(log->getBase(), 2.);

        // Reference to view_2 color space.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(7));
        OCIO::ConstExponentTransformRcPtr exp
            = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);

        OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

        double values[4] = { -1. };
        exp->getValue(values);

        OCIO_CHECK_EQUAL(values[0], 2.4);
        OCIO_CHECK_EQUAL(values[1], 2.4);
        OCIO_CHECK_EQUAL(values[2], 2.4);
        OCIO_CHECK_EQUAL(values[3], 1.);
    }

    // Repeat in inverse direction.

    dt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    vp->setDisplayViewTransform(dt);
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
    OCIO_REQUIRE_ASSERT(proc);

    OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

    OCIO_REQUIRE_ASSERT(groupTransform);
    OCIO_CHECK_NO_THROW(groupTransform->validate());
    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 8);

    // Apply the inverse view transform, channel view, and looks.
    {
        OCIO::ConstTransformRcPtr tr;

        // View_2 to reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));
        auto exp = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);

        OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        double values[4] = { -1. };
        exp->getValue(values);

        OCIO_CHECK_EQUAL(values[0], 2.4);
        OCIO_CHECK_EQUAL(values[1], 2.4);
        OCIO_CHECK_EQUAL(values[2], 2.4);
        OCIO_CHECK_EQUAL(values[3], 1.);

        // Reference to look_4 process color space (log_1).
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
        auto log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);
        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(log->getBase(), 2.);

        // Channel View transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(2));
        auto mattr = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mattr);
        OCIO_CHECK_EQUAL(mattr->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        mattr->getMatrix(m);
        OCIO_CHECK_EQUAL(m[0],  1 / 1.1);
        OCIO_CHECK_EQUAL(m[5],  1 / 1.2);
        OCIO_CHECK_EQUAL(m[10], 1 / 1.1);

        // Look_4 transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(3));
        auto cdl = OCIO::DynamicPtrCast<const OCIO::CDLTransform>(tr);
        OCIO_REQUIRE_ASSERT(cdl);
        OCIO_CHECK_EQUAL(cdl->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        double rgb[3] = { -1. };
        OCIO_CHECK_NO_THROW(cdl->getSlope(rgb));
        OCIO_CHECK_EQUAL(rgb[0], 1.2);
        OCIO_CHECK_EQUAL(rgb[1], 2.2);
        OCIO_CHECK_EQUAL(rgb[2], 1.2);

        // Look_3 transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(4));
        cdl = OCIO::DynamicPtrCast<const OCIO::CDLTransform>(tr);
        OCIO_REQUIRE_ASSERT(cdl);
        OCIO_CHECK_EQUAL(cdl->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_NO_THROW(cdl->getSlope(rgb));
        OCIO_CHECK_EQUAL(rgb[0], 1.);
        OCIO_CHECK_EQUAL(rgb[1], 2.);
        OCIO_CHECK_EQUAL(rgb[2], 1.);

        // Look_3 process color space (log_1) to lin_1.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(5));
        log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);
        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(log->getBase(), 2.);
    }
    // LinearCC color space conversion and transform.
    {
        // LinearCC transform.
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(6));

        OCIO::ConstFixedFunctionTransformRcPtr fft
            = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(fft);

        OCIO_CHECK_EQUAL(fft->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

        // LnearCC color space conversion.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(7));

        OCIO::ConstExponentTransformRcPtr exp
            = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);

        OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

        double values[4] = { -1. };
        exp->getValue(values);

        OCIO_CHECK_EQUAL(values[0], 2.6);
        OCIO_CHECK_EQUAL(values[1], 2.6);
        OCIO_CHECK_EQUAL(values[2], 2.6);
        OCIO_CHECK_EQUAL(values[3], 1.);
    }

    // Channel view with alpha will cause color space conversions to be skipped if
    // data bypass is enabled (looks are also bypassed).

    m[3] = 0.1;
    mat->setMatrix(m);
    vp->setChannelView(mat);
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
    OCIO_CHECK_ASSERT(proc);

    OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

    OCIO_CHECK_ASSERT(groupTransform);
    OCIO_CHECK_NO_THROW(groupTransform->validate());
    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 2);

    {
        OCIO::ConstTransformRcPtr tr;

        // Channel view.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));
        auto mattr = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mattr);

        // LinearCC transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
        auto fft = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(fft);
    }

    // Looks are still applied if looks override is used.

    vp->setLooksOverrideEnabled(true);
    vp->setLooksOverride(cfg->getDisplayViewLooks("DISP_2", "VIEW_2"));

    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
    OCIO_CHECK_ASSERT(proc);

    OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

    OCIO_CHECK_ASSERT(groupTransform);
    OCIO_CHECK_NO_THROW(groupTransform->validate());
    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 4);

    {
        OCIO::ConstTransformRcPtr tr;

        // Channel view.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));
        auto mattr = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mattr);

        // Look_4 transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
        auto cdl = OCIO::DynamicPtrCast<const OCIO::CDLTransform>(tr);
        OCIO_REQUIRE_ASSERT(cdl);

        // Look_3 transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(2));
        cdl = OCIO::DynamicPtrCast<const OCIO::CDLTransform>(tr);
        OCIO_REQUIRE_ASSERT(cdl);

        // LinearCC transform.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(3));
        auto fft = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(fft);
    }

    dt->setDataBypass(false);
    vp->setDisplayViewTransform(dt);
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
    OCIO_CHECK_ASSERT(proc);

    OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

    OCIO_CHECK_ASSERT(groupTransform);
    OCIO_CHECK_NO_THROW(groupTransform->validate());
    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 8);
}

OCIO_ADD_TEST(LegacyViewingPipeline, fullPipelineNoLook)
{
    //
    // Validate BuildDisplayOps where the display/view is a simple color space
    // (i.e., no ViewTransform).
    //

    const std::string src{ "source" };
    const std::string dst{ "destination" };
    const std::string linearCS{ "linear_cs" };
    const std::string timingCS{ "color_timing_cs" };

    OCIO::ConfigRcPtr cfg = OCIO::Config::CreateRaw()->createEditableCopy();
    auto csSource = OCIO::ColorSpace::Create();
    csSource->setName(src.c_str());
    auto mat = OCIO::MatrixTransform::Create();
    constexpr double offsetSrc[4] = { 0., 0.1, 0.2, 0. };
    mat->setOffset(offsetSrc);
    csSource->setTransform(mat, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    cfg->addColorSpace(csSource);

    auto cs = OCIO::ColorSpace::Create();
    cs->setName(dst.c_str());
    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    cfg->addColorSpace(cs);

    cs = OCIO::ColorSpace::Create();
    cs->setName(linearCS.c_str());
    ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    cfg->addColorSpace(cs);
    cfg->setRole(OCIO::ROLE_SCENE_LINEAR, linearCS.c_str());

    cs = OCIO::ColorSpace::Create();
    cs->setName(timingCS.c_str());
    ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_TO_REFERENCE);
    cfg->addColorSpace(cs);
    cfg->setRole(OCIO::ROLE_COLOR_TIMING, timingCS.c_str());

    const std::string display{ "display" };
    const std::string view{ "view" };
    OCIO_CHECK_NO_THROW(cfg->addDisplayView(display.c_str(), view.c_str(), dst.c_str(), ""));

    OCIO_CHECK_NO_THROW(cfg->validate());

    auto dt = OCIO::DisplayViewTransform::Create();
    dt->setSrc(src.c_str());

    dt->setDisplay(display.c_str());
    dt->setView(view.c_str());

    OCIO::LegacyViewingPipelineRcPtr vp = OCIO::LegacyViewingPipeline::Create();
    vp->setDisplayViewTransform(dt);

    auto linearCC = OCIO::MatrixTransform::Create();
    constexpr double offsetLinearCC[4] = { 0.2, 0.3, 0.4, 0. };
    linearCC->setOffset(offsetLinearCC);
    vp->setLinearCC(linearCC);
    auto timimgCC = OCIO::ExponentTransform::Create();
    constexpr double valueTimingCC[4] = { 2.2, 2.3, 2.4, 1. };
    timimgCC->setValue(valueTimingCC);
    vp->setColorTimingCC(timimgCC);
    constexpr double offsetCV[4] = { 0.2, 0.1, 0.1, 0. };
    auto cvTrans = OCIO::MatrixTransform::Create();
    cvTrans->setOffset(offsetCV);
    vp->setChannelView(cvTrans);
    auto displayCC = OCIO::ExposureContrastTransform::Create();
    vp->setDisplayCC(displayCC);

    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
        OCIO_REQUIRE_ASSERT(proc);

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

        OCIO_REQUIRE_ASSERT(groupTransform);
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 10);

        // 0-1. InputCS -> scene linear role:
        //     0. Input to reference.
        //     1. Scene linear role from reference.
        // 2. LinearCC.
        // 3-4. Scene linear -> color timing role:
        //     4. Scene linear role to reference.
        //     5. ColorTiming from reference. 
        // 5. ColorTimingCC.
        // * No look.
        // 6. ChannelView.
        // 7-8. Color timing role -> display/view color space:
        //     7. ColorTiming to reference.
        //     8. DisplayCS from reference.
        // 9. DisplayCC.

        // 0. Input to reference.
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));
        auto mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);
        OCIO_CHECK_EQUAL(mat->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        double offset[4]{ 0. };
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetSrc[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetSrc[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetSrc[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetSrc[3]);

        // 1. Scene linear role from reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
        auto ff = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(ff);
        OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_ACES_GLOW_10);

        // 2. LinearCC.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(2));
        mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);
        OCIO_CHECK_EQUAL(mat->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetLinearCC[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetLinearCC[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetLinearCC[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetLinearCC[3]);

        // 3. Scene linear role to reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(3));
        ff = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(ff);
        OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);

        // 4. ColorTiming from reference. 
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(4));
        ff = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(ff);
        OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_RGB_TO_HSV);

        // 5. ColorTimingCC.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(5));
        auto exp = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);
        OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        double expVals[4]{ 0. };
        exp->getValue(expVals);
        OCIO_CHECK_EQUAL(expVals[0], valueTimingCC[0]);
        OCIO_CHECK_EQUAL(expVals[1], valueTimingCC[1]);
        OCIO_CHECK_EQUAL(expVals[2], valueTimingCC[2]);
        OCIO_CHECK_EQUAL(expVals[3], valueTimingCC[3]);

        // 6. ChannelView.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(6));
        mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);
        OCIO_CHECK_EQUAL(mat->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetCV[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetCV[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetCV[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetCV[3]);

        // 7. ColorTiming to reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(7));
        ff = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(ff);
        OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);

        // 8. DisplayCS from reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(8));
        ff = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
        OCIO_REQUIRE_ASSERT(ff);
        OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_ACES_GLOW_03);

        // 9. DisplayCC.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(9));
        auto ec = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ec);
    }

    //
    // Using a scene-referred ViewTransform.
    //

    const std::string dsp{ "display" };
    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    cs->setName(dsp.c_str());
    auto ec = OCIO::ExposureContrastTransform::Create();
    cs->setTransform(ec, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    cfg->addColorSpace(cs);

    const std::string scenevt{ "scene_vt" };
    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName(scenevt.c_str());
    auto log = OCIO::LogTransform::Create();
    log->setBase(4.2);
    vt->setTransform(log, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    cfg->addViewTransform(vt);

    const std::string viewt{ "viewt" };
    OCIO_CHECK_NO_THROW(cfg->addDisplayView(display.c_str(), viewt.c_str(), scenevt.c_str(),
                                            dsp.c_str(), "", "", ""));
    OCIO_CHECK_NO_THROW(cfg->validate());

    dt->setView(viewt.c_str());
    vp->setDisplayViewTransform(dt);

    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
        OCIO_REQUIRE_ASSERT(proc);

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

        OCIO_REQUIRE_ASSERT(groupTransform);
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        // Getting an additional op for the reference space change.
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 11);

        // Same as previous up to colorTiming to reference.
        // 0. Input to reference.
        // 1. Scene linear role from reference.
        // 2. LinearCC.
        // 3. Scene linear role to reference.
        // 4. ColorTiming from reference. 
        // 5. ColorTimingCC.
        // 6. ChannelView.
        // 7. ColorTiming to reference.

        // 8. Changing from scene-referred space to display-referred space done
        //    with the specified view transform.
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(8));
        auto log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);
        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(log->getBase(), 4.2);

        // 9. DisplayCS from reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(9));
        auto ec = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ec);

        // 10. DisplayCC.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(10));
        ec = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ec);
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
    cfg->addViewTransform(vt);

    // Replace view display.
    OCIO_CHECK_NO_THROW(cfg->addDisplayView(display.c_str(), viewt.c_str(), displayvt.c_str(),
                                            dsp.c_str(), "", "", ""));
    OCIO_CHECK_NO_THROW(cfg->validate());

    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
        OCIO_REQUIRE_ASSERT(proc);

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

        OCIO_REQUIRE_ASSERT(groupTransform);
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        // Getting an additional op for the display to display view transform.
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 12);

        // Same as previous up to scene-referred to display referred.
        // 0. Input to reference.
        // 1. Scene linear role from reference.
        // 2. LinearCC.
        // 3. Scene linear role to reference.
        // 4. ColorTiming from reference. 
        // 5. ColorTimingCC.
        // 6. ChannelView.
        // 7. ColorTiming to reference.
        // 8. Changing from scene-referred space to display-referred space using the
        //    default view transform.

        // 9. Display-referred reference to display-referred reference using the specified view transform.
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(9));
        auto log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);
        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(log->getBase(), 2.1);

        // 10. DisplayCS from reference.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(10));
        auto ec = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ec);

        // 11. DisplayCC.
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(11));
        ec = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ec);
    }

    // Using a named transform.
    auto nt = OCIO::NamedTransform::Create();
    nt->setName("nt1");
    auto ntTrans = OCIO::MatrixTransform::Create();
    constexpr double offsetNT[4] = { 0.01, 0.05, 0.1, 0. };
    ntTrans->setOffset(offsetNT);
    nt->setTransform(ntTrans, OCIO::TRANSFORM_DIR_FORWARD);
    cfg->addNamedTransform(nt);

    {
        const std::string viewnt{ "viewnt" };
        OCIO_CHECK_NO_THROW(cfg->addDisplayView(display.c_str(), viewnt.c_str(), "nt1", ""));
        OCIO_CHECK_NO_THROW(cfg->validate());

        dt->setView(viewnt.c_str());
        vp->setDisplayViewTransform(dt);

        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
        OCIO_REQUIRE_ASSERT(proc);

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

        OCIO_REQUIRE_ASSERT(groupTransform);
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 5);

        // 0. LinearCC.
        // 1. ColorTimingCC.
        // 2. ChannelView.
        // 3. Named transform.
        // 4. DisplayCC.

        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));
        auto mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
        auto exp = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(2));
        mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(3));
        mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);
        OCIO_CHECK_EQUAL(mat->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        double offset[4]{ 0. };
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetNT[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetNT[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetNT[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetNT[3]);

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(4));
        auto ec = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ec);
    }

    dt->setView(viewt.c_str());
    vp->setDisplayViewTransform(dt);
    csSource->setIsData(true);
    cfg->addColorSpace(csSource);
    OCIO_CHECK_NO_THROW(cfg->validate());

    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
        OCIO_REQUIRE_ASSERT(proc);

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

        OCIO_REQUIRE_ASSERT(groupTransform);
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        // Color space conversion is skipped.
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 4);

        // With isData true, the view/display transform is not applied.  The CC and channelView
        // are applied, but without converting to their usual process spaces.
        // 0. LinearCC.
        // 1. ColorTimingCC.
        // 2. ChannelView.
        // 3. DisplayCC.

        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));
        auto mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
        auto exp = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(2));
        mat = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
        OCIO_REQUIRE_ASSERT(mat);

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(3));
        auto ec = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ec);
    }
}

OCIO_ADD_TEST(LegacyViewingPipeline, processorWithNoOpLook)
{
    //
    // Validate LegacyViewingPipeline::getProcessor when a noop look override
    // is specified.
    //

    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(cfg->validate());

    OCIO::DisplayViewTransformRcPtr dt = OCIO::DisplayViewTransform::Create();
    dt->setDisplay("DISP_2");
    dt->setView("VIEW_2");
    dt->setSrc("in_1");

    OCIO::LegacyViewingPipelineRcPtr vp = OCIO::LegacyViewingPipeline::Create();
    vp->setDisplayViewTransform(dt);
    vp->setLooksOverrideEnabled(true);
    vp->setLooksOverride("look_noop");

    // Processor in forward direction.

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
    OCIO_REQUIRE_ASSERT(proc);

    OCIO::GroupTransformRcPtr groupTransform;
    OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

    OCIO_REQUIRE_ASSERT(groupTransform);
    OCIO_CHECK_NO_THROW(groupTransform->validate());

    // Repeat in inverse direction.

    dt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    vp->setDisplayViewTransform(dt);
    vp->setLooksOverrideEnabled(true);
    vp->setLooksOverride("look_noop");

    // Processor in inverse direction.

    OCIO_CHECK_NO_THROW(proc = vp->getProcessor(cfg, cfg->getCurrentContext()));
    OCIO_REQUIRE_ASSERT(proc);

    OCIO_CHECK_NO_THROW(groupTransform = proc->createGroupTransform());

    OCIO_REQUIRE_ASSERT(groupTransform);
    OCIO_CHECK_NO_THROW(groupTransform->validate());
}
