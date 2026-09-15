// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <pystring.h>

#include "UnitTestUtils.h"

#include "ConfigUtils.h"
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(MergeConfigs, config_utils_update_reference)
{
    std::istringstream bss;
    std::istringstream iss;

    constexpr const char * BASE {
R"(ocio_profile_version: 2.1
name: base
environment:
  TEXTURE_SPACE: sRGB - Texture
  SHOT: 001a

search_path:
  - luts
  - .

roles:
  cie_xyz_d65_interchange: CIE-XYZ-D65

file_rules:
  - !<Rule> {name: Default, colorspace: ACEScg}

#inactive_colorspaces: [sRGB - Texture, ACEScg]

display_colorspaces:   # reference space = cie xyz d65
  - !<ColorSpace>
    name: sRGB - Display
    aliases: [srgb_display]
    family: Display-Basic
    description: from base
    from_display_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_sRGB}

  - !<ColorSpace>
    name: CIE-XYZ-D65
    aliases: [cie_xyz_d65]
    description: The \"CIE XYZ (D65)\" display connection colorspace.
    isdata: false

colorspaces:   # reference space = aces2065-1
  - !<ColorSpace>
    name: ACEScg
    aliases: [aces]
    family: ACES-Linear
    description: from base
    to_scene_reference: !<BuiltinTransform> {style: ACEScg_to_ACES2065-1}

  - !<ColorSpace>
    name: ap0
    family: ACES-Linear
    description: from base

  - !<ColorSpace>
    name: sRGB - Texture
    family: Texture
    aliases: [srgb, srgb_tx]
    description: from base
    from_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: look
    aliases:
    family:
    description: from base
    from_scene_reference: !<ColorSpaceTransform> {src: ACEScg, dst: sRGB - Texture}
)" };

    constexpr const char * INPUT {
R"(ocio_profile_version: 2.1
name: input
search_path: lut_dir
#inactive_colorspaces: [ACES2065-1]

roles:
  cie_xyz_d65_interchange: CIE-XYZ-D65

file_rules:
  - !<Rule> {name: Default, colorspace: sRGB}

displays:
  sRGB - Display:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: ACES 1.0 - SDR Video, view_transform: ACES 1.0 - SDR Video, display_colorspace: sRGB - Display}

view_transforms:
  - !<ViewTransform>
    name: ACES 1.0 - SDR Video
    from_scene_reference:  !<GroupTransform>
      children:
        # Matrix from rec.709 to aces2065-1
        - !<MatrixTransform> {matrix: [ 0.439632981919, 0.382988698152, 0.177378319929, 0, 0.089776442959, 0.813439428749, 0.096784128292, 0, 0.017541170383, 0.111546553302, 0.870912276314, 0, 0, 0, 0, 1 ]}
        # Built-in transform from aces2065-1 to cie-xyz
        - !<BuiltinTransform> {style: ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO_1.0}
        # Matrix from cie-xyz to linear rec.709
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ViewTransform>
    name: vt scene ref
    from_scene_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}
    to_scene_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055}

  - !<ViewTransform>
    name: vt display ref
    description: from base
    from_display_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}
    to_display_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055}

display_colorspaces:   # reference space = linear rec 709
  - !<ColorSpace>
    name: sRGB - Display
    aliases: [srgb_display]
    family: Display~Standard
    description: from input
    from_display_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: CIE-XYZ-D65
    aliases: [cie_xyz_d65]
    description: The \"CIE XYZ (D65)\" display connection colorspace.
    isdata: false
    from_display_reference: !<MatrixTransform> {matrix: [ 0.412390799266, 0.357584339384, 0.180480788402, 0, 0.212639005872, 0.715168678768, 0.072192315361, 0, 0.019330818716, 0.119194779795, 0.950532152250, 0, 0, 0, 0, 1 ]}

