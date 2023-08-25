// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/DisplayViewTransform.cpp"

#include "ops/cdl/CDLOpData.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "ops/gamma/GammaOpData.h"
#include "ops/log/LogOpData.h"
#include "ops/matrix/MatrixOpData.h"
#include "ops/range/RangeOpData.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(DisplayViewTransform, basic)
{
    const std::string empty;
    auto dt = OCIO::DisplayViewTransform::Create();

    OCIO_CHECK_EQUAL(dt->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(empty, dt->getSrc());
    OCIO_CHECK_EQUAL(empty, dt->getDisplay());
    OCIO_CHECK_EQUAL(empty, dt->getView());
    OCIO_CHECK_ASSERT(!dt->getLooksBypass());
    OCIO_CHECK_ASSERT(dt->getDataBypass());
    dt->setDataBypass(false);
    OCIO_CHECK_ASSERT(!dt->getDataBypass());
    dt->setDataBypass(true);
    OCIO_CHECK_ASSERT(dt->getDataBypass());

    const std::string inputCS{ "inputCS" };
    dt->setSrc(inputCS.c_str());
    OCIO_CHECK_EQUAL(inputCS, dt->getSrc());

    const std::string display{ "display" };
    dt->setDisplay(display.c_str());
    OCIO_CHECK_EQUAL(display, dt->getDisplay());

    const std::string view{ "view" };
    dt->setView(view.c_str());
    OCIO_CHECK_EQUAL(view, dt->getView());

    OCIO_CHECK_NO_THROW(dt->validate());

    dt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(dt->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    dt->setSrc("");
    OCIO_CHECK_THROW_WHAT(dt->validate(), OCIO::Exception,
                          "DisplayViewTransform: empty source color space name");
    dt->setSrc(inputCS.c_str());

    dt->setDisplay("");
    OCIO_CHECK_THROW_WHAT(dt->validate(), OCIO::Exception, "DisplayViewTransform: empty display name");
    dt->setDisplay(display.c_str());

    dt->setView("");
    OCIO_CHECK_THROW_WHAT(dt->validate(), OCIO::Exception, "DisplayViewTransform: empty view name");
    dt->setView(view.c_str());

    OCIO_CHECK_NO_THROW(dt->validate());

    dt->setLooksBypass(true);
    OCIO_CHECK_ASSERT(dt->getLooksBypass());

    dt->setDataBypass(false);

    // Verify that copy has same values.
    auto t = dt->createEditableCopy();
    dt = OCIO_DYNAMIC_POINTER_CAST<OCIO::DisplayViewTransform>(t);
    OCIO_CHECK_EQUAL(inputCS, dt->getSrc());
    OCIO_CHECK_EQUAL(display, dt->getDisplay());
    OCIO_CHECK_EQUAL(view, dt->getView());
    OCIO_CHECK_EQUAL(dt->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(dt->getLooksBypass());
    OCIO_CHECK_ASSERT(!dt->getDataBypass());
}

OCIO_ADD_TEST(DisplayViewTransform, build_ops)
{
    //
    // Validate BuildDisplayOps where the display/view is a simple color space
    // (i.e., no ViewTransform).
    //

    const std::string src{ "source" };
    const std::string dst{ "destination" };

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
    auto ff = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    cs->setTransform(ff, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    const std::string display{ "display" };
    const std::string view{ "view" };
    OCIO_CHECK_NO_THROW(config->addDisplayView(display.c_str(), view.c_str(), dst.c_str(), ""));

    OCIO_CHECK_NO_THROW(config->validate());

    auto dt = OCIO::DisplayViewTransform::Create();
    dt->setSrc(src.c_str());

    dt->setDisplay(display.c_str());
    dt->setView(view.c_str());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 4);
        OCIO_CHECK_NO_THROW(ops.validate());

        // No look.
        // Input -> destination color space:

        // 0. GPU Allocation No-op.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 1. Input color space (source) to reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);
        auto matData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(data);
        OCIO_CHECK_EQUAL(offset[0], matData->getOffsetValue(0));
        OCIO_CHECK_EQUAL(offset[1], matData->getOffsetValue(1));
        OCIO_CHECK_EQUAL(offset[2], matData->getOffsetValue(2));
        OCIO_CHECK_EQUAL(offset[3], matData->getOffsetValue(3));

        // 2. Display color space (destination) from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::FixedFunctionType);
        auto ffData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(data);
        OCIO_CHECK_EQUAL(ffData->getStyle(), OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        // 3. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    //
    // Using a scene-referred ViewTransform.
    //

    // Create a new display color space and use the same name as the display.
    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    cs->setName(display.c_str());
    auto ec = OCIO::ExposureContrastTransform::Create();
    cs->setTransform(ec, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(cs);

    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("default_vt");
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setSat(1.2);
    vt->setTransform(cdl, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    config->addViewTransform(vt);

    const std::string scenevt{ "scene_vt" };
    vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName(scenevt.c_str());
    auto log = OCIO::LogTransform::Create();
    log->setBase(4.2);
    vt->setTransform(log, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    config->addViewTransform(vt);

    const std::string viewt{ "viewt" };
    // Explicitly use the display color space named "display".
    OCIO_CHECK_NO_THROW(config->addDisplayView(display.c_str(), viewt.c_str(), scenevt.c_str(),
                                               display.c_str(), "", "", ""));
    OCIO_CHECK_NO_THROW(config->validate());

    dt->setView(viewt.c_str());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        // Getting an additional op for the view transform.
        OCIO_CHECK_EQUAL(ops.size(), 5);
        OCIO_CHECK_NO_THROW(ops.validate());

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

        // 2. View Transform (converts scene-referred to display-referred reference space).
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(4.2, logData->getBase());

        // 3. DisplayCS from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);

        // 4. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);
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
    OCIO_CHECK_NO_THROW(config->addDisplayView(display.c_str(), viewt.c_str(), displayvt.c_str(),
                                               display.c_str(), "", "", ""));
    OCIO_CHECK_NO_THROW(config->validate());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        // Getting an additional op for the reference space change.
        OCIO_CHECK_EQUAL(ops.size(), 6);
        OCIO_CHECK_NO_THROW(ops.validate());

        // 0. GPU Allocation No-op.
        auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
        auto data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

        // 1. Input to reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::MatrixType);

        // 2. Convert from the scene-referred reference space to the display-referred reference
        //    space (using the default view transform).
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::CDLType);

        // 3. The view's View Transform converts from the display-referred reference space to the
        //    same display-referred reference space.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(2.1, logData->getBase());

        // 4. DisplayCS from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);

        // 5. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[5]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    // Redo the same test using a shared view that uses USE_DISPLAY_NAME.  The results should be
    // exactly the same

    const std::string sharedView{ "shared_view" };
    OCIO_CHECK_NO_THROW(config->addSharedView(sharedView.c_str(), displayvt.c_str(),
                                              OCIO::OCIO_VIEW_USE_DISPLAY_NAME, "", "", ""));
    OCIO_CHECK_NO_THROW(config->addDisplaySharedView(display.c_str(), sharedView.c_str()));

    // This is valid because shared view refers to a view transform, is used in "display" and
    // there is a color space named "display".
    OCIO_CHECK_NO_THROW(config->validate());
    dt->setView(sharedView.c_str());

    {
        // Same as previous.
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(ops.size(), 6);

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

        // 2. Changing from scene-referred space to display-referred space done
        //    using the default scene view transform.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::CDLType);

        // 3. Display-referred reference to display-referred reference using the specified view
        //    transform.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::LogType);
        auto logData = OCIO_DYNAMIC_POINTER_CAST<const OCIO::LogOpData>(data);
        OCIO_CHECK_EQUAL(2.1, logData->getBase());

        // 4. DisplayCS from reference.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::ExposureContrastType);

        // 5. GPU Allocation No-op.
        op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[5]);
        data = op->data();
        OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);
    }

    // Repeat with data color space.
    csSource->setIsData(true);
    config->addColorSpace(csSource);
    OCIO_CHECK_NO_THROW(config->validate());

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        // Data color space conversion is skipped.
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }

    // Process data color space.
    dt->setDataBypass(false);

    {
        // Getting same results as before.
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                                  config->getCurrentContext(), *dt,
                                                  OCIO::TRANSFORM_DIR_FORWARD));

        // Data color space conversion is not skipped.
        OCIO_CHECK_EQUAL(ops.size(), 6);
    }
}

namespace
{
void ValidateTransform(OCIO::ConstOpRcPtr & op, const std::string name,
                       OCIO::TransformDirection dir, unsigned line)
{
    OCIO_REQUIRE_EQUAL_FROM(op->data()->getFormatMetadata().getNumAttributes(), 1, line);
    OCIO_CHECK_EQUAL_FROM(name, op->data()->getFormatMetadata().getAttributeValue(0), line);

    auto cdl = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(op->data());
    OCIO_REQUIRE_ASSERT_FROM(cdl, line);
    OCIO_CHECK_EQUAL_FROM(dir, cdl->getDirection(), line);
}
}

OCIO_ADD_TEST(DisplayViewTransform, build_ops_with_looks)
{
    //
    // Validate BuildDisplayOps using a display-referred ViewTransform and a look with a
    // display-referred ProcessSpace.
    //

    constexpr char CONFIG[]{ R"(
ocio_profile_version: 2

roles:
  default: raw

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
  display:
    - !<View> {name: view, view_transform: display_vt, display_colorspace: displayCSOut, looks: look}
    - !<View> {name: viewNoVT, colorspace: displayCSOut, looks: look}
    - !<View> {name: viewVTNT, view_transform: nt_forward, display_colorspace: displayCSOut}
    - !<View> {name: viewCSNT, colorspace: nt_inverse, looks: look}
    - !<View> {name: viewCSNTNoLook, colorspace: nt_inverse}

looks:
  - !<Look>
    name: look
    process_space: displayCSProcess
    transform: !<CDLTransform> {name: look forward, sat: 1.5}
    inverse_transform: !<CDLTransform> {name: look inverse, sat: 1.5}

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {name: display vt to ref, sat: 1.5}
    from_display_reference: !<CDLTransform> {name: display vt from ref, sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: displayCSIn
    to_display_reference: !<CDLTransform> {name: in cs to ref, sat: 1.5}
    from_display_reference: !<CDLTransform> {name: in cs from ref, sat: 1.5}

  - !<ColorSpace>
    name: displayCSOut
    to_display_reference: !<CDLTransform> {name: out cs to ref, sat: 1.5}
    from_display_reference: !<CDLTransform> {name: out cs from ref, sat: 1.5}

  - !<ColorSpace>
    name: displayCSProcess
    to_display_reference: !<CDLTransform> {name: process cs to ref, sat: 1.5}
    from_display_reference: !<CDLTransform> {name: process cs from ref, sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    description: A raw color space.
    isdata: true

named_transforms:
  - !<NamedTransform>
    name: nt_forward
    transform: !<CDLTransform> {name: forward transform for nt_forward, sat: 1.5}

  - !<NamedTransform>
    name: nt_inverse
    inverse_transform: !<CDLTransform> {name: inverse transform for nt_inverse, sat: 1.5}
)" };

    std::istringstream is;
    is.str(CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    auto dt = OCIO::DisplayViewTransform::Create();
    dt->setSrc("displayCSIn");
    dt->setDisplay("display");
    dt->setView("view");

    //
    // Test in forward direction.
    //

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 11);
    OCIO_CHECK_NO_THROW(ops.validate());

    // DisplayCSIn->displayCSProcess:
    //       0. GPU Allocation No-op.
    //       1. In to reference.
    //       2. Look process space from reference.
    //       3. GPU Allocation No-op.
    //     4-5. Look-noop + look transform.
    // DisplayCSProcess->display reference:
    //       6. GPU Allocation No-op.
    //       7. DisplayCSProcess to display reference.
    //       8. Display-referred VT.
    // Reference->DisplayCSOutput:
    //       9. DisplayCSOutput from display reference.
    //      10. GPU Allocation No-op.

    // 0. GPU Allocation No-op.
    auto op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    auto data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 1. In to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    ValidateTransform(op, "in cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 2. Look process space from reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "process cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

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
    ValidateTransform(op, "look forward", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 6. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 7. DisplayCSProcess to display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[7]);
    data = op->data();
    ValidateTransform(op, "process cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 8. Display-referred VT.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[8]);
    data = op->data();
    ValidateTransform(op, "display vt from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 9. DisplayCSOutput from display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[9]);
    data = op->data();
    ValidateTransform(op, "out cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 10. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[10]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    //
    // Test in inverse direction.
    //

    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_REQUIRE_EQUAL(ops.size(), 11);
    OCIO_CHECK_NO_THROW(ops.validate());

    // 0. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 1. DisplayCSOutput to display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    data = op->data();
    ValidateTransform(op, "out cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 2. Display-referred VT.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "display vt to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 3. DisplayCSProcess from display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
    data = op->data();
    ValidateTransform(op, "process cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 4. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 5. Look No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[5]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 6. Look transform (inverse).
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
    data = op->data();
    ValidateTransform(op, "look inverse", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 7. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[7]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 8. Look process space to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[8]);
    data = op->data();
    ValidateTransform(op, "process cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 9. In from reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[9]);
    data = op->data();
    ValidateTransform(op, "in cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 10. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[10]);
    data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    //
    // Check that looks can be bypassed.
    //

    dt->setLooksBypass(true);
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO_CHECK_NO_THROW(ops.validate());

    // 0. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 1. In to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    data = op->data();
    ValidateTransform(op, "in cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 2. Display-referred VT.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "display vt from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 3. DisplayCSOutput from display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
    data = op->data();
    ValidateTransform(op, "out cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 4. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    //
    // Tests without a view transform.
    //

    dt->setLooksBypass(false);
    dt->setView("viewNoVT");

    //
    // Test in forward direction.
    //

    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 10);
    OCIO_CHECK_NO_THROW(ops.validate());

    // 0. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 1. In to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    ValidateTransform(op, "in cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 2. Look process space from reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "process cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

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
    ValidateTransform(op, "look forward", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 6. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 7. DisplayCSProcess to display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[7]);
    data = op->data();
    ValidateTransform(op, "process cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 8. DisplayCSOutput from display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[8]);
    data = op->data();
    ValidateTransform(op, "out cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 9. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[9]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    //
    // Test in inverse direction.
    //

    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 10);
    OCIO_CHECK_NO_THROW(ops.validate());

    // 0. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 1. DisplayCSOutput to display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    data = op->data();
    ValidateTransform(op, "out cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 2. DisplayCSProcess from display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "process cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 3. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 4. Look No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 5. Look transform (inverse).
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[5]);
    data = op->data();
    ValidateTransform(op, "look inverse", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 6. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 7. Look process space to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[7]);
    data = op->data();
    ValidateTransform(op, "process cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 8. In from reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[8]);
    data = op->data();
    ValidateTransform(op, "in cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 9. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[9]);
    data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    //
    // Using named transforms.
    //

    //
    // Src can't be a named transform.
    //
    ops.clear();
    dt->setSrc("nt_forward");
    dt->setView("view");

    OCIO_CHECK_THROW_WHAT(OCIO::BuildDisplayOps(ops, *config,
                                                config->getCurrentContext(), *dt,
                                                OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception,
                          "Cannot find source color space named 'nt_forward'");

    //
    // View color space is a named transform: looks are applied on src then the named transform is
    // applied.
    //
    ops.clear();
    dt->setSrc("displayCSIn");
    dt->setView("viewCSNT");

    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 7);
    OCIO_CHECK_NO_THROW(ops.validate());

    // 0. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 1. In to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    ValidateTransform(op, "in cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 2. Look process space from reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "process cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

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
    ValidateTransform(op, "look forward", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 6. Named transform.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
    ValidateTransform(op, "inverse transform for nt_inverse", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);

    //
    // Same in inverse direction.
    //
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 7);
    OCIO_CHECK_NO_THROW(ops.validate());
    // 0. Named transform.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    ValidateTransform(op, "inverse transform for nt_inverse", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 1. Look No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 2. Look transform (inverse).
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "look inverse", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 3. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[3]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // 4. Look process space to reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[4]);
    data = op->data();
    ValidateTransform(op, "process cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 5. In from reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[5]);
    data = op->data();
    ValidateTransform(op, "in cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // 6. GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[6]);
    data = op->data();
    OCIO_REQUIRE_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    //
    // View color space is a named transform and no look.
    //
    ops.clear();
    dt->setView("viewCSNTNoLook");

    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.validate());
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    ValidateTransform(op, "inverse transform for nt_inverse", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);

    //
    // Same in inverse direction.
    //
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.validate());
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    ValidateTransform(op, "inverse transform for nt_inverse", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    //
    // View transforn is a named transform: named transform and dst conversion are applied.
    //
    ops.clear();
    dt->setSrc("displayCSIn");
    dt->setView("viewVTNT");

    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(ops.validate());
    // Named transform.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    ValidateTransform(op, "forward transform for nt_forward", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // DisplayCSOutput from display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    data = op->data();
    ValidateTransform(op, "out cs from ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    //
    // Same in inverse direction: dst conversion and named transform are applied.
    //
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::BuildDisplayOps(ops, *config,
                                              config->getCurrentContext(), *dt,
                                              OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(ops.validate());

    // GPU Allocation No-op.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[0]);
    data = op->data();
    OCIO_CHECK_EQUAL(data->getType(), OCIO::OpData::NoOpType);

    // DisplayCSOutput to display reference.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[1]);
    ValidateTransform(op, "out cs to ref", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // Named transform.
    op = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Op>(ops[2]);
    data = op->data();
    ValidateTransform(op, "forward transform for nt_forward", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
}

OCIO_ADD_TEST(DisplayViewTransform, config_load)
{
    constexpr const char * SIMPLE_CONFIG{ R"(
ocio_profile_version: 2

roles:
  default: raw

displays:
  displayName:
    - !<View> {name: viewName, colorspace: out}

colorspaces:
  - !<ColorSpace>
    name: raw

  - !<ColorSpace>
    name: in
    to_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}

  - !<ColorSpace>
    name: out
    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}

  - !<ColorSpace>
    name: test
    from_scene_reference: !<DisplayViewTransform> {src: in, display: displayName, view: viewName}
    to_scene_reference: !<DisplayViewTransform> {src: in, display: displayName, view: viewName, looks_bypass: true, data_bypass: false}
)" };

    std::istringstream is;
    is.str(SIMPLE_CONFIG);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));

    OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace("test");
    OCIO_REQUIRE_ASSERT(cs);
    auto tr = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    OCIO_REQUIRE_ASSERT(tr);
    auto displayTr = OCIO_DYNAMIC_POINTER_CAST<const OCIO::DisplayViewTransform>(tr);
    OCIO_REQUIRE_ASSERT(displayTr);
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, displayTr->getDirection());
    OCIO_CHECK_EQUAL(std::string("in"), displayTr->getSrc());
    OCIO_CHECK_EQUAL(std::string("displayName"), displayTr->getDisplay());
    OCIO_CHECK_EQUAL(std::string("viewName"), displayTr->getView());
    OCIO_CHECK_ASSERT(!displayTr->getLooksBypass());
    OCIO_CHECK_ASSERT(displayTr->getDataBypass());

    tr = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
    OCIO_REQUIRE_ASSERT(tr);
    displayTr = OCIO_DYNAMIC_POINTER_CAST<const OCIO::DisplayViewTransform>(tr);
    OCIO_REQUIRE_ASSERT(displayTr);
    OCIO_CHECK_ASSERT(displayTr->getLooksBypass());
    OCIO_CHECK_ASSERT(!displayTr->getDataBypass());
}

OCIO_ADD_TEST(DisplayViewTransform, apply_fwd_inv)
{
    constexpr char CONFIG[]{ R"(
ocio_profile_version: 2

roles:
  default: raw

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
  display:
    - !<View> {name: view, view_transform: display_vt, display_colorspace: displayCSOut, looks: look}
    - !<View> {name: viewNoVT, colorspace: displayCSOut, looks: look}

looks:
  - !<Look>
    name: look
    process_space: displayCSProcess
    transform: !<MatrixTransform> {offset: [0.1, 0.2, 0.3, 0]}

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<MatrixTransform> {offset: [0.2, 0.2, 0.4, 0]}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<MatrixTransform> {offset: [0.3, 0.1, 0.1, 0]}

display_colorspaces:
  - !<ColorSpace>
    name: displayCSOut
    to_display_reference: !<MatrixTransform> {offset: [0.25, 0.15, 0.35, 0]}

  - !<ColorSpace>
    name: displayCSProcess
    to_display_reference: !<MatrixTransform> {offset: [0.1, 0.1, 0.1, 0]}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    description: A raw color space.
    isdata: true

  - !<ColorSpace>
    name: displayCSIn
    to_scene_reference: !<MatrixTransform> {offset: [-0.15, 0.15, 0.15, 0.05]}
)" };

    std::istringstream is;
    is.str(CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // Create a display transform using a view that use a view transform and a scene-referred
    // input color space. Create forward and inverse processor and apply them one after the
    // other to a set of pixels. Finally check that the processor created from a group that
    // holds the forward transform and the inverse transform is a no-op.

    auto dt = OCIO::DisplayViewTransform::Create();
    dt->setSrc("displayCSIn");
    dt->setDisplay("display");
    dt->setView("view");

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(dt));
    // Remove the no-ops, since they are useless here.
    OCIO_CHECK_NO_THROW(proc = proc->getOptimizedProcessor(OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_ASSERT(proc);
    OCIO_CHECK_EQUAL(proc->getNumTransforms(), 7);
    auto procGroup = proc->createGroupTransform();
    OCIO_REQUIRE_ASSERT(procGroup);
    OCIO_CHECK_EQUAL(procGroup->getNumTransforms(), 7);
    OCIO::ConstCPUProcessorRcPtr cpuProc;
    OCIO_CHECK_NO_THROW(cpuProc = proc->getDefaultCPUProcessor());
    OCIO_REQUIRE_ASSERT(cpuProc);
    OCIO_CHECK_EQUAL(proc->getNumTransforms(), 7);

    OCIO::ConstProcessorRcPtr procInv;
    OCIO_CHECK_NO_THROW(procInv = config->getProcessor(dt, OCIO::TRANSFORM_DIR_INVERSE));
    // Remove the no-ops, since they are useless here.
    OCIO_CHECK_NO_THROW(procInv = procInv->getOptimizedProcessor(OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_ASSERT(procInv);
    OCIO_CHECK_EQUAL(procInv->getNumTransforms(), 7);
    procGroup = procInv->createGroupTransform();
    OCIO_REQUIRE_ASSERT(procGroup);
    OCIO_CHECK_EQUAL(procGroup->getNumTransforms(), 7);
    OCIO::ConstCPUProcessorRcPtr cpuProcInv;
    OCIO_CHECK_NO_THROW(cpuProcInv = procInv->getDefaultCPUProcessor());
    OCIO_REQUIRE_ASSERT(cpuProcInv);

    const float ref[] = { 0.0f, 0.1f, 0.2f, 0.0f,
                          0.3f, 0.4f, 0.5f, 0.5f,
                          0.6f, 0.7f, 0.8f, 0.7f,
                          0.9f, 1.0f, 1.1f, 1.0f };

    constexpr float error = 1e-6f;
    for (int i = 0; i < 4; ++i)
    {
        float rgba[] = { ref[4 * i + 0],
                         ref[4 * i + 1],
                         ref[4 * i + 2],
                         ref[4 * i + 3] };
        cpuProc->applyRGBA(rgba);
        cpuProcInv->applyRGBA(rgba);
        OCIO_CHECK_CLOSE(rgba[0], ref[4 * i + 0], error);
        OCIO_CHECK_CLOSE(rgba[1], ref[4 * i + 1], error);
        OCIO_CHECK_CLOSE(rgba[2], ref[4 * i + 2], error);
        OCIO_CHECK_CLOSE(rgba[3], ref[4 * i + 3], error);
    }

    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(dt);
    auto dtInv = dt->createEditableCopy();
    dtInv->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    group->appendTransform(dtInv);

    // Note that optimization does only happen once each transform has been converted to ops.
    auto groupProc = config->getProcessor(group);
    groupProc = groupProc->getOptimizedProcessor(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                                 OCIO::OPTIMIZATION_DEFAULT);
    OCIO_CHECK_ASSERT(groupProc->isNoOp());

    // Do a similar test using a display transform that does not use a view transform.

    dt->setDisplay("display");
    dt->setView("viewNoVT");

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(dt));
    // Remove the no-ops, since they are useless here.
    OCIO_CHECK_NO_THROW(proc = proc->getOptimizedProcessor(OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_ASSERT(proc);
    OCIO_CHECK_EQUAL(proc->getNumTransforms(), 6);
    procGroup = proc->createGroupTransform();
    OCIO_REQUIRE_ASSERT(procGroup);
    OCIO_CHECK_EQUAL(procGroup->getNumTransforms(), 6);
    OCIO_CHECK_NO_THROW(cpuProc = proc->getDefaultCPUProcessor());
    OCIO_REQUIRE_ASSERT(cpuProc);

    OCIO_CHECK_NO_THROW(procInv = config->getProcessor(dt, OCIO::TRANSFORM_DIR_INVERSE));
    // Remove the no-ops, since they are useless here.
    OCIO_CHECK_NO_THROW(procInv = procInv->getOptimizedProcessor(OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_ASSERT(procInv);
    OCIO_CHECK_EQUAL(procInv->getNumTransforms(), 6);
    procGroup = procInv->createGroupTransform();
    OCIO_REQUIRE_ASSERT(procGroup);
    OCIO_CHECK_EQUAL(procGroup->getNumTransforms(), 6);
    OCIO_CHECK_NO_THROW(cpuProcInv = procInv->getDefaultCPUProcessor());
    OCIO_REQUIRE_ASSERT(cpuProcInv);

    for (int i = 0; i < 4; ++i)
    {
        float rgba[] = { ref[4 * i + 0],
                         ref[4 * i + 1],
                         ref[4 * i + 2],
                         ref[4 * i + 3] };
        cpuProc->applyRGBA(rgba);
        cpuProcInv->applyRGBA(rgba);
        OCIO_CHECK_CLOSE(rgba[0], ref[4 * i + 0], error);
        OCIO_CHECK_CLOSE(rgba[1], ref[4 * i + 1], error);
        OCIO_CHECK_CLOSE(rgba[2], ref[4 * i + 2], error);
        OCIO_CHECK_CLOSE(rgba[3], ref[4 * i + 3], error);
    }

    group = OCIO::GroupTransform::Create();
    group->appendTransform(dt);
    dtInv = dt->createEditableCopy();
    dtInv->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    group->appendTransform(dtInv);

    groupProc = config->getProcessor(group);
    groupProc = groupProc->getOptimizedProcessor(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                                 OCIO::OPTIMIZATION_DEFAULT);
    OCIO_CHECK_ASSERT(groupProc->isNoOp());

    //
    // Check that the correct error message is thrown in various scenarios.
    //

    dt->setSrc("displayCSIn");
    dt->setDisplay("display");
    dt->setView("view");

    // Empty arguments get handled in DisplayViewTransform::validate.

    // Display name is empty.
    dt->setDisplay("");
    OCIO_CHECK_THROW_WHAT(config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform: empty display name."
    );
    dt->setDisplay("display");

    // View name is empty.
    dt->setView("");
    OCIO_CHECK_THROW_WHAT(config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform: empty view name."
    );
    dt->setView("view");

    // Source CS is empty.
    dt->setSrc("");
    OCIO_CHECK_THROW_WHAT(config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform: empty source color space name."
    );

    // More detailed error handling is in DisplayViewTransform::BuildDisplayOps.

    // Source CS doesn't exist in the config.
    dt->setSrc("missing cs");
    OCIO_CHECK_THROW_WHAT(config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform error. Cannot find source color space named 'missing cs'."
    );
    dt->setSrc("displayCSIn");

    // Display doesn't exist in the config.
    dt->setDisplay("missing display");
    OCIO_CHECK_THROW_WHAT(config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform error. Display 'missing display' not found."
    );
    dt->setDisplay("display");

    OCIO::ConfigRcPtr e_config = config->createEditableCopy();

    // View references a view transform that doesn't exist in the config.
    OCIO_CHECK_NO_THROW(e_config->addDisplayView("display", "bad_view", "missing vt",
                                                 "displayCSOut", "", "", ""));
    dt->setView("bad_view");
    OCIO_CHECK_THROW_WHAT(e_config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform error. The view transform 'missing vt' is neither "
                          "a view transform nor a named transform."
    );

    // View doesn't exist in the config.
    dt->setView("missing view");
    OCIO_CHECK_THROW_WHAT(config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform error. The display 'display' does not have "
                          "view 'missing view'."
    );

    // View references a "display_colorspace" that doesn't exist in the config.
    OCIO_CHECK_NO_THROW(e_config->addDisplayView("display", "bad_view", "display_vt",
                                                 "missing cs", "", "", ""));
    dt->setView("bad_view");
    OCIO_CHECK_THROW_WHAT(e_config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform error. The view 'bad_view' refers to a display "
                          "color space 'missing cs' that can't be found."
    );
    // As with most of these, validation also fails.
    OCIO_CHECK_THROW_WHAT(e_config->validate(),
                          OCIO::Exception,
                          "Config failed display view validation. Display 'display' has a view 'bad_view' that "
                          "refers to a color space or a named transform, 'missing cs', which is not defined."
    );

    // View references a "colorspace" that doesn't exist in the config.
    OCIO_CHECK_NO_THROW(e_config->addDisplayView("display", "bad_view", "missing cs", ""));
    dt->setView("bad_view");
    OCIO_CHECK_THROW_WHAT(e_config->getProcessor(dt),
                          OCIO::Exception,
                          "DisplayViewTransform error. Cannot find color space or named transform "
                          "with name 'missing cs'."
    );

    // Check a few more scenarios.

    // Missing look.
    OCIO_CHECK_NO_THROW(e_config->addDisplayView("display", "bad_view", "display_vt",
                                                 "displayCSOut", "missing look", "", ""));
    dt->setView("bad_view");
    OCIO_CHECK_THROW_WHAT(e_config->getProcessor(dt),
                          OCIO::Exception,
                          "RunLookTokens error. The specified look, 'missing look', cannot be "
                          "found.  (looks: look)."
    );
    dt->setView("view");

    // Missing viewing rule does not currently throw when getting a processor.
    OCIO_CHECK_NO_THROW(e_config->addDisplayView("display", "bad_view", "display_vt",
                                                 "displayCSOut", "", "missing rule", "desc: foo"));
    OCIO_CHECK_NO_THROW(e_config->getProcessor(dt));
    // But validation fails.
    OCIO_CHECK_THROW_WHAT(e_config->validate(),
                          OCIO::Exception,
                          "Config failed display view validation. Display 'display' has a view 'bad_view' refers "
                          "to a viewing rule, 'missing rule', which is not defined."
    );
}

OCIO_ADD_TEST(DisplayViewTransform, context_variables)
{
    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 2

environment: { FILE: cdl_test1.cc }

roles:
  default: cs1

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
    - !<View> {name: View1, colorspace: cs1}
    - !<View> {name: View2, colorspace: cs4}
    - !<View> {name: View3, view_transform: vt1, display_colorspace: dcs1}
    - !<View> {name: View4, view_transform: vt1, display_colorspace: dcs2}
    - !<View> {name: View5, view_transform: vt2, display_colorspace: dcs1}
    - !<View> {name: View6, view_transform: vt2, display_colorspace: dcs2}
    - !<View> {name: View10, colorspace: cs1, looks: look1}
    - !<View> {name: View11, colorspace: cs1, looks: look2}
    - !<View> {name: View12, colorspace: cs1, looks: look3}
    - !<View> {name: View13, view_transform: vt1, display_colorspace: dcs2, looks: +look1}
    - !<View> {name: View14, view_transform: vt1, display_colorspace: dcs2, looks: +look2}
    - !<View> {name: View15, view_transform: vt1, display_colorspace: dcs2, looks: +look3}
    - !<View> {name: View16, view_transform: vt2, display_colorspace: dcs2, looks: +look1}
    - !<View> {name: View17, view_transform: vt2, display_colorspace: dcs2, looks: +look2}
    - !<View> {name: View18, view_transform: vt2, display_colorspace: dcs2, looks: +look3}

looks:
  - !<Look>
    name: look1
    process_space: default
    transform: !<FileTransform> {src: $FILE}
  - !<Look>
    name: look2
    process_space: default
    transform: !<LookTransform> {src: default, dst: cs2, looks: +look1}
  - !<Look>
    name: look3
    process_space: default
    transform: !<CDLTransform> {offset: [0.1, 0.1, 0.1]}

view_transforms:
  - !<ViewTransform>
    name: vt1
    to_scene_reference: !<FileTransform> {src: $FILE}
  - !<ViewTransform>
    name: vt2
    to_scene_reference: !<MatrixTransform> {offset: [0.2, 0.2, 0.4, 0]}

display_colorspaces:
  - !<ColorSpace>
    name: dcs1
    to_display_reference: !<FileTransform> {src: $FILE}
  - !<ColorSpace>
    name: dcs2
    to_display_reference: !<MatrixTransform> {offset: [0.25, 0.15, 0.35, 0]}

colorspaces:
  - !<ColorSpace>
    name: cs1
    allocation: uniform
  - !<ColorSpace>
    name: cs2
    allocation: uniform
    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}
  - !<ColorSpace>
    name: cs3
    allocation: uniform
    from_scene_reference: !<MatrixTransform> {offset: [0.1, 0.2, 0.3, 0]}
  - !<ColorSpace>
    name: cs4
    allocation: uniform
    from_scene_reference: !<FileTransform> {src: $FILE}
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);

    OCIO::ContextRcPtr usedContextVars = OCIO::Context::Create();

    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    cfg->setSearchPath(OCIO::GetTestFilesDir().c_str());
    OCIO_CHECK_NO_THROW(cfg->validate());

    auto dt = OCIO::DisplayViewTransform::Create();
    dt->setSrc("cs1");
    dt->setDisplay("Disp1");

    dt->setView("View1");
    OCIO_CHECK_ASSERT(!CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View2");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View3");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View4");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View5");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View6");
    OCIO_CHECK_ASSERT(!CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    // Validations including looks.

    dt->setView("View10");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View11");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View12");
    OCIO_CHECK_ASSERT(!CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View13");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View14");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View15");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View16");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View17");
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));

    dt->setView("View18");
    OCIO_CHECK_ASSERT(!CollectContextVariables(*cfg, *cfg->getCurrentContext(), *dt, usedContextVars));
}
