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
  aces_interchange: ap0
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
inactive_colorspaces: [ACES2065-1]

roles:
  aces_interchange: ACES2065-1
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
#    aliases: [srgb_display]
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

    // scene-referred reference space

    // Get the transform to convert the scene-referred reference space.
    auto inputToBaseGtScene = OCIO::ConfigUtils::getRefSpaceConverter(inputConfig, 
                                                                      baseConfig, 
                                                                      OCIO::REFERENCE_SPACE_SCENE);
    {
        // Convert each one of the scene-referred color spaces and check the result.
        std::vector<OCIO::ConstColorSpaceRcPtr> colorspaces;
        for (int i = 0; i < inputConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_SCENE, OCIO::COLORSPACE_ALL); ++i)
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
        // But transforms are not simplified, for clarity in what was done.
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
                mtx0->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 2.521686186744, 1e-5f);
            }

            {
                std::vector<double> m44(16, 0.0);
                auto mtx1 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(1));
                OCIO_REQUIRE_ASSERT(mtx1);
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
            mtx0->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.4396329819194919, 1e-5f);
            OCIO_CHECK_CLOSE(m44[1], 0.3829886981515535, 1e-5f);

            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
        }

        // rec709 had no transforms but now needs the same matrix.
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
            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE))
            OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE))
        }
    }


    // display-referred reference space

    // Get the transform to convert the display-referred reference space.
    auto inputToBaseGtDisplay = OCIO::ConfigUtils::getRefSpaceConverter(inputConfig, 
                                                                        baseConfig, 
                                                                        OCIO::REFERENCE_SPACE_DISPLAY);
    {   
        // Convert each one of the display-referred color spaces and check the result.
        std::vector<OCIO::ConstColorSpaceRcPtr> colorspaces;
        for (int i = 0; i < inputConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, OCIO::COLORSPACE_ALL); ++i)
        {
            const char * name = inputConfig->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, 
                                                                      OCIO::COLORSPACE_ALL, 
                                                                      i);
            OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace(name);
            // display-referred colorspace
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
            mtx0->getMatrix(m44.data());
            //OCIO_CHECK_CLOSE(m44[0], 3.240969941907037, 1e-5f);
            //OCIO_CHECK_CLOSE(m44[1], -1.537383177571058, 1e-5f);

            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }

        // CIE-XYZ-D65 had a matrix but no longer needs transforms, it's now the reference space.
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
                mtx0->getMatrix(m44.data());
                //OCIO_CHECK_CLOSE(m44[0], 3.240969941907, 1e-5f);
            }

            {
                OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
                std::vector<double> m44(16, 0.0);
                auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(1));
                OCIO_REQUIRE_ASSERT(mtx0);
                mtx0->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 0.412390799266, 1e-5f);
            }
        }
    }

    {
        // Convert each one of the view transforms and check the result.
        // The view transform of the input config goes from linear rec.709 to linear rec.709.
        // This needs to be adapted to the base config that goes from cie-xyz to aces2065-1.
        std::vector<OCIO::ConstViewTransformRcPtr> viewTransforms;
        for (int v = 0; v < inputConfig->getNumViewTransforms(); v++)
        {
            const char * name = inputConfig->getViewTransformNameByIndex(v);
            viewTransforms.push_back(inputConfig->getViewTransform(name));
        }

        OCIO::ViewTransformRcPtr vt = viewTransforms[0]->createEditableCopy();
        OCIO::ConfigUtils::updateReferenceView(vt, inputToBaseGtScene, inputToBaseGtDisplay);
        OCIO_REQUIRE_ASSERT(!vt->getTransform(OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE))

        OCIO::ConstTransformRcPtr t = vt->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
        OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
        auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
        OCIO_REQUIRE_ASSERT(gtx);
        OCIO_CHECK_EQUAL(gtx->getNumTransforms(), 3);

        // Matrix from cie-xyz to rec.709.
        {
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX); 
            std::vector<double> m44(16, 0.0);
            auto mtx0 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(0));
            OCIO_REQUIRE_ASSERT(mtx0);
            mtx0->getMatrix(m44.data());
            //OCIO_CHECK_CLOSE(m44[0], 3.240969941907, 1e-5f);
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
                mtx0->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 0.439632981919, 1e-5f);
            }

            OCIO_CHECK_EQUAL(gtxA->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_BUILTIN);

            {
                std::vector<double> m44(16, 0.0);
                auto mtx2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtxA->getTransform(2));
                OCIO_REQUIRE_ASSERT(mtx2);
                mtx2->getMatrix(m44.data());
                OCIO_CHECK_CLOSE(m44[0], 3.240969941905, 1e-5f);
            }
        }

        // Matrix from rec.709 to aces2065-1.
        {
            std::vector<double> m44(16, 0.0);
            auto mtx2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gtx->getTransform(2));
            OCIO_REQUIRE_ASSERT(mtx2);
            mtx2->getMatrix(m44.data());
            OCIO_CHECK_CLOSE(m44[0], 0.439632981919, 1e-5f);
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
        - !<MatrixTransform> {matrix: [2.521686, -1.134131, -0.387555, 0, -0.2764799, 1.372719, -0.09623917, 0, -0.01537806, -0.152975, 1.168353, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: very approx. standard RGB
    isdata: false
    description: sRGB - Texture with truncated matrix values and different gamma
    from_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [2.521686, -1.134131, -0.387555, 0, -0.2764799, 1.372719, -0.09623917, 0, -0.01537806, -0.152975, 1.168353, 0, 0, 0, 0, 1]}
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
    name: pq display
    isdata: false
    description: Rec.2100-PQ - Display
    from_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ}
)" };

    iss.str(INPUT);

    OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromFile("ocio://cg-config-v1.0.0_aces-v1.3_ocio-v2.1");
    OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

    std::vector<OCIO::ConstColorSpaceRcPtr> colorspaces;
    OCIO::SearchReferenceSpaceType refType = OCIO::SEARCH_REFERENCE_SPACE_SCENE;
    for (int i = 0; i < inputConfig->getNumColorSpaces(refType, OCIO::COLORSPACE_ALL); ++i)
    {
        const char * name = inputConfig->getColorSpaceNameByIndex(refType, OCIO::COLORSPACE_ALL, i);
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace(name);
        colorspaces.push_back(cs);
    }

    // Check 'ref_space'
    {
//        OCIO::ConstColorSpaceRcPtr cs = colorspaces[0];
// TODO: could omit the vector of color spaces to make this easier to read.
        OCIO::ConstColorSpaceRcPtr cs = inputConfig->getColorSpace("ref_space");
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string("ACES2065-1"));
    }

    // Check 'Unknown'
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[1];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string("Raw"));
    }

    // Check 'standard RGB'
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[2];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string("sRGB - Texture"));
    }

    // Check 'approx. standard RGB'.  Matrix values are different but within tolerance
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[3];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string("sRGB - Texture"));
    }

    // Check 'very approx. standard RGB'.  Gamma value is outside tolerance -- no match.
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[4];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string(""));
    }
    
    // Check 'ACES cct'.
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[5];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string("ACEScct"));
    }

    // Check 'ACES cg'
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[6];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string("ACEScg"));
    }

    // Check 'ACES cg' but with the wrong ref space type -- no match.
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[6];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_DISPLAY);
        OCIO_CHECK_EQUAL(name, std::string(""));
    }

    // Check 'pq display'.
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[7];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_DISPLAY);
        OCIO_CHECK_EQUAL(name, std::string("Rec.2100-PQ - Display"));
    }
    
    // Check 'pq display' but with wrong ref space type -- no match.
    {
        OCIO::ConstColorSpaceRcPtr cs = colorspaces[7];
        const char * name = OCIO::ConfigUtils::findEquivalentColorspace(baseConfig, 
                                                                        cs, 
                                                                        OCIO::REFERENCE_SPACE_SCENE);
        OCIO_CHECK_EQUAL(name, std::string(""));
    }
}