// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/CoreRendererConfig.h"
#include "builtinconfigs/BuiltinConfigUtils.h"

using namespace OCIO_NAMESPACE::BUILTIN_CONFIG_UTILS;

namespace OCIO_NAMESPACE
{

namespace CORERENDERERCONFIG
{

ConstConfigRcPtr Create_CORE_RENDERER_CONFIG_V100_RC1();

void Register(BuiltinConfigRegistryImpl & registry) noexcept
{
    // If a new built-in config is added, do not forget to update the LATEST_CORE_RENDERER_BUILTIN_CONFIG_URI
    // variable (in BuiltinConfigRegistry.cpp).

    registry.addBuiltin(
        "core-renderer-config-v1.0.0-rc1",
        "ASWF Color Interop Forum -- Color Space Encodings for Texture Assets and CG Rendering",
        true,
        Create_CORE_RENDERER_CONFIG_V100_RC1
    );
}


// Creates config "core-renderer-config-v1.0.0-rc1" from scratch using OCIO C++ API
ConstConfigRcPtr Create_CORE_RENDERER_CONFIG_V100_RC1()
{
    auto cfg = Config::Create();
    cfg->setVersion(2, 0);
    cfg->setStrictParsingEnabled(0);
    cfg->setFamilySeparator('/');
    static const double luma[] = { 0.2126, 0.7152, 0.0722 };
    cfg->setDefaultLumaCoefs(luma);
    cfg->setName(u8R"(core-renderer-config-v1.0.0-rc1)");
    cfg->setDescription(u8R"(Color Space Encodings for Texture Assets and CG Rendering
ASWF Color Interop Forum Recommendation)");

    // Roles
    {
        cfg->setRole(u8R"(aces_interchange)", u8R"(ACES2065-1)");
        cfg->setRole(u8R"(default)", u8R"(sRGB - Scene-referred)");
        cfg->setRole(u8R"(scene_linear)", u8R"(ACEScg)");
    }

    // File Rules
    {
        auto rules = FileRules::Create();
        rules->setDefaultRuleColorSpace(u8R"(default)");
        cfg->setFileRules(rules);
    }

    // Viewing Rules
    {
        auto rules = ViewingRules::Create();
        cfg->setViewingRules(rules);
    }

    // Displays
    {
        cfg->addDisplayView(u8R"(Raw)", u8R"(Raw)", "", u8R"(Data)", "", "", "");

    }
    cfg->setActiveDisplays("");
    cfg->setActiveViews("");
    cfg->setInactiveColorSpaces("");

    // View Transforms
    {
        cfg->setDefaultViewTransformName("");
    }

    // Scene Color Spaces
    {

        // ACES2065-1
        {
            static const char* aliases[] = { u8R"(lin_ap0_scene)","" };
            AddColorSpace(cfg, u8R"(ACES2065-1)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(scene-linear)"
                , ""
                , ""
                , false
                , nullptr
                , nullptr
                , u8R"(This is the config reference space, other transforms convert to this space.)"
            );
        }

        // ACEScg
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.695452241357452, 0.140678696470294, 0.163869062172254, 0, 0.0447945633720377, 0.859671118456422, 0.0955343181715404, 0, -0.00552588255811355, 0.00402521030597869, 1.00150067225214, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_ap1_scene)","" };
            AddColorSpace(cfg, u8R"(ACEScg)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(scene-linear)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Linear Rec.709 (sRGB)
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.439632981919491, 0.382988698151554, 0.177378319928955, 0, 0.0897764429588424, 0.813439428748981, 0.0967841282921771, 0, 0.0175411703831727, 0.111546553302387, 0.87091227631444, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_rec709_scene)","" };
            AddColorSpace(cfg, u8R"(Linear Rec.709 (sRGB))"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(scene-linear)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Linear P3-D65
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.518933487597981, 0.28625658638669, 0.194809926015329, 0, 0.0738593830470598, 0.819845163936986, 0.106295453015954, 0, -0.000307011368446647, 0.0438070502536223, 0.956499961114824, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_displayp3_scene)","" };
            AddColorSpace(cfg, u8R"(Linear P3-D65)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(scene-linear)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Linear Rec.2020
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.679085634706912, 0.157700914643159, 0.163213450649929, 0, 0.0460020030800595, 0.859054673002908, 0.0949433239170327, 0, -0.000573943187616196, 0.0284677684080264, 0.97210617477959, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_rec2020_scene)","" };
            AddColorSpace(cfg, u8R"(Linear Rec.2020)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(scene-linear)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Linear AdobeRGB
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.614763305501725, 0.200243702572018, 0.184992991926256, 0, 0.125539404683864, 0.773521622216629, 0.100938973099507, 0, 0.0245287963611042, 0.0671715435381276, 0.908299660100768, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_adobergb_scene)","" };
            AddColorSpace(cfg, u8R"(Linear AdobeRGB)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(scene-linear)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // CIE XYZ-D65 - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 1.0634954914942, 0.00640891019711789, -0.0158067866176054, 0, -0.492074127923892, 1.36822340747333, 0.0913370883144736, 0, -0.00281646163925351, 0.00464417105680067, 0.916418574593656, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_ciexyzd65_scene)","" };
            AddColorSpace(cfg, u8R"(CIE XYZ-D65 - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(scene-linear)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // sRGB - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = ExponentWithLinearTransform::Create();
                trTo0->setNegativeStyle(NEGATIVE_LINEAR);
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setGamma({ 2.4, 2.4, 2.4, 1 });
                trTo0->setOffset({ 0.055, 0.055, 0.055, 0 });
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.439632981919491, 0.382988698151554, 0.177378319928955, 0, 0.0897764429588424, 0.813439428748981, 0.0967841282921771, 0, 0.0175411703831727, 0.111546553302387, 0.87091227631444, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(srgb_rec709_scene)","" };
            AddColorSpace(cfg, u8R"(sRGB - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(sdr-video)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Gamma 2.2 Rec.709 - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = ExponentTransform::Create();
                trTo0->setNegativeStyle(NEGATIVE_PASS_THRU);
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setValue({ 2.2, 2.2, 2.2, 1 });
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.439632981919491, 0.382988698151554, 0.177378319928955, 0, 0.0897764429588424, 0.813439428748981, 0.0967841282921771, 0, 0.0175411703831727, 0.111546553302387, 0.87091227631444, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(g22_rec709_scene)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.2 Rec.709 - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(sdr-video)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Gamma 1.8 Rec.709 - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = ExponentTransform::Create();
                trTo0->setNegativeStyle(NEGATIVE_PASS_THRU);
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setValue({ 1.8, 1.8, 1.8, 1 });
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.439632981919491, 0.382988698151554, 0.177378319928955, 0, 0.0897764429588424, 0.813439428748981, 0.0967841282921771, 0, 0.0175411703831727, 0.111546553302387, 0.87091227631444, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(g18_rec709_scene)","" };
            AddColorSpace(cfg, u8R"(Gamma 1.8 Rec.709 - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(sdr-video)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // sRGB Encoded AP1 - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = ExponentWithLinearTransform::Create();
                trTo0->setNegativeStyle(NEGATIVE_LINEAR);
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setGamma({ 2.4, 2.4, 2.4, 1 });
                trTo0->setOffset({ 0.055, 0.055, 0.055, 0 });
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.695452241357452, 0.140678696470294, 0.163869062172254, 0, 0.0447945633720377, 0.859671118456422, 0.0955343181715404, 0, -0.00552588255811355, 0.00402521030597869, 1.00150067225214, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(srgb_ap1_scene)","" };
            AddColorSpace(cfg, u8R"(sRGB Encoded AP1 - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(sdr-video)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Gamma 2.2 AP1 - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = ExponentTransform::Create();
                trTo0->setNegativeStyle(NEGATIVE_PASS_THRU);
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setValue({ 2.2, 2.2, 2.2, 1 });
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.695452241357452, 0.140678696470294, 0.163869062172254, 0, 0.0447945633720377, 0.859671118456422, 0.0955343181715404, 0, -0.00552588255811355, 0.00402521030597869, 1.00150067225214, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(g22_ap1_scene)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.2 AP1 - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(sdr-video)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // sRGB Encoded P3-D65 - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = ExponentWithLinearTransform::Create();
                trTo0->setNegativeStyle(NEGATIVE_LINEAR);
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setGamma({ 2.4, 2.4, 2.4, 1 });
                trTo0->setOffset({ 0.055, 0.055, 0.055, 0 });
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.518933487597981, 0.28625658638669, 0.194809926015329, 0, 0.0738593830470598, 0.819845163936986, 0.106295453015954, 0, -0.000307011368446647, 0.0438070502536223, 0.956499961114824, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(srgb_displayp3_scene)","" };
            AddColorSpace(cfg, u8R"(sRGB Encoded P3-D65 - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(sdr-video)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // AdobeRGB - Scene-referred
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            {
                auto trTo0 = ExponentTransform::Create();
                trTo0->setNegativeStyle(NEGATIVE_PASS_THRU);
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setValue({ 2.19921875, 2.19921875, 2.19921875, 1 });
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.614763305501725, 0.200243702572018, 0.184992991926256, 0, 0.125539404683864, 0.773521622216629, 0.100938973099507, 0, 0.0245287963611042, 0.0671715435381276, 0.908299660100768, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(g22_adobergb_scene)","" };
            AddColorSpace(cfg, u8R"(AdobeRGB - Scene-referred)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(sdr-video)"
                , ""
                , ""
                , false
                , nullptr
                , trTo
                , ""
            );
        }

        // Data
        {
            AddColorSpace(cfg, u8R"(Data)"
                , REFERENCE_SPACE_SCENE, nullptr
                , BIT_DEPTH_F32
                , nullptr
                , u8R"(data)"
                , ""
                , ""
                , true
                , nullptr
                , nullptr
                , ""
            );
        }

        // Unknown
        {
            AddColorSpace(cfg, u8R"(Unknown)"
                , REFERENCE_SPACE_SCENE, nullptr
                , BIT_DEPTH_F32
                , nullptr
                , ""
                , ""
                , ""
                , true
                , nullptr
                , nullptr
                , u8R"(This is not actually a color space, but adding it to reserve the name.)"
            );
        }
    }

    // Display Color Spaces
    {
    }

    // Named Transforms
    {
    }
    return cfg;
};


} // namespace CGCONFIG
} // namespace OCIO_NAMESPACE
