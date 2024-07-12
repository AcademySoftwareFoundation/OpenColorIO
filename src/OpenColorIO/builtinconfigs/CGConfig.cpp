// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/CGConfig.h"

namespace OCIO_NAMESPACE
{
// Create the built-in configs for all versions of the OCIO CG config for ACES.
// For backwards compatibility, previous versions are kept in the registry but the
// isRecommended flag should be set to false.

namespace CGCONFIG
{

ConstConfigRcPtr Create_CG_CONFIG_V100_ACES_V13_OCIO_V21();
ConstConfigRcPtr Create_CG_CONFIG_V210_ACES_V13_OCIO_V23();

void Register(BuiltinConfigRegistryImpl & registry) noexcept
{
    // If a new built-in config is added, do not forget to update the LATEST_CG_BUILTIN_CONFIG_URI
    // variable (in BuiltinConfigRegistry.cpp).

    registry.addBuiltin(
        "cg-config-v1.0.0_aces-v1.3_ocio-v2.1",
        "Academy Color Encoding System - CG Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]",
        false,
        Create_CG_CONFIG_V100_ACES_V13_OCIO_V21
    );

    registry.addBuiltin(
        "cg-config-v2.1.0_aces-v1.3_ocio-v2.3",
        "Academy Color Encoding System - CG Config [COLORSPACES v2.0.0] [ACES v1.3] [OCIO v2.3]",
        true,
        Create_CG_CONFIG_V210_ACES_V13_OCIO_V23
    );
}

namespace 
{
    void AddColorSpace(ConfigRcPtr& cfg
        , const char* name
        , ReferenceSpaceType type
        , const char** aliases
        , BitDepth bitDepth
        , const char** categories
        , const char* encoding
        , const char* eqGroup
        , const char* family
        , bool isData
        , ConstTransformRcPtr trFrom
        , ConstTransformRcPtr trTo
        , const char* desc)
    {
        auto out = ColorSpace::Create(type);

        if (aliases)
        {
            for (int i = 0;; ++i)
            {
                const char* alias = aliases[i];
                if (!alias || !*alias)
                    break;
                out->addAlias(alias);
            }
        }

        if (categories)
        {
            for (int i = 0;; ++i)
            {
                const char* cat = categories[i];
                if (!cat || !*cat)
                    break;
                out->addCategory(cat);
            }
        }

        out->setBitDepth(bitDepth);
        out->setDescription(desc);
        out->setEncoding(encoding);
        out->setEqualityGroup(eqGroup);
        out->setFamily(family);
        out->setName(name);
        out->setIsData(isData);
        if (trFrom)
            out->setTransform(trFrom, COLORSPACE_DIR_FROM_REFERENCE);
        if (trTo)
            out->setTransform(trTo, COLORSPACE_DIR_TO_REFERENCE);

        cfg->addColorSpace(out);
    }

