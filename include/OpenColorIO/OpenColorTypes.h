// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPENCOLORTYPES_H
#define INCLUDED_OCIO_OPENCOLORTYPES_H

#include "OpenColorABI.h"

#ifndef OCIO_NAMESPACE
#error This header cannot be used directly. Use <OpenColorIO/OpenColorIO.h> instead.
#endif

#include <limits>
#include <string>
#include <functional>


/*!rst::
C++ Types
=========
*/

namespace OCIO_NAMESPACE
{
// Predeclare all class ptr definitions

// Core

class OCIOEXPORT Config;
typedef OCIO_SHARED_PTR<const Config> ConstConfigRcPtr;
typedef OCIO_SHARED_PTR<Config> ConfigRcPtr;

class OCIOEXPORT FileRules;
typedef OCIO_SHARED_PTR<const FileRules> ConstFileRulesRcPtr;
typedef OCIO_SHARED_PTR<FileRules> FileRulesRcPtr;

class OCIOEXPORT ViewingRules;
typedef OCIO_SHARED_PTR<const ViewingRules> ConstViewingRulesRcPtr;
typedef OCIO_SHARED_PTR<ViewingRules> ViewingRulesRcPtr;

class OCIOEXPORT ColorSpace;
typedef OCIO_SHARED_PTR<const ColorSpace> ConstColorSpaceRcPtr;
typedef OCIO_SHARED_PTR<ColorSpace> ColorSpaceRcPtr;

class OCIOEXPORT ColorSpaceSet;
typedef OCIO_SHARED_PTR<const ColorSpaceSet> ConstColorSpaceSetRcPtr;
typedef OCIO_SHARED_PTR<ColorSpaceSet> ColorSpaceSetRcPtr;

class OCIOEXPORT Look;
typedef OCIO_SHARED_PTR<const Look> ConstLookRcPtr;
typedef OCIO_SHARED_PTR<Look> LookRcPtr;

class OCIOEXPORT ViewTransform;
typedef OCIO_SHARED_PTR<const ViewTransform> ConstViewTransformRcPtr;
typedef OCIO_SHARED_PTR<ViewTransform> ViewTransformRcPtr;

class OCIOEXPORT Context;
typedef OCIO_SHARED_PTR<const Context> ConstContextRcPtr;
typedef OCIO_SHARED_PTR<Context> ContextRcPtr;

class OCIOEXPORT Processor;
typedef OCIO_SHARED_PTR<const Processor> ConstProcessorRcPtr;
typedef OCIO_SHARED_PTR<Processor> ProcessorRcPtr;

class OCIOEXPORT CPUProcessor;
typedef OCIO_SHARED_PTR<const CPUProcessor> ConstCPUProcessorRcPtr;
typedef OCIO_SHARED_PTR<CPUProcessor> CPUProcessorRcPtr;

class OCIOEXPORT GPUProcessor;
typedef OCIO_SHARED_PTR<const GPUProcessor> ConstGPUProcessorRcPtr;
typedef OCIO_SHARED_PTR<GPUProcessor> GPUProcessorRcPtr;

class OCIOEXPORT ProcessorMetadata;
typedef OCIO_SHARED_PTR<const ProcessorMetadata> ConstProcessorMetadataRcPtr;
typedef OCIO_SHARED_PTR<ProcessorMetadata> ProcessorMetadataRcPtr;

class OCIOEXPORT Baker;
typedef OCIO_SHARED_PTR<const Baker> ConstBakerRcPtr;
typedef OCIO_SHARED_PTR<Baker> BakerRcPtr;

class OCIOEXPORT ImageDesc;
typedef OCIO_SHARED_PTR<ImageDesc> ImageDescRcPtr;
typedef OCIO_SHARED_PTR<const ImageDesc> ConstImageDescRcPtr;

class OCIOEXPORT Exception;

class OCIOEXPORT GpuShaderCreator;
typedef OCIO_SHARED_PTR<GpuShaderCreator> GpuShaderCreatorRcPtr;
typedef OCIO_SHARED_PTR<const GpuShaderCreator> ConstGpuShaderCreatorRcPtr;

class OCIOEXPORT GpuShaderDesc;
typedef OCIO_SHARED_PTR<GpuShaderDesc> GpuShaderDescRcPtr;
typedef OCIO_SHARED_PTR<const GpuShaderDesc> ConstGpuShaderDescRcPtr;

class OCIOEXPORT BuiltinTransformRegistry;
typedef OCIO_SHARED_PTR<BuiltinTransformRegistry> BuiltinTransformRegistryRcPtr;
typedef OCIO_SHARED_PTR<const BuiltinTransformRegistry> ConstBuiltinTransformRegistryRcPtr;


// Transforms

class OCIOEXPORT Transform;
typedef OCIO_SHARED_PTR<const Transform> ConstTransformRcPtr;
typedef OCIO_SHARED_PTR<Transform> TransformRcPtr;

class OCIOEXPORT AllocationTransform;
typedef OCIO_SHARED_PTR<const AllocationTransform> ConstAllocationTransformRcPtr;
typedef OCIO_SHARED_PTR<AllocationTransform> AllocationTransformRcPtr;

class OCIOEXPORT BuiltinTransform;
typedef OCIO_SHARED_PTR<const BuiltinTransform> ConstBuiltinTransformRcPtr;
typedef OCIO_SHARED_PTR<BuiltinTransform> BuiltinTransformRcPtr;

class OCIOEXPORT CDLTransform;
typedef OCIO_SHARED_PTR<const CDLTransform> ConstCDLTransformRcPtr;
typedef OCIO_SHARED_PTR<CDLTransform> CDLTransformRcPtr;

class OCIOEXPORT ColorSpaceTransform;
typedef OCIO_SHARED_PTR<const ColorSpaceTransform> ConstColorSpaceTransformRcPtr;
typedef OCIO_SHARED_PTR<ColorSpaceTransform> ColorSpaceTransformRcPtr;

class OCIOEXPORT DisplayViewTransform;
typedef OCIO_SHARED_PTR<const DisplayViewTransform> ConstDisplayViewTransformRcPtr;
typedef OCIO_SHARED_PTR<DisplayViewTransform> DisplayViewTransformRcPtr;

class OCIOEXPORT DynamicProperty;
typedef OCIO_SHARED_PTR<const DynamicProperty> ConstDynamicPropertyRcPtr;
typedef OCIO_SHARED_PTR<DynamicProperty> DynamicPropertyRcPtr;

class OCIOEXPORT ExponentTransform;
typedef OCIO_SHARED_PTR<const ExponentTransform> ConstExponentTransformRcPtr;
typedef OCIO_SHARED_PTR<ExponentTransform> ExponentTransformRcPtr;

class OCIOEXPORT ExponentWithLinearTransform;
typedef OCIO_SHARED_PTR<const ExponentWithLinearTransform> ConstExponentWithLinearTransformRcPtr;
typedef OCIO_SHARED_PTR<ExponentWithLinearTransform> ExponentWithLinearTransformRcPtr;

class OCIOEXPORT ExposureContrastTransform;
typedef OCIO_SHARED_PTR<const ExposureContrastTransform> ConstExposureContrastTransformRcPtr;
typedef OCIO_SHARED_PTR<ExposureContrastTransform> ExposureContrastTransformRcPtr;

class OCIOEXPORT FileTransform;
typedef OCIO_SHARED_PTR<const FileTransform> ConstFileTransformRcPtr;
typedef OCIO_SHARED_PTR<FileTransform> FileTransformRcPtr;

class OCIOEXPORT FixedFunctionTransform;
typedef OCIO_SHARED_PTR<const FixedFunctionTransform> ConstFixedFunctionTransformRcPtr;
typedef OCIO_SHARED_PTR<FixedFunctionTransform> FixedFunctionTransformRcPtr;

class OCIOEXPORT GroupTransform;
typedef OCIO_SHARED_PTR<const GroupTransform> ConstGroupTransformRcPtr;
typedef OCIO_SHARED_PTR<GroupTransform> GroupTransformRcPtr;

class OCIOEXPORT LogAffineTransform;
typedef OCIO_SHARED_PTR<const LogAffineTransform> ConstLogAffineTransformRcPtr;
typedef OCIO_SHARED_PTR<LogAffineTransform> LogAffineTransformRcPtr;

class OCIOEXPORT LogCameraTransform;
typedef OCIO_SHARED_PTR<const LogCameraTransform> ConstLogCameraTransformRcPtr;
typedef OCIO_SHARED_PTR<LogCameraTransform> LogCameraTransformRcPtr;

class OCIOEXPORT LookTransform;
typedef OCIO_SHARED_PTR<const LookTransform> ConstLookTransformRcPtr;
typedef OCIO_SHARED_PTR<LookTransform> LookTransformRcPtr;

class OCIOEXPORT LogTransform;
typedef OCIO_SHARED_PTR<const LogTransform> ConstLogTransformRcPtr;
typedef OCIO_SHARED_PTR<LogTransform> LogTransformRcPtr;

class OCIOEXPORT Lut1DTransform;
typedef OCIO_SHARED_PTR<const Lut1DTransform> ConstLut1DTransformRcPtr;
typedef OCIO_SHARED_PTR<Lut1DTransform> Lut1DTransformRcPtr;

class OCIOEXPORT Lut3DTransform;
typedef OCIO_SHARED_PTR<const Lut3DTransform> ConstLut3DTransformRcPtr;
typedef OCIO_SHARED_PTR<Lut3DTransform> Lut3DTransformRcPtr;

class OCIOEXPORT MatrixTransform;
typedef OCIO_SHARED_PTR<const MatrixTransform> ConstMatrixTransformRcPtr;
typedef OCIO_SHARED_PTR<MatrixTransform> MatrixTransformRcPtr;

class OCIOEXPORT RangeTransform;
typedef OCIO_SHARED_PTR<const RangeTransform> ConstRangeTransformRcPtr;
typedef OCIO_SHARED_PTR<RangeTransform> RangeTransformRcPtr;

template <class T, class U>
inline OCIO_SHARED_PTR<T> DynamicPtrCast(OCIO_SHARED_PTR<U> const & ptr)
{
    return OCIO_DYNAMIC_POINTER_CAST<T,U>(ptr);
}

/// Define the logging function signature.
using LoggingFunction = std::function<void(const char*)>;

// Enums

enum LoggingLevel
{
    LOGGING_LEVEL_NONE = 0,
    LOGGING_LEVEL_WARNING = 1,
    LOGGING_LEVEL_INFO = 2,
    LOGGING_LEVEL_DEBUG = 3,
    LOGGING_LEVEL_UNKNOWN = 255,