colorspaces:   # reference space = linear rec 709
  - !<ColorSpace>
    name: ACES2065-1
    aliases: [aces]
    family: ACES~Linear
    description: from input
    to_scene_reference: !<MatrixTransform> {matrix: [ 2.521686186744, -1.134130988240, -0.387555198504, 0, -0.276479914230, 1.372719087668, -0.096239173438, 0, -0.015378064966, -0.152975335867, 1.168353400833, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: sRGB
    family: Texture~
    description: from input
    to_scene_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055}

  - !<ColorSpace>
    name: rec709
    description: from input

  - !<ColorSpace>
    name: raw
    description: from base
    isdata: true
)" };

    bss.str(BASE);
    iss.str(INPUT);

    OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
    OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
    OCIO::ConstConfigRcPtr cgConfig = OCIO::Config::CreateFromFile("ocio://cg-config-v1.0.0_aces-v1.3_ocio-v2.1");

    // Scene-referred reference space check.

    // Get the transform to convert the scene-referred reference space.
    auto inputToBaseGtScene = OCIO::ConfigUtils::getRefSpaceConverter(inputConfig, 
                                                                      baseConfig, 
                                                                      OCIO::REFERENCE_SPACE_SCENE);
    {
        // Convert each one of the scene-referred color spaces and check the result.
        std::vector<OCIO::ConstColorSpaceRcPtr> colorspaces;
        const int numSpaces = inputConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE,
                                                             OCIO::COLORSPACE_ALL);
        for (int i = 0; i < numSpaces; ++i)
        {
            const char * name = inputConfig->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_SCENE, 
                                                                      OCIO::COLORSPACE_ALL, 
                                                                      i);
            OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace(name);
            // scene-referred colorspace
            colorspaces.push_back(cs);
        }   
        OCIO_CHECK_EQUAL(colorspaces.size(), 4);

        // ACES2065-1 no longer needs transforms, it is now the reference space.
        // But transforms are not simplified, for clarity in what was done,
        // so it is a forward and inverse of the same matrix.
        {
            OCIO::ColorSpaceRcPtr cs = colorspaces.at(0)->createEditableCopy();
            OCIO::ConfigUtils::updateReferenceColorspace(cs, inputToBaseGtScene);
            OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);

            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 2);
            {
                std::vector<double> m44(16, 0.0);
                auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
                OCIO_REQUIRE_ASSERT(mtx0);
                OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
                mtx0->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 2.521686186744, 1e-5f);
            }
            {
                std::vector<double> m44(16, 0.0);
                auto mtx1 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(1));
                OCIO_REQUIRE_ASSERT(mtx1);
                OCIO_CHECK_ASSERT(mtx1->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
                mtx1->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 0.4396329819194919, 1e-5f);
            }

            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        }

        // sRGB now needs a Rec.709 to ACES2065-1 matrix after the exponent.
        {
            OCIO::ColorSpaceRcPtr cs = colorspaces.at(1)->createEditableCopy();
            OCIO::ConfigUtils::updateReferenceColorspace(cs, inputToBaseGtScene);
            OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);

            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 2);
            
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(1));
            OCIO_REQUIRE_ASSERT(mtx0);
            OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
            mtx0->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.4396329819194919, 1e-5f);
            OCIO_CHECK_CLOSE(m44[1], 0.3829886981515535, 1e-5f);

            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        }

        // rec709 had no transforms but now needs the Rec.709 to ACES2065-1 matrix.
        {
            OCIO::ColorSpaceRcPtr cs = colorspaces.at(2)->createEditableCopy();
            OCIO::ConfigUtils::updateReferenceColorspace(cs, inputToBaseGtScene);
            OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
            OCIO_REQUIRE_ASSERT(t);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 1);
            
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
            OCIO_REQUIRE_ASSERT(mtx0);
            OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
            mtx0->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.4396329819194919, 1e-5f);
            OCIO_CHECK_CLOSE(m44[1], 0.3829886981515535, 1e-5f);

            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        }

        // raw had no transforms and none are added since isdata is true.
        {
            OCIO::ColorSpaceRcPtr cs = colorspaces.at(3)->createEditableCopy();
            OCIO::ConfigUtils::updateReferenceColorspace(cs, inputToBaseGtScene);
            OCIO_CHECK_EQUAL(cs->isData(), true);
            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        }
    }

    // Display-referred reference space check.

    // Get the transform to convert the display-referred reference space.
    auto inputToBaseGtDisplay = OCIO::ConfigUtils::getRefSpaceConverter(inputConfig, 
                                                                        baseConfig, 
                                                                        OCIO::REFERENCE_SPACE_DISPLAY);
    {   
        // Convert each one of the display-referred color spaces and check the result.
        std::vector<OCIO::ConstColorSpaceRcPtr> colorspaces;
        const int numSpaces = inputConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY,
                                                             OCIO::COLORSPACE_ALL);
        for (int i = 0; i < numSpaces; ++i)
        {
            const char * name = inputConfig->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, 
                                                                      OCIO::COLORSPACE_ALL, 
                                                                      i);
            OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace(name);
            colorspaces.push_back(cs);
        }   
        OCIO_CHECK_EQUAL(colorspaces.size(), 2);

        // sRGB - Display needs a CIE-XYZ to Rec.709 matrix before the exponent.
        {
            OCIO::ColorSpaceRcPtr cs = colorspaces.at(0)->createEditableCopy();
            OCIO::ConfigUtils::updateReferenceColorspace(cs, inputToBaseGtDisplay);
            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));

            OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 2);
            
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
            OCIO_REQUIRE_ASSERT(mtx0);
            OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_INVERSE);
            mtx0->getMatrix(m44.data());
            // The coefficients are for Rec.709 to CIE-XYZ, but the direction is inverse.
            OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
            OCIO_CHECK_CLOSE(m44[1], 0.357584339384, 1e-5f);

            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }

        // CIE-XYZ-D65 had a matrix but no longer needs transforms, it's now the reference space.
        // An additional matrix is added to invert the original matrix.
        {
            OCIO::ColorSpaceRcPtr cs = colorspaces.at(1)->createEditableCopy();
            OCIO::ConfigUtils::updateReferenceColorspace(cs, inputToBaseGtDisplay);
            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));

            OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 2);

            {
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
                std::vector<double> m44(16, 0.0);
                auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
                OCIO_REQUIRE_ASSERT(mtx0);
                OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_INVERSE);
                mtx0->getMatrix(m44.data());
                // The coefficients are for Rec.709 to CIE-XYZ, but the direction is inverse.
                OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
            }
            {
                OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
                std::vector<double> m44(16, 0.0);
                auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(1));
                OCIO_REQUIRE_ASSERT(mtx0);
                OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
                mtx0->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
            }
        }
    }

    // View transform reference space check.

    // Convert each one of the view transforms and check the result.
    // The view transform of the input config goes from linear rec.709 to linear rec.709.
    // This needs to be adapted to the base config that goes from cie-xyz to aces2065-1.
    std::vector<OCIO::ConstViewTransformRcPtr> viewTransforms;
    for (int v = 0; v < inputConfig->getNumViewTransforms(); v++)
    {
        const char * name = inputConfig->getViewTransformNameByIndex(v);
        viewTransforms.push_back(inputConfig->getViewTransform(name));
    }
    OCIO_CHECK_EQUAL(viewTransforms.size(), 3);

    {
        OCIO::ViewTransformRcPtr vt = viewTransforms[0]->createEditableCopy();
        OCIO::ConfigUtils::updateReferenceView(vt, inputToBaseGtScene, inputToBaseGtDisplay);
        OCIO_REQUIRE_ASSERT(!vt->getTransform(OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE))

        OCIO::ConstTransformRcPtr t = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
        OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
        auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
        OCIO_REQUIRE_ASSERT(gtx);
        OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 3);

        // Matrix from ACES2065-1 to Rec.709.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
            OCIO_REQUIRE_ASSERT(mtx0);
            OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_INVERSE);
            mtx0->getMatrix(m44.data());
            // The coefficients are for Rec.709 to ACES2065-1, but the direction is inverse.
            OCIO_CHECK_CLOSE(m44[0], 0.439632981919, 1e-5f);
        }

        // The original group transform from the input config.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
            auto gtxA = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(gtx->getTransform(1));
            OCIO_REQUIRE_ASSERT(gtxA);
            OCIO_CHECK_EQUAL(gtxA->getNumTransforms(), 3);
            {
                std::vector<double> m44(16, 0.0);
                auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtxA->getTransform(0));
                OCIO_REQUIRE_ASSERT(mtx0);
                OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
                mtx0->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 0.439632981919, 1e-5f);
            }
            OCIO_CHECK_EQUAL(gtxA->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_BUILTIN);
            {
                std::vector<double> m44(16, 0.0);
                auto mtx2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtxA->getTransform(2));
                OCIO_REQUIRE_ASSERT(mtx2);
                OCIO_CHECK_ASSERT(mtx2->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
                mtx2->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 3.240969941905, 1e-5f);
            }
        }

        // Matrix from Rec.709 to CIE-XYZ.
        {
            std::vector<double> m44(16, 0.0);
            auto mtx2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(2));
            OCIO_REQUIRE_ASSERT(mtx2);
            OCIO_CHECK_ASSERT(mtx2->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
            mtx2->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
        }        
    }

    // Verify that both directions are converted correctly.
    {
        OCIO::ViewTransformRcPtr vt = viewTransforms[1]->createEditableCopy();
        OCIO::ConfigUtils::updateReferenceView(vt, inputToBaseGtScene, inputToBaseGtDisplay);

        OCIO_CHECK_EQUAL(vt->getReferenceSpaceType(), OCIO::REFERENCE_SPACE_SCENE);

        {
            OCIO::ConstTransformRcPtr t = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 3);
        }
        OCIO::ConstTransformRcPtr t = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE);
        OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
        auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
        OCIO_REQUIRE_ASSERT(gtx);
        OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 3);

        // Matrix from CIE-XYZ to Rec.709.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
            std::vector<double> m44(16, 0.0);
            auto mtx2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
            OCIO_REQUIRE_ASSERT(mtx2);
            OCIO_CHECK_ASSERT(mtx2->getDirection() == OCIO::TRANSFORM_DIR_INVERSE);
            mtx2->getMatrix(m44.data());
            // The coefficients are for Rec.709 to CIE-XYZ, but the direction is inverse.
            OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
        }        
        // The original transform from the input config.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }
        // Matrix from Rec.709 to ACES2065-1.
        {
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(2));
            OCIO_REQUIRE_ASSERT(mtx0);
            OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
            mtx0->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.439632981919, 1e-5f);
        }
    }

    // Verify that display-ref view transforms have the display converter on both sides.
    {
        OCIO::ViewTransformRcPtr vt = viewTransforms[2]->createEditableCopy();
        OCIO::ConfigUtils::updateReferenceView(vt, inputToBaseGtScene, inputToBaseGtDisplay);

        OCIO_CHECK_EQUAL(vt->getReferenceSpaceType(), OCIO::REFERENCE_SPACE_DISPLAY);

        // FROM-REF direction.

        OCIO::ConstTransformRcPtr t = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
        OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
        auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
        OCIO_REQUIRE_ASSERT(gtx);
        OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 3);

        // Because the reference space type is display, use the XYZ matrix on both ends.
        // Matrix from CIE-XYZ to Rec.709.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
            std::vector<double> m44(16, 0.0);
            auto mtx2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
            OCIO_REQUIRE_ASSERT(mtx2);
            OCIO_CHECK_ASSERT(mtx2->getDirection() == OCIO::TRANSFORM_DIR_INVERSE);
            mtx2->getMatrix(m44.data());
            // The coefficients are for Rec.709 to CIE-XYZ, but the direction is inverse.
            OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
        }        
        // The original transform from the input config.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }
        // Matrix from Rec.709 to CIE-XYZ.
        {
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(2));
            OCIO_REQUIRE_ASSERT(mtx0);
            OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
            mtx0->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
        }

        // TO-REF direction.

        OCIO::ConstTransformRcPtr t1 = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE);
        OCIO_CHECK_EQUAL(t1->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
        gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t1);
        OCIO_REQUIRE_ASSERT(gtx);
        OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 3);

        // Matrix from CIE-XYZ to Rec.709.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
            std::vector<double> m44(16, 0.0);
            auto mtx2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
            OCIO_REQUIRE_ASSERT(mtx2);
            OCIO_CHECK_ASSERT(mtx2->getDirection() == OCIO::TRANSFORM_DIR_INVERSE);
            mtx2->getMatrix(m44.data());
            // The coefficients are for Rec.709 to CIE-XYZ, but the direction is inverse.
            OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
        }        
        // The original transform from the input config.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }
        // Because the reference space type is display, use the XYZ matrix on both ends.
        // Matrix from Rec.709 to CIE-XYZ.
        {
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(2));
            OCIO_REQUIRE_ASSERT(mtx0);
            OCIO_CHECK_ASSERT(mtx0->getDirection() == OCIO::TRANSFORM_DIR_FORWARD);
            mtx0->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
        }
    }

    // Test when the conversion is a no-op (base to base).
    // Get the transform to convert the scene-referred reference space.
    inputToBaseGtScene = OCIO::ConfigUtils::getRefSpaceConverter(baseConfig, 
                                                                 baseConfig, 
                                                                 OCIO::REFERENCE_SPACE_SCENE);

    {
        OCIO::ColorSpaceRcPtr cs = inputConfig->getColorSpace("ACES2065-1")->createEditableCopy();
        OCIO::ConfigUtils::updateReferenceColorspace(cs, inputToBaseGtScene);
        OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
        OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
    }

    {
        OCIO::ViewTransformRcPtr vt = viewTransforms[1]->createEditableCopy();
        OCIO::ConfigUtils::updateReferenceView(vt, inputToBaseGtScene, inputToBaseGtDisplay);
        {
            OCIO::ConstTransformRcPtr t = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 2);
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
        }
    }

    // Both sides are now no-ops.
    inputToBaseGtDisplay = OCIO::ConfigUtils::getRefSpaceConverter(inputConfig, 
                                                                   inputConfig, 
                                                                   OCIO::REFERENCE_SPACE_DISPLAY);
    {
        OCIO::ViewTransformRcPtr vt = viewTransforms[1]->createEditableCopy();
        OCIO::ConfigUtils::updateReferenceView(vt, inputToBaseGtScene, inputToBaseGtDisplay);
        {
            OCIO::ConstTransformRcPtr t = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
            OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }
    }
}

