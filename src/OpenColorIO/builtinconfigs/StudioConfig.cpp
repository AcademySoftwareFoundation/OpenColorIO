// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/StudioConfig.h"

namespace OCIO_NAMESPACE
{
// Create the built-in configs for all versions of the OCIO Studio config for ACES.
// For backwards compatibility, previous versions are kept in the registry but the
// isRecommended flag should be set to false.

namespace STUDIOCONFIG
{

ConstConfigRcPtr Create_STUDIO_CONFIG_V100_ACES_V13_OCIO_V21();
ConstConfigRcPtr Create_STUDIO_CONFIG_V210_ACES_V13_OCIO_V23();

void Register(BuiltinConfigRegistryImpl & registry) noexcept
{
    // If a new built-in config is added, do not forget to update the LATEST_STUDIO_BUILTIN_CONFIG_URI
    // variable (in BuiltinConfigRegistry.cpp).

    registry.addBuiltin(
        "studio-config-v1.0.0_aces-v1.3_ocio-v2.1",
        "Academy Color Encoding System - Studio Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]",
        false,
        Create_STUDIO_CONFIG_V100_ACES_V13_OCIO_V21
    );

    registry.addBuiltin(
        "studio-config-v2.1.0_aces-v1.3_ocio-v2.3",
        "Academy Color Encoding System - Studio Config [COLORSPACES v2.0.0] [ACES v1.3] [OCIO v2.3]",
        true,
        Create_STUDIO_CONFIG_V210_ACES_V13_OCIO_V23
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

// Creates config "studio-config-v1.0.0_aces-v1.3_ocio-v2.1" from scratch using OCIO C++ API
ConstConfigRcPtr Create_STUDIO_CONFIG_V100_ACES_V13_OCIO_V21()
{
    auto cfg = Config::Create();
    cfg->setVersion(2, 1);
    cfg->setStrictParsingEnabled(1);
    cfg->setFamilySeparator('/');
    static double luma[] = { 0.2126, 0.7152, 0.0722 };
    cfg->setDefaultLumaCoefs(luma);
    cfg->setName(u8R"(studio-config-v1.0.0_aces-v1.3_ocio-v2.1)");
    cfg->setDescription(u8R"(Academy Color Encoding System - Studio Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]
------------------------------------------------------------------------------------------

This "OpenColorIO" config is geared toward studios requiring a config that includes a wide variety of camera colorspaces, displays and looks.

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
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Video (D60 sim on D65))", u8R"(ACES 1.0 - SDR Video (D60 sim on D65))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Video (P3 lim))", u8R"(ACES 1.1 - SDR Video (P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Video (Rec.709 lim))", u8R"(ACES 1.1 - SDR Video (Rec.709 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Cinema)", u8R"(ACES 1.0 - SDR Cinema)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))", u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))", u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))", u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))", u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))", u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(Un-tone-mapped)", u8R"(Un-tone-mapped)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
    }

    // Displays
    {
        cfg->addDisplayView(u8R"(sRGB - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(ACES 1.0 - SDR Video (D60 sim on D65))");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(ACES 1.0 - SDR Video (D60 sim on D65))");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(ACES 1.1 - SDR Video (P3 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(ACES 1.1 - SDR Video (Rec.709 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.2100-HLG - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.2100-HLG - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-HLG - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.2100-PQ - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(ST2084-P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-D60 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-D60 - Display)", u8R"(ACES 1.0 - SDR Cinema)");
        cfg->addDisplaySharedView(u8R"(P3-D60 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.0 - SDR Cinema)");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-DCI - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-DCI - Display)", u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))");
        cfg->addDisplaySharedView(u8R"(P3-DCI - Display)", u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))");
        cfg->addDisplaySharedView(u8R"(P3-DCI - Display)", u8R"(Un-tone-mapped)");

    }
    cfg->setActiveDisplays(u8R"(sRGB - Display, Rec.1886 Rec.709 - Display, Rec.1886 Rec.2020 - Display, Rec.2100-HLG - Display, Rec.2100-PQ - Display, ST2084-P3-D65 - Display, P3-D60 - Display, P3-D65 - Display, P3-DCI - Display)");
    cfg->setActiveViews(u8R"(ACES 1.0 - SDR Video, ACES 1.0 - SDR Video (D60 sim on D65), ACES 1.1 - SDR Video (P3 lim), ACES 1.1 - SDR Video (Rec.709 lim), ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (1000 nits & P3 lim), ACES 1.1 - HDR Video (2000 nits & P3 lim), ACES 1.1 - HDR Video (4000 nits & P3 lim), ACES 1.0 - SDR Cinema, ACES 1.1 - SDR Cinema (D60 sim on D65), ACES 1.1 - SDR Cinema (Rec.709 lim), ACES 1.0 - SDR Cinema (D60 sim on DCI), ACES 1.1 - SDR Cinema (D65 sim on DCI), ACES 1.1 - HDR Cinema (108 nits & P3 lim), Un-tone-mapped, Raw)");
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

        // ACES 1.0 - SDR Video (D60 sim on D65)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-D60sim-D65_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Video (D60 sim on D65))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video simulating D60 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.RGBmonitor_D60sim_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec709_D60sim_100nits_dim.a1.0.3)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Video (P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Video (P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_P3D65limited_100nits_dim.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Video (Rec.709 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-REC709lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Video (Rec.709 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_Rec709limited_100nits_dim.a1.1.0)");
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

        // ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-2000nit-15nit-REC2020lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 2000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_2000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-4000nit-15nit-REC2020lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 4000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_4000nits_15nits_ST2084.a1.1.0)");
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

        // ACES 1.1 - HDR Video (2000 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-2000nit-15nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 2000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_2000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (4000 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-4000nit-15nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 4000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_4000nits_15nits_ST2084.a1.1.0)");
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

        // ACES 1.1 - SDR Cinema (D60 sim on D65)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D60sim-D65_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 cinema simulating D60 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_D60sim_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Cinema (Rec.709 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-REC709lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR cinema

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_Rec709limited_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.0 - SDR Cinema (D60 sim on DCI)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D60sim-DCI_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR DCI cinema simulating D60 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3DCI_48nits.a1.0.3)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Cinema (D65 sim on DCI)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D65sim-DCI_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR DCI cinema simulating D65 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3DCI_D65sim_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Cinema (108 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-CINEMA-108nit-7.2nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 108 nit HDR D65 cinema

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_108nits_7point2nits_ST2084.a1.1.0)");
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

        // ADX10
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ADX10_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(Input - ADX - ADX10)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ADX10)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ADX10 to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ADX10_to_ACES.a1.0.3)"
);
        }

        // ADX16
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ADX16_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(Input - ADX - ADX16)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ADX16)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ADX16 to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ADX16_to_ACES.a1.0.3)"
);
        }

        // Linear ARRI Wide Gamut 3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear ARRI Wide Gamut 3 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.680205505106279, 0.236136601606481, 0.0836578932872399, 0, 0.0854149797421404, 1.01747087860704, -0.102885858349182, 0, 0.00205652166929683, -0.0625625003847921, 1.0605059787155, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_arri_wide_gamut_3)",u8R"(Input - ARRI - Linear - ALEXA Wide Gamut)",u8R"(lin_alexawide)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear ARRI Wide Gamut 3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear ARRI Wide Gamut 3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:Linear_ARRI_Wide_Gamut_3_to_ACES2065-1:1.0)"
);
        }

        // ARRI LogC3 (EI800)
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(ARRI LogC3 (EI800) to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.0105909904954696, 0.0105909904954696, 0.0105909904954696 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.247189638318671, 0.247189638318671, 0.247189638318671 });
                trTo0->setLogSideOffsetValue({ 0.385536998692443, 0.385536998692443, 0.385536998692443 });
                trTo0->setLinSideSlopeValue({ 5.55555555555556, 5.55555555555556, 5.55555555555556 });
                trTo0->setLinSideOffsetValue({ 0.0522722750251688, 0.0522722750251688, 0.0522722750251688 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.680205505106279, 0.236136601606481, 0.0836578932872399, 0, 0.0854149797421404, 1.01747087860704, -0.102885858349182, 0, 0.00205652166929683, -0.0625625003847921, 1.0605059787155, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(arri_logc3_ei800)",u8R"(Input - ARRI - V3 LogC (EI800) - Wide Gamut)",u8R"(logc3ei800_alexawide)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ARRI LogC3 (EI800))"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ARRI LogC3 (EI800) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC3_EI800_to_ACES2065-1:1.0)"
);
        }

        // Linear ARRI Wide Gamut 4
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear ARRI Wide Gamut 4 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.750957362824734, 0.144422786709757, 0.104619850465509, 0, 0.000821837079380207, 1.007397584885, -0.00821942196438358, 0, -0.000499952143533471, -0.000854177231436971, 1.00135412937497, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_arri_wide_gamut_4)",u8R"(lin_awg4)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear ARRI Wide Gamut 4)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear ARRI Wide Gamut 4 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:Linear_ARRI_Wide_Gamut_4_to_ACES2065-1:1.0)"
);
        }

        // ARRI LogC4
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(ARRI LogC4 to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ -0.0180569961199113, -0.0180569961199113, -0.0180569961199113 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.0647954196341293, 0.0647954196341293, 0.0647954196341293 });
                trTo0->setLogSideOffsetValue({ -0.295908392682586, -0.295908392682586, -0.295908392682586 });
                trTo0->setLinSideSlopeValue({ 2231.82630906769, 2231.82630906769, 2231.82630906769 });
                trTo0->setLinSideOffsetValue({ 64, 64, 64 });
                trTo0->setBase(2);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.750957362824734, 0.144422786709757, 0.104619850465509, 0, 0.000821837079380207, 1.007397584885, -0.00821942196438358, 0, -0.000499952143533471, -0.000854177231436971, 1.00135412937497, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(arri_logc4)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ARRI LogC4)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ARRI LogC4 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC4_to_ACES2065-1:1.0)"
);
        }

        // BMDFilm WideGamut Gen5
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Blackmagic Film Wide Gamut (Gen 5) to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.005, 0.005, 0.005 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.0869287606549122, 0.0869287606549122, 0.0869287606549122 });
                trTo0->setLogSideOffsetValue({ 0.530013339229194, 0.530013339229194, 0.530013339229194 });
                trTo0->setLinSideSlopeValue({ 1, 1, 1 });
                trTo0->setLinSideOffsetValue({ 0.00549407243225781, 0.00549407243225781, 0.00549407243225781 });
                trTo0->setBase(2.71828182845905);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.647091325580708, 0.242595385134207, 0.110313289285085, 0, 0.0651915997328519, 1.02504756760476, -0.0902391673376125, 0, -0.0275570729194699, -0.0805887097177784, 1.10814578263725, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(bmdfilm_widegamut_gen5)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(BMDFilm WideGamut Gen5)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Blackmagic Film Wide Gamut (Gen 5) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:BMDFilm_WideGamut_Gen5_to_ACES2065-1:1.0)"
);
        }

        // DaVinci Intermediate WideGamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(DaVinci Intermediate Wide Gamut to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.00262409, 0.00262409, 0.00262409 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.07329248, 0.07329248, 0.07329248 });
                trTo0->setLogSideOffsetValue({ 0.51304736, 0.51304736, 0.51304736 });
                trTo0->setLinSideSlopeValue({ 1, 1, 1 });
                trTo0->setLinSideOffsetValue({ 0.0075, 0.0075, 0.0075 });
                trTo0->setLinearSlopeValue({ 10.44426855, 10.44426855, 10.44426855 });
                trTo0->setBase(2);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.748270290272981, 0.167694659554328, 0.0840350501726906, 0, 0.0208421234689102, 1.11190474268894, -0.132746866157851, 0, -0.0915122574225729, -0.127746712807307, 1.21925897022988, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(davinci_intermediate_widegamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(DaVinci Intermediate WideGamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert DaVinci Intermediate Wide Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:DaVinci_Intermediate_WideGamut_to_ACES2065-1:1.0)"
);
        }

        // Linear BMD WideGamut Gen5
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Blackmagic Wide Gamut (Gen 5) to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.647091325580708, 0.242595385134207, 0.110313289285085, 0, 0.0651915997328519, 1.02504756760476, -0.0902391673376125, 0, -0.0275570729194699, -0.0805887097177784, 1.10814578263725, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_bmd_widegamut_gen5)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear BMD WideGamut Gen5)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Blackmagic Wide Gamut (Gen 5) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:Linear_BMD_WideGamut_Gen5_to_ACES2065-1:1.0)"
);
        }

        // Linear DaVinci WideGamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear DaVinci Wide Gamut to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.748270290272981, 0.167694659554328, 0.0840350501726906, 0, 0.0208421234689102, 1.11190474268894, -0.132746866157851, 0, -0.0915122574225729, -0.127746712807307, 1.21925897022988, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_davinci_widegamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear DaVinci WideGamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear DaVinci Wide Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:Linear_DaVinci_WideGamut_to_ACES2065-1:1.0)"
);
        }

        // CanonLog3 CinemaGamut D55
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(CANON_CLOG3-CGAMUT_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(canonlog3_cinemagamut_d55)",u8R"(Input - Canon - Canon-Log3 - Cinema Gamut Daylight)",u8R"(canonlog3_cgamutday)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(CanonLog3 CinemaGamut D55)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Canon)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Canon Log 3 Cinema Gamut to ACES2065-1)"
            );
        }

        // Linear CinemaGamut D55
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Canon Cinema Gamut (Daylight) to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.763064454775734, 0.14902116113706, 0.0879143840872056, 0, 0.00365745670512393, 1.10696038037622, -0.110617837081339, 0, -0.0094077940457189, -0.218383304989987, 1.22779109903571, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_cinemagamut_d55)",u8R"(Input - Canon - Linear - Canon Cinema Gamut Daylight)",u8R"(lin_canoncgamutday)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear CinemaGamut D55)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Canon)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Canon Cinema Gamut (Daylight) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Canon:Input:Linear-CinemaGamut-D55_to_ACES2065-1:1.0)"
);
        }

        // Linear V-Gamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Panasonic V-Gamut to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.72461670413153, 0.166915288193706, 0.108468007674764, 0, 0.021390245413146, 0.984908155703054, -0.00629840111620089, 0, -0.00923556287076561, -0.00105690563900513, 1.01029246850977, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_vgamut)",u8R"(Input - Panasonic - Linear - V-Gamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear V-Gamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Panasonic)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Panasonic V-Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Panasonic:Input:Linear_VGamut_to_ACES2065-1:1.0)"
);
        }

        // V-Log V-Gamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Panasonic V-Log - V-Gamut to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01, 0.01, 0.01 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.241514, 0.241514, 0.241514 });
                trTo0->setLogSideOffsetValue({ 0.598206, 0.598206, 0.598206 });
                trTo0->setLinSideSlopeValue({ 1, 1, 1 });
                trTo0->setLinSideOffsetValue({ 0.00873, 0.00873, 0.00873 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.72461670413153, 0.166915288193706, 0.108468007674764, 0, 0.021390245413146, 0.984908155703054, -0.00629840111620089, 0, -0.00923556287076561, -0.00105690563900513, 1.01029246850977, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(vlog_vgamut)",u8R"(Input - Panasonic - V-Log - V-Gamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(V-Log V-Gamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Panasonic)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Panasonic V-Log - V-Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Panasonic:Input:VLog_VGamut_to_ACES2065-1:1.0)"
);
        }

        // Linear REDWideGamutRGB
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear REDWideGamutRGB to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.785058804068092, 0.0838587565440846, 0.131082439387823, 0, 0.0231738348454756, 1.08789754919233, -0.111071384037806, 0, -0.0737604353682082, -0.314590072290208, 1.38835050765842, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_redwidegamutrgb)",u8R"(Input - RED - Linear - REDWideGamutRGB)",u8R"(lin_rwg)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear REDWideGamutRGB)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/RED)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear REDWideGamutRGB to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:RED:Input:Linear_REDWideGamutRGB_to_ACES2065-1:1.0)"
);
        }

        // Log3G10 REDWideGamutRGB
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(RED Log3G10 REDWideGamutRGB to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ -0.01, -0.01, -0.01 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.224282, 0.224282, 0.224282 });
                trTo0->setLogSideOffsetValue({ 0, 0, 0 });
                trTo0->setLinSideSlopeValue({ 155.975327, 155.975327, 155.975327 });
                trTo0->setLinSideOffsetValue({ 2.55975327, 2.55975327, 2.55975327 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.785058804068092, 0.0838587565440846, 0.131082439387823, 0, 0.0231738348454756, 1.08789754919233, -0.111071384037806, 0, -0.0737604353682082, -0.314590072290208, 1.38835050765842, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(log3g10_redwidegamutrgb)",u8R"(Input - RED - REDLog3G10 - REDWideGamutRGB)",u8R"(rl3g10_rwg)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Log3G10 REDWideGamutRGB)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/RED)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert RED Log3G10 REDWideGamutRGB to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:RED:Input:Log3G10_REDWideGamutRGB_to_ACES2065-1:1.0)"
);
        }

        // Linear S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.75298259539984, 0.143370216235557, 0.103647188364603, 0, 0.0217076974414429, 1.01531883550528, -0.0370265329467195, 0, -0.00941605274963355, 0.00337041785882367, 1.00604563489081, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_sgamut3)",u8R"(Input - Sony - Linear - S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_SGamut3_to_ACES2065-1:1.0)"
);
        }

        // Linear S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.638788667185978, 0.272351433711262, 0.0888598991027595, 0, -0.00391590602528224, 1.0880732308974, -0.0841573248721177, 0, -0.0299072021239151, -0.0264325799101947, 1.05633978203411, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_sgamut3cine)",u8R"(Input - Sony - Linear - S-Gamut3.Cine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_SGamut3Cine_to_ACES2065-1:1.0)"
);
        }

        // Linear Venice S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Venice S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.793329741146434, 0.0890786256206771, 0.117591633232888, 0, 0.0155810585252582, 1.03271230692988, -0.0482933654551394, 0, -0.0188647477991488, 0.0127694120973433, 1.0060953357018, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_venice_sgamut3)",u8R"(Input - Sony - Linear - Venice S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear Venice S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Venice S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_Venice_SGamut3_to_ACES2065-1:1.0)"
);
        }

        // Linear Venice S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Venice S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.674257092126512, 0.220571735923397, 0.10517117195009, 0, -0.00931360607857167, 1.10595886142466, -0.0966452553460855, 0, -0.0382090673002312, -0.017938376600236, 1.05614744390047, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_venice_sgamut3cine)",u8R"(Input - Sony - Linear - Venice S-Gamut3.Cine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear Venice S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Venice S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_Venice_SGamut3Cine_to_ACES2065-1:1.0)"
);
        }

        // S-Log3 S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.75298259539984, 0.143370216235557, 0.103647188364603, 0, 0.0217076974414429, 1.01531883550528, -0.0370265329467195, 0, -0.00941605274963355, 0.00337041785882367, 1.00604563489081, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_sgamut3)",u8R"(Input - Sony - S-Log3 - S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_SGamut3_to_ACES2065-1:1.0)"
);
        }

        // S-Log3 S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.638788667185978, 0.272351433711262, 0.0888598991027595, 0, -0.00391590602528224, 1.0880732308974, -0.0841573248721177, 0, -0.0299072021239151, -0.0264325799101947, 1.05633978203411, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_sgamut3cine)",u8R"(Input - Sony - S-Log3 - S-Gamut3.Cine)",u8R"(slog3_sgamutcine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_SGamut3Cine_to_ACES2065-1:1.0)"
);
        }

        // S-Log3 Venice S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 Venice S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.793329741146434, 0.089078625620677, 0.117591633232888, 0, 0.0155810585252582, 1.03271230692988, -0.0482933654551394, 0, -0.0188647477991488, 0.0127694120973433, 1.00609533570181, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_venice_sgamut3)",u8R"(Input - Sony - S-Log3 - Venice S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 Venice S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 Venice S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_Venice_SGamut3_to_ACES2065-1:1.0)"
);
        }

        // S-Log3 Venice S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 Venice S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.674257092126512, 0.220571735923397, 0.10517117195009, 0, -0.00931360607857167, 1.10595886142466, -0.0966452553460855, 0, -0.0382090673002312, -0.017938376600236, 1.05614744390047, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_venice_sgamut3cine)",u8R"(Input - Sony - S-Log3 - Venice S-Gamut3.Cine)",u8R"(slog3_venice_sgamutcine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 Venice S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 Venice S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_Venice_SGamut3Cine_to_ACES2065-1:1.0)"
);
        }

        // Camera Rec.709
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Camera Rec.709)");
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
                trFrom1->setGamma({ 2.22222222222222, 2.22222222222222, 2.22222222222222, 1 });
                trFrom1->setOffset({ 0.099, 0.099, 0.099, 0 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(camera_rec709)",u8R"(Utility - Rec.709 - Camera)",u8R"(rec709_camera)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Camera Rec.709)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility/ITU)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to Rec.709 camera OETF Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:ITU:Utility:AP0_to_Camera_Rec709:1.0)"
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

        // Rec.1886 Rec.2020 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.2020)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec1886_rec2020_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.1886 Rec.2020 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.1886/Rec.2020 (UHD video))"
            );
        }

        // Rec.2100-HLG - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.2100-HLG-1000nit)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec2100_hlg_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.2100-HLG - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(hdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.2100-HLG, 1000 nit)"
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

        // P3-D60 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_G2.6-P3-D60-BFD)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(p3d60_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(P3-D60 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Gamma 2.6, P3-D60 (Bradford adaptation))"
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

        // P3-DCI - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_G2.6-P3-DCI-BFD)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(p3_dci_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(P3-DCI - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Gamma 2.6, P3-DCI (DCI white with Bradford adaptation))"
            );
        }
    }

    // Named Transforms
    {

        // ARRI LogC3 - Curve (EI800)
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(ARRI LogC3 Curve (EI800) to Relative Scene Linear)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.0105909904954696, 0.0105909904954696, 0.0105909904954696 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.247189638318671, 0.247189638318671, 0.247189638318671 });
                trFwd0->setLogSideOffsetValue({ 0.385536998692443, 0.385536998692443, 0.385536998692443 });
                trFwd0->setLinSideSlopeValue({ 5.55555555555556, 5.55555555555556, 5.55555555555556 });
                trFwd0->setLinSideOffsetValue({ 0.0522722750251688, 0.0522722750251688, 0.0522722750251688 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(arri_logc3_crv_ei800)",u8R"(Input - ARRI - Curve - V3 LogC (EI800))",u8R"(crv_logc3ei800)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(ARRI LogC3 - Curve (EI800))"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/ARRI)"
                , trFwd
                , nullptr
                , u8R"(Convert ARRI LogC3 Curve (EI800) to Relative Scene Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC3_Curve_EI800_to_Linear:1.0
)"
);
        }

        // ARRI LogC4 - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(ARRI LogC4 Curve to Relative Scene Linear)");
            {
                auto trFwd0 = LogCameraTransform::Create({ -0.0180569961199113, -0.0180569961199113, -0.0180569961199113 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.0647954196341293, 0.0647954196341293, 0.0647954196341293 });
                trFwd0->setLogSideOffsetValue({ -0.295908392682586, -0.295908392682586, -0.295908392682586 });
                trFwd0->setLinSideSlopeValue({ 2231.82630906769, 2231.82630906769, 2231.82630906769 });
                trFwd0->setLinSideOffsetValue({ 64, 64, 64 });
                trFwd0->setBase(2);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(arri_logc4_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(ARRI LogC4 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/ARRI)"
                , trFwd
                , nullptr
                , u8R"(Convert ARRI LogC4 Curve to Relative Scene Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC4_Curve_to_Linear:1.0
)"
);
        }

        // BMDFilm Gen5 Log - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(Blackmagic Film (Gen 5) Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.005, 0.005, 0.005 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.0869287606549122, 0.0869287606549122, 0.0869287606549122 });
                trFwd0->setLogSideOffsetValue({ 0.530013339229194, 0.530013339229194, 0.530013339229194 });
                trFwd0->setLinSideSlopeValue({ 1, 1, 1 });
                trFwd0->setLinSideOffsetValue({ 0.00549407243225781, 0.00549407243225781, 0.00549407243225781 });
                trFwd0->setBase(2.71828182845905);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(bmdfilm_gen5_log_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(BMDFilm Gen5 Log - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/BlackmagicDesign)"
                , trFwd
                , nullptr
                , u8R"(Convert Blackmagic Film (Gen 5) Log to Blackmagic Film (Gen 5) Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:BMDFilm_Gen5_Log-Curve_to_Linear:1.0
)"
);
        }

        // DaVinci Intermediate Log - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(DaVinci Intermediate Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.00262409, 0.00262409, 0.00262409 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.07329248, 0.07329248, 0.07329248 });
                trFwd0->setLogSideOffsetValue({ 0.51304736, 0.51304736, 0.51304736 });
                trFwd0->setLinSideSlopeValue({ 1, 1, 1 });
                trFwd0->setLinSideOffsetValue({ 0.0075, 0.0075, 0.0075 });
                trFwd0->setLinearSlopeValue({ 10.44426855, 10.44426855, 10.44426855 });
                trFwd0->setBase(2);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(davinci_intermediate_log_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(DaVinci Intermediate Log - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/BlackmagicDesign)"
                , trFwd
                , nullptr
                , u8R"(Convert DaVinci Intermediate Log to DaVinci Intermediate Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:DaVinci_Intermediate_Log-Curve_to_Linear:1.0
)"
);
        }

        // V-Log - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(Panasonic V-Log Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.01, 0.01, 0.01 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.241514, 0.241514, 0.241514 });
                trFwd0->setLogSideOffsetValue({ 0.598206, 0.598206, 0.598206 });
                trFwd0->setLinSideSlopeValue({ 1, 1, 1 });
                trFwd0->setLinSideOffsetValue({ 0.00873, 0.00873, 0.00873 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(vlog_crv)",u8R"(Input - Panasonic - Curve - V-Log)",u8R"(crv_vlog)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(V-Log - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/Panasonic)"
                , trFwd
                , nullptr
                , u8R"(Convert Panasonic V-Log Log (arbitrary primaries) to Panasonic V-Log Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:Panasonic:Input:VLog-Curve_to_Linear:1.0
)"
);
        }

        // Log3G10 - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(RED Log3G10 Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ -0.01, -0.01, -0.01 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.224282, 0.224282, 0.224282 });
                trFwd0->setLogSideOffsetValue({ 0, 0, 0 });
                trFwd0->setLinSideSlopeValue({ 155.975327, 155.975327, 155.975327 });
                trFwd0->setLinSideOffsetValue({ 2.55975327, 2.55975327, 2.55975327 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(log3g10_crv)",u8R"(Input - RED - Curve - REDLog3G10)",u8R"(crv_rl3g10)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(Log3G10 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/RED)"
                , trFwd
                , nullptr
                , u8R"(Convert RED Log3G10 Log (arbitrary primaries) to RED Log3G10 Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:RED:Input:Log3G10-Curve_to_Linear:1.0
)"
);
        }

        // S-Log3 - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(S-Log3 Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trFwd0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trFwd0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trFwd0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trFwd0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(slog3_crv)",u8R"(Input - Sony - Curve - S-Log3)",u8R"(crv_slog3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(S-Log3 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/Sony)"
                , trFwd
                , nullptr
                , u8R"(Convert S-Log3 Log (arbitrary primaries) to S-Log3 Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3-Curve_to_Linear:1.0
)"
);
        }

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

        // Rec.709 - Curve
        {
            auto trInv = GroupTransform::Create();
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            trInv->getFormatMetadata().setName(u8R"(Linear to Rec.709)");
            {
                auto trInv0 = ExponentWithLinearTransform::Create();
                trInv0->setNegativeStyle(NEGATIVE_LINEAR);
                trInv0->setDirection(TRANSFORM_DIR_INVERSE);
                trInv0->setGamma({ 2.22222222222222, 2.22222222222222, 2.22222222222222, 1 });
                trInv0->setOffset({ 0.099, 0.099, 0.099, 0 });
                trInv->appendTransform(trInv0);
            }
            static const char* aliases[] = { u8R"(rec709_crv)",u8R"(Utility - Curve - Rec.709)",u8R"(crv_rec709)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(Rec.709 - Curve)"
                , aliases
                , cats
                , u8R"(sdr-video)"
                , u8R"(Utility/ITU)"
                , nullptr
                , trInv
                , u8R"(Convert generic linear RGB to generic gamma-corrected RGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:ITU:Utility:Linear_to_Rec709-Curve:1.0
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

        // ST-2084 - Curve
        {
            auto trInv = BuiltinTransform::Create();
            trInv->setStyle(u8R"(CURVE - LINEAR_to_ST-2084)");
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(st_2084_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(ST-2084 - Curve)"
                , aliases
                , cats
                , u8R"(hdr-video)"
                , u8R"(Utility)"
                , nullptr
                , trInv
                , u8R"(Convert linear nits/100 to SMPTE ST-2084 (PQ) full-range)"
            );
        }
    }
    return cfg;
};