    LOGGING_LEVEL_DEFAULT = LOGGING_LEVEL_INFO
};

/**
 * OCIO does not mandate the image state of the main reference space and it is not
 * required to be scene-referred.  This enum is used in connection with the display color space
 * and view transform features which do assume that the main reference space is scene-referred
 * and the display reference space is display-referred.  If a config used a non-scene-referred
 * reference space, presumably it would not use either display color spaces or view transforms,
 * so this enum becomes irrelevant.
 */
enum ReferenceSpaceType
{
    REFERENCE_SPACE_SCENE = 0,  ///< the main scene reference space
    REFERENCE_SPACE_DISPLAY     ///< the reference space for display color spaces
};

enum SearchReferenceSpaceType
{
    SEARCH_REFERENCE_SPACE_SCENE = 0,
    SEARCH_REFERENCE_SPACE_DISPLAY,
    SEARCH_REFERENCE_SPACE_ALL
};

enum ColorSpaceVisibility
{
    COLORSPACE_ACTIVE = 0,
    COLORSPACE_INACTIVE,
    COLORSPACE_ALL
};

enum ViewType
{
    VIEW_SHARED = 0,
    VIEW_DISPLAY_DEFINED
};

enum ColorSpaceDirection
{
    COLORSPACE_DIR_UNKNOWN = 0,
    COLORSPACE_DIR_TO_REFERENCE,
    COLORSPACE_DIR_FROM_REFERENCE
};

enum ViewTransformDirection
{
    VIEWTRANSFORM_DIR_UNKNOWN = 0,
    VIEWTRANSFORM_DIR_TO_REFERENCE,
    VIEWTRANSFORM_DIR_FROM_REFERENCE
};

enum TransformDirection
{
    TRANSFORM_DIR_UNKNOWN = 0,
    TRANSFORM_DIR_FORWARD,
    TRANSFORM_DIR_INVERSE
};

/**
 * Specify the interpolation type to use
 * If the specified interpolation type is not supported in the requested
 * context (for example, using tetrahedral interpolationon 1D LUTs)
 * an exception will be thrown.
 *
 * INTERP_DEFAULT will choose the default interpolation type for the requested
 * context:
 *
 * 1D LUT INTERP_DEFAULT: LINEAR
 * 3D LUT INTERP_DEFAULT: LINEAR
 *
 * INTERP_BEST will choose the best interpolation type for the requested
 * context:
 *
 * 1D LUT INTERP_BEST: LINEAR
 * 3D LUT INTERP_BEST: TETRAHEDRAL
 *
 * Note: INTERP_BEST and INTERP_DEFAULT are subject to change in minor
 * releases, so if you care about locking off on a specific interpolation
 * type, we'd recommend directly specifying it.
 */
enum Interpolation
{
    INTERP_UNKNOWN = 0,
    INTERP_NEAREST = 1,     ///< nearest neighbor
    INTERP_LINEAR = 2,      ///< linear interpolation (trilinear for Lut3D)
    INTERP_TETRAHEDRAL = 3, ///< tetrahedral interpolation (Lut3D only)
    INTERP_CUBIC = 4,       ///< cubic interpolation (not supported)