    void AddNamedTramsform(ConfigRcPtr& cfg
        , const char* name
        , const char** aliases
        , const char** categories
        , const char* encoding
        , const char* family
        , ConstTransformRcPtr trFwd
        , ConstTransformRcPtr trInv
        , const char* desc)
    {
        auto out = NamedTransform::Create();

        out->setName(name);
        out->setDescription(desc);
        out->setEncoding(encoding);
        out->setFamily(family);
        if (trFwd)
            out->setTransform(trFwd, TRANSFORM_DIR_FORWARD);
        if (trInv)
            out->setTransform(trInv, TRANSFORM_DIR_INVERSE);

        if (aliases)
        {
            for (int i = 0;; ++i)
            {
                const char* alias = aliases[i];
                if (!alias || !*alias)
                    break;
                out->addAlias(alias);
            }
        }

        if (categories)
        {
            for (int i = 0;; ++i)
            {
                const char* cat = categories[i];
                if (!cat || !*cat)
                    break;
                out->addCategory(cat);
            }
        }

        cfg->addNamedTransform(out);
    }

} // anonymous namespace

// Creates config "cg-config-v1.0.0_aces-v1.3_ocio-v2.1" from scratch using OCIO C++ API
ConstConfigRcPtr Create_CG_CONFIG_V100_ACES_V13_OCIO_V21()
{
    auto cfg = Config::Create();
    cfg->setVersion(2, 1);
    cfg->setStrictParsingEnabled(1);
    cfg->setFamilySeparator('/');
    static double luma[] = { 0.2126, 0.7152, 0.0722 };
    cfg->setDefaultLumaCoefs(luma);
    cfg->setName(u8R"(cg-config-v1.0.0_aces-v1.3_ocio-v2.1)");
    cfg->setDescription(u8R"(Academy Color Encoding System - CG Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]
--------------------------------------------------------------------------------------

This minimalistic "OpenColorIO" config is geared toward computer graphics artists requiring a lean config that does not include camera colorspaces and the less common displays and looks.

Generated with "OpenColorIO-Config-ACES" v1.0.0 on the 2022/10/26 at 05:59.)");

    // Roles
    {
        cfg->setRole(u8R"(aces_interchange)", u8R"(ACES2065-1)");
        cfg->setRole(u8R"(cie_xyz_d65_interchange)", u8R"(CIE-XYZ-D65)");
        cfg->setRole(u8R"(color_picking)", u8R"(sRGB - Texture)");
        cfg->setRole(u8R"(color_timing)", u8R"(ACEScct)");
        cfg->setRole(u8R"(compositing_log)", u8R"(ACEScct)");
        cfg->setRole(u8R"(data)", u8R"(Raw)");
        cfg->setRole(u8R"(matte_paint)", u8R"(sRGB - Texture)");
        cfg->setRole(u8R"(scene_linear)", u8R"(ACEScg)");
        cfg->setRole(u8R"(texture_paint)", u8R"(ACEScct)");
    }

    // File Rules
    {
        auto rules = FileRules::Create();
        rules->setDefaultRuleColorSpace(u8R"(ACES2065-1)");
        cfg->setFileRules(rules);
    }

    // Viewing Rules
    {
        auto rules = ViewingRules::Create();
        cfg->setViewingRules(rules);
    }

    // Shared Views
    {
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Video)", u8R"(ACES 1.0 - SDR Video)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Cinema)", u8R"(ACES 1.0 - SDR Cinema)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(Un-tone-mapped)", u8R"(Un-tone-mapped)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
    }

    // Displays
    {
        cfg->addDisplayView(u8R"(sRGB - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.2100-PQ - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(ST2084-P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.0 - SDR Cinema)");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(Un-tone-mapped)");

    }
    cfg->setActiveDisplays(u8R"(sRGB - Display, Rec.1886 Rec.709 - Display, Rec.2100-PQ - Display, ST2084-P3-D65 - Display, P3-D65 - Display)");
    cfg->setActiveViews(u8R"(ACES 1.0 - SDR Video, ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (1000 nits & P3 lim), ACES 1.0 - SDR Cinema, Un-tone-mapped, Raw)");
    cfg->setInactiveColorSpaces(u8R"(CIE-XYZ-D65, sRGB - Display, Rec.1886 Rec.709 - Display, Rec.1886 Rec.2020 - Display, sRGB - Display, Rec.1886 Rec.709 - Display, Rec.1886 Rec.2020 - Display, Rec.1886 Rec.2020 - Display, Rec.2100-HLG - Display, Rec.2100-PQ - Display, Rec.2100-PQ - Display, Rec.2100-PQ - Display, ST2084-P3-D65 - Display, ST2084-P3-D65 - Display, ST2084-P3-D65 - Display, P3-D60 - Display, P3-D65 - Display, P3-D65 - Display, P3-D65 - Display, P3-DCI - Display, P3-DCI - Display, ST2084-P3-D65 - Display)");

    // Looks
    {

        //ACES 1.3 Reference Gamut Compression
        {
            auto trFwd = BuiltinTransform::Create();
            trFwd->setStyle(u8R"(ACES-LMT - ACES 1.3 Reference Gamut Compression)");
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            auto look = Look::Create();
            look->setName(u8R"(ACES 1.3 Reference Gamut Compression)");
            look->setDescription(u8R"(LMT (applied in ACES2065-1) to compress scene-referred values from common cameras into the AP1 gamut

ACEStransformID: urn:ampas:aces:transformId:v1.5:LMT.Academy.GamutCompress.a1.3.0)");
            look->setProcessSpace(u8R"(ACES2065-1)");
            look->setTransform(trFwd);
            cfg->addLook(look);
        }
    }