// Creates config "studio-config-v2.1.0_aces-v1.3_ocio-v2.3" from scratch using OCIO C++ API
ConstConfigRcPtr Create_STUDIO_CONFIG_V210_ACES_V13_OCIO_V23()
{
    auto cfg = Config::Create();
    cfg->setVersion(2, 3);
    cfg->setStrictParsingEnabled(1);
    cfg->setFamilySeparator('/');
    static double luma[] = { 0.2126, 0.7152, 0.0722 };
    cfg->setDefaultLumaCoefs(luma);
    cfg->setName(u8R"(studio-config-v2.1.0_aces-v1.3_ocio-v2.3)");
    cfg->setDescription(u8R"(Academy Color Encoding System - Studio Config [COLORSPACES v2.1.0] [ACES v1.3] [OCIO v2.3]
------------------------------------------------------------------------------------------

This "OpenColorIO" config is geared toward studios requiring a config that includes a wide variety of camera colorspaces, displays and looks.)");

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
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Video (D60 sim on D65))", u8R"(ACES 1.0 - SDR Video (D60 sim on D65))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Video (P3 lim))", u8R"(ACES 1.1 - SDR Video (P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Video (Rec.709 lim))", u8R"(ACES 1.1 - SDR Video (Rec.709 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))", u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))", u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Cinema)", u8R"(ACES 1.0 - SDR Cinema)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))", u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))", u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))", u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))", u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))", u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
        cfg->addSharedView(u8R"(Un-tone-mapped)", u8R"(Un-tone-mapped)", u8R"(<USE_DISPLAY_NAME>)", "", "", "");
    }

    // Displays
    {
        cfg->addDisplayView(u8R"(sRGB - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(ACES 1.0 - SDR Video (D60 sim on D65))");
        cfg->addDisplaySharedView(u8R"(sRGB - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Display P3 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Display P3 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Display P3 - Display)", u8R"(ACES 1.0 - SDR Video (D60 sim on D65))");
        cfg->addDisplaySharedView(u8R"(Display P3 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(ACES 1.0 - SDR Video (D60 sim on D65))");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.709 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(ACES 1.0 - SDR Video)");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(ACES 1.1 - SDR Video (P3 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(ACES 1.1 - SDR Video (Rec.709 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.1886 Rec.2020 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.2100-HLG - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.2100-HLG - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-HLG - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(Rec.2100-PQ - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))");
        cfg->addDisplaySharedView(u8R"(Rec.2100-PQ - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(ST2084-P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (1000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))");
        cfg->addDisplaySharedView(u8R"(ST2084-P3-D65 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-D60 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-D60 - Display)", u8R"(ACES 1.0 - SDR Cinema)");
        cfg->addDisplaySharedView(u8R"(P3-D60 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-D65 - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.0 - SDR Cinema)");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))");
        cfg->addDisplaySharedView(u8R"(P3-D65 - Display)", u8R"(Un-tone-mapped)");

        cfg->addDisplayView(u8R"(P3-DCI - Display)", u8R"(Raw)", "", u8R"(Raw)", "", "", "");
        cfg->addDisplaySharedView(u8R"(P3-DCI - Display)", u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))");
        cfg->addDisplaySharedView(u8R"(P3-DCI - Display)", u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))");
        cfg->addDisplaySharedView(u8R"(P3-DCI - Display)", u8R"(Un-tone-mapped)");

    }
    cfg->setActiveDisplays(u8R"(sRGB - Display, Display P3 - Display, Rec.1886 Rec.709 - Display, Rec.1886 Rec.2020 - Display, Rec.2100-HLG - Display, Rec.2100-PQ - Display, ST2084-P3-D65 - Display, P3-D60 - Display, P3-D65 - Display, P3-DCI - Display)");
    cfg->setActiveViews(u8R"(ACES 1.0 - SDR Video, ACES 1.0 - SDR Video (D60 sim on D65), ACES 1.1 - SDR Video (P3 lim), ACES 1.1 - SDR Video (Rec.709 lim), ACES 1.1 - HDR Video (1000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim), ACES 1.1 - HDR Video (1000 nits & P3 lim), ACES 1.1 - HDR Video (2000 nits & P3 lim), ACES 1.1 - HDR Video (4000 nits & P3 lim), ACES 1.0 - SDR Cinema, ACES 1.1 - SDR Cinema (Rec.709 lim), ACES 1.0 - SDR Cinema (D60 sim on DCI), ACES 1.1 - SDR Cinema (D60 sim on D65), ACES 1.1 - SDR Cinema (D65 sim on DCI), ACES 1.1 - HDR Cinema (108 nits & P3 lim), Un-tone-mapped, Raw)");
    cfg->setInactiveColorSpaces(u8R"(CIE-XYZ-D65, sRGB - Display, Display P3 - Display, Rec.1886 Rec.709 - Display, Rec.1886 Rec.2020 - Display, Rec.2100-HLG - Display, Rec.2100-PQ - Display, ST2084-P3-D65 - Display, P3-D60 - Display, P3-D65 - Display, P3-DCI - Display)");

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

        // ACES 1.0 - SDR Video (D60 sim on D65)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-D60sim-D65_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Video (D60 sim on D65))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video simulating D60 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.RGBmonitor_D60sim_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.DisplayP3_D60sim_dim.a1.0.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec709_D60sim_100nits_dim.a1.0.3

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.RGBmonitor_D60sim_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.DisplayP3_D60sim_dim.a1.0.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.Rec709_D60sim_100nits_dim.a1.0.3)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Video (P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Video (P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_P3D65limited_100nits_dim.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Video (Rec.709 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-REC709lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Video (Rec.709 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_Rec709limited_100nits_dim.a1.1.0)");
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

        // ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-2000nit-15nit-REC2020lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (2000 nits & Rec.2020 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 2000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_2000nits_15nits_ST2084.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_2000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-4000nit-15nit-REC2020lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (4000 nits & Rec.2020 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 4000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_4000nits_15nits_ST2084.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_4000nits_15nits_ST2084.a1.1.0)");
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

        // ACES 1.1 - HDR Video (2000 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-2000nit-15nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (2000 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 2000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_2000nits_15nits_ST2084.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_2000nits_15nits_ST2084.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Video (4000 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-4000nit-15nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Video (4000 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 4000 nit HDR D65 video

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_4000nits_15nits_ST2084.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_4000nits_15nits_ST2084.a1.1.0)");
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

        // ACES 1.1 - SDR Cinema (Rec.709 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-REC709lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Cinema (Rec.709 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR cinema

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_Rec709limited_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.0 - SDR Cinema (D60 sim on DCI)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D60sim-DCI_1.0)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.0 - SDR Cinema (D60 sim on DCI))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR DCI cinema simulating D60 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3DCI_48nits.a1.0.3

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3DCI_48nits.a1.0.3)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Cinema (D60 sim on D65)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D60sim-D65_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Cinema (D60 sim on D65))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR D65 cinema simulating D60 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D65_D60sim_48nits.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3D65_D60sim_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - SDR Cinema (D65 sim on DCI)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D65sim-DCI_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - SDR Cinema (D65 sim on DCI))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for SDR DCI cinema simulating D65 white

ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3DCI_D65sim_48nits.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3DCI_D65sim_48nits.a1.1.0)");
            vt->setFamily("");
            vt->setTransform(trFrom, VIEWTRANSFORM_DIR_FROM_REFERENCE);
            cfg->addViewTransform(vt);
        }

        // ACES 1.1 - HDR Cinema (108 nits & P3 lim)
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-CINEMA-108nit-7.2nit-P3lim_1.1)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            auto vt = ViewTransform::Create(REFERENCE_SPACE_SCENE);
            vt->setName(u8R"(ACES 1.1 - HDR Cinema (108 nits & P3 lim))");
            vt->setDescription(u8R"(Component of ACES Output Transforms for 108 nit HDR D65 cinema

ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.P3D65_108nits_7point2nits_ST2084.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.P3D65_108nits_7point2nits_ST2084.a1.1.0)");
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

        // ADX10
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ADX10_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(Input - ADX - ADX10)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ADX10)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ADX10 to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ADX10_to_ACES.a1.0.3)"
);
        }

        // ADX16
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(ADX16_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(Input - ADX - ADX16)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ADX16)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(ACES)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ADX16 to ACES2065-1

ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ADX16_to_ACES.a1.0.3)"
);
        }

        // Linear ARRI Wide Gamut 3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear ARRI Wide Gamut 3 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.680205505106279, 0.236136601606481, 0.0836578932872398, 0, 0.0854149797421404, 1.01747087860704, -0.102885858349182, 0, 0.00205652166929683, -0.0625625003847921, 1.06050597871549, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_arri_wide_gamut_3)",u8R"(Input - ARRI - Linear - ALEXA Wide Gamut)",u8R"(lin_alexawide)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear ARRI Wide Gamut 3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear ARRI Wide Gamut 3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:Linear_ARRI_Wide_Gamut_3_to_ACES2065-1:1.0)"
);
        }

        // ARRI LogC3 (EI800)
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(ARRI LogC3 (EI800) to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.0105909904954696, 0.0105909904954696, 0.0105909904954696 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.247189638318671, 0.247189638318671, 0.247189638318671 });
                trTo0->setLogSideOffsetValue({ 0.385536998692443, 0.385536998692443, 0.385536998692443 });
                trTo0->setLinSideSlopeValue({ 5.55555555555556, 5.55555555555556, 5.55555555555556 });
                trTo0->setLinSideOffsetValue({ 0.0522722750251688, 0.0522722750251688, 0.0522722750251688 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.680205505106279, 0.236136601606481, 0.0836578932872398, 0, 0.0854149797421404, 1.01747087860704, -0.102885858349182, 0, 0.00205652166929683, -0.0625625003847921, 1.06050597871549, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(arri_logc3_ei800)",u8R"(Input - ARRI - V3 LogC (EI800) - Wide Gamut)",u8R"(logc3ei800_alexawide)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ARRI LogC3 (EI800))"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ARRI LogC3 (EI800) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC3_EI800_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.ARRI.Alexa-v3-logC-EI800.a1.v2

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_LogC_EI800_AWG.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.LogC_EI800_AWG_to_ACES.a1.1.0)"
);
        }

        // Linear ARRI Wide Gamut 4
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear ARRI Wide Gamut 4 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.750957362824734, 0.144422786709757, 0.104619850465509, 0, 0.000821837079380207, 1.007397584885, -0.00821942196438358, 0, -0.000499952143533471, -0.000854177231436971, 1.00135412937497, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_arri_wide_gamut_4)",u8R"(lin_awg4)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear ARRI Wide Gamut 4)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear ARRI Wide Gamut 4 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:Linear_ARRI_Wide_Gamut_4_to_ACES2065-1:1.0)"
);
        }

        // ARRI LogC4
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(ARRI LogC4 to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ -0.0180569961199113, -0.0180569961199113, -0.0180569961199113 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.0647954196341293, 0.0647954196341293, 0.0647954196341293 });
                trTo0->setLogSideOffsetValue({ -0.295908392682586, -0.295908392682586, -0.295908392682586 });
                trTo0->setLinSideSlopeValue({ 2231.82630906769, 2231.82630906769, 2231.82630906769 });
                trTo0->setLinSideOffsetValue({ 64, 64, 64 });
                trTo0->setBase(2);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.750957362824734, 0.144422786709757, 0.104619850465509, 0, 0.000821837079380207, 1.007397584885, -0.00821942196438358, 0, -0.000499952143533471, -0.000854177231436971, 1.00135412937497, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(arri_logc4)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(ARRI LogC4)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/ARRI)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert ARRI LogC4 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC4_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.ARRI.ARRI-LogC4.a1.v1

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.LogC4_to_ACES.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_LogC4.a1.1.0)"
);
        }

        // BMDFilm WideGamut Gen5
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Blackmagic Film Wide Gamut (Gen 5) to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.005, 0.005, 0.005 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.0869287606549122, 0.0869287606549122, 0.0869287606549122 });
                trTo0->setLogSideOffsetValue({ 0.530013339229194, 0.530013339229194, 0.530013339229194 });
                trTo0->setLinSideSlopeValue({ 1, 1, 1 });
                trTo0->setLinSideOffsetValue({ 0.00549407243225781, 0.00549407243225781, 0.00549407243225781 });
                trTo0->setBase(2.71828182845905);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.647091325580708, 0.242595385134207, 0.110313289285085, 0, 0.0651915997328519, 1.02504756760476, -0.0902391673376125, 0, -0.0275570729194699, -0.0805887097177784, 1.10814578263725, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(bmdfilm_widegamut_gen5)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(BMDFilm WideGamut Gen5)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Blackmagic Film Wide Gamut (Gen 5) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:BMDFilm_WideGamut_Gen5_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.BlackmagicDesign.BMDFilm_WideGamut_Gen5.a1.v1

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_BMDFilm_WideGamut_Gen5.a1.v1
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.BMDFilm_WideGamut_Gen5_to_ACES.a1.v1)"
);
        }

        // DaVinci Intermediate WideGamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(DaVinci Intermediate Wide Gamut to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.00262409, 0.00262409, 0.00262409 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.07329248, 0.07329248, 0.07329248 });
                trTo0->setLogSideOffsetValue({ 0.51304736, 0.51304736, 0.51304736 });
                trTo0->setLinSideSlopeValue({ 1, 1, 1 });
                trTo0->setLinSideOffsetValue({ 0.0075, 0.0075, 0.0075 });
                trTo0->setLinearSlopeValue({ 10.44426855, 10.44426855, 10.44426855 });
                trTo0->setBase(2);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.748270290272981, 0.167694659554328, 0.0840350501726906, 0, 0.0208421234689102, 1.11190474268894, -0.132746866157851, 0, -0.0915122574225729, -0.127746712807307, 1.21925897022988, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(davinci_intermediate_widegamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(DaVinci Intermediate WideGamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert DaVinci Intermediate Wide Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:DaVinci_Intermediate_WideGamut_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.DaVinci_Intermediate_WideGamut_to_ACES.a1.v1)"
);
        }

        // Linear BMD WideGamut Gen5
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Blackmagic Wide Gamut (Gen 5) to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.647091325580708, 0.242595385134207, 0.110313289285085, 0, 0.0651915997328519, 1.02504756760476, -0.0902391673376125, 0, -0.0275570729194699, -0.0805887097177784, 1.10814578263725, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_bmd_widegamut_gen5)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear BMD WideGamut Gen5)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Blackmagic Wide Gamut (Gen 5) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:Linear_BMD_WideGamut_Gen5_to_ACES2065-1:1.0)"
);
        }

        // Linear DaVinci WideGamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear DaVinci Wide Gamut to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.748270290272981, 0.167694659554328, 0.0840350501726906, 0, 0.0208421234689102, 1.11190474268894, -0.132746866157851, 0, -0.0915122574225729, -0.127746712807307, 1.21925897022988, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_davinci_widegamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear DaVinci WideGamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/BlackmagicDesign)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear DaVinci Wide Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:Linear_DaVinci_WideGamut_to_ACES2065-1:1.0)"
);
        }

        // CanonLog2 CinemaGamut D55
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(CANON_CLOG2-CGAMUT_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(canonlog2_cinemagamut_d55)",u8R"(Input - Canon - Canon-Log2 - Cinema Gamut Daylight)",u8R"(canonlog2_cgamutday)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(CanonLog2 CinemaGamut D55)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Canon)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Canon Log 2 Cinema Gamut (Daylight) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Canon:Input:CanonLog2_CinemaGamut-D55_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.CLog2_CGamut_to_ACES.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_CLog2_CGamut.a1.1.0)"
);
        }

        // CanonLog3 CinemaGamut D55
        {
            auto trTo = BuiltinTransform::Create();
            trTo->setStyle(u8R"(CANON_CLOG3-CGAMUT_to_ACES2065-1)");
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(canonlog3_cinemagamut_d55)",u8R"(Input - Canon - Canon-Log3 - Cinema Gamut Daylight)",u8R"(canonlog3_cgamutday)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(CanonLog3 CinemaGamut D55)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Canon)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Canon Log 3 Cinema Gamut (Daylight) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Canon:Input:CanonLog3_CinemaGamut-D55_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.CLog3_CGamut_to_ACES.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_CLog3_CGamut.a1.1.0)"
);
        }

        // Linear CinemaGamut D55
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Canon Cinema Gamut (Daylight) to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.763064454775734, 0.14902116113706, 0.0879143840872056, 0, 0.00365745670512393, 1.10696038037622, -0.110617837081339, 0, -0.0094077940457189, -0.218383304989987, 1.22779109903571, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_cinemagamut_d55)",u8R"(Input - Canon - Linear - Canon Cinema Gamut Daylight)",u8R"(lin_canoncgamutday)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear CinemaGamut D55)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Canon)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Canon Cinema Gamut (Daylight) to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Canon:Input:Linear-CinemaGamut-D55_to_ACES2065-1:1.0)"
);
        }

        // Linear V-Gamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Panasonic V-Gamut to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.72461670413153, 0.166915288193706, 0.108468007674764, 0, 0.021390245413146, 0.984908155703054, -0.00629840111620089, 0, -0.00923556287076561, -0.00105690563900513, 1.01029246850977, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_vgamut)",u8R"(Input - Panasonic - Linear - V-Gamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear V-Gamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Panasonic)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Panasonic V-Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Panasonic:Input:Linear_VGamut_to_ACES2065-1:1.0)"
);
        }

        // V-Log V-Gamut
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Panasonic V-Log - V-Gamut to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01, 0.01, 0.01 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.241514, 0.241514, 0.241514 });
                trTo0->setLogSideOffsetValue({ 0.598206, 0.598206, 0.598206 });
                trTo0->setLinSideSlopeValue({ 1, 1, 1 });
                trTo0->setLinSideOffsetValue({ 0.00873, 0.00873, 0.00873 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.72461670413153, 0.166915288193706, 0.108468007674764, 0, 0.021390245413146, 0.984908155703054, -0.00629840111620089, 0, -0.00923556287076561, -0.00105690563900513, 1.01029246850977, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(vlog_vgamut)",u8R"(Input - Panasonic - V-Log - V-Gamut)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(V-Log V-Gamut)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Panasonic)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Panasonic V-Log - V-Gamut to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Panasonic:Input:VLog_VGamut_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.VLog_VGamut_to_ACES.a1.1.0

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_VLog_VGamut.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.Panasonic.VLog_VGamut.a1.v1)"
);
        }

        // Linear REDWideGamutRGB
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear REDWideGamutRGB to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.785058804068092, 0.0838587565440846, 0.131082439387823, 0, 0.0231738348454756, 1.08789754919233, -0.111071384037806, 0, -0.0737604353682082, -0.314590072290208, 1.38835050765842, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_redwidegamutrgb)",u8R"(Input - RED - Linear - REDWideGamutRGB)",u8R"(lin_rwg)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear REDWideGamutRGB)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/RED)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear REDWideGamutRGB to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:RED:Input:Linear_REDWideGamutRGB_to_ACES2065-1:1.0)"
);
        }

        // Log3G10 REDWideGamutRGB
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(RED Log3G10 REDWideGamutRGB to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ -0.01, -0.01, -0.01 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.224282, 0.224282, 0.224282 });
                trTo0->setLogSideOffsetValue({ 0, 0, 0 });
                trTo0->setLinSideSlopeValue({ 155.975327, 155.975327, 155.975327 });
                trTo0->setLinSideOffsetValue({ 2.55975327, 2.55975327, 2.55975327 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.785058804068092, 0.0838587565440846, 0.131082439387823, 0, 0.0231738348454756, 1.08789754919233, -0.111071384037806, 0, -0.0737604353682082, -0.314590072290208, 1.38835050765842, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(log3g10_redwidegamutrgb)",u8R"(Input - RED - REDLog3G10 - REDWideGamutRGB)",u8R"(rl3g10_rwg)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Log3G10 REDWideGamutRGB)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/RED)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert RED Log3G10 REDWideGamutRGB to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:RED:Input:Log3G10_REDWideGamutRGB_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.RED.Log3G10_REDWideGamutRGB.a1.v1

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_Log3G10_RWG.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.Log3G10_RWG_to_ACES.a1.1.0)"
);
        }

        // Linear S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.75298259539984, 0.143370216235557, 0.103647188364603, 0, 0.0217076974414429, 1.01531883550528, -0.0370265329467195, 0, -0.00941605274963355, 0.00337041785882367, 1.00604563489081, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_sgamut3)",u8R"(Input - Sony - Linear - S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_SGamut3_to_ACES2065-1:1.0)"
);
        }

        // Linear S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.638788667185978, 0.272351433711262, 0.0888598991027595, 0, -0.00391590602528224, 1.0880732308974, -0.0841573248721177, 0, -0.0299072021239151, -0.0264325799101947, 1.05633978203411, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_sgamut3cine)",u8R"(Input - Sony - Linear - S-Gamut3.Cine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_SGamut3Cine_to_ACES2065-1:1.0)"
);
        }

        // Linear Venice S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Venice S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.793329741146434, 0.0890786256206771, 0.117591633232888, 0, 0.0155810585252582, 1.03271230692988, -0.0482933654551394, 0, -0.0188647477991488, 0.0127694120973433, 1.0060953357018, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_venice_sgamut3)",u8R"(Input - Sony - Linear - Venice S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear Venice S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Venice S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_Venice_SGamut3_to_ACES2065-1:1.0)"
);
        }

        // Linear Venice S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Linear Venice S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = MatrixTransform::Create();
                trTo0->setDirection(TRANSFORM_DIR_FORWARD);
                trTo0->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo0Mat[] = { 0.674257092126512, 0.220571735923397, 0.10517117195009, 0, -0.00931360607857167, 1.10595886142466, -0.0966452553460855, 0, -0.0382090673002312, -0.017938376600236, 1.05614744390047, 0, 0, 0, 0, 1 };
                trTo0->setMatrix(trTo0Mat);
                trTo->appendTransform(trTo0);
            }
            static const char* aliases[] = { u8R"(lin_venice_sgamut3cine)",u8R"(Input - Sony - Linear - Venice S-Gamut3.Cine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Linear Venice S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(scene-linear)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Linear Venice S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:Linear_Venice_SGamut3Cine_to_ACES2065-1:1.0)"
);
        }

        // S-Log3 S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.75298259539984, 0.143370216235557, 0.103647188364603, 0, 0.0217076974414429, 1.01531883550528, -0.0370265329467195, 0, -0.00941605274963355, 0.00337041785882367, 1.00604563489081, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_sgamut3)",u8R"(Input - Sony - S-Log3 - S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_SGamut3_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.Sony.SLog3_SGamut3.a1.v1

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_SLog3_SGamut3.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.SLog3_SGamut3_to_ACES.a1.1.0)"
);
        }

        // S-Log3 S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.638788667185978, 0.272351433711262, 0.0888598991027595, 0, -0.00391590602528224, 1.0880732308974, -0.0841573248721177, 0, -0.0299072021239151, -0.0264325799101947, 1.05633978203411, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_sgamut3cine)",u8R"(Input - Sony - S-Log3 - S-Gamut3.Cine)",u8R"(slog3_sgamutcine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_SGamut3Cine_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.Sony.SLog3_SGamut3Cine.a1.v1

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_SLog3_SGamut3Cine.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.SLog3_SGamut3Cine_to_ACES.a1.1.0)"
);
        }

        // S-Log3 Venice S-Gamut3
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 Venice S-Gamut3 to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.793329741146434, 0.089078625620677, 0.117591633232888, 0, 0.0155810585252582, 1.03271230692988, -0.0482933654551394, 0, -0.0188647477991488, 0.0127694120973433, 1.00609533570181, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_venice_sgamut3)",u8R"(Input - Sony - S-Log3 - Venice S-Gamut3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 Venice S-Gamut3)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 Venice S-Gamut3 to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_Venice_SGamut3_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.Sony.Venice_SLog3_SGamut3.a1.v1

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_SLog3_Venice_SGamut3.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.SLog3_Venice_SGamut3_to_ACES.a1.1.0)"
);
        }

        // S-Log3 Venice S-Gamut3.Cine
        {
            auto trTo = GroupTransform::Create();
            trTo->setDirection(TRANSFORM_DIR_FORWARD);
            trTo->getFormatMetadata().setName(u8R"(Sony S-Log3 Venice S-Gamut3.Cine to ACES2065-1)");
            {
                auto trTo0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trTo0->setDirection(TRANSFORM_DIR_INVERSE);
                trTo0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trTo0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trTo0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trTo0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trTo0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trTo0->setBase(10);
                trTo->appendTransform(trTo0);
                auto trTo1 = MatrixTransform::Create();
                trTo1->setDirection(TRANSFORM_DIR_FORWARD);
                trTo1->setOffset(std::array<double, 4>({ 0, 0, 0, 0 }).data());
                static const double trTo1Mat[] = { 0.674257092126512, 0.220571735923397, 0.10517117195009, 0, -0.00931360607857167, 1.10595886142466, -0.0966452553460855, 0, -0.0382090673002312, -0.017938376600236, 1.05614744390047, 0, 0, 0, 0, 1 };
                trTo1->setMatrix(trTo1Mat);
                trTo->appendTransform(trTo1);
            }
            static const char* aliases[] = { u8R"(slog3_venice_sgamut3cine)",u8R"(Input - Sony - S-Log3 - Venice S-Gamut3.Cine)",u8R"(slog3_venice_sgamutcine)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(S-Log3 Venice S-Gamut3.Cine)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(log)"
                , ""
                , u8R"(Input/Sony)"
                , false
                , nullptr
                , trTo
                , u8R"(Convert Sony S-Log3 Venice S-Gamut3.Cine to ACES2065-1

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3_Venice_SGamut3Cine_to_ACES2065-1:1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:IDT.Sony.Venice_SLog3_SGamut3Cine.a1.v1

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_SLog3_Venice_SGamut3Cine.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.SLog3_Venice_SGamut3Cine_to_ACES.a1.1.0)"
);
        }

        // Camera Rec.709
        {
            auto trFrom = GroupTransform::Create();
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            trFrom->getFormatMetadata().setName(u8R"(AP0 to Camera Rec.709)");
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
                trFrom1->setGamma({ 2.22222222222222, 2.22222222222222, 2.22222222222222, 1 });
                trFrom1->setOffset({ 0.099, 0.099, 0.099, 0 });
                trFrom->appendTransform(trFrom1);
            }
            static const char* aliases[] = { u8R"(camera_rec709)",u8R"(Utility - Rec.709 - Camera)",u8R"(rec709_camera)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Camera Rec.709)"
                , REFERENCE_SPACE_SCENE, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Utility/ITU)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert ACES2065-1 to Rec.709 camera OETF Rec.709 primaries, D65 white point