    INTERP_DEFAULT = 254,   ///< the default interpolation type
    INTERP_BEST = 255       ///< the 'best' suitable interpolation type
};

/**
 * Used in a configuration file to indicate the bit-depth of a color space,
 * and by the \ref Processor to specify the input and output bit-depths of 
 * images to process.
 * Note that \ref Processor only supports: UINT8, UINT10, UINT12, UINT16, F16 and F32.
 */
enum BitDepth
{
    BIT_DEPTH_UNKNOWN = 0,
    BIT_DEPTH_UINT8,
    BIT_DEPTH_UINT10,
    BIT_DEPTH_UINT12,
    BIT_DEPTH_UINT14,
    BIT_DEPTH_UINT16,
    BIT_DEPTH_UINT32,
    BIT_DEPTH_F16,
    BIT_DEPTH_F32
};

/// Used by :cpp:class`Lut1DTransform` to control optional hue restoration algorithm.
enum Lut1DHueAdjust
{
    HUE_NONE = 0, // No adjustment.
    HUE_DW3       // Algorithm used in ACES Output Transforms through v0.7.
};

/// Used by \ref PackedImageDesc to indicate the channel ordering of the image to process.
enum ChannelOrdering
{
    CHANNEL_ORDERING_RGBA = 0,
    CHANNEL_ORDERING_BGRA,
    CHANNEL_ORDERING_ABGR,
    CHANNEL_ORDERING_RGB,
    CHANNEL_ORDERING_BGR
};

enum Allocation {
    ALLOCATION_UNKNOWN = 0,
    ALLOCATION_UNIFORM,
    ALLOCATION_LG2
};

/// Used when there is a choice of hardware shader language.
enum GpuLanguage
{
    GPU_LANGUAGE_UNKNOWN = 0,
    GPU_LANGUAGE_CG,                ///< Nvidia Cg shader
    GPU_LANGUAGE_GLSL_1_0,          ///< OpenGL Shading Language
    GPU_LANGUAGE_GLSL_1_3,          ///< OpenGL Shading Language
    GPU_LANGUAGE_GLSL_4_0,          ///< OpenGL Shading Language
    GPU_LANGUAGE_HLSL_DX11          ///< DirectX Shading Language
};

enum EnvironmentMode
{
    ENV_ENVIRONMENT_UNKNOWN = 0,
    ENV_ENVIRONMENT_LOAD_PREDEFINED,
    ENV_ENVIRONMENT_LOAD_ALL
};

/// A RangeTransform may be set to clamp the values, or not.
enum RangeStyle
{
    RANGE_NO_CLAMP = 0,
    RANGE_CLAMP
};

/// Enumeration of the :cpp:class:`FixedFunctionTransform` transform algorithms.
enum FixedFunctionStyle
{
    FIXED_FUNCTION_ACES_RED_MOD_03 = 0, ///< Red modifier (ACES 0.3/0.7)
    FIXED_FUNCTION_ACES_RED_MOD_10,     ///< Red modifier (ACES 1.0)
    FIXED_FUNCTION_ACES_GLOW_03,        ///< Glow function (ACES 0.3/0.7)
    FIXED_FUNCTION_ACES_GLOW_10,        ///< Glow function (ACES 1.0)
    FIXED_FUNCTION_ACES_DARK_TO_DIM_10, ///< Dark to dim surround correction (ACES 1.0)
    FIXED_FUNCTION_REC2100_SURROUND,    ///< Rec.2100 surround correction (takes one double for the gamma param)
    FIXED_FUNCTION_RGB_TO_HSV,          ///< Classic RGB to HSV function
    FIXED_FUNCTION_XYZ_TO_xyY,          ///< CIE XYZ to 1931 xy chromaticity coordinates
    FIXED_FUNCTION_XYZ_TO_uvY,          ///< CIE XYZ to 1976 u'v' chromaticity coordinates
    FIXED_FUNCTION_XYZ_TO_LUV           ///< CIE XYZ to 1976 CIELUV colour space (D65 white)
};

/// Enumeration of the :cpp:class:`ExposureContrastTransform` transform algorithms.
enum ExposureContrastStyle
{
    EXPOSURE_CONTRAST_LINEAR = 0,      ///< E/C to be applied to a linear space image
    EXPOSURE_CONTRAST_VIDEO,           ///< E/C to be applied to a video space image
    EXPOSURE_CONTRAST_LOGARITHMIC      ///< E/C to be applied to a log space image
};

/**
 * Enumeration of the :cpp:class:`CDLTransform` transform algorithms.
 * 
 * \note
 *      The default for reading .cc/.ccc/.cdl files, config file YAML, and CDLTransform is no-clamp,
 *      since that is what is primarily desired in VFX.  However, the CLF format default is ASC.
 */
enum CDLStyle
{
    CDL_ASC = 0,    ///< ASC CDL specification v1.2
    CDL_NO_CLAMP,   ///< CDL that does not clamp
    CDL_TRANSFORM_DEFAULT = CDL_NO_CLAMP
};

/**
 * Negative values handling style for \ref ExponentTransform and
 * \ref ExponentWithLinearTransform transform algorithms.
 */
enum NegativeStyle
{
    NEGATIVE_CLAMP = 0, ///< Clamp negative values
    NEGATIVE_MIRROR,    ///< Positive curve is rotated 180 degrees around the origin to handle negatives.
    NEGATIVE_PASS_THRU, ///< Negative values are passed through unchanged.
    NEGATIVE_LINEAR     ///< Linearly extrapolate the curve for negative values.
};

enum DynamicPropertyType
{
    DYNAMIC_PROPERTY_EXPOSURE = 0, ///< Image exposure value (double floating point value)
    DYNAMIC_PROPERTY_CONTRAST,     ///< Image contrast value (double floating point value)
    DYNAMIC_PROPERTY_GAMMA         ///< Image gamma value (double floating point value)
};

enum DynamicPropertyValueType
{
    DYNAMIC_PROPERTY_DOUBLE, ///< Value is a double
    DYNAMIC_PROPERTY_BOOL    ///< Value is a bool
};

/// Provides control over how the ops in a Processor are combined in order to improve performance.
enum OptimizationFlags : unsigned long
{
    // Below are listed all the optimization types.