    // View Transforms
    {
        cfg->setDefaultViewTransformName(u8R"(Un-tone-mapped)");

        // ACES 1.0 - SDR Video
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Video)");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.RGBmonitor_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec709_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_100nits_dim.a1.0.3)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-1000nit-15nit-REC2020lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 1000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_1000nits_15nits_HLG.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_1000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (1000 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-1000nit-15nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 1000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_1000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.0 - SDR Cinema
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Cinema)");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR cinema

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D60_48nits.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // Un-tone-mapped
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(Un-tone-mapped)");
            vt->setDescription("");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }
    }

    // Scene Color Spaces
    {

        // ACES2065-1
        {
            static const char* aliases[] = { u8R"(aces2065_1)",u8R"(ACES - ACES2065-1)",u8R"(lin_ap0)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ACES2065-1)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , nullptr
                , u8R"(The "Academy Color Encoding System" reference colorspace.)"
            );
        }

        // ACEScc
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ACEScc_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(ACES - ACEScc)",u8R"(acescc_ap1)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ACEScc)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ACEScc to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScc_to_ACES.a1.0.3)"
);
        }

        // ACEScct
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ACEScct_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(ACES - ACEScct)",u8R"(acescct_ap1)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)","" };
            AddColorSpace(cfg, u8R"(ACEScct)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ACEScct to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScct_to_ACES.a1.0.3)"
);
        }

        // ACEScg
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ACEScg_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(ACES - ACEScg)",u8R"(lin_ap1)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)","" };
            AddColorSpace(cfg, u8R"(ACEScg)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ACEScg to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScg_to_ACES.a1.0.3)"
);
        }

        // Linear P3-D65
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Linear P3-D65)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.02490528596679, -0.689069761034766, -0.335835524932019, 0, -0.183597032256178, 1.28950620775902, -0.105909175502841, 0, 0.00905856112234766, -0.0592796840575522, 1.0502211229352, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
            }
            static const char* aliases[] = { u8R"(lin_p3d65)",u8R"(Utility - Linear - P3-D65)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)","" };
            AddColorSpace(cfg, u8R"(Linear P3-D65)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to linear P3 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Linear_P3-D65:1.0)"
);
        }

        // Linear Rec.2020
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Linear Rec.2020)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 1.49040952054172, -0.26617091926613, -0.224238601275593, 0, -0.0801674998722558, 1.18216712109757, -0.10199962122531, 0, 0.00322763119162216, -0.0347764757450576, 1.03154884455344, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
            }
            static const char* aliases[] = { u8R"(lin_rec2020)",u8R"(Utility - Linear - Rec.2020)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear Rec.2020)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to linear Rec.2020 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Linear_Rec2020:1.0)"
);
        }

        // Linear Rec.709 (sRGB)
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Linear Rec.709 (sRGB))");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
            }
            static const char* aliases[] = { u8R"(lin_rec709_srgb)",u8R"(Utility - Linear - Rec.709)",u8R"(lin_rec709)",u8R"(lin_srgb)",u8R"(Utility - Linear - sRGB)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)","" };
            AddColorSpace(cfg, u8R"(Linear Rec.709 (sRGB))"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to linear Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Linear_Rec709:1.0)"
);
        }

        // Gamma 1.8 Rec.709 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 1.8 Rec.709 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 1.8, 1.8, 1.8, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g18_rec709_tx)",u8R"(Utility - Gamma 1.8 - Rec.709 - Texture)",u8R"(g18_rec709)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Gamma 1.8 Rec.709 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 1.8 gamma-corrected Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma1.8_Rec709-Texture:1.0)"
);
        }

        // Gamma 2.2 AP1 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 2.2 AP1 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 2.2, 2.2, 2.2, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g22_ap1_tx)",u8R"(g22_ap1)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.2 AP1 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 2.2 gamma-corrected AP1 primaries, D60 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma2.2_AP1-Texture:1.0)"
);
        }

        // Gamma 2.2 Rec.709 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 2.2 Rec.709 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 2.2, 2.2, 2.2, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g22_rec709_tx)",u8R"(Utility - Gamma 2.2 - Rec.709 - Texture)",u8R"(g22_rec709)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.2 Rec.709 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 2.2 gamma-corrected Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma2.2_Rec709-Texture:1.0)"
);
        }

        // Gamma 2.4 Rec.709 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 2.4 Rec.709 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 2.4, 2.4, 2.4, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g24_rec709_tx)",u8R"(g24_rec709)",u8R"(rec709_display)",u8R"(Utility - Rec.709 - Display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.4 Rec.709 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 2.4 gamma-corrected Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma2.4_Rec709-Texture:1.0)"
);
        }

        // sRGB Encoded AP1 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to sRGB Encoded AP1 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentWithLinearTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_LINEAR);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setGamma({ 2.4, 2.4, 2.4, 1 });
                trFrom1->setOffset({ 0.055, 0.055, 0.055, 0 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(srgb_encoded_ap1_tx)",u8R"(srgb_ap1)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(sRGB Encoded AP1 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to sRGB Encoded AP1 primaries, D60 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_sRGB_Encoded_AP1-Texture:1.0)"
);
        }

        // sRGB - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to sRGB Rec.709)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentWithLinearTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_LINEAR);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setGamma({ 2.4, 2.4, 2.4, 1 });
                trFrom1->setOffset({ 0.055, 0.055, 0.055, 0 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(srgb_tx)",u8R"(Utility - sRGB - Texture)",u8R"(srgb_texture)",u8R"(Input - Generic - sRGB - Texture)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(sRGB - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , ""
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to sRGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_sRGB-Texture:1.0)"
);
        }

        // Raw
        {
            static const char* aliases[] = { u8R"(Utility - Raw)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Raw)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , ""
                , ""
                , u8R"(Utility)"
                , true
                , nullptr
                , nullptr
                , u8R"(The utility "Raw" colorspace.)"
            );
        }
    }

    // Display Color Spaces
    {

        // CIE-XYZ-D65
        {
            static const char* aliases[] = { u8R"(cie_xyz_d65)","" };
            AddColorSpace(cfg, u8R"(CIE-XYZ-D65)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , nullptr
                , ""
                , ""
                , ""
                , false
                , nullptr
                , nullptr
                , u8R"-(The "CIE XYZ (D65)" display connection colorspace.)-"
            );
        }

        // sRGB - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_sRGB)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(srgb_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(sRGB - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to sRGB (piecewise EOTF))"
            );
        }

        // Rec.1886 Rec.709 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.709)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec1886_rec709_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.1886 Rec.709 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.1886/Rec.709 (HD video))"
            );
        }

        // Rec.2100-PQ - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec2100_pq_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.2100-PQ - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(hdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.2100-PQ)"
            );
        }

        // ST2084-P3-D65 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(st2084_p3d65_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ST2084-P3-D65 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(hdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to ST-2084 (PQ), P3-D65 primaries)"
            );
        }

        // P3-D65 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_G2.6-P3-D65)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(p3d65_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(P3-D65 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Gamma 2.6, P3-D65)"
            );
        }
    }

    // Named Transforms
    {

        // Rec.1886 - Curve
        {
            auto trInv = GroupTransform::Create();
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            trInv->getFormatMetadata().setName(u8R"(Linear to Rec.1886)");
            {
                auto trInv0 = ExponentTransform::Create();
                trInv0->setNegativeStyle(NEGATIVE_PASS_THRU);
                trInv0->setDirection(TRANSFORM_DIR_INVERSE);
                trInv0->setValue({ 2.4, 2.4, 2.4, 1 });
                trInv->appendTransform(trInv0);
            }
            static const char* aliases[] = { u8R"(rec1886_crv)",u8R"(Utility - Curve - Rec.1886)",u8R"(crv_rec1886)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(Rec.1886 - Curve)"
                , aliases
                , cats
                , u8R"(sdr-video)"
                , u8R"(Utility)"
                , nullptr
                , trInv
                , u8R"(Convert generic linear RGB to generic gamma-corrected RGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:Linear_to_Rec1886-Curve:1.0
)"
);
        }

        // sRGB - Curve
        {
            auto trInv = GroupTransform::Create();
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            trInv->getFormatMetadata().setName(u8R"(Linear to sRGB)");
            {
                auto trInv0 = ExponentWithLinearTransform::Create();
                trInv0->setNegativeStyle(NEGATIVE_LINEAR);
                trInv0->setDirection(TRANSFORM_DIR_INVERSE);
                trInv0->setGamma({ 2.4, 2.4, 2.4, 1 });
                trInv0->setOffset({ 0.055, 0.055, 0.055, 0 });
                trInv->appendTransform(trInv0);
            }
            static const char* aliases[] = { u8R"(srgb_crv)",u8R"(Utility - Curve - sRGB)",u8R"(crv_srgb)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(sRGB - Curve)"
                , aliases
                , cats
                , u8R"(sdr-video)"
                , u8R"(Utility)"
                , nullptr
                , trInv
                , u8R"(Convert generic linear RGB to generic gamma-corrected RGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:Linear_to_sRGB-Curve:1.0
)"
);
        }
    }
    return cfg;
};