CLFtransformID: urn:aswf:ocio:transformId:1.0:ITU:Utility:AP0_to_Camera_Rec709:1.0)"
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

        // Rec.1886 Rec.2020 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.2020)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec1886_rec2020_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.1886 Rec.2020 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.1886/Rec.2020 (UHD video)

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.Rec2020_100nits_dim.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_P3D65limited_100nits_dim.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.Rec2020_Rec709limited_100nits_dim.a1.1.0)"
);
        }

        // Rec.2100-HLG - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_REC.2100-HLG-1000nit)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(rec2100_hlg_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(Rec.2100-HLG - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(hdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Rec.2100-HLG, 1000 nit

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:RRTODT.Academy.Rec2020_1000nits_15nits_HLG.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvRRTODT.Academy.Rec2020_1000nits_15nits_HLG.a1.1.0)"
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

        // P3-D60 - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_G2.6-P3-D60-BFD)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(p3d60_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(P3-D60 - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Gamma 2.6, P3-D60 (Bradford adaptation)

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3D60_48nits.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3D60_48nits.a1.0.3)"
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

        // P3-DCI - Display
        {
            auto trFrom = BuiltinTransform::Create();
            trFrom->setStyle(u8R"(DISPLAY - CIE-XYZ-D65_to_G2.6-P3-DCI-BFD)");
            trFrom->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(p3_dci_display)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddColorSpace(cfg, u8R"(P3-DCI - Display)"
                , REFERENCE_SPACE_DISPLAY, aliases
                , BIT_DEPTH_F32
                , cats
                , u8R"(sdr-video)"
                , ""
                , u8R"(Display)"
                , false
                , trFrom
                , nullptr
                , u8R"(Convert CIE XYZ (D65 white) to Gamma 2.6, P3-DCI (DCI white with Bradford adaptation)

AMF Components
--------------
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3DCI_48nits.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3DCI_48nits.a1.0.3
ACEStransformID: urn:ampas:aces:transformId:v1.5:ODT.Academy.P3DCI_D65sim_48nits.a1.1.0
ACEStransformID: urn:ampas:aces:transformId:v1.5:InvODT.Academy.P3DCI_D65sim_48nits.a1.1.0)"
);
        }
    }

    // Named Transforms
    {

        // ARRI LogC3 - Curve (EI800)
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(ARRI LogC3 Curve (EI800) to Relative Scene Linear)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.0105909904954696, 0.0105909904954696, 0.0105909904954696 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.247189638318671, 0.247189638318671, 0.247189638318671 });
                trFwd0->setLogSideOffsetValue({ 0.385536998692443, 0.385536998692443, 0.385536998692443 });
                trFwd0->setLinSideSlopeValue({ 5.55555555555556, 5.55555555555556, 5.55555555555556 });
                trFwd0->setLinSideOffsetValue({ 0.0522722750251688, 0.0522722750251688, 0.0522722750251688 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(arri_logc3_crv_ei800)",u8R"(Input - ARRI - Curve - V3 LogC (EI800))",u8R"(crv_logc3ei800)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(ARRI LogC3 - Curve (EI800))"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/ARRI)"
                , trFwd
                , nullptr
                , u8R"(Convert ARRI LogC3 Curve (EI800) to Relative Scene Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC3_Curve_EI800_to_Linear:1.0
)"
);
        }

        // ARRI LogC4 - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(ARRI LogC4 Curve to Relative Scene Linear)");
            {
                auto trFwd0 = LogCameraTransform::Create({ -0.0180569961199113, -0.0180569961199113, -0.0180569961199113 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.0647954196341293, 0.0647954196341293, 0.0647954196341293 });
                trFwd0->setLogSideOffsetValue({ -0.295908392682586, -0.295908392682586, -0.295908392682586 });
                trFwd0->setLinSideSlopeValue({ 2231.82630906769, 2231.82630906769, 2231.82630906769 });
                trFwd0->setLinSideOffsetValue({ 64, 64, 64 });
                trFwd0->setBase(2);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(arri_logc4_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(ARRI LogC4 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/ARRI)"
                , trFwd
                , nullptr
                , u8R"(Convert ARRI LogC4 Curve to Relative Scene Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:ARRI:Input:ARRI_LogC4_Curve_to_Linear:1.0
)"
);
        }

        // BMDFilm Gen5 Log - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(Blackmagic Film (Gen 5) Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.005, 0.005, 0.005 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.0869287606549122, 0.0869287606549122, 0.0869287606549122 });
                trFwd0->setLogSideOffsetValue({ 0.530013339229194, 0.530013339229194, 0.530013339229194 });
                trFwd0->setLinSideSlopeValue({ 1, 1, 1 });
                trFwd0->setLinSideOffsetValue({ 0.00549407243225781, 0.00549407243225781, 0.00549407243225781 });
                trFwd0->setBase(2.71828182845905);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(bmdfilm_gen5_log_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(BMDFilm Gen5 Log - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/BlackmagicDesign)"
                , trFwd
                , nullptr
                , u8R"(Convert Blackmagic Film (Gen 5) Log to Blackmagic Film (Gen 5) Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:BMDFilm_Gen5_Log-Curve_to_Linear:1.0
)"
);
        }

        // DaVinci Intermediate Log - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(DaVinci Intermediate Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.00262409, 0.00262409, 0.00262409 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.07329248, 0.07329248, 0.07329248 });
                trFwd0->setLogSideOffsetValue({ 0.51304736, 0.51304736, 0.51304736 });
                trFwd0->setLinSideSlopeValue({ 1, 1, 1 });
                trFwd0->setLinSideOffsetValue({ 0.0075, 0.0075, 0.0075 });
                trFwd0->setLinearSlopeValue({ 10.44426855, 10.44426855, 10.44426855 });
                trFwd0->setBase(2);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(davinci_intermediate_log_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(DaVinci Intermediate Log - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/BlackmagicDesign)"
                , trFwd
                , nullptr
                , u8R"(Convert DaVinci Intermediate Log to DaVinci Intermediate Linear

CLFtransformID: urn:aswf:ocio:transformId:1.0:BlackmagicDesign:Input:DaVinci_Intermediate_Log-Curve_to_Linear:1.0
)"
);
        }

        // C-Log2 - Curve
        {
            auto trFwd = BuiltinTransform::Create();
            trFwd->setStyle(u8R"(CURVE - CANON_CLOG2_to_LINEAR)");
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(clog2_crv)",u8R"(Input - Canon - Curve - Canon-Log2)",u8R"(crv_canonlog2)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(C-Log2 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/Canon)"
                , trFwd
                , nullptr
                , u8R"(Convert CLog2 Log (arbitrary primaries) to CLog2 Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:Canon:Input:CLog2-Curve_to_Linear:1.0
)"
);
        }

        // C-Log3 - Curve
        {
            auto trFwd = BuiltinTransform::Create();
            trFwd->setStyle(u8R"(CURVE - CANON_CLOG3_to_LINEAR)");
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(clog3_crv)",u8R"(Input - Canon - Curve - Canon-Log3)",u8R"(crv_canonlog3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(C-Log3 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/Canon)"
                , trFwd
                , nullptr
                , u8R"(Convert CLog3 Log (arbitrary primaries) to CLog3 Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:Canon:Input:CLog3-Curve_to_Linear:1.0
)"
);
        }

        // V-Log - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(Panasonic V-Log Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.01, 0.01, 0.01 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.241514, 0.241514, 0.241514 });
                trFwd0->setLogSideOffsetValue({ 0.598206, 0.598206, 0.598206 });
                trFwd0->setLinSideSlopeValue({ 1, 1, 1 });
                trFwd0->setLinSideOffsetValue({ 0.00873, 0.00873, 0.00873 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(vlog_crv)",u8R"(Input - Panasonic - Curve - V-Log)",u8R"(crv_vlog)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(V-Log - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/Panasonic)"
                , trFwd
                , nullptr
                , u8R"(Convert Panasonic V-Log Log (arbitrary primaries) to Panasonic V-Log Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:Panasonic:Input:VLog-Curve_to_Linear:1.0
)"
);
        }

        // Log3G10 - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(RED Log3G10 Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ -0.01, -0.01, -0.01 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.224282, 0.224282, 0.224282 });
                trFwd0->setLogSideOffsetValue({ 0, 0, 0 });
                trFwd0->setLinSideSlopeValue({ 155.975327, 155.975327, 155.975327 });
                trFwd0->setLinSideOffsetValue({ 2.55975327, 2.55975327, 2.55975327 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(log3g10_crv)",u8R"(Input - RED - Curve - REDLog3G10)",u8R"(crv_rl3g10)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(Log3G10 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/RED)"
                , trFwd
                , nullptr
                , u8R"(Convert RED Log3G10 Log (arbitrary primaries) to RED Log3G10 Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:RED:Input:Log3G10-Curve_to_Linear:1.0
)"
);
        }

        // S-Log3 - Curve
        {
            auto trFwd = GroupTransform::Create();
            trFwd->setDirection(TRANSFORM_DIR_FORWARD);
            trFwd->getFormatMetadata().setName(u8R"(S-Log3 Log to Linear Curve)");
            {
                auto trFwd0 = LogCameraTransform::Create({ 0.01125, 0.01125, 0.01125 });
                trFwd0->setDirection(TRANSFORM_DIR_INVERSE);
                trFwd0->setLogSideSlopeValue({ 0.255620723362659, 0.255620723362659, 0.255620723362659 });
                trFwd0->setLogSideOffsetValue({ 0.410557184750733, 0.410557184750733, 0.410557184750733 });
                trFwd0->setLinSideSlopeValue({ 5.26315789473684, 5.26315789473684, 5.26315789473684 });
                trFwd0->setLinSideOffsetValue({ 0.0526315789473684, 0.0526315789473684, 0.0526315789473684 });
                trFwd0->setLinearSlopeValue({ 6.62194371177582, 6.62194371177582, 6.62194371177582 });
                trFwd0->setBase(10);
                trFwd->appendTransform(trFwd0);
            }
            static const char* aliases[] = { u8R"(slog3_crv)",u8R"(Input - Sony - Curve - S-Log3)",u8R"(crv_slog3)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(S-Log3 - Curve)"
                , aliases
                , cats
                , u8R"(log)"
                , u8R"(Input/Sony)"
                , trFwd
                , nullptr
                , u8R"(Convert S-Log3 Log (arbitrary primaries) to S-Log3 Linear (arbitrary primaries)

CLFtransformID: urn:aswf:ocio:transformId:1.0:Sony:Input:SLog3-Curve_to_Linear:1.0
)"
);
        }

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

        // Rec.709 - Curve
        {
            auto trInv = GroupTransform::Create();
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            trInv->getFormatMetadata().setName(u8R"(Linear to Rec.709)");
            {
                auto trInv0 = ExponentWithLinearTransform::Create();
                trInv0->setNegativeStyle(NEGATIVE_LINEAR);
                trInv0->setDirection(TRANSFORM_DIR_INVERSE);
                trInv0->setGamma({ 2.22222222222222, 2.22222222222222, 2.22222222222222, 1 });
                trInv0->setOffset({ 0.099, 0.099, 0.099, 0 });
                trInv->appendTransform(trInv0);
            }
            static const char* aliases[] = { u8R"(rec709_crv)",u8R"(Utility - Curve - Rec.709)",u8R"(crv_rec709)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(Rec.709 - Curve)"
                , aliases
                , cats
                , u8R"(sdr-video)"
                , u8R"(Utility/ITU)"
                , nullptr
                , trInv
                , u8R"(Convert generic linear RGB to generic gamma-corrected RGB

CLFtransformID: urn:aswf:ocio:transformId:1.0:ITU:Utility:Linear_to_Rec709-Curve:1.0
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

        // ST-2084 - Curve
        {
            auto trInv = BuiltinTransform::Create();
            trInv->setStyle(u8R"(CURVE - LINEAR_to_ST-2084)");
            trInv->setDirection(TRANSFORM_DIR_FORWARD);
            static const char* aliases[] = { u8R"(st_2084_crv)","" };
            static const char* cats[] = { u8R"(file-io)","" };
            AddNamedTramsform(cfg, u8R"(ST-2084 - Curve)"
                , aliases
                , cats
                , u8R"(hdr-video)"
                , u8R"(Utility)"
                , nullptr
                , trInv
                , u8R"(Convert generic linear RGB to generic ST.2084 (PQ) encoded RGB mapping 1.0 to 100nits

CLFtransformID: urn:aswf:ocio:transformId:1.0:OCIO:Utility:Linear_to_ST2084-Curve:1.0
)"
);
        }
    }
    return cfg;
};

} // namespace STUDIOCONFIG
} // namespace OCIO_NAMESPACE