    /// Do not optimize.
    OPTIMIZATION_NONE                            = 0x00000000,

    /// Replace identity ops (other than gamma).
    OPTIMIZATION_IDENTITY                        = 0x00000001,
    /// Replace identity gamma ops.
    OPTIMIZATION_IDENTITY_GAMMA                  = 0x00000002,
    /// Replace a pair of ops where one is the inverse of the other.
    OPTIMIZATION_PAIR_IDENTITY_CDL               = 0x00000004,
    OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST = 0x00000008,
    OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION    = 0x00000010,
    OPTIMIZATION_PAIR_IDENTITY_GAMMA             = 0x00000020,
    OPTIMIZATION_PAIR_IDENTITY_LUT1D             = 0x00000040,
    OPTIMIZATION_PAIR_IDENTITY_LUT3D             = 0x00000080,
    OPTIMIZATION_PAIR_IDENTITY_LOG               = 0x00000100,

    /// Compose a pair of ops into a single op.
    OPTIMIZATION_COMP_EXPONENT                   = 0x00000200,
    OPTIMIZATION_COMP_GAMMA                      = 0x00000400,
    OPTIMIZATION_COMP_MATRIX                     = 0x00000800,
    OPTIMIZATION_COMP_LUT1D                      = 0x00001000,
    OPTIMIZATION_COMP_LUT3D                      = 0x00002000,
    OPTIMIZATION_COMP_RANGE                      = 0x00004000,

