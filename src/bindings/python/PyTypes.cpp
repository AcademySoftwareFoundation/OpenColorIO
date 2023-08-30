// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyImageDesc.h"
#include "PySystemMonitors.h"
#include "PyBuiltinConfigRegistry.h"
#include "PyBuiltinTransformRegistry.h"
#include "PyDynamicProperty.h"

namespace OCIO_NAMESPACE
{

void bindPyTypes(py::module & m)
{
    // OpenColorIO
    py::class_<Baker, BakerRcPtr /* holder */>(
        m, "Baker", 
        DOC(Baker));

    py::class_<PyBuiltinConfigRegistry>(
        m, "BuiltinConfigRegistry", 
        DOC(BuiltinConfigRegistry));

    py::class_<ColorSpace, ColorSpaceRcPtr /* holder */>(
        m, "ColorSpace", 
        DOC(ColorSpace));

    py::class_<ColorSpaceSet, ColorSpaceSetRcPtr /* holder */>(
        m, "ColorSpaceSet",
        DOC(ColorSpaceSet));

    py::class_<Config, ConfigRcPtr /* holder */>(
        m, "Config",
        DOC(Config));

    py::class_<Context, ContextRcPtr /* holder */>(
        m, "Context", 
        DOC(Context));

    py::class_<CPUProcessor, CPUProcessorRcPtr /* holder */>(
        m, "CPUProcessor", 
        DOC(CPUProcessor));

    py::class_<FileRules, FileRulesRcPtr /* holder */>(
        m, "FileRules", 
        DOC(FileRules));

    py::class_<GPUProcessor, GPUProcessorRcPtr /* holder */>(
        m, "GPUProcessor", 
        DOC(GPUProcessor));

    py::class_<GpuShaderCreator, GpuShaderCreatorRcPtr /* holder */>(
        m, "GpuShaderCreator", 
        DOC(GpuShaderCreator));

    py::class_<GpuShaderDesc, GpuShaderDescRcPtr /* holder */, GpuShaderCreator /* base */>(
        m, "GpuShaderDesc", 
        DOC(GpuShaderDesc));

    py::class_<PyImageDesc>(
         m, "ImageDesc", 
         DOC(ImageDesc));

    py::class_<PyPackedImageDesc, PyImageDesc /* base */>(
         m, "PackedImageDesc", 
         DOC(PackedImageDesc));

    py::class_<PyPlanarImageDesc, PyImageDesc /* base */>(
        m, "PlanarImageDesc", 
        DOC(PlanarImageDesc));

    py::class_<Look, LookRcPtr /* holder */>(
        m, "Look", 
        DOC(Look));

    py::class_<NamedTransform, NamedTransformRcPtr /* holder */>(
        m, "NamedTransform", 
        DOC(NamedTransform));

    py::class_<Processor, ProcessorRcPtr /* holder */>(
        m, "Processor", 
        DOC(Processor));

    py::class_<ProcessorMetadata, ProcessorMetadataRcPtr /* holder */>(
        m, "ProcessorMetadata",
        DOC(ProcessorMetadata));

    py::class_<PySystemMonitors>(
        m, "SystemMonitors", 
        DOC(SystemMonitors));

    py::class_<ViewingRules, ViewingRulesRcPtr /* holder */>(
        m, "ViewingRules", 
        DOC(ViewingRules));

    py::class_<ViewTransform, ViewTransformRcPtr /* holder */>(
        m, "ViewTransform", 
        DOC(ViewTransform));

    // OpenColorIOTransforms
    py::class_<PyBuiltinTransformRegistry>(
        m, "BuiltinTransformRegistry", 
        DOC(BuiltinTransformRegistry));

    py::class_<PyDynamicProperty>(
        m, "DynamicProperty", 
        DOC(DynamicProperty));

    py::class_<FormatMetadata>(
        m, "FormatMetadata", 
        DOC(FormatMetadata));

    py::class_<GradingRGBM>(
        m, "GradingRGBM", 
        DOC(GradingRGBM));

    py::class_<GradingPrimary>(
        m, "GradingPrimary", 
        DOC(GradingPrimary));

    py::class_<GradingRGBMSW>(
        m, "GradingRGBMSW", 
        DOC(GradingRGBMSW));

    py::class_<GradingTone>(
        m, "GradingTone", 
        DOC(GradingTone));

    py::class_<GradingControlPoint>(
        m, "GradingControlPoint", 
        DOC(GradingControlPoint));

    py::class_<GradingBSplineCurve, GradingBSplineCurveRcPtr /*holder*/>(
        m, "GradingBSplineCurve", 
        DOC(GradingBSplineCurve));

    py::class_<GradingRGBCurve, GradingRGBCurveRcPtr /*holder*/>(
        m, "GradingRGBCurve", 
        DOC(GradingRGBCurve));

    py::class_<Transform, TransformRcPtr /* holder */>(
        m, "Transform", 
        DOC(Transform));

    py::class_<AllocationTransform, 
               AllocationTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "AllocationTransform",
        DOC(AllocationTransform));

    py::class_<BuiltinTransform, BuiltinTransformRcPtr /* holder */, Transform /* base */>(
        m, "BuiltinTransform",
        DOC(BuiltinTransform));

    py::class_<CDLTransform, CDLTransformRcPtr /* holder */, Transform /* base */>(
        m, "CDLTransform", 
        DOC(CDLTransform));