OCIO_ADD_TEST(MergeConfigs, config_utils_find_equivalent_colorspace)
{
    std::istringstream iss;

    constexpr const char * INPUT {
R"(ocio_profile_version: 2.1
environment:
  {}
search_path: ""

file_rules:
  - !<Rule> {name: Default, colorspace: ref_space}

inactive_colorspaces: []

colorspaces:
  - !<ColorSpace>
    name: ref_space
    description: The reference space, but with a different name.
    isdata: false

  - !<ColorSpace>
    name: Unknown
    description: Raw but with a different name.
    isdata: true

  - !<ColorSpace>
    name: requires ACES cct
    isdata: false
    description: A color space that requires a space lower in the config that may not have been added yet.
    to_scene_reference: !<ColorSpaceTransform> {src: ACES cct, dst: ref_space}

  - !<ColorSpace>
    name: standard RGB
    isdata: false
    description: sRGB - Texture
    from_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: approx. standard RGB
    isdata: false
    description: sRGB - Texture with truncated matrix values
    from_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [2.522, -1.134, -0.388, 0, -0.276, 1.373, -0.0962, 0, -0.0154, -0.153, 1.168, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: very approx. standard RGB
    isdata: false
    description: sRGB - Texture with truncated matrix values and different gamma
    from_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [2.52, -1.13, -0.39, 0, -0.28, 1.37, -0.096, 0, -0.015, -0.15, 1.17, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.42, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: ACES cct
    isdata: false
    description: ACEScct
    to_scene_reference: !<BuiltinTransform> {style: ACEScct_to_ACES2065-1}

  - !<ColorSpace>
    name: ACES cg
    isdata: false
    description: ACEScg but with a Matrix rather than Builtin Transform.
    to_reference: !<MatrixTransform> {matrix: [ 0.695452241357, 0.140678696470, 0.163869062172, 0, 0.044794563372, 0.859671118456, 0.095534318172, 0, -0.005525882558, 0.004025210306, 1.001500672252, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: pq display scene
    isdata: false
    description: Rec.2100-PQ - incorrectly as scene-referred
    from_reference: !<GroupTransform>
      children:
        - !<BuiltinTransform> {style: UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD}
        - !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ}

display_colorspaces:
  - !<ColorSpace>
    name: pq display
    isdata: false
    description: Rec.2100-PQ - Display
    from_display_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ}

  - !<ColorSpace>
    name: ACES cg display
    isdata: false
    description: ACEScg - incorrectly as a display
    to_display_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [ 0.695452241357, 0.140678696470, 0.163869062172, 0, 0.044794563372, 0.859671118456, 0.095534318172, 0, -0.005525882558, 0.004025210306, 1.001500672252, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: Rec.601 - Display
    isdata: false
    from_display_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [3.50600328272467, -1.73979072630283, -0.544058268362742, 0, -1.06904755985382, 1.97777888272879, 0.0351714193371952, 0, 0.0563065917341277, -0.196975654820772, 1.04995232821873, 0, 0, 0, 0, 1]}
        - !<ExponentTransform> {value: 2.4, style: mirror, direction: inverse}

  - !<ColorSpace>
    name: Rec.601 (PAL) - Display
    isdata: false
    from_display_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [3.06336109008327, -1.39339017490737, -0.47582373799753, 0, -0.96924363628088, 1.87596750150772, 0.0415550574071756, 0, 0.0678610475535669, -0.228799269620496, 1.06908961801603, 0, 0, 0, 0, 1]}
        - !<ExponentTransform> {value: 2.4, style: mirror, direction: inverse}
)" };

    iss.str(INPUT);

    OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromFile("ocio://cg-config-v1.0.0_aces-v1.3_ocio-v2.1");
    OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

    OCIO::ConfigUtils::ColorSpaceFingerprints fingerprints;
    OCIO::ConfigUtils::initializeColorSpaceFingerprints(fingerprints, baseConfig);

    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("ref_space");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("ACES2065-1"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("standard RGB");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("sRGB - Texture"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("approx. standard RGB");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("sRGB - Texture"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("very approx. standard RGB");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string(""));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("ACES cct");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("ACEScct"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("requires ACES cct");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("ACEScct"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("ACES cg");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("ACEScg"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("ACES cg display");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string(""));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("pq display");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("Rec.2100-PQ - Display"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("pq display scene");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string(""));
    }

    // Validate that the fingerprints for the smallest likely gamuts are positive.
    std::vector<float> fingerprintVals;
    OCIO_CHECK_ASSERT(!OCIO::ConfigUtils::calcColorSpaceFingerprint(fingerprintVals, fingerprints.testVals,
        inputConfig, inputConfig->getColorSpace("Rec.601 - Display")));
    for (size_t i = 0; i < fingerprintVals.size(); i++)
    {
        OCIO_CHECK_ASSERT(fingerprintVals[i] >= 0.);
    }

    OCIO_CHECK_ASSERT(!OCIO::ConfigUtils::calcColorSpaceFingerprint(fingerprintVals, fingerprints.testVals,
        inputConfig, inputConfig->getColorSpace("Rec.601 (PAL) - Display")));
    for (size_t i = 0; i < fingerprintVals.size(); i++)
    {
        OCIO_CHECK_ASSERT(fingerprintVals[i] >= 0.);
    }

    // Test the overload that uses the test values of the input config rather than those of
    // the base config.  This is what Config::LocateBuiltinColorSpace uses, since it allows
    // the color spaces of two configs to be compared without first adjusting the reference
    // space of the input color space.

    OCIO::ConfigUtils::TestVals inputTestVals;
    OCIO::ConfigUtils::initializeTestVals(inputTestVals, inputConfig);

    // The base config has both interchange roles.
    OCIO_CHECK_ASSERT(fingerprints.testVals.sceneRefTestValsConverted);
    OCIO_CHECK_ASSERT(fingerprints.testVals.displayRefTestValsConverted);

    // The input config has neither interchange role, so the heuristics are used.  They are
    // able to identify the scene-referred reference space ("ref_space"), but they do not
    // support display-referred spaces.  (The display-referred test values happen to be
    // correct anyway here, since the display reference space is CIE-XYZ-D65.)
    OCIO_CHECK_ASSERT(inputTestVals.sceneRefTestValsConverted);
    OCIO_CHECK_ASSERT(!inputTestVals.displayRefTestValsConverted);

    // The reference space of the input config is also ACES2065-1, so the results are the
    // same as the ones above.
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("ACES cg");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputTestVals,
                                                                        inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("ACEScg"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("standard RGB");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputTestVals,
                                                                        inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("sRGB - Texture"));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("very approx. standard RGB");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputTestVals,
                                                                        inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string(""));
    }
    {
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("pq display");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(fingerprints, inputTestVals,
                                                                        inputConfig, cs);
        OCIO_CHECK_EQUAL(name, std::string("Rec.2100-PQ - Display"));
    }
}

OCIO_ADD_TEST(ConfigUtils, sanitize_id_token)
{
    // Lower-case allowed characters pass through unchanged.
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("abc.-_~/*#%^+()[]|09"),
                     std::string("abc.-_~/*#%^+()[]|09"));

    // Upper-case letters are lowered.
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("ABCxyz"), std::string("abcxyz"));

    // Explicit character mappings.
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken(" \t\n\r"), std::string("____"));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("{}<>"), std::string("()()"));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken(","), std::string("."));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken(";:"), std::string("||"));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("'\""), std::string("##"));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("\\"), std::string("/"));

    // Any other disallowed ASCII character becomes '*'.
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("a!b@c?"), std::string("a*b*c*"));

    // A multi-byte UTF-8 codepoint becomes a single '^', regardless of how many bytes it takes
    // to encode, so the result does not depend on the input's byte-level UTF-8 encoding.
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("a\xC3\xA9z"),         // "a" + 'é' (2 bytes) + "z"
                     std::string("a^z"));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("a\xE4\xB8\xADz"),     // "a" + '中' (3 bytes) + "z"
                     std::string("a^z"));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("a\xF0\x9F\x98\x80z"), // "a" + emoji (4 bytes) + "z"
                     std::string("a^z"));

    // Malformed UTF-8 (a stray continuation/lead byte not followed by the expected continuation
    // bytes) is still handled safely, one '^' per byte actually consumed.
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("a\x80z"), std::string("a^z"));
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken("a\xC3z"), std::string("a^z"));

    // Empty string.
    OCIO_CHECK_EQUAL(OCIO::ConfigUtils::SanitizeIDToken(""), std::string(""));
}