    /**
     * For integer and half bit-depths only, replace separable ops (i.e. no
     * channel crosstalk ops) by a single 1D LUT of input bit-depth domain.
     */
    OPTIMIZATION_COMP_SEPARABLE_PREFIX           = 0x00008000,

    /**
     * Implement inverse Lut1D and Lut3D evaluations using a a forward LUT
     * (faster but less accurate).  Note that GPU evals always do FAST.
     */
    OPTIMIZATION_LUT_INV_FAST                    = 0x00010000,

    /**
     * Turn off dynamic control of any ops that offer adjustment of parameter
     * values after finalization (e.g. ExposureContrast).
     */
    OPTIMIZATION_NO_DYNAMIC_PROPERTIES           = 0x00020000,

    /// Apply all possible optimizations.
    OPTIMIZATION_ALL                             = 0xFFFFFFFF,

    // The following groupings of flags are provided as a convenient way to select an overall
    // optimization level.

    OPTIMIZATION_LOSSLESS   = (OPTIMIZATION_IDENTITY |
                                OPTIMIZATION_IDENTITY_GAMMA |
                                OPTIMIZATION_PAIR_IDENTITY_CDL |
                                OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST |
                                OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION |
                                OPTIMIZATION_PAIR_IDENTITY_GAMMA |
                                OPTIMIZATION_PAIR_IDENTITY_LOG |
                                OPTIMIZATION_PAIR_IDENTITY_LUT1D |
                                OPTIMIZATION_PAIR_IDENTITY_LUT3D |
                                OPTIMIZATION_COMP_EXPONENT |
                                OPTIMIZATION_COMP_GAMMA |
                                OPTIMIZATION_COMP_MATRIX |
                                OPTIMIZATION_COMP_RANGE),

