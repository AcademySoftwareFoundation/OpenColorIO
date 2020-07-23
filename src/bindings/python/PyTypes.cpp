// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

void bindPyTypes(py::module & m)
{
    // Enums
    py::enum_<LoggingLevel>(m, "LoggingLevel", DS(LoggingLevel), py::arithmetic())
        .value("LOGGING_LEVEL_NONE", LOGGING_LEVEL_NONE, DS(LoggingLevel, LOGGING_LEVEL_NONE))
        .value("LOGGING_LEVEL_WARNING", LOGGING_LEVEL_WARNING, DS(LoggingLevel, LOGGING_LEVEL_WARNING))
        .value("LOGGING_LEVEL_INFO", LOGGING_LEVEL_INFO, DS(LoggingLevel, LOGGING_LEVEL_INFO))
        .value("LOGGING_LEVEL_DEBUG", LOGGING_LEVEL_DEBUG, DS(LoggingLevel, LOGGING_LEVEL_DEBUG))
        .value("LOGGING_LEVEL_UNKNOWN", LOGGING_LEVEL_UNKNOWN, DS(LoggingLevel, LOGGING_LEVEL_UNKNOWN))
        .export_values();

    py::enum_<ReferenceSpaceType>(m, "ReferenceSpaceType", DS(ReferenceSpaceType))
        .value("REFERENCE_SPACE_SCENE", REFERENCE_SPACE_SCENE, DS(ReferenceSpaceType, REFERENCE_SPACE_SCENE))
        .value("REFERENCE_SPACE_DISPLAY", REFERENCE_SPACE_DISPLAY, DS(ReferenceSpaceType, REFERENCE_SPACE_DISPLAY))
        .export_values();

    py::enum_<SearchReferenceSpaceType>(m, "SearchReferenceSpaceType", DS(SearchReferenceSpaceType))
        .value("SEARCH_REFERENCE_SPACE_SCENE", SEARCH_REFERENCE_SPACE_SCENE, DS(SearchReferenceSpaceType, SEARCH_REFERENCE_SPACE_SCENE))
        .value("SEARCH_REFERENCE_SPACE_DISPLAY", SEARCH_REFERENCE_SPACE_DISPLAY, DS(SearchReferenceSpaceType, SEARCH_REFERENCE_SPACE_DISPLAY))
        .value("SEARCH_REFERENCE_SPACE_ALL", SEARCH_REFERENCE_SPACE_ALL, DS(SearchReferenceSpaceType, SEARCH_REFERENCE_SPACE_ALL))
        .export_values();

    py::enum_<ColorSpaceVisibility>(m, "ColorSpaceVisibility", DS(ColorSpaceVisibility))
        .value("COLORSPACE_ACTIVE", COLORSPACE_ACTIVE, DS(ColorSpaceVisibility, COLORSPACE_ACTIVE))
        .value("COLORSPACE_INACTIVE", COLORSPACE_INACTIVE, DS(ColorSpaceVisibility, COLORSPACE_INACTIVE))
        .value("COLORSPACE_ALL", COLORSPACE_ALL, DS(ColorSpaceVisibility, COLORSPACE_ALL))
        .export_values();

    py::enum_<ColorSpaceDirection>(m, "ColorSpaceDirection", DS(ColorSpaceDirection))
        .value("COLORSPACE_DIR_UNKNOWN", COLORSPACE_DIR_UNKNOWN, DS(ColorSpaceDirection, COLORSPACE_DIR_UNKNOWN))
        .value("COLORSPACE_DIR_TO_REFERENCE", COLORSPACE_DIR_TO_REFERENCE, DS(ColorSpaceDirection, COLORSPACE_DIR_TO_REFERENCE))
        .value("COLORSPACE_DIR_FROM_REFERENCE", COLORSPACE_DIR_FROM_REFERENCE, DS(ColorSpaceDirection, COLORSPACE_DIR_FROM_REFERENCE))
        .export_values();

    py::enum_<ViewTransformDirection>(m, "ViewTransformDirection", DS(ViewTransformDirection))
        .value("VIEWTRANSFORM_DIR_UNKNOWN", VIEWTRANSFORM_DIR_UNKNOWN, DS(ViewTransformDirection, VIEWTRANSFORM_DIR_UNKNOWN))
        .value("VIEWTRANSFORM_DIR_TO_REFERENCE", VIEWTRANSFORM_DIR_TO_REFERENCE, DS(ViewTransformDirection, VIEWTRANSFORM_DIR_TO_REFERENCE))
        .value("VIEWTRANSFORM_DIR_FROM_REFERENCE", VIEWTRANSFORM_DIR_FROM_REFERENCE, DS(ViewTransformDirection, VIEWTRANSFORM_DIR_FROM_REFERENCE))
        .export_values();

    py::enum_<TransformDirection>(m, "TransformDirection", DS(TransformDirection))
        .value("TRANSFORM_DIR_UNKNOWN", TRANSFORM_DIR_UNKNOWN, DS(TransformDirection, TRANSFORM_DIR_UNKNOWN))
        .value("TRANSFORM_DIR_FORWARD", TRANSFORM_DIR_FORWARD, DS(TransformDirection, TRANSFORM_DIR_FORWARD))
        .value("TRANSFORM_DIR_INVERSE", TRANSFORM_DIR_INVERSE, DS(TransformDirection, TRANSFORM_DIR_INVERSE))
        .export_values();

    py::enum_<Interpolation>(m, "Interpolation", DS(Interpolation))
        .value("INTERP_UNKNOWN", INTERP_UNKNOWN, DS(Interpolation, INTERP_UNKNOWN))
        .value("INTERP_NEAREST", INTERP_NEAREST, DS(Interpolation, INTERP_NEAREST))
        .value("INTERP_LINEAR", INTERP_LINEAR, DS(Interpolation, INTERP_LINEAR))
        .value("INTERP_TETRAHEDRAL", INTERP_TETRAHEDRAL, DS(Interpolation, INTERP_TETRAHEDRAL))
        .value("INTERP_CUBIC", INTERP_CUBIC, DS(Interpolation, INTERP_CUBIC))
        .value("INTERP_DEFAULT", INTERP_DEFAULT, DS(Interpolation, INTERP_DEFAULT))
        .value("INTERP_BEST", INTERP_BEST, DS(Interpolation, INTERP_BEST))
        .export_values();

    py::enum_<BitDepth>(m, "BitDepth", DS(BitDepth), py::arithmetic())
        .value("BIT_DEPTH_UNKNOWN", BIT_DEPTH_UNKNOWN, DS(BitDepth, BIT_DEPTH_UNKNOWN))
        .value("BIT_DEPTH_UINT8", BIT_DEPTH_UINT8, DS(BitDepth, BIT_DEPTH_UINT8))
        .value("BIT_DEPTH_UINT10", BIT_DEPTH_UINT10, DS(BitDepth, BIT_DEPTH_UINT10))
        .value("BIT_DEPTH_UINT12", BIT_DEPTH_UINT12, DS(BitDepth, BIT_DEPTH_UINT12))
        .value("BIT_DEPTH_UINT14", BIT_DEPTH_UINT14, DS(BitDepth, BIT_DEPTH_UINT14))
        .value("BIT_DEPTH_UINT16", BIT_DEPTH_UINT16, DS(BitDepth, BIT_DEPTH_UINT16))
        .value("BIT_DEPTH_UINT32", BIT_DEPTH_UINT32, DS(BitDepth, BIT_DEPTH_UINT32))
        .value("BIT_DEPTH_F16", BIT_DEPTH_F16, DS(BitDepth, BIT_DEPTH_F16))
        .value("BIT_DEPTH_F32", BIT_DEPTH_F32, DS(BitDepth, BIT_DEPTH_F32))
        .export_values();

    py::enum_<Lut1DHueAdjust>(m, "Lut1DHueAdjust", DS(Lut1DHueAdjust))
        .value("HUE_NONE", HUE_NONE, DS(Lut1DHueAdjust, HUE_NONE))
        .value("HUE_DW3", HUE_DW3, DS(Lut1DHueAdjust, HUE_NONE))
        .export_values();

    py::enum_<ChannelOrdering>(m, "ChannelOrdering", DS(ChannelOrdering))
        .value("CHANNEL_ORDERING_RGBA", CHANNEL_ORDERING_RGBA, DS(ChannelOrdering, CHANNEL_ORDERING_RGBA))
        .value("CHANNEL_ORDERING_BGRA", CHANNEL_ORDERING_BGRA, DS(ChannelOrdering, CHANNEL_ORDERING_BGRA))
        .value("CHANNEL_ORDERING_ABGR", CHANNEL_ORDERING_ABGR, DS(ChannelOrdering, CHANNEL_ORDERING_ABGR))
        .value("CHANNEL_ORDERING_RGB", CHANNEL_ORDERING_RGB, DS(ChannelOrdering, CHANNEL_ORDERING_RGB))
        .value("CHANNEL_ORDERING_BGR", CHANNEL_ORDERING_BGR, DS(ChannelOrdering, CHANNEL_ORDERING_BGR))
        .export_values();

    py::enum_<Allocation>(m, "Allocation", DS(Allocation))
        .value("ALLOCATION_UNKNOWN", ALLOCATION_UNKNOWN, DS(Allocation, ALLOCATION_UNKNOWN))
        .value("ALLOCATION_UNIFORM", ALLOCATION_UNIFORM, DS(Allocation, ALLOCATION_UNIFORM))
        .value("ALLOCATION_LG2", ALLOCATION_LG2, DS(Allocation, ALLOCATION_LG2))
        .export_values();

    py::enum_<GpuLanguage>(m, "GpuLanguage", DS(GpuLanguage))
        .value("GPU_LANGUAGE_UNKNOWN", GPU_LANGUAGE_UNKNOWN, DS(GpuLanguage, GPU_LANGUAGE_UNKNOWN))
        .value("GPU_LANGUAGE_CG", GPU_LANGUAGE_CG, DS(GpuLanguage, GPU_LANGUAGE_CG))
        .value("GPU_LANGUAGE_GLSL_1_0", GPU_LANGUAGE_GLSL_1_0, DS(GpuLanguage, GPU_LANGUAGE_GLSL_1_0))
        .value("GPU_LANGUAGE_GLSL_1_3", GPU_LANGUAGE_GLSL_1_3, DS(GpuLanguage, GPU_LANGUAGE_GLSL_1_3))
        .value("GPU_LANGUAGE_GLSL_4_0", GPU_LANGUAGE_GLSL_4_0, DS(GpuLanguage, GPU_LANGUAGE_GLSL_4_0))
        .value("GPU_LANGUAGE_HLSL_DX11", GPU_LANGUAGE_HLSL_DX11, DS(GpuLanguage, GPU_LANGUAGE_HLSL_DX11))
        .export_values();

    py::enum_<EnvironmentMode>(m, "EnvironmentMode", DS(EnvironmentMode))
        .value("ENV_ENVIRONMENT_UNKNOWN", ENV_ENVIRONMENT_UNKNOWN, DS(EnvironmentMode, ENV_ENVIRONMENT_UNKNOWN))
        .value("ENV_ENVIRONMENT_LOAD_PREDEFINED", ENV_ENVIRONMENT_LOAD_PREDEFINED, DS(EnvironmentMode, ENV_ENVIRONMENT_LOAD_PREDEFINED))
        .value("ENV_ENVIRONMENT_LOAD_ALL", ENV_ENVIRONMENT_LOAD_ALL, DS(EnvironmentMode, ENV_ENVIRONMENT_LOAD_ALL))
        .export_values();

    py::enum_<RangeStyle>(m, "RangeStyle")
        .value("RANGE_NO_CLAMP", RANGE_NO_CLAMP)
        .value("RANGE_CLAMP", RANGE_CLAMP)
        .export_values();

    py::enum_<FixedFunctionStyle>(m, "FixedFunctionStyle")
        .value("FIXED_FUNCTION_ACES_RED_MOD_03", FIXED_FUNCTION_ACES_RED_MOD_03)
        .value("FIXED_FUNCTION_ACES_RED_MOD_10", FIXED_FUNCTION_ACES_RED_MOD_10)
        .value("FIXED_FUNCTION_ACES_GLOW_03", FIXED_FUNCTION_ACES_GLOW_03)
        .value("FIXED_FUNCTION_ACES_GLOW_10", FIXED_FUNCTION_ACES_GLOW_10)
        .value("FIXED_FUNCTION_ACES_DARK_TO_DIM_10", FIXED_FUNCTION_ACES_DARK_TO_DIM_10)
        .value("FIXED_FUNCTION_REC2100_SURROUND", FIXED_FUNCTION_REC2100_SURROUND)
        .value("FIXED_FUNCTION_RGB_TO_HSV", FIXED_FUNCTION_RGB_TO_HSV)
        .value("FIXED_FUNCTION_XYZ_TO_xyY", FIXED_FUNCTION_XYZ_TO_xyY)
        .value("FIXED_FUNCTION_XYZ_TO_uvY", FIXED_FUNCTION_XYZ_TO_uvY)
        .value("FIXED_FUNCTION_XYZ_TO_LUV", FIXED_FUNCTION_XYZ_TO_LUV)
        .export_values();

    py::enum_<ExposureContrastStyle>(m, "ExposureContrastStyle", DS(ExposureContrastStyle))
        .value("EXPOSURE_CONTRAST_LINEAR", EXPOSURE_CONTRAST_LINEAR, DS(ExposureContrastStyle, EXPOSURE_CONTRAST_LINEAR))
        .value("EXPOSURE_CONTRAST_VIDEO", EXPOSURE_CONTRAST_VIDEO, DS(ExposureContrastStyle, EXPOSURE_CONTRAST_VIDEO))
        .value("EXPOSURE_CONTRAST_LOGARITHMIC", EXPOSURE_CONTRAST_LOGARITHMIC, DS(ExposureContrastStyle, EXPOSURE_CONTRAST_LOGARITHMIC))
        .export_values();

    py::enum_<CDLStyle>(m, "CDLStyle", DS(CDLStyle))
        .value("CDL_ASC", CDL_ASC, DS(CDLStyle, CDL_ASC))
        .value("CDL_NO_CLAMP", CDL_NO_CLAMP, DS(CDLStyle, CDL_NO_CLAMP))
        .value("CDL_TRANSFORM_DEFAULT", CDL_TRANSFORM_DEFAULT, DS(CDLStyle, CDL_TRANSFORM_DEFAULT))
        .export_values();

    py::enum_<NegativeStyle>(m, "NegativeStyle", DS(NegativeStyle))
        .value("NEGATIVE_CLAMP", NEGATIVE_CLAMP, DS(NegativeStyle, NEGATIVE_CLAMP))
        .value("NEGATIVE_MIRROR", NEGATIVE_MIRROR, DS(NegativeStyle, NEGATIVE_MIRROR))
        .value("NEGATIVE_PASS_THRU", NEGATIVE_PASS_THRU, DS(NegativeStyle, NEGATIVE_PASS_THRU))
        .value("NEGATIVE_LINEAR", NEGATIVE_LINEAR, DS(NegativeStyle, NEGATIVE_LINEAR))
        .export_values();

    py::enum_<DynamicPropertyType>(m, "DynamicPropertyType", DS(DynamicPropertyType))
        .value("DYNAMIC_PROPERTY_EXPOSURE", DYNAMIC_PROPERTY_EXPOSURE, DS(DynamicPropertyType, DYNAMIC_PROPERTY_EXPOSURE))
        .value("DYNAMIC_PROPERTY_CONTRAST", DYNAMIC_PROPERTY_CONTRAST, DS(DynamicPropertyType, DYNAMIC_PROPERTY_CONTRAST))
        .value("DYNAMIC_PROPERTY_GAMMA", DYNAMIC_PROPERTY_GAMMA, DS(DynamicPropertyType, DYNAMIC_PROPERTY_GAMMA))
        .export_values();

    py::enum_<DynamicPropertyValueType>(m, "DynamicPropertyValueType", DS(DynamicPropertyValueType))
        .value("DYNAMIC_PROPERTY_DOUBLE", DYNAMIC_PROPERTY_DOUBLE, DS(DynamicPropertyValueType, DYNAMIC_PROPERTY_DOUBLE))
        .value("DYNAMIC_PROPERTY_BOOL", DYNAMIC_PROPERTY_BOOL, DS(DynamicPropertyValueType, DYNAMIC_PROPERTY_BOOL))
        .export_values();

    py::enum_<OptimizationFlags>(m, "OptimizationFlags", DS(OptimizationFlags), py::arithmetic())
        .value("OPTIMIZATION_NONE", OPTIMIZATION_NONE, DS(OptimizationFlags, OPTIMIZATION_NONE))
        .value("OPTIMIZATION_IDENTITY", OPTIMIZATION_IDENTITY, DS(OptimizationFlags, OPTIMIZATION_IDENTITY))
        .value("OPTIMIZATION_IDENTITY_GAMMA", OPTIMIZATION_IDENTITY_GAMMA, DS(OptimizationFlags, OPTIMIZATION_IDENTITY_GAMMA))
        .value("OPTIMIZATION_PAIR_IDENTITY_CDL", OPTIMIZATION_PAIR_IDENTITY_CDL, DS(OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_CDL))
        .value("OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST", OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST, DS(OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST))
        .value("OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION", OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION, DS(OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION))
        .value("OPTIMIZATION_PAIR_IDENTITY_GAMMA", OPTIMIZATION_PAIR_IDENTITY_GAMMA, DS(OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_GAMMA))
        .value("OPTIMIZATION_PAIR_IDENTITY_LUT1D", OPTIMIZATION_PAIR_IDENTITY_LUT1D, DS(OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_LUT1D))
        .value("OPTIMIZATION_PAIR_IDENTITY_LUT3D", OPTIMIZATION_PAIR_IDENTITY_LUT3D, DS(OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_LUT3D))
        .value("OPTIMIZATION_PAIR_IDENTITY_LOG", OPTIMIZATION_PAIR_IDENTITY_LOG, DS(OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_LOG))
        .value("OPTIMIZATION_COMP_EXPONENT", OPTIMIZATION_COMP_EXPONENT, DS(OptimizationFlags, OPTIMIZATION_COMP_EXPONENT))
        .value("OPTIMIZATION_COMP_GAMMA", OPTIMIZATION_COMP_GAMMA, DS(OptimizationFlags, OPTIMIZATION_COMP_GAMMA))
        .value("OPTIMIZATION_COMP_MATRIX", OPTIMIZATION_COMP_MATRIX, DS(OptimizationFlags, OPTIMIZATION_COMP_MATRIX))
        .value("OPTIMIZATION_COMP_LUT1D", OPTIMIZATION_COMP_LUT1D, DS(OptimizationFlags, OPTIMIZATION_COMP_LUT1D))
        .value("OPTIMIZATION_COMP_LUT3D", OPTIMIZATION_COMP_LUT3D, DS(OptimizationFlags, OPTIMIZATION_COMP_LUT3D))
        .value("OPTIMIZATION_COMP_RANGE", OPTIMIZATION_COMP_RANGE, DS(OptimizationFlags, OPTIMIZATION_COMP_RANGE))
        .value("OPTIMIZATION_COMP_SEPARABLE_PREFIX", OPTIMIZATION_COMP_SEPARABLE_PREFIX, DS(OptimizationFlags, OPTIMIZATION_COMP_SEPARABLE_PREFIX))
        .value("OPTIMIZATION_LUT_INV_FAST", OPTIMIZATION_LUT_INV_FAST, DS(OptimizationFlags, OPTIMIZATION_LUT_INV_FAST))
        .value("OPTIMIZATION_NO_DYNAMIC_PROPERTIES", OPTIMIZATION_NO_DYNAMIC_PROPERTIES, DS(OptimizationFlags, OPTIMIZATION_NO_DYNAMIC_PROPERTIES))
        .value("OPTIMIZATION_ALL", OPTIMIZATION_ALL, DS(OptimizationFlags, OPTIMIZATION_ALL))
        .value("OPTIMIZATION_LOSSLESS", OPTIMIZATION_LOSSLESS, DS(OptimizationFlags, OPTIMIZATION_LOSSLESS))
        .value("OPTIMIZATION_VERY_GOOD", OPTIMIZATION_VERY_GOOD, DS(OptimizationFlags, OPTIMIZATION_VERY_GOOD))
        .value("OPTIMIZATION_GOOD", OPTIMIZATION_GOOD, DS(OptimizationFlags, OPTIMIZATION_GOOD))
        .value("OPTIMIZATION_DRAFT", OPTIMIZATION_DRAFT, DS(OptimizationFlags, OPTIMIZATION_DRAFT))
        .value("OPTIMIZATION_DEFAULT", OPTIMIZATION_DEFAULT, DS(OptimizationFlags, OPTIMIZATION_DEFAULT))
        .export_values();

    // Conversion
    m.def("BoolToString", &BoolToString, DS(BoolToString), "value"_a);
    m.def("BoolFromString", &BoolFromString, DS(BoolFromString), "str"_a);

    m.def("LoggingLevelToString", &LoggingLevelToString, DS(LoggingLevelToString), "level"_a);
    m.def("LoggingLevelFromString", &LoggingLevelFromString, DS(LoggingLevelFromString), "str"_a);

    m.def("TransformDirectionToString", &TransformDirectionToString, DS(TransformDirectionToString), "direction"_a);
    m.def("TransformDirectionFromString", &TransformDirectionFromString, DS(TransformDirectionFromString), "str"_a);

    m.def("GetInverseTransformDirection", &GetInverseTransformDirection, DS(GetInverseTransformDirection), "direction"_a);
    m.def("CombineTransformDirections", &CombineTransformDirections, DS(CombineTransformDirections), "direction1"_a, "direction2"_a);

    m.def("ColorSpaceDirectionToString", &ColorSpaceDirectionToString, DS(ColorSpaceDirectionToString), "direction"_a);
    m.def("ColorSpaceDirectionFromString", &ColorSpaceDirectionFromString, DS(ColorSpaceDirectionFromString), "str"_a);

    m.def("BitDepthToString", &BitDepthToString, DS(BitDepthToString), "bitDepth"_a);
    m.def("BitDepthFromString", &BitDepthFromString, DS(BitDepthFromString), "str"_a);
    m.def("BitDepthIsFloat", &BitDepthIsFloat, DS(BitDepthIsFloat), "bitDepth"_a);
    m.def("BitDepthToInt", &BitDepthToInt, DS(BitDepthToInt), "bitDepth"_a);

    m.def("AllocationToString", &AllocationToString, DS(AllocationToString), "allocation"_a);
    m.def("AllocationFromString", &AllocationFromString, DS(AllocationFromString), "str"_a);

    m.def("InterpolationToString", &InterpolationToString, DS(InterpolationToString), "interpolation"_a);
    m.def("InterpolationFromString", &InterpolationFromString, DS(InterpolationFromString), "str"_a);

    m.def("GpuLanguageToString", &GpuLanguageToString, DS(GpuLanguageToString), "language"_a);
    m.def("GpuLanguageFromString", &GpuLanguageFromString, DS(GpuLanguageFromString), "str"_a);

    m.def("EnvironmentModeToString", &EnvironmentModeToString, DS(EnvironmentModeToString), "mode"_a);
    m.def("EnvironmentModeFromString", &EnvironmentModeFromString, DS(EnvironmentModeFromString), "str"_a);

    m.def("CDLStyleToString", &CDLStyleToString, DS(CDLStyleToString), "style"_a);
    m.def("CDLStyleFromString", &CDLStyleFromString, DS(CDLStyleFromString), "str"_a);

    m.def("RangeStyleToString", &RangeStyleToString, DS(RangeStyleToString), "style"_a);
    m.def("RangeStyleFromString", &RangeStyleFromString, DS(RangeStyleFromString), "str"_a);

    m.def("FixedFunctionStyleToString", &FixedFunctionStyleToString, DS(FixedFunctionStyleToString), "style"_a);
    m.def("FixedFunctionStyleFromString", &FixedFunctionStyleFromString, DS(FixedFunctionStyleFromString), "str"_a);

    m.def("ExposureContrastStyleToString", &ExposureContrastStyleToString, DS(ExposureContrastStyleToString), "style"_a);
    m.def("ExposureContrastStyleFromString", &ExposureContrastStyleFromString, DS(ExposureContrastStyleFromString), "str"_a);

    m.def("NegativeStyleToString", &NegativeStyleToString, DS(NegativeStyleToString), "style"_a);
    m.def("NegativeStyleFromString", &NegativeStyleFromString, DS(NegativeStyleFromString), "str"_a);

    // Envar
    m.attr("OCIO_CONFIG_ENVVAR") = OCIO_CONFIG_ENVVAR;
    m.attr("OCIO_ACTIVE_DISPLAYS_ENVVAR") = OCIO_ACTIVE_DISPLAYS_ENVVAR;
    m.attr("OCIO_ACTIVE_VIEWS_ENVVAR") = OCIO_ACTIVE_VIEWS_ENVVAR;
    m.attr("OCIO_INACTIVE_COLORSPACES_ENVVAR") = OCIO_INACTIVE_COLORSPACES_ENVVAR;

    // Roles
    m.attr("ROLE_DEFAULT") = ROLE_DEFAULT;
    m.attr("ROLE_REFERENCE") = ROLE_REFERENCE;
    m.attr("ROLE_DATA") = ROLE_DATA;
    m.attr("ROLE_COLOR_PICKING") = ROLE_COLOR_PICKING;
    m.attr("ROLE_SCENE_LINEAR") = ROLE_SCENE_LINEAR;
    m.attr("ROLE_COMPOSITING_LOG") = ROLE_COMPOSITING_LOG;
    m.attr("ROLE_COLOR_TIMING") = ROLE_COLOR_TIMING;
    m.attr("ROLE_TEXTURE_PAINT") = ROLE_TEXTURE_PAINT;
    m.attr("ROLE_MATTE_PAINT") = ROLE_MATTE_PAINT;

    // FormatMetadata
    m.attr("METADATA_DESCRIPTION") = METADATA_DESCRIPTION;
    m.attr("METADATA_INFO") = METADATA_INFO;
    m.attr("METADATA_INPUT_DESCRIPTOR") = METADATA_INPUT_DESCRIPTOR;
    m.attr("METADATA_OUTPUT_DESCRIPTOR") = METADATA_OUTPUT_DESCRIPTOR;
    m.attr("METADATA_NAME") = METADATA_NAME;
    m.attr("METADATA_ID") = METADATA_ID;
}

} // namespace OCIO_NAMESPACE