    py::class_<ColorSpaceTransform, 
               ColorSpaceTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "ColorSpaceTransform",
        DOC(ColorSpaceTransform));

    py::class_<DisplayViewTransform, 
               DisplayViewTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "DisplayViewTransform", 
        DOC(DisplayViewTransform));

    py::class_<ExponentTransform, ExponentTransformRcPtr /* holder */, Transform /* base */>(
        m, "ExponentTransform", 
        DOC(ExponentTransform));

    py::class_<ExponentWithLinearTransform, 
               ExponentWithLinearTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "ExponentWithLinearTransform", 
        DOC(ExponentWithLinearTransform));

    py::class_<ExposureContrastTransform, 
               ExposureContrastTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "ExposureContrastTransform", 
        DOC(ExposureContrastTransform));

    py::class_<FileTransform, FileTransformRcPtr /* holder */, Transform /* base */>(
        m, "FileTransform", 
        DOC(FileTransform));

    py::class_<FixedFunctionTransform, 
               FixedFunctionTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "FixedFunctionTransform", 
        DOC(FixedFunctionTransform));

    py::class_<GradingPrimaryTransform,
               GradingPrimaryTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "GradingPrimaryTransform", 
        DOC(GradingPrimaryTransform));

    py::class_<GradingRGBCurveTransform,
               GradingRGBCurveTransformRcPtr /* holder */, 
               Transform /* base */>(
        m, "GradingRGBCurveTransform", 
        DOC(GradingRGBCurveTransform));

    py::class_<GradingToneTransform,
               GradingToneTransformRcPtr /* holder */,
               Transform /* base */>(
        m, "GradingToneTransform", 
        DOC(GradingToneTransform));

    py::class_<GroupTransform, GroupTransformRcPtr /* holder */, Transform /* base */>(
        m, "GroupTransform", 
        DOC(GroupTransform));

    py::class_<LogAffineTransform, LogAffineTransformRcPtr /* holder */, Transform /* base */>(
        m, "LogAffineTransform",
        DOC(LogAffineTransform));

    py::class_<LogCameraTransform, LogCameraTransformRcPtr /* holder */, Transform /* base */>(
        m, "LogCameraTransform", 
        DOC(LogCameraTransform));

    py::class_<LogTransform, LogTransformRcPtr /* holder */, Transform /* base */>(
        m, "LogTransform", 
        DOC(LogTransform));

    py::class_<LookTransform, LookTransformRcPtr /* holder */, Transform /* base */>(
        m, "LookTransform", 
        DOC(LookTransform));

    py::class_<Lut1DTransform, Lut1DTransformRcPtr /* holder */, Transform /* base */>(
        m, "Lut1DTransform", 
        DOC(Lut1DTransform));

    py::class_<Lut3DTransform, Lut3DTransformRcPtr /* holder */, Transform /* base */>(
        m, "Lut3DTransform", 
        DOC(Lut3DTransform));

    py::class_<MatrixTransform, MatrixTransformRcPtr /* holder */, Transform /* base */>(
        m, "MatrixTransform", 
        DOC(MatrixTransform));

    py::class_<RangeTransform, RangeTransformRcPtr /* holder */, Transform /* base */>(
        m, "RangeTransform", 
        DOC(RangeTransform));

    // OpenColorIOAppHelpers
    py::class_<ColorSpaceMenuParameters, ColorSpaceMenuParametersRcPtr /* holder */>(
        m, "ColorSpaceMenuParameters", 
        DOC(ColorSpaceMenuParameters));

    py::class_<ColorSpaceMenuHelper, ColorSpaceMenuHelperRcPtr /* holder */>(
        m, "ColorSpaceMenuHelper", 
        DOC(ColorSpaceMenuHelper));

    py::class_<LegacyViewingPipeline, LegacyViewingPipelineRcPtr /* holder */>(
        m, "LegacyViewingPipeline", 
        DOC(LegacyViewingPipeline));

    py::class_<MixingSlider>(
        m, "MixingSlider", 
        DOC(MixingSlider));

    py::class_<MixingColorSpaceManager, MixingColorSpaceManagerRcPtr /* holder */>(
        m, "MixingColorSpaceManager", 
        DOC(MixingColorSpaceManager));

    // Enums
    py::enum_<LoggingLevel>(
        m, "LoggingLevel", py::arithmetic(), 
        DOC(PyOpenColorIO, LoggingLevel))

        .value("LOGGING_LEVEL_NONE", LOGGING_LEVEL_NONE, 
               DOC(PyOpenColorIO, LoggingLevel, LOGGING_LEVEL_NONE))
        .value("LOGGING_LEVEL_WARNING", LOGGING_LEVEL_WARNING, 
               DOC(PyOpenColorIO, LoggingLevel, LOGGING_LEVEL_WARNING))
        .value("LOGGING_LEVEL_INFO", LOGGING_LEVEL_INFO, 
               DOC(PyOpenColorIO, LoggingLevel, LOGGING_LEVEL_INFO))
        .value("LOGGING_LEVEL_DEBUG", LOGGING_LEVEL_DEBUG, 
               DOC(PyOpenColorIO, LoggingLevel, LOGGING_LEVEL_DEBUG))
        .value("LOGGING_LEVEL_UNKNOWN", LOGGING_LEVEL_UNKNOWN, 
               DOC(PyOpenColorIO, LoggingLevel, LOGGING_LEVEL_UNKNOWN))
        .export_values();

    py::enum_<ReferenceSpaceType>(
        m, "ReferenceSpaceType", 
        DOC(PyOpenColorIO, ReferenceSpaceType))

        .value("REFERENCE_SPACE_SCENE", REFERENCE_SPACE_SCENE, 
               DOC(PyOpenColorIO, ReferenceSpaceType, REFERENCE_SPACE_SCENE))
        .value("REFERENCE_SPACE_DISPLAY", REFERENCE_SPACE_DISPLAY, 
               DOC(PyOpenColorIO, ReferenceSpaceType, REFERENCE_SPACE_DISPLAY))
        .export_values();

    py::enum_<SearchReferenceSpaceType>(
        m, "SearchReferenceSpaceType", 
        DOC(PyOpenColorIO, SearchReferenceSpaceType))

        .value("SEARCH_REFERENCE_SPACE_SCENE", SEARCH_REFERENCE_SPACE_SCENE, 
               DOC(PyOpenColorIO, SearchReferenceSpaceType, SEARCH_REFERENCE_SPACE_SCENE))
        .value("SEARCH_REFERENCE_SPACE_DISPLAY", SEARCH_REFERENCE_SPACE_DISPLAY, 
               DOC(PyOpenColorIO, SearchReferenceSpaceType, SEARCH_REFERENCE_SPACE_DISPLAY))
        .value("SEARCH_REFERENCE_SPACE_ALL", SEARCH_REFERENCE_SPACE_ALL, 
               DOC(PyOpenColorIO, SearchReferenceSpaceType, SEARCH_REFERENCE_SPACE_ALL))
        .export_values();

    py::enum_<NamedTransformVisibility>(
        m, "NamedTransformVisibility",
        DOC(PyOpenColorIO, NamedTransformVisibility))

        .value("NAMEDTRANSFORM_ACTIVE", NAMEDTRANSFORM_ACTIVE, 
               DOC(PyOpenColorIO, NamedTransformVisibility, NAMEDTRANSFORM_ACTIVE))
        .value("NAMEDTRANSFORM_INACTIVE", NAMEDTRANSFORM_INACTIVE, 
               DOC(PyOpenColorIO, NamedTransformVisibility, NAMEDTRANSFORM_INACTIVE))
        .value("NAMEDTRANSFORM_ALL", NAMEDTRANSFORM_ALL, 
               DOC(PyOpenColorIO, NamedTransformVisibility, NAMEDTRANSFORM_ALL))
        .export_values();

    py::enum_<ColorSpaceVisibility>(
        m, "ColorSpaceVisibility", 
        DOC(PyOpenColorIO, ColorSpaceVisibility))

        .value("COLORSPACE_ACTIVE", COLORSPACE_ACTIVE, 
               DOC(PyOpenColorIO, ColorSpaceVisibility, COLORSPACE_ACTIVE))
        .value("COLORSPACE_INACTIVE", COLORSPACE_INACTIVE, 
               DOC(PyOpenColorIO, ColorSpaceVisibility, COLORSPACE_INACTIVE))
        .value("COLORSPACE_ALL", COLORSPACE_ALL, 
               DOC(PyOpenColorIO, ColorSpaceVisibility, COLORSPACE_ALL))
        .export_values();

    py::enum_<ViewType>(
        m, "ViewType", 
        DOC(PyOpenColorIO, ViewType))

        .value("VIEW_SHARED", VIEW_SHARED, 
               DOC(PyOpenColorIO, ViewType, VIEW_SHARED))
        .value("VIEW_DISPLAY_DEFINED", VIEW_DISPLAY_DEFINED, 
               DOC(PyOpenColorIO, ViewType, VIEW_DISPLAY_DEFINED))
        .export_values();

    py::enum_<ColorSpaceDirection>(
        m, "ColorSpaceDirection", 
        DOC(PyOpenColorIO, ColorSpaceDirection))

        .value("COLORSPACE_DIR_TO_REFERENCE", COLORSPACE_DIR_TO_REFERENCE, 
               DOC(PyOpenColorIO, ColorSpaceDirection, COLORSPACE_DIR_TO_REFERENCE))
        .value("COLORSPACE_DIR_FROM_REFERENCE", COLORSPACE_DIR_FROM_REFERENCE, 
               DOC(PyOpenColorIO, ColorSpaceDirection, COLORSPACE_DIR_FROM_REFERENCE))
        .export_values();

    py::enum_<ViewTransformDirection>(
        m, "ViewTransformDirection", 
        DOC(PyOpenColorIO, ViewTransformDirection))

        .value("VIEWTRANSFORM_DIR_TO_REFERENCE", VIEWTRANSFORM_DIR_TO_REFERENCE, 
               DOC(PyOpenColorIO, ViewTransformDirection, VIEWTRANSFORM_DIR_TO_REFERENCE))
        .value("VIEWTRANSFORM_DIR_FROM_REFERENCE", VIEWTRANSFORM_DIR_FROM_REFERENCE, 
               DOC(PyOpenColorIO, ViewTransformDirection, VIEWTRANSFORM_DIR_FROM_REFERENCE))
        .export_values();

    py::enum_<TransformDirection>(
        m, "TransformDirection", 
        DOC(PyOpenColorIO, TransformDirection))

        .value("TRANSFORM_DIR_FORWARD", TRANSFORM_DIR_FORWARD, 
               DOC(PyOpenColorIO, TransformDirection, TRANSFORM_DIR_FORWARD))
        .value("TRANSFORM_DIR_INVERSE", TRANSFORM_DIR_INVERSE, 
               DOC(PyOpenColorIO, TransformDirection, TRANSFORM_DIR_INVERSE))
        .export_values();

    py::enum_<TransformType>(
        m, "TransformType",
        DOC(PyOpenColorIO, TransformType))

        .value("TRANSFORM_TYPE_ALLOCATION", TRANSFORM_TYPE_ALLOCATION, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_ALLOCATION))
        .value("TRANSFORM_TYPE_BUILTIN", TRANSFORM_TYPE_BUILTIN, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_BUILTIN))
        .value("TRANSFORM_TYPE_CDL", TRANSFORM_TYPE_CDL, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_CDL))
        .value("TRANSFORM_TYPE_COLORSPACE", TRANSFORM_TYPE_COLORSPACE, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_COLORSPACE))
        .value("TRANSFORM_TYPE_DISPLAY_VIEW", TRANSFORM_TYPE_DISPLAY_VIEW, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_DISPLAY_VIEW))
        .value("TRANSFORM_TYPE_EXPONENT", TRANSFORM_TYPE_EXPONENT, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_EXPONENT))
        .value("TRANSFORM_TYPE_EXPONENT_WITH_LINEAR", TRANSFORM_TYPE_EXPONENT_WITH_LINEAR, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_EXPONENT_WITH_LINEAR))
        .value("TRANSFORM_TYPE_EXPOSURE_CONTRAST", TRANSFORM_TYPE_EXPOSURE_CONTRAST, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_EXPOSURE_CONTRAST))
        .value("TRANSFORM_TYPE_FILE", TRANSFORM_TYPE_FILE, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_FILE))
        .value("TRANSFORM_TYPE_FIXED_FUNCTION", TRANSFORM_TYPE_FIXED_FUNCTION, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_FIXED_FUNCTION))
        .value("TRANSFORM_TYPE_GRADING_PRIMARY", TRANSFORM_TYPE_GRADING_PRIMARY, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_GRADING_PRIMARY))
        .value("TRANSFORM_TYPE_GRADING_RGB_CURVE", TRANSFORM_TYPE_GRADING_RGB_CURVE, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_GRADING_RGB_CURVE))
        .value("TRANSFORM_TYPE_GRADING_TONE", TRANSFORM_TYPE_GRADING_TONE, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_GRADING_TONE))
        .value("TRANSFORM_TYPE_GROUP", TRANSFORM_TYPE_GROUP, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_GROUP))
        .value("TRANSFORM_TYPE_LOG_AFFINE", TRANSFORM_TYPE_LOG_AFFINE, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_LOG_AFFINE))
        .value("TRANSFORM_TYPE_LOG_CAMERA", TRANSFORM_TYPE_LOG_CAMERA, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_LOG_CAMERA))
        .value("TRANSFORM_TYPE_LOG", TRANSFORM_TYPE_LOG, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_LOG))
        .value("TRANSFORM_TYPE_LOOK", TRANSFORM_TYPE_LOOK, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_LOOK))
        .value("TRANSFORM_TYPE_LUT1D", TRANSFORM_TYPE_LUT1D, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_LUT1D))
        .value("TRANSFORM_TYPE_LUT3D", TRANSFORM_TYPE_LUT3D, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_LUT3D))
        .value("TRANSFORM_TYPE_MATRIX", TRANSFORM_TYPE_MATRIX, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_MATRIX))
        .value("TRANSFORM_TYPE_RANGE", TRANSFORM_TYPE_RANGE, 
               DOC(PyOpenColorIO, TransformType, TRANSFORM_TYPE_RANGE))
        .export_values();

    py::enum_<Interpolation>(
        m, "Interpolation", 
        DOC(PyOpenColorIO, Interpolation))

        .value("INTERP_UNKNOWN", INTERP_UNKNOWN, 
               DOC(PyOpenColorIO, Interpolation, INTERP_UNKNOWN))
        .value("INTERP_NEAREST", INTERP_NEAREST, 
               DOC(PyOpenColorIO, Interpolation, INTERP_NEAREST))
        .value("INTERP_LINEAR", INTERP_LINEAR, 
               DOC(PyOpenColorIO, Interpolation, INTERP_LINEAR))
        .value("INTERP_TETRAHEDRAL", INTERP_TETRAHEDRAL, 
               DOC(PyOpenColorIO, Interpolation, INTERP_TETRAHEDRAL))
        .value("INTERP_CUBIC", INTERP_CUBIC, 
               DOC(PyOpenColorIO, Interpolation, INTERP_CUBIC))
        .value("INTERP_DEFAULT", INTERP_DEFAULT, 
               DOC(PyOpenColorIO, Interpolation, INTERP_DEFAULT))
        .value("INTERP_BEST", INTERP_BEST, 
               DOC(PyOpenColorIO, Interpolation, INTERP_BEST))
        .export_values();

    py::enum_<BitDepth>(
        m, "BitDepth", py::arithmetic(), 
        DOC(PyOpenColorIO, BitDepth))

        .value("BIT_DEPTH_UNKNOWN", BIT_DEPTH_UNKNOWN, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_UNKNOWN))
        .value("BIT_DEPTH_UINT8", BIT_DEPTH_UINT8, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_UINT8))
        .value("BIT_DEPTH_UINT10", BIT_DEPTH_UINT10, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_UINT10))
        .value("BIT_DEPTH_UINT12", BIT_DEPTH_UINT12, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_UINT12))
        .value("BIT_DEPTH_UINT14", BIT_DEPTH_UINT14, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_UINT14))
        .value("BIT_DEPTH_UINT16", BIT_DEPTH_UINT16, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_UINT16))
        .value("BIT_DEPTH_UINT32", BIT_DEPTH_UINT32, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_UINT32))
        .value("BIT_DEPTH_F16", BIT_DEPTH_F16, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_F16))
        .value("BIT_DEPTH_F32", BIT_DEPTH_F32, 
               DOC(PyOpenColorIO, BitDepth, BIT_DEPTH_F32))
        .export_values();

    py::enum_<Lut1DHueAdjust>(
        m, "Lut1DHueAdjust", 
        DOC(PyOpenColorIO, Lut1DHueAdjust))

        .value("HUE_NONE", HUE_NONE, 
               DOC(PyOpenColorIO, Lut1DHueAdjust, HUE_NONE))
        .value("HUE_DW3", HUE_DW3,
               DOC(PyOpenColorIO, Lut1DHueAdjust, HUE_DW3))
        .value("HUE_WYPN", HUE_WYPN,
               DOC(PyOpenColorIO, Lut1DHueAdjust, HUE_WYPN))
        .export_values();

    py::enum_<ChannelOrdering>(
        m, "ChannelOrdering", 
        DOC(PyOpenColorIO, ChannelOrdering))

        .value("CHANNEL_ORDERING_RGBA", CHANNEL_ORDERING_RGBA, 
               DOC(PyOpenColorIO, ChannelOrdering, CHANNEL_ORDERING_RGBA))
        .value("CHANNEL_ORDERING_BGRA", CHANNEL_ORDERING_BGRA, 
               DOC(PyOpenColorIO, ChannelOrdering, CHANNEL_ORDERING_BGRA))
        .value("CHANNEL_ORDERING_ABGR", CHANNEL_ORDERING_ABGR, 
               DOC(PyOpenColorIO, ChannelOrdering, CHANNEL_ORDERING_ABGR))
        .value("CHANNEL_ORDERING_RGB", CHANNEL_ORDERING_RGB, 
               DOC(PyOpenColorIO, ChannelOrdering, CHANNEL_ORDERING_RGB))
        .value("CHANNEL_ORDERING_BGR", CHANNEL_ORDERING_BGR, 
               DOC(PyOpenColorIO, ChannelOrdering, CHANNEL_ORDERING_BGR))
        .export_values();

    py::enum_<Allocation>(
        m, "Allocation", 
        DOC(PyOpenColorIO, Allocation))

        .value("ALLOCATION_UNKNOWN", ALLOCATION_UNKNOWN, 
               DOC(PyOpenColorIO, Allocation, ALLOCATION_UNKNOWN))
        .value("ALLOCATION_UNIFORM", ALLOCATION_UNIFORM, 
               DOC(PyOpenColorIO, Allocation, ALLOCATION_UNIFORM))
        .value("ALLOCATION_LG2", ALLOCATION_LG2, 
               DOC(PyOpenColorIO, Allocation, ALLOCATION_LG2))
        .export_values();

    py::enum_<GpuLanguage>(
        m, "GpuLanguage", 
        DOC(PyOpenColorIO, GpuLanguage))

        .value("GPU_LANGUAGE_CG", GPU_LANGUAGE_CG, 
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_CG))
        .value("GPU_LANGUAGE_GLSL_1_2", GPU_LANGUAGE_GLSL_1_2, 
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_GLSL_1_2))
        .value("GPU_LANGUAGE_GLSL_1_3", GPU_LANGUAGE_GLSL_1_3, 
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_GLSL_1_3))
        .value("GPU_LANGUAGE_GLSL_4_0", GPU_LANGUAGE_GLSL_4_0, 
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_GLSL_4_0))
        .value("GPU_LANGUAGE_GLSL_ES_1_0", GPU_LANGUAGE_GLSL_ES_1_0, 
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_GLSL_ES_1_0))
        .value("GPU_LANGUAGE_GLSL_ES_3_0", GPU_LANGUAGE_GLSL_ES_3_0, 
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_GLSL_ES_3_0))
        .value("GPU_LANGUAGE_HLSL_DX11", GPU_LANGUAGE_HLSL_DX11, 
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_HLSL_DX11))
        .value("GPU_LANGUAGE_MSL_2_0", GPU_LANGUAGE_MSL_2_0,
               DOC(PyOpenColorIO, GpuLanguage, GPU_LANGUAGE_MSL_2_0))
        .value("LANGUAGE_OSL_1", LANGUAGE_OSL_1,
               DOC(PyOpenColorIO, GpuLanguage, LANGUAGE_OSL_1))
        .export_values();

    py::enum_<EnvironmentMode>(
        m, "EnvironmentMode", 
        DOC(PyOpenColorIO, EnvironmentMode))

        .value("ENV_ENVIRONMENT_UNKNOWN", ENV_ENVIRONMENT_UNKNOWN, 
               DOC(PyOpenColorIO, EnvironmentMode, ENV_ENVIRONMENT_UNKNOWN))
        .value("ENV_ENVIRONMENT_LOAD_PREDEFINED", ENV_ENVIRONMENT_LOAD_PREDEFINED, 
               DOC(PyOpenColorIO, EnvironmentMode, ENV_ENVIRONMENT_LOAD_PREDEFINED))
        .value("ENV_ENVIRONMENT_LOAD_ALL", ENV_ENVIRONMENT_LOAD_ALL, 
               DOC(PyOpenColorIO, EnvironmentMode, ENV_ENVIRONMENT_LOAD_ALL))
        .export_values();

    py::enum_<RangeStyle>(
        m, "RangeStyle", 
        DOC(PyOpenColorIO, RangeStyle))

        .value("RANGE_NO_CLAMP", RANGE_NO_CLAMP, 
               DOC(PyOpenColorIO, RangeStyle, RANGE_NO_CLAMP))
        .value("RANGE_CLAMP", RANGE_CLAMP, 
               DOC(PyOpenColorIO, RangeStyle, RANGE_CLAMP))
        .export_values();

    py::enum_<FixedFunctionStyle>(
        m, "FixedFunctionStyle", 
        DOC(PyOpenColorIO, FixedFunctionStyle))

        .value("FIXED_FUNCTION_ACES_RED_MOD_03", FIXED_FUNCTION_ACES_RED_MOD_03, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_RED_MOD_03))
        .value("FIXED_FUNCTION_ACES_RED_MOD_10", FIXED_FUNCTION_ACES_RED_MOD_10, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_RED_MOD_10))
        .value("FIXED_FUNCTION_ACES_GLOW_03", FIXED_FUNCTION_ACES_GLOW_03, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_GLOW_03))
        .value("FIXED_FUNCTION_ACES_GLOW_10", FIXED_FUNCTION_ACES_GLOW_10, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_GLOW_10))
        .value("FIXED_FUNCTION_ACES_DARK_TO_DIM_10", FIXED_FUNCTION_ACES_DARK_TO_DIM_10, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_DARK_TO_DIM_10))
        .value("FIXED_FUNCTION_REC2100_SURROUND", FIXED_FUNCTION_REC2100_SURROUND, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_REC2100_SURROUND))
        .value("FIXED_FUNCTION_RGB_TO_HSV", FIXED_FUNCTION_RGB_TO_HSV, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_RGB_TO_HSV))
        .value("FIXED_FUNCTION_XYZ_TO_xyY", FIXED_FUNCTION_XYZ_TO_xyY, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_XYZ_TO_xyY))
        .value("FIXED_FUNCTION_XYZ_TO_uvY", FIXED_FUNCTION_XYZ_TO_uvY, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_XYZ_TO_uvY))
        .value("FIXED_FUNCTION_XYZ_TO_LUV", FIXED_FUNCTION_XYZ_TO_LUV, 
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_XYZ_TO_LUV))
        .value("FIXED_FUNCTION_ACES_GAMUTMAP_02", FIXED_FUNCTION_ACES_GAMUTMAP_02,
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_GAMUTMAP_02))
        .value("FIXED_FUNCTION_ACES_GAMUTMAP_07", FIXED_FUNCTION_ACES_GAMUTMAP_07,
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_GAMUTMAP_07))
        .value("FIXED_FUNCTION_ACES_GAMUT_COMP_13", FIXED_FUNCTION_ACES_GAMUT_COMP_13,
               DOC(PyOpenColorIO, FixedFunctionStyle, FIXED_FUNCTION_ACES_GAMUT_COMP_13))
        .export_values();

    py::enum_<ExposureContrastStyle>(
        m, "ExposureContrastStyle", 
        DOC(PyOpenColorIO, ExposureContrastStyle))

        .value("EXPOSURE_CONTRAST_LINEAR", EXPOSURE_CONTRAST_LINEAR, 
               DOC(PyOpenColorIO, ExposureContrastStyle, EXPOSURE_CONTRAST_LINEAR))
        .value("EXPOSURE_CONTRAST_VIDEO", EXPOSURE_CONTRAST_VIDEO, 
               DOC(PyOpenColorIO, ExposureContrastStyle, EXPOSURE_CONTRAST_VIDEO))
        .value("EXPOSURE_CONTRAST_LOGARITHMIC", EXPOSURE_CONTRAST_LOGARITHMIC, 
               DOC(PyOpenColorIO, ExposureContrastStyle, EXPOSURE_CONTRAST_LOGARITHMIC))
        .export_values();

    py::enum_<CDLStyle>(
        m, "CDLStyle", 
        DOC(PyOpenColorIO, LoggingLevel))

        .value("CDL_ASC", CDL_ASC, 
               DOC(PyOpenColorIO, CDLStyle, CDL_ASC))
        .value("CDL_NO_CLAMP", CDL_NO_CLAMP, 
               DOC(PyOpenColorIO, CDLStyle, CDL_NO_CLAMP))
        .value("CDL_TRANSFORM_DEFAULT", CDL_TRANSFORM_DEFAULT, 
               DOC(PyOpenColorIO, CDLStyle, CDL_TRANSFORM_DEFAULT))
        .export_values();

    py::enum_<NegativeStyle>(
        m, "NegativeStyle", 
        DOC(PyOpenColorIO, NegativeStyle))

        .value("NEGATIVE_CLAMP", NEGATIVE_CLAMP, 
               DOC(PyOpenColorIO, NegativeStyle, NEGATIVE_CLAMP))
        .value("NEGATIVE_MIRROR", NEGATIVE_MIRROR, 
               DOC(PyOpenColorIO, NegativeStyle, NEGATIVE_MIRROR))
        .value("NEGATIVE_PASS_THRU", NEGATIVE_PASS_THRU, 
               DOC(PyOpenColorIO, NegativeStyle, NEGATIVE_PASS_THRU))
        .value("NEGATIVE_LINEAR", NEGATIVE_LINEAR, 
               DOC(PyOpenColorIO, NegativeStyle, NEGATIVE_LINEAR))
        .export_values();

    py::enum_<GradingStyle>(
        m, "GradingStyle", 
        DOC(PyOpenColorIO, GradingStyle))

        .value("GRADING_LOG", GRADING_LOG, 
               DOC(PyOpenColorIO, GradingStyle, GRADING_LOG))
        .value("GRADING_LIN", GRADING_LIN, 
               DOC(PyOpenColorIO, GradingStyle, GRADING_LIN))
        .value("GRADING_VIDEO", GRADING_VIDEO, 
               DOC(PyOpenColorIO, GradingStyle, GRADING_VIDEO))
        .export_values();

    py::enum_<DynamicPropertyType>(
        m, "DynamicPropertyType", 
        DOC(PyOpenColorIO, DynamicPropertyType))

        .value("DYNAMIC_PROPERTY_EXPOSURE", DYNAMIC_PROPERTY_EXPOSURE, 
               DOC(PyOpenColorIO, DynamicPropertyType, DYNAMIC_PROPERTY_EXPOSURE))
        .value("DYNAMIC_PROPERTY_CONTRAST", DYNAMIC_PROPERTY_CONTRAST, 
               DOC(PyOpenColorIO, DynamicPropertyType, DYNAMIC_PROPERTY_CONTRAST))
        .value("DYNAMIC_PROPERTY_GAMMA", DYNAMIC_PROPERTY_GAMMA, 
               DOC(PyOpenColorIO, DynamicPropertyType, DYNAMIC_PROPERTY_GAMMA))
        .value("DYNAMIC_PROPERTY_GRADING_PRIMARY", DYNAMIC_PROPERTY_GRADING_PRIMARY, 
               DOC(PyOpenColorIO, DynamicPropertyType, DYNAMIC_PROPERTY_GRADING_PRIMARY))
        .value("DYNAMIC_PROPERTY_GRADING_RGBCURVE", DYNAMIC_PROPERTY_GRADING_RGBCURVE, 
               DOC(PyOpenColorIO, DynamicPropertyType, DYNAMIC_PROPERTY_GRADING_RGBCURVE))
        .value("DYNAMIC_PROPERTY_GRADING_TONE", DYNAMIC_PROPERTY_GRADING_TONE, 
               DOC(PyOpenColorIO, DynamicPropertyType, DYNAMIC_PROPERTY_GRADING_TONE))
        .export_values();

    py::enum_<RGBCurveType>(
        m, "RGBCurveType", 
        DOC(PyOpenColorIO, RGBCurveType))

        .value("RGB_RED", RGB_RED, 
               DOC(PyOpenColorIO, RGBCurveType, RGB_RED))
        .value("RGB_GREEN", RGB_GREEN, 
               DOC(PyOpenColorIO, RGBCurveType, RGB_GREEN))
        .value("RGB_BLUE", RGB_BLUE, 
               DOC(PyOpenColorIO, RGBCurveType, RGB_BLUE))
        .value("RGB_MASTER", RGB_MASTER, 
               DOC(PyOpenColorIO, RGBCurveType, RGB_MASTER))
        .value("RGB_NUM_CURVES", RGB_NUM_CURVES, 
               DOC(PyOpenColorIO, RGBCurveType, RGB_NUM_CURVES))
        .export_values();

    py::enum_<UniformDataType>(
        m, "UniformDataType", 
        DOC(PyOpenColorIO, UniformDataType))

        .value("UNIFORM_DOUBLE", UNIFORM_DOUBLE, 
               DOC(PyOpenColorIO, UniformDataType, UNIFORM_DOUBLE))
        .value("UNIFORM_BOOL", UNIFORM_BOOL, 
               DOC(PyOpenColorIO, UniformDataType, UNIFORM_BOOL))
        .value("UNIFORM_FLOAT3", UNIFORM_FLOAT3, 
               DOC(PyOpenColorIO, UniformDataType, UNIFORM_FLOAT3))
        .value("UNIFORM_VECTOR_FLOAT", UNIFORM_VECTOR_FLOAT, 
               DOC(PyOpenColorIO, UniformDataType, UNIFORM_VECTOR_FLOAT))
        .value("UNIFORM_VECTOR_INT", UNIFORM_VECTOR_INT, 
               DOC(PyOpenColorIO, UniformDataType, UNIFORM_VECTOR_INT))
        .value("UNIFORM_UNKNOWN", UNIFORM_UNKNOWN, 
               DOC(PyOpenColorIO, UniformDataType, UNIFORM_UNKNOWN))
        .export_values();

    py::enum_<OptimizationFlags>(
        m, "OptimizationFlags", py::arithmetic(), 
        DOC(PyOpenColorIO, OptimizationFlags))

        .value("OPTIMIZATION_NONE", OPTIMIZATION_NONE, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_NONE))
        .value("OPTIMIZATION_IDENTITY", OPTIMIZATION_IDENTITY, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_IDENTITY))
        .value("OPTIMIZATION_IDENTITY_GAMMA", OPTIMIZATION_IDENTITY_GAMMA, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_IDENTITY_GAMMA))
        .value("OPTIMIZATION_PAIR_IDENTITY_CDL", OPTIMIZATION_PAIR_IDENTITY_CDL, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_CDL))
        .value("OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST", 
               OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST))
        .value("OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION", 
               OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION))
        .value("OPTIMIZATION_PAIR_IDENTITY_GAMMA", OPTIMIZATION_PAIR_IDENTITY_GAMMA, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_GAMMA))
        .value("OPTIMIZATION_PAIR_IDENTITY_LUT1D", OPTIMIZATION_PAIR_IDENTITY_LUT1D, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_LUT1D))
        .value("OPTIMIZATION_PAIR_IDENTITY_LUT3D", OPTIMIZATION_PAIR_IDENTITY_LUT3D, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_LUT3D))
        .value("OPTIMIZATION_PAIR_IDENTITY_LOG", OPTIMIZATION_PAIR_IDENTITY_LOG, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_LOG))
        .value("OPTIMIZATION_PAIR_IDENTITY_GRADING", OPTIMIZATION_PAIR_IDENTITY_GRADING, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_PAIR_IDENTITY_GRADING))
        .value("OPTIMIZATION_COMP_EXPONENT", OPTIMIZATION_COMP_EXPONENT, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_COMP_EXPONENT))
        .value("OPTIMIZATION_COMP_GAMMA", OPTIMIZATION_COMP_GAMMA, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_COMP_GAMMA))
        .value("OPTIMIZATION_COMP_MATRIX", OPTIMIZATION_COMP_MATRIX, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_COMP_MATRIX))
        .value("OPTIMIZATION_COMP_LUT1D", OPTIMIZATION_COMP_LUT1D, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_COMP_LUT1D))
        .value("OPTIMIZATION_COMP_LUT3D", OPTIMIZATION_COMP_LUT3D, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_COMP_LUT3D))
        .value("OPTIMIZATION_COMP_RANGE", OPTIMIZATION_COMP_RANGE, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_COMP_RANGE))
        .value("OPTIMIZATION_COMP_SEPARABLE_PREFIX", OPTIMIZATION_COMP_SEPARABLE_PREFIX, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_COMP_SEPARABLE_PREFIX))
        .value("OPTIMIZATION_LUT_INV_FAST", OPTIMIZATION_LUT_INV_FAST, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_LUT_INV_FAST))
        .value("OPTIMIZATION_FAST_LOG_EXP_POW", OPTIMIZATION_FAST_LOG_EXP_POW, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_FAST_LOG_EXP_POW))
        .value("OPTIMIZATION_SIMPLIFY_OPS", OPTIMIZATION_SIMPLIFY_OPS, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_SIMPLIFY_OPS))
        .value("OPTIMIZATION_NO_DYNAMIC_PROPERTIES", OPTIMIZATION_NO_DYNAMIC_PROPERTIES, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_NO_DYNAMIC_PROPERTIES))
        .value("OPTIMIZATION_ALL", OPTIMIZATION_ALL, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_ALL))
        .value("OPTIMIZATION_LOSSLESS", OPTIMIZATION_LOSSLESS, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_LOSSLESS))
        .value("OPTIMIZATION_VERY_GOOD", OPTIMIZATION_VERY_GOOD, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_VERY_GOOD))
        .value("OPTIMIZATION_GOOD", OPTIMIZATION_GOOD, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_GOOD))
        .value("OPTIMIZATION_DRAFT", OPTIMIZATION_DRAFT, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_DRAFT))
        .value("OPTIMIZATION_DEFAULT", OPTIMIZATION_DEFAULT, 
               DOC(PyOpenColorIO, OptimizationFlags, OPTIMIZATION_DEFAULT))
        .export_values();

    py::enum_<ProcessorCacheFlags>(
        m, "ProcessorCacheFlags", 
        DOC(PyOpenColorIO, ProcessorCacheFlags))

        .value("PROCESSOR_CACHE_OFF", PROCESSOR_CACHE_OFF, 
               DOC(PyOpenColorIO, ProcessorCacheFlags, PROCESSOR_CACHE_OFF))
        .value("PROCESSOR_CACHE_ENABLED", PROCESSOR_CACHE_ENABLED, 
               DOC(PyOpenColorIO, ProcessorCacheFlags, PROCESSOR_CACHE_ENABLED))
        .value("PROCESSOR_CACHE_SHARE_DYN_PROPERTIES", PROCESSOR_CACHE_SHARE_DYN_PROPERTIES, 
               DOC(PyOpenColorIO, ProcessorCacheFlags, PROCESSOR_CACHE_SHARE_DYN_PROPERTIES))
        .value("PROCESSOR_CACHE_DEFAULT", PROCESSOR_CACHE_DEFAULT, 
               DOC(PyOpenColorIO, ProcessorCacheFlags, PROCESSOR_CACHE_DEFAULT))
        .export_values();

    // Conversion
    m.def("BoolToString", &BoolToString, "value"_a, 
          DOC(PyOpenColorIO, BoolToString));
    m.def("BoolFromString", &BoolFromString, "str"_a, 
          DOC(PyOpenColorIO, BoolFromString));

    m.def("LoggingLevelToString", &LoggingLevelToString, "level"_a, 
          DOC(PyOpenColorIO, LoggingLevelToString));
    m.def("LoggingLevelFromString", &LoggingLevelFromString, "str"_a, 
          DOC(PyOpenColorIO, LoggingLevelFromString));

    m.def("TransformDirectionToString", &TransformDirectionToString, "direction"_a, 
          DOC(PyOpenColorIO, TransformDirectionToString));
    m.def("TransformDirectionFromString", &TransformDirectionFromString, "str"_a, 
          DOC(PyOpenColorIO, TransformDirectionFromString));

    m.def("GetInverseTransformDirection", &GetInverseTransformDirection, "direction"_a, 
          DOC(PyOpenColorIO, GetInverseTransformDirection));
    m.def("CombineTransformDirections", &CombineTransformDirections, "direction1"_a, "direction2"_a, 
          DOC(PyOpenColorIO, CombineTransformDirections));

    m.def("BitDepthToString", &BitDepthToString, "bitDepth"_a, 
          DOC(PyOpenColorIO, BitDepthToString));
    m.def("BitDepthFromString", &BitDepthFromString, "str"_a, 
          DOC(PyOpenColorIO, BitDepthFromString));
    m.def("BitDepthIsFloat", &BitDepthIsFloat, "bitDepth"_a, 
          DOC(PyOpenColorIO, BitDepthIsFloat));
    m.def("BitDepthToInt", &BitDepthToInt, "bitDepth"_a, 
          DOC(PyOpenColorIO, BitDepthToInt));

    m.def("AllocationToString", &AllocationToString, "allocation"_a, 
          DOC(PyOpenColorIO, AllocationToString));
    m.def("AllocationFromString", &AllocationFromString, "str"_a, 
          DOC(PyOpenColorIO, AllocationFromString));

    m.def("InterpolationToString", &InterpolationToString, "interpolation"_a, 
          DOC(PyOpenColorIO, InterpolationToString));
    m.def("InterpolationFromString", &InterpolationFromString, "str"_a, 
          DOC(PyOpenColorIO, InterpolationFromString));

    m.def("GpuLanguageToString", &GpuLanguageToString, "language"_a, 
          DOC(PyOpenColorIO, GpuLanguageToString));
    m.def("GpuLanguageFromString", &GpuLanguageFromString, "str"_a, 
          DOC(PyOpenColorIO, GpuLanguageFromString));

    m.def("EnvironmentModeToString", &EnvironmentModeToString, "mode"_a, 
          DOC(PyOpenColorIO, EnvironmentModeToString));
    m.def("EnvironmentModeFromString", &EnvironmentModeFromString, "str"_a, 
          DOC(PyOpenColorIO, EnvironmentModeFromString));

    m.def("CDLStyleToString", &CDLStyleToString, "style"_a, 
          DOC(PyOpenColorIO, CDLStyleToString));
    m.def("CDLStyleFromString", &CDLStyleFromString, "str"_a, 
          DOC(PyOpenColorIO, CDLStyleFromString));

    m.def("RangeStyleToString", &RangeStyleToString, "style"_a, 
          DOC(PyOpenColorIO, RangeStyleToString));
    m.def("RangeStyleFromString", &RangeStyleFromString, "str"_a, 
          DOC(PyOpenColorIO, RangeStyleFromString));

    m.def("FixedFunctionStyleToString", &FixedFunctionStyleToString, "style"_a, 
          DOC(PyOpenColorIO, FixedFunctionStyleToString));
    m.def("FixedFunctionStyleFromString", &FixedFunctionStyleFromString, "str"_a, 
          DOC(PyOpenColorIO, FixedFunctionStyleFromString));

    m.def("GradingStyleToString", &GradingStyleToString, "style"_a, 
          DOC(PyOpenColorIO, GradingStyleToString));
    m.def("GradingStyleFromString", &GradingStyleFromString, "str"_a, 
          DOC(PyOpenColorIO, GradingStyleFromString));

    m.def("ExposureContrastStyleToString", &ExposureContrastStyleToString, "style"_a, 
          DOC(PyOpenColorIO, ExposureContrastStyleToString));
    m.def("ExposureContrastStyleFromString", &ExposureContrastStyleFromString, "str"_a, 
          DOC(PyOpenColorIO, ExposureContrastStyleFromString));

    m.def("NegativeStyleToString", &NegativeStyleToString, "style"_a, 
          DOC(PyOpenColorIO, NegativeStyleToString));
    m.def("NegativeStyleFromString", &NegativeStyleFromString, "str"_a, 
          DOC(PyOpenColorIO, NegativeStyleFromString));

    // Env. variables
    m.attr("OCIO_CONFIG_ENVVAR") = OCIO_CONFIG_ENVVAR;
    m.attr("OCIO_ACTIVE_DISPLAYS_ENVVAR") = OCIO_ACTIVE_DISPLAYS_ENVVAR;
    m.attr("OCIO_ACTIVE_VIEWS_ENVVAR") = OCIO_ACTIVE_VIEWS_ENVVAR;
    m.attr("OCIO_INACTIVE_COLORSPACES_ENVVAR") = OCIO_INACTIVE_COLORSPACES_ENVVAR;
    m.attr("OCIO_OPTIMIZATION_FLAGS_ENVVAR") = OCIO_OPTIMIZATION_FLAGS_ENVVAR;
    m.attr("OCIO_USER_CATEGORIES_ENVVAR") = OCIO_USER_CATEGORIES_ENVVAR;

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
    m.attr("ROLE_RENDERING") = ROLE_RENDERING;
    m.attr("ROLE_INTERCHANGE_SCENE") = ROLE_INTERCHANGE_SCENE;
    m.attr("ROLE_INTERCHANGE_DISPLAY") = ROLE_INTERCHANGE_DISPLAY;

    // Shared View
    m.attr("OCIO_VIEW_USE_DISPLAY_NAME") = OCIO_VIEW_USE_DISPLAY_NAME;

    // FormatMetadata
    m.attr("METADATA_DESCRIPTION") = METADATA_DESCRIPTION;
    m.attr("METADATA_INFO") = METADATA_INFO;
    m.attr("METADATA_INPUT_DESCRIPTOR") = METADATA_INPUT_DESCRIPTOR;
    m.attr("METADATA_OUTPUT_DESCRIPTOR") = METADATA_OUTPUT_DESCRIPTOR;
    m.attr("METADATA_NAME") = METADATA_NAME;
    m.attr("METADATA_ID") = METADATA_ID;

    // Caches
    m.attr("OCIO_DISABLE_ALL_CACHES") = OCIO_DISABLE_ALL_CACHES;
    m.attr("OCIO_DISABLE_PROCESSOR_CACHES") = OCIO_DISABLE_PROCESSOR_CACHES;
    m.attr("OCIO_DISABLE_CACHE_FALLBACK") = OCIO_DISABLE_CACHE_FALLBACK;

    m.attr("OCIO_CONFIG_DEFAULT_NAME") = OCIO_CONFIG_DEFAULT_NAME;
    m.attr("OCIO_CONFIG_DEFAULT_FILE_EXT") = OCIO_CONFIG_DEFAULT_FILE_EXT;
    m.attr("OCIO_CONFIG_ARCHIVE_FILE_EXT") = OCIO_CONFIG_ARCHIVE_FILE_EXT;

    m.attr("OCIO_BUILTIN_URI_PREFIX") = OCIO_BUILTIN_URI_PREFIX;
}

} // namespace OCIO_NAMESPACE