    OPTIMIZATION_VERY_GOOD  = (OPTIMIZATION_LOSSLESS |
                                OPTIMIZATION_COMP_LUT1D |
                                OPTIMIZATION_LUT_INV_FAST |
                                OPTIMIZATION_COMP_SEPARABLE_PREFIX),

    OPTIMIZATION_GOOD       = OPTIMIZATION_VERY_GOOD | OPTIMIZATION_COMP_LUT3D,

    /// For quite lossy optimizations.
    OPTIMIZATION_DRAFT      = OPTIMIZATION_ALL,

    OPTIMIZATION_DEFAULT    = OPTIMIZATION_VERY_GOOD
};


// Conversion

extern OCIOEXPORT const char * BoolToString(bool val);
extern OCIOEXPORT bool BoolFromString(const char * s);

extern OCIOEXPORT const char * LoggingLevelToString(LoggingLevel level);
extern OCIOEXPORT LoggingLevel LoggingLevelFromString(const char * s);

extern OCIOEXPORT const char * TransformDirectionToString(TransformDirection dir);
extern OCIOEXPORT TransformDirection TransformDirectionFromString(const char * s);

extern OCIOEXPORT TransformDirection GetInverseTransformDirection(TransformDirection dir);
extern OCIOEXPORT TransformDirection CombineTransformDirections(TransformDirection d1,
                                                                TransformDirection d2);

extern OCIOEXPORT const char * ColorSpaceDirectionToString(ColorSpaceDirection dir);
extern OCIOEXPORT ColorSpaceDirection ColorSpaceDirectionFromString(const char * s);

extern OCIOEXPORT const char * BitDepthToString(BitDepth bitDepth);
extern OCIOEXPORT BitDepth BitDepthFromString(const char * s);
extern OCIOEXPORT bool BitDepthIsFloat(BitDepth bitDepth);
extern OCIOEXPORT int BitDepthToInt(BitDepth bitDepth);

extern OCIOEXPORT const char * AllocationToString(Allocation allocation);
extern OCIOEXPORT Allocation AllocationFromString(const char * s);

extern OCIOEXPORT const char * InterpolationToString(Interpolation interp);
extern OCIOEXPORT Interpolation InterpolationFromString(const char * s);

extern OCIOEXPORT const char * GpuLanguageToString(GpuLanguage language);
extern OCIOEXPORT GpuLanguage GpuLanguageFromString(const char * s);

extern OCIOEXPORT const char * EnvironmentModeToString(EnvironmentMode mode);
extern OCIOEXPORT EnvironmentMode EnvironmentModeFromString(const char * s);

extern OCIOEXPORT const char * CDLStyleToString(CDLStyle style);
extern OCIOEXPORT CDLStyle CDLStyleFromString(const char * style);

extern OCIOEXPORT const char * RangeStyleToString(RangeStyle style);
extern OCIOEXPORT RangeStyle RangeStyleFromString(const char * style);

extern OCIOEXPORT const char * FixedFunctionStyleToString(FixedFunctionStyle style);
extern OCIOEXPORT FixedFunctionStyle FixedFunctionStyleFromString(const char * style);

extern OCIOEXPORT const char * ExposureContrastStyleToString(ExposureContrastStyle style);
extern OCIOEXPORT ExposureContrastStyle ExposureContrastStyleFromString(const char * style);

extern OCIOEXPORT const char * NegativeStyleToString(NegativeStyle style);
extern OCIOEXPORT NegativeStyle NegativeStyleFromString(const char * style);

// TODO: Move to .rst
/*
Envvar
******

These environmental variables are used by the OpenColorIO library
(i.e. these variables are defined in src/OpenColorIO/Config.cpp).
*/


/// The envvar 'OCIO' provides a path to the config file used by \ref Config::CreateFromEnv
extern OCIOEXPORT const char * OCIO_CONFIG_ENVVAR;

/**
 * The envvar 'OCIO_ACTIVE_DISPLAYS' provides a list of displays overriding the 'active_displays'
 * list from the config file.
 */
extern OCIOEXPORT const char * OCIO_ACTIVE_DISPLAYS_ENVVAR;

/**
 * The envvar 'OCIO_ACTIVE_VIEWS' provides a list of views overriding the 'active_views'
 * list from the config file.
 */
extern OCIOEXPORT const char * OCIO_ACTIVE_VIEWS_ENVVAR;

/**
 * The envvar 'OCIO_INACTIVE_COLORSPACES' provides a list of inactive color spaces
 * overriding the 'inactive_color_spaces' list from the config file.
 */
extern OCIOEXPORT const char * OCIO_INACTIVE_COLORSPACES_ENVVAR;

/**
 * The envvar 'OCIO_OPTIMIZATION_FLAGS' provides a way to force a given optimization level.
 * Remove the variable or set the value to empty to not use it. Set the value of the variable
 * to the desired optimization level as either an integer or hexadecimal value.
 * Ex: OCIO_OPTIMIZATION_FLAGS="20479" or "0x4FFF" for OPTIMIZATION_LOSSLESS.
 */
extern OCIOEXPORT const char * OCIO_OPTIMIZATION_FLAGS_ENVVAR;

// TODO: Move to .rst
/*!rst::
Roles
*****

ColorSpace Roles are used so that plugins, in addition to this API can have
abstract ways of asking for common colorspaces, without referring to them
by hardcoded names.

Internal::
    Extracting color space from file path - (ROLE_DEFAULT)

App Helpers::
    ViewingPipeline         - (ROLE_SCENE_LINEAR (LinearCC for exposure))
                              (ROLE_COLOR_TIMING (ColorTimingCC))
    MixingColorSpaceManager - (ROLE_COLOR_PICKING)

External Plugins (currently known)::

    Colorpicker UIs       - (ROLE_COLOR_PICKING)
    Compositor LogConvert - (ROLE_SCENE_LINEAR, ROLE_COMPOSITING_LOG)

*/

/// "default"
extern OCIOEXPORT const char * ROLE_DEFAULT;
/// "reference"
extern OCIOEXPORT const char * ROLE_REFERENCE;
/// "data"
extern OCIOEXPORT const char * ROLE_DATA;
/// "color_picking"
extern OCIOEXPORT const char * ROLE_COLOR_PICKING;
/// "scene_linear"
extern OCIOEXPORT const char * ROLE_SCENE_LINEAR;
/// "compositing_log"
extern OCIOEXPORT const char * ROLE_COMPOSITING_LOG;
/// "color_timing"
extern OCIOEXPORT const char * ROLE_COLOR_TIMING;
/**
 * This role defines the transform for painting textures. In some
 * workflows this is just a inverse display gamma with some limits
 */
extern OCIOEXPORT const char * ROLE_TEXTURE_PAINT;
/**
 * This role defines the transform for matte painting. In some workflows
 * this is a 1D HDR to LDR allocation. It is normally combined with
 * another display transform in the host app for preview.
 */
extern OCIOEXPORT const char * ROLE_MATTE_PAINT;

/*!rst::
Shared View
***********

*/

/**
 * A shared view using this for the color space name will use a display color space that
 * has the same name as the display the shared view is used by.
 */
extern OCIOEXPORT const char * OCIO_VIEW_USE_DISPLAY_NAME;

// TODO: Move to .rst
/*!rst::
FormatMetadata
**************

These constants describe various types of rich metadata. They are used with FormatMetadata
objects as the "name" part of a (name, value) pair. All of these types of metadata are
supported in the CLF/CTF file formats whereas other formats support some or none of them.

Although the string constants used here match those used in the CLF/CTF formats, the concepts
are generic, so the goal is for other file formats to reuse the same constants within a
FormatMetadata object (even if the syntax used in a given format is somewhat different).

*/

/**
 * A description string -- used as the "Description" element in CLF/CTF and CDL, and to
 * hold comments for other LUT formats when baking.
 */
extern OCIOEXPORT const char * METADATA_DESCRIPTION;

/**
 * A block of informative metadata such as the "Info" element in CLF/CTF.
 * Usually contains child elements.
 */
extern OCIOEXPORT const char * METADATA_INFO;

/**
 * A string describing the expected input color space -- used as the "InputDescriptor"
 * element in CLF/CTF and the "InputDescription" in CDL.
 */
extern OCIOEXPORT const char * METADATA_INPUT_DESCRIPTOR;

/**
 * A string describing the output color space -- used as the "OutputDescriptor" element
 * in CLF/CTF and the "OutputDescription" in CDL.
 */
extern OCIOEXPORT const char * METADATA_OUTPUT_DESCRIPTOR;

/**
 * A name string -- used as a "name" attribute in CLF/CTF elements.  Use on a GroupTransform
 * to get/set the name for the CLF/CTF ProcessList.  Use on an individual Transform
 * (i.e. MatrixTransform, etc.) to get/set the name of the corresponding process node.
 */
extern OCIOEXPORT const char * METADATA_NAME;

/**
 * An ID string -- used as an "id" attribute in CLF/CTF elements.  Use on a GroupTransform
 * to get/set the id for the CLF/CTF ProcessList.  Use on an individual Transform
 * (i.e. MatrixTransform, etc.) to get/set the id of the corresponding process node.
 */
extern OCIOEXPORT const char * METADATA_ID;

} // namespace OCIO_NAMESPACE

#endif