OCIO_ADD_TEST(ConfigUtils, generate_local_id_for_color_space)
{
    auto cfg = OCIO::Config::Create();
    cfg->setName("My Studio!");

    auto cs = OCIO::ColorSpace::Create();
    cs->setName("sRGB Encoded,Space");
    cs->addAlias("myalias");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(cs));

    // Config name and color space name are each sanitized per Annex C: uppercase is lowered,
    // spaces become '_', ',' becomes '.', and '!' (not allowed, no explicit mapping) becomes '*'.
    std::string id;
    OCIO_CHECK_NO_THROW(id = cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"));
    OCIO_CHECK_EQUAL(id, std::string("my_studio*:local:srgb_encoded.space"));

    // An alias resolves to the same ID, since the canonical color space name is used, not the
    // caller-supplied string.
    OCIO_CHECK_NO_THROW(id = cfg->generateLocalIDForColorSpace("myalias"));
    OCIO_CHECK_EQUAL(id, std::string("my_studio*:local:srgb_encoded.space"));

    // A config with no name set cannot generate a local ID.
    auto cfgNoName = OCIO::Config::Create();
    OCIO_CHECK_NO_THROW(cfgNoName->addColorSpace(cs));
    OCIO_CHECK_THROW_WHAT(cfgNoName->generateLocalIDForColorSpace("sRGB Encoded,Space"),
                          OCIO::Exception, "May not generate a local interop ID if the config "
                          "name is empty.");

    // Null / empty color space name.
    OCIO_CHECK_THROW_WHAT(cfg->generateLocalIDForColorSpace(nullptr), OCIO::Exception,
                          "srcColorSpaceName must not be null or empty");
    OCIO_CHECK_THROW_WHAT(cfg->generateLocalIDForColorSpace(""), OCIO::Exception,
                          "srcColorSpaceName must not be null or empty");

    // Color space that does not exist in the config.
    OCIO_CHECK_THROW_WHAT(cfg->generateLocalIDForColorSpace("does_not_exist"), OCIO::Exception,
                          "does not contain the requested color space");

    // Renaming the config is reflected immediately.
    cfg->setName("Other Studio");
    OCIO_CHECK_NO_THROW(id = cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"));
    OCIO_CHECK_EQUAL(id, std::string("other_studio:local:srgb_encoded.space"));

    // Disallow usage of config names for one of the OCIO configs for ACES that already
    // contains interop IDs for all color spaces. Trying to avoid the situation where
    // someone edits one of these configs but forgets to change the config name. This
    // would result in a meaningless interop ID.

    // A config named like one of the built-in configs at version 4.0.0 or higher already has
    // interop IDs populated, so it may not be used as the namespace of a locally-generated one.
    cfg->setName("cg-config-v4.0.0_aces-v2.0_ocio-v2.5");
    OCIO_CHECK_THROW_WHAT(cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"),
                          OCIO::Exception, "May not generate a local interop ID if the name matches");

    // Don't allow anything that would sanitize to one of the disallowed config names.
    cfg->setName("CG-config-v4.0.0_aces-v2.0_ocio-v2.5");
    OCIO_CHECK_THROW_WHAT(cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"),
                          OCIO::Exception, "May not generate a local interop ID if the name matches");

    // Example of a potential future built-in config name.
    cfg->setName("studio-config-v5.1.2_aces-v2.2_ocio-v2.7");
    OCIO_CHECK_THROW_WHAT(cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"),
                          OCIO::Exception, "May not generate a local interop ID if the name matches");

    // Similar, but with more digits in the ACES version.
    cfg->setName("studio-config-v4.1.2_aces-v2.2.1_ocio-v2.7");
    OCIO_CHECK_THROW_WHAT(cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"),
                          OCIO::Exception, "May not generate a local interop ID if the name matches");

    // A built-in-looking config name with a major version below 4 does not have interop IDs, so
    // it is still legal to use as a local ID namespace.
    cfg->setName("studio-config-v3.0.0_aces-v2.0_ocio-v2.4");
    OCIO_CHECK_NO_THROW(id = cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"));
    OCIO_CHECK_EQUAL(id, std::string("studio-config-v3.0.0_aces-v2.0_ocio-v2.4:local:"
                                     "srgb_encoded.space"));

    // A name that starts like a built-in config and has a version 4.0.0+, but continues with
    // something other than the exact "_aces-<version>_ocio-<version>" suffix, is not actually
    // one of the reserved built-in config names and so remains legal.
    cfg->setName("cg-config-v4.0.0_aces-v2.0_ocio-v2.5_custom");
    OCIO_CHECK_NO_THROW(id = cfg->generateLocalIDForColorSpace("sRGB Encoded,Space"));
    OCIO_CHECK_EQUAL(id, std::string("cg-config-v4.0.0_aces-v2.0_ocio-v2.5_custom:local:srgb_encoded.space"));
}

OCIO_ADD_TEST(ConfigUtils, find_color_space_for_id)
{
    auto cfg = OCIO::Config::Create();
    cfg->setName("MyStudio!");

    auto acescg = OCIO::ColorSpace::Create();
    acescg->setName("ACEScg");
    acescg->addAlias("lin_ap1");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(acescg));

    auto srgb = OCIO::ColorSpace::Create();
    srgb->setName("srgb");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(srgb));

    auto foo = OCIO::ColorSpace::Create();
    foo->setName("foo");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(foo));

    auto fancy = OCIO::ColorSpace::Create();
    fancy->setName("My Color Space!");
    fancy->addAlias("Fancy Alias");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(fancy));

    // A different name that sanitizes to the same base as the color space above (both '!' and
    // '?' sanitize to '*').  The color space added first wins.
    auto fancyToo = OCIO::ColorSpace::Create();
    fancyToo->setName("My Color Space?");
    OCIO_CHECK_NO_THROW(cfg->addColorSpace(fancyToo));

    OCIO::ConstColorSpaceRcPtr cs;

    // Mode 1: exact name and exact alias.
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID("ACEScg"));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "ACEScg");
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID("lin_ap1"));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "ACEScg");

    // Mode 2: one namespace, falls back to the base name.
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID("somestudio:acescg"));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "ACEScg");

    // Two-namespace ID whose one-level-stripped remainder is not itself a valid name/alias:
    // no recursive stripping, so this must not match "ACEScg".
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("outer:inner:acescg"));

    // Mode 3 (local): outer namespace matches this config's sanitized name.
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID("mystudio*:local:acescg"));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "ACEScg");

    // Mode 3 negative: outer namespace does not match the config's name.
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("othername:local:acescg"));
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID(":local:acescg"));
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("::local:acescg"));

    // Mode 3 with a color space whose name must be sanitized in order to match the ID base.
    // (The first of the two color spaces that sanitize to this base is the one returned.)
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID("mystudio*:local:my_color_space*"));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "My Color Space!");

    // Aliases are searched as well, and are also compared in their sanitized form.
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID("mystudio*:local:fancy_alias"));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "My Color Space!");

    // The sanitized comparison is only used for the local mode fall-back, so the sanitized
    // base does not match on its own or with a namespace that is not this config's name.
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("my_color_space*"));
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("othername:local:my_color_space*"));

    // Empty inner namespace: "my-studio::srgb" strips to ":srgb", which must not match "srgb".
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("my-studio::srgb"));

    // Use of "local" as an ordinary namespace is not allowed, it may only be used as a keyword
    // for the local (mode 3) form of an ID.
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("local:foo"));

    // Garbage input: extra colons, empty string, and null all resolve to "not found" without
    // throwing.
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID("a:b:c:d"));
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID(""));
    OCIO_CHECK_ASSERT(!cfg->findColorSpaceForID(nullptr));

    // Round-trip: an ID generated by this config is resolved back to the same color space.
    const std::string generated = cfg->generateLocalIDForColorSpace("ACEScg");
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID(generated.c_str()));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "ACEScg");

    // Round-trip for a color space whose name requires more than case sanitizing.
    const std::string generatedFancy = cfg->generateLocalIDForColorSpace("My Color Space!");
    OCIO_CHECK_EQUAL(generatedFancy, std::string("mystudio*:local:my_color_space*"));
    OCIO_CHECK_ASSERT(cs = cfg->findColorSpaceForID(generatedFancy.c_str()));
    OCIO_CHECK_EQUAL(std::string(cs->getName()), "My Color Space!");

    // A config with no name never matches a local mode ID, not even one with an empty outer
    // namespace (such an ID could not have been generated by an unnamed config).
    auto cfgNoName = OCIO::Config::Create();
    OCIO_CHECK_NO_THROW(cfgNoName->addColorSpace(foo));
    OCIO_CHECK_ASSERT(cs = cfgNoName->findColorSpaceForID("foo"));
    OCIO_CHECK_ASSERT(!cfgNoName->findColorSpaceForID(":local:foo"));
    OCIO_CHECK_ASSERT(!cfgNoName->findColorSpaceForID("mystudio*:local:foo"));
}