// Creates config "cg-config-v2.1.0_aces-v1.3_ocio-v2.3" from scratch using OCIO C++ API
ConstConfigRcPtr Create_CG_CONFIG_V210_ACES_V13_OCIO_V23()
{
    auto cfg = Config::Create();
    cfg->setVersion(2, 3);
    cfg->setStrictParsingEnabled(1);
    cfg->setFamilySeparator('/');
    static double luma[] = { 0.2126, 0.7152, 0.0722 };
    cfg->setDefaultLumaCoefs(luma);
    cfg->setName(u8R"(cg-config-v2.1.0_aces-v1.3_ocio-v2.3)");
    cfg->setDescription(u8R"(Academy Color Encoding System - CG Config [COLORSPACES v2.1.0] [ACES v1.3] [OCIO v2.3]
--------------------------------------------------------------------------------------

This minimalistic "OpenColorIO" config is geared toward computer graphics artists requiring a lean config that does not include camera colorspaces and the less common displays and looks.)");

    // Roles
    {
        cfg->setRole(u8R"(aces_interchange)", u8R"(ACES2065-1)");
        cfg->setRole(u8R"(cie_xyz_d65_interchange)", u8R"(CIE-XYZ-D65)");
        cfg->setRole(u8R"(color_picking)", u8R"(sRGB - Texture)");
        cfg->setRole(u8R"(color_timing)", u8R"(ACEScct)");
        cfg->setRole(u8R"(compositing_log)", u8R"(ACEScct)");
        cfg->setRole(u8R"(data)", u8R"(Raw)");
        cfg->setRole(u8R"(matte_paint)", u8R"(ACEScct)");
        cfg->setRole(u8R"(scene_linear)", u8R"(ACEScg)");
        cfg->setRole(u8R"(texture_paint)", u8R"(sRGB - Texture)");
    }

    // File Rules
    {
        auto rules = FileRules::Create();
        rules->setDefaultRuleColorSpace(u8R"(ACES2065-1)");
        cfg->setFileRules(rules);
    }

    // Viewing Rules
    {
        auto rules = ViewingRules::Create();
        cfg->setViewingRules(rules);
    }

    // Shared Views
    {
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Video)", u8R"(ACES 1.0 - SDR Video)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Cinema)", u8R"(ACES 1.0 - SDR Cinema)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(Un-tone-mapped)", u8R"(Un-tone-mapped)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
    }

    // Displays
    {
        cfg->addDisplayView(u8R"(sRGB - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Display P3 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Display P3 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Display P3 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.2100-PQ - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(ST2084-P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.0 - SDR Cinema)");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(Un-tone-mapped)");

    }
    cfg->setActiveDisplays(u8R"(sRGB - Display, Display P3 - Display, Rec.1886 Rec.709 - Display, Rec.2100-PQ - Display, ST2084-P3-D65 - Display, P3-D65 - Display)");
    cfg->setActiveViews(u8R"(ACES 1.0 - SDR Video, ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (1000 nits & P3 lim), ACES 1.0 - SDR Cinema, Un-tone-mapped, Raw)");
    cfg->setInactiveColorSpaces(u8R"(CIE-XYZ-D65, sRGB - Display, Display P3 - Display, Rec.1886 Rec.709 - Display, Rec.2100-PQ - Display, ST2084-P3-D65 - Display, P3-D65 - Display)");

    // Looks
    {

        //ACES 1.3 Reference Gamut Compression
        {
            auto trFwd = BuiltinTransform::Create();
            trFwd->setStyle(u8R"(ACES-LMT - ACES 1.3 Reference Gamut Compression)");
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            auto look = Look::Create();
            look->setName(u8R"(ACES 1.3 Reference Gamut Compression)");
            look->setDescription(u8R"(LMT (applied in ACES2065-1) to compress scene-referred values from common cameras into the AP1 gamut

ACEStransformID: urn:ampas:aces:transformId:v1.5:LMT.Academy.ReferenceGamutCompress.a1.v1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvLMT.Academy.ReferenceGamutCompress.a1.v1.0)");
            look->setProcessSpace(u8R"(ACES2065-1)");
            look->setTransform(trFwd);
            cfg->addLook(look);
        }
    }

    // View Transforms
    {
        cfg->setDefaultViewTransformName(u8R"(Un-tone-mapped)");

        // ACES 1.0 - SDR Video
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Video)");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.RGBmonitor_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.DisplayP3_dim.a1.0.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec709_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_100nits_dim.a1.0.3

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.RGBmonitor_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.DisplayP3_dim.a1.0.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.Rec709_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.Rec2020_100nits_dim.a1.0.3)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-1000nit-15nit-REC2020lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 1000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_1000nits_15nits_HLG.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_1000nits_15nits_ST2084.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_1000nits_15nits_HLG.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_1000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (1000 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-1000nit-15nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 1000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_1000nits_15nits_ST2084.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_1000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.0 - SDR Cinema
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Cinema)");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR cinema

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D60_48nits.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_48nits.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3D60_48nits.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3D65_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // Un-tone-mapped
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(Un-tone-mapped)");
            vt->setDescription("");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }
    }

    // Scene Color Spaces
    {

        // ACES2065-1
        {
            static const char* aliases[] = { u8R"(aces2065_1)",u8R"(ACES - ACES2065-1)",u8R"(lin_ap0)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ACES2065-1)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , nullptr
                , u8R"(The "Academy Color Encoding System" reference colorspace.)"
            );
        }

        // ACEScc
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ACEScc_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(ACES - ACEScc)",u8R"(acescc_ap1)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ACEScc)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ACEScc to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScc_to_ACES.a1.0.3

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_ACEScc.a1.0.3)"
);
        }

        // ACEScct
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ACEScct_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(ACES - ACEScct)",u8R"(acescct_ap1)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)","" };
            AddColorSpace(cfg, u8R"(ACEScct)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ACEScct to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScct_to_ACES.a1.0.3

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_ACEScct.a1.0.3)"
);
        }

        // ACEScg
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ACEScg_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(ACES - ACEScg)",u8R"(lin_ap1)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(ACEScg)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ACEScg to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACEScg_to_ACES.a1.0.3

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_ACEScg.a1.0.3)"
);
        }

        // Linear P3-D65
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Linear P3-D65)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.02490528596679, -0.689069761034766, -0.335835524932019, 0, -0.183597032256178, 1.28950620775902, -0.105909175502841, 0, 0.00905856112234766, -0.0592796840575522, 1.0502211229352, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
            }
            static const char* aliases[] = { u8R"(lin_p3d65)",u8R"(Utility - Linear - P3-D65)",u8R"(lin_displayp3)",u8R"(Linear Display P3)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Linear P3-D65)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to linear P3 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Linear_P3-D65:1.0)"
);
        }

        // Linear Rec.2020
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Linear Rec.2020)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 1.49040952054172, -0.26617091926613, -0.224238601275593, 0, -0.0801674998722558, 1.18216712109757, -0.10199962122531, 0, 0.00322763119162216, -0.0347764757450576, 1.03154884455344, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
            }
            static const char* aliases[] = { u8R"(lin_rec2020)",u8R"(Utility - Linear - Rec.2020)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Linear Rec.2020)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to linear Rec.2020 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Linear_Rec2020:1.0)"
);
        }

        // Linear Rec.709 (sRGB)
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Linear Rec.709 (sRGB))");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
            }
            static const char* aliases[] = { u8R"(lin_rec709_srgb)",u8R"(Utility - Linear - Rec.709)",u8R"(lin_rec709)",u8R"(lin_srgb)",u8R"(Utility - Linear - sRGB)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(working-space)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Linear Rec.709 (sRGB))"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to linear Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Linear_Rec709:1.0)"
);
        }

        // Gamma 1.8 Rec.709 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 1.8 Rec.709 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 1.8, 1.8, 1.8, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g18_rec709_tx)",u8R"(Utility - Gamma 1.8 - Rec.709 - Texture)",u8R"(g18_rec709)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Gamma 1.8 Rec.709 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 1.8 gamma-corrected Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma1.8_Rec709-Texture:1.0)"
);
        }

        // Gamma 2.2 AP1 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 2.2 AP1 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 2.2, 2.2, 2.2, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g22_ap1_tx)",u8R"(g22_ap1)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.2 AP1 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 2.2 gamma-corrected AP1 primaries, ACES ~=D60 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma2.2_AP1-Texture:1.0)"
);
        }

        // Gamma 2.2 Rec.709 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 2.2 Rec.709 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 2.2, 2.2, 2.2, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g22_rec709_tx)",u8R"(Utility - Gamma 2.2 - Rec.709 - Texture)",u8R"(g22_rec709)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.2 Rec.709 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 2.2 gamma-corrected Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma2.2_Rec709-Texture:1.0)"
);
        }

        // Gamma 2.4 Rec.709 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Gamma 2.4 Rec.709 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_PASS_THRU);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setValue({ 2.4, 2.4, 2.4, 1 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(g24_rec709_tx)",u8R"(g24_rec709)",u8R"(rec709_display)",u8R"(Utility - Rec.709 - Display)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Gamma 2.4 Rec.709 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to 2.4 gamma-corrected Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_Gamma2.4_Rec709-Texture:1.0)"
);
        }

        // sRGB Encoded AP1 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to sRGB Encoded AP1 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentWithLinearTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_LINEAR);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setGamma({ 2.4, 2.4, 2.4, 1 });
                trFrom1->setOffset({ 0.055, 0.055, 0.055, 0 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(srgb_encoded_ap1_tx)",u8R"(srgb_ap1)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(sRGB Encoded AP1 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to sRGB Encoded AP1 primaries, ACES ~=D60 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_sRGB_Encoded_AP1-Texture:1.0)"
);
        }

        // sRGB Encoded P3-D65 - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to sRGB Encoded P3-D65 - Texture)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.02490528596679, -0.689069761034766, -0.335835524932019, 0, -0.183597032256178, 1.28950620775902, -0.105909175502841, 0, 0.00905856112234766, -0.0592796840575522, 1.0502211229352, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentWithLinearTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_LINEAR);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setGamma({ 2.4, 2.4, 2.4, 1 });
                trFrom1->setOffset({ 0.055, 0.055, 0.055, 0 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(srgb_encoded_p3d65_tx)",u8R"(srgb_p3d65)",u8R"(srgb_displayp3)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(sRGB Encoded P3-D65 - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to sRGB Encoded P3-D65 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_sRGB_Encoded_P3-D65-Texture:1.0)"
);
        }

        // sRGB - Texture
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to sRGB Rec.709)");
            {
                auto trFrom0 = MatrixTransform::Create();
                trFrom0->setDirection(TRANSFORM_DIR_FORWARD);
                trFrom0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trFrom0Mat[] = { 2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1 };
                trFrom0->setMatrix(trFrom0Mat);
                trFrom->appendTransform(trFrom0);
                auto trFrom1 = ExponentWithLinearTransform::Create();
                trFrom1->setNegativeStyle(NEGATIVE_LINEAR);
                trFrom1->setDirection(TRANSFORM_DIR_INVERSE);
                trFrom1->setGamma({ 2.4, 2.4, 2.4, 1 });
                trFrom1->setOffset({ 0.055, 0.055, 0.055, 0 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(srgb_tx)",u8R"(Utility - sRGB - Texture)",u8R"(srgb_texture)",u8R"(Input - Generic - sRGB - Texture)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(sRGB - Texture)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , ""
                , ""
                , u8R"(Utility)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to sRGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:AP0_to_sRGB-Texture:1.0)"
);
        }

        // Raw
        {
            static const char* aliases[] = { u8R"(Utility - Raw)","" };
            static const char* cats[] = { u8R"(file-io)",u8R"(texture)","" };
            AddColorSpace(cfg, u8R"(Raw)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , ""
                , ""
                , u8R"(Utility)"
                , true
                , nullptr
                , nullptr
                , u8R"(The utility "Raw" colorspace.)"
            );
        }
    }

    // Display Color Spaces
    {

        // CIE-XYZ-D65
        {
            static const char* aliases[] = { u8R"(cie_xyz_d65)","" };
            AddColorSpace(cfg, u8R"(CIE-XYZ-D65)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , nullptr
                , ""
                , ""
                , ""
                , false
                , nullptr
                , nullptr
                , u8R"-(The "CIE XYZ (D65)" display connection colorspace.)-"
            );
        }

        // sRGB - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_sRGB)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(srgb_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(sRGB - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to sRGB (piecewise EOTF)

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.RGBmonitor_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.RGBmonitor_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.RGBmonitor_D60sim_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.RGBmonitor_D60sim_100nits_dim.a1.0.3)"
);
        }

        // Display P3 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_DisplayP3)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(displayp3_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Display P3 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Apple Display P3

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.DisplayP3_dim.a1.0.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.DisplayP3_dim.a1.0.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.DisplayP3_D60sim_dim.a1.0.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.DisplayP3_D60sim_dim.a1.0.0)"
);
        }

        // Rec.1886 Rec.709 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.709)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec1886_rec709_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.1886 Rec.709 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.1886/Rec.709 (HD video)

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec709_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.Rec709_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec709_D60sim_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.Rec709_D60sim_100nits_dim.a1.0.3)"
);
        }

        // Rec.2100-PQ - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec2100_pq_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.2100-PQ - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(hdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.2100-PQ

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_1000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_1000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_2000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_2000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_4000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_4000nits_15nits_ST2084.a1.1.0)"
);
        }

        // ST2084-P3-D65 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(st2084_p3d65_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ST2084-P3-D65 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(hdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to ST-2084 (PQ), P3-D65 primaries

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_1000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_1000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_2000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_2000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_4000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_4000nits_15nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_108nits_7point2nits_ST2084.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_108nits_7point2nits_ST2084.a1.1.0)"
);
        }

        // P3-D65 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_G2.6-P3-D65)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(p3d65_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(P3-D65 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Gamma 2.6, P3-D65

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_48nits.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3D65_48nits.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_Rec709limited_48nits.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_D60sim_48nits.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3D65_D60sim_48nits.a1.1.0)"
);
        }
    }

    // Named Transforms
    {

        // Rec.1886 - Curve
        {
            auto trInv = GroupTransform::Create();
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            trInv->getFormatMetadata().setName(u8R"(Linear to Rec.1886)");
            {
                auto trInv0 = ExponentTransform::Create();
                trInv0->setNegativeStyle(NEGATIVE_PASS_THRU);
                trInv0->setDirection(TRANSFORM_DIR_INVERSE);
                trInv0->setValue({ 2.4, 2.4, 2.4, 1 });
                trInv->appendTransform(trInv0);
            }
            static const char* aliases[] = { u8R"(rec1886_crv)",u8R"(Utility - Curve - Rec.1886)",u8R"(crv_rec1886)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(Rec.1886 - Curve)"
                , aliases
                , cats
                , u8R"(sdr-video)"
                , u8R"(Utility)"
                , nullptr
                , trInv
                , u8R"(Convert generic linear RGB to Rec.1886 encoded RGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:Linear_to_Rec1886-Curve:1.0
)"
);
        }

        // sRGB - Curve
        {
            auto trInv = GroupTransform::Create();
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            trInv->getFormatMetadata().setName(u8R"(Linear to sRGB)");
            {
                auto trInv0 = ExponentWithLinearTransform::Create();
                trInv0->setNegativeStyle(NEGATIVE_LINEAR);
                trInv0->setDirection(TRANSFORM_DIR_INVERSE);
                trInv0->setGamma({ 2.4, 2.4, 2.4, 1 });
                trInv0->setOffset({ 0.055, 0.055, 0.055, 0 });
                trInv->appendTransform(trInv0);
            }
            static const char* aliases[] = { u8R"(srgb_crv)",u8R"(Utility - Curve - sRGB)",u8R"(crv_srgb)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(sRGB - Curve)"
                , aliases
                , cats
                , u8R"(sdr-video)"
                , u8R"(Utility)"
                , nullptr
                , trInv
                , u8R"(Convert generic linear RGB to sRGB encoded RGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:Linear_to_sRGB-Curve:1.0
)"
);
        }
    }
    return cfg;
};



} // namespace CGCONFIG
} // namespace OCIO_NAMESPACE
