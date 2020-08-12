..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Types
*****

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.



LoggingLevel 
============

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::LoggingLevel 

            *Values:*

            .. cpp:enumerator:: LOGGING_LEVEL_NONE = 0 

            .. cpp:enumerator:: LOGGING_LEVEL_WARNING = 1 

            .. cpp:enumerator:: LOGGING_LEVEL_INFO = 2 

            .. cpp:enumerator:: LOGGING_LEVEL_DEBUG = 3 

            .. cpp:enumerator:: LOGGING_LEVEL_UNKNOWN = 255 

            .. cpp:enumerator:: LOGGING_LEVEL_DEFAULT = `LOGGING_LEVEL_INFO`_ 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.LoggingLevel

            Members:

            * LOGGING_LEVEL_NONE

            * LOGGING_LEVEL_WARNING

            * LOGGING_LEVEL_INFO

            * LOGGING_LEVEL_DEBUG

            * LOGGING_LEVEL_UNKNOWN

ReferenceSpaceType 
==================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::ReferenceSpaceType 

            OCIO does not mandate the image state of the main reference space
            and it is not required to be scene-referred. This enum is used in
            connection with the display color space and view transform features
            which do assume that the main reference space is scene-referred and
            the display reference space is display-referred. If a config used a
            non-scene-referred reference space, presumably it would not use
            either display color spaces or view transforms, so this enum
            becomes irrelevant.

            *Values:*

            .. cpp:enumerator:: REFERENCE_SPACE_SCENE = 0 

                the main scene reference space

            .. cpp:enumerator:: REFERENCE_SPACE_DISPLAY 

                the reference space for display color spaces

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.ReferenceSpaceType

            Members:

            * REFERENCE_SPACE_SCENE

            * REFERENCE_SPACE_DISPLAY

SearchReferenceSpaceType 
========================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::SearchReferenceSpaceType 

            *Values:*

            .. cpp:enumerator:: SEARCH_REFERENCE_SPACE_SCENE = 0 

            .. cpp:enumerator:: SEARCH_REFERENCE_SPACE_DISPLAY 

            .. cpp:enumerator:: SEARCH_REFERENCE_SPACE_ALL

   .. group-tab:: Python

        .. py:class:: PyOpenColorIO.SearchReferenceSpaceType

            Members:

            * SEARCH_REFERENCE_SPACE_SCENE

            * SEARCH_REFERENCE_SPACE_DISPLAY

            * SEARCH_REFERENCE_SPACE_ALL

ColorSpaceVisibility 
====================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::ColorSpaceVisibility 

            *Values:*

            .. cpp:enumerator:: COLORSPACE_ACTIVE = 0 

            .. cpp:enumerator:: COLORSPACE_INACTIVE 

            .. cpp:enumerator:: COLORSPACE_ALL 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.ColorSpaceVisibility

            Members:

            * COLORSPACE_ACTIVE

            * COLORSPACE_INACTIVE

            * COLORSPACE_ALL

ColorSpaceDirection 
===================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::ColorSpaceDirection 

            *Values:*

            .. cpp:enumerator:: COLORSPACE_DIR_UNKNOWN = 0 

            .. cpp:enumerator:: COLORSPACE_DIR_TO_REFERENCE 

            .. cpp:enumerator:: COLORSPACE_DIR_FROM_REFERENCE

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.ColorSpaceDirection

            Members:

            * COLORSPACE_DIR_UNKNOWN

            * COLORSPACE_DIR_TO_REFERENCE

            * COLORSPACE_DIR_FROM_REFERENCE

ViewTransformDirection 
======================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::ViewTransformDirection 

            *Values:*

            .. cpp:enumerator:: VIEWTRANSFORM_DIR_UNKNOWN = 0 

            .. cpp:enumerator:: VIEWTRANSFORM_DIR_TO_REFERENCE 

            .. cpp:enumerator:: VIEWTRANSFORM_DIR_FROM_REFERENCE 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.ViewTransformDirection

            Members:

            * VIEWTRANSFORM_DIR_UNKNOWN

            * VIEWTRANSFORM_DIR_TO_REFERENCE

            * VIEWTRANSFORM_DIR_FROM_REFERENCE

TransformDirection 
==================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::TransformDirection 

            *Values:*

            .. cpp:enumerator:: TRANSFORM_DIR_UNKNOWN = 0 

            .. cpp:enumerator:: TRANSFORM_DIR_FORWARD 

            .. cpp:enumerator:: TRANSFORM_DIR_INVERSE 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.TransformDirection

            Members:

            * TRANSFORM_DIR_UNKNOWN

            * TRANSFORM_DIR_FORWARD

            * TRANSFORM_DIR_INVERSE

Interpolation 
=============

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::Interpolation 

            Specify the interpolation type to use If the specified
            interpolation type is not supported in the requested context (for
            example, using tetrahedral interpolationon 1D LUTs) an exception
            will be thrown.

            INTERP_DEFAULT will choose the default interpolation type for the
            requested context:

            1D LUT INTERP_DEFAULT: LINEAR 3D LUT INTERP_DEFAULT: LINEAR

            INTERP_BEST will choose the best interpolation type for the
            requested context:

            1D LUT INTERP_BEST: LINEAR 3D LUT INTERP_BEST: TETRAHEDRAL

            Note: INTERP_BEST and INTERP_DEFAULT are subject to change in minor
            releases, so if you care about locking off on a specific
            interpolation type, we’d recommend directly specifying it.

            *Values:*

            .. cpp:enumerator:: INTERP_UNKNOWN = 0 

            .. cpp:enumerator:: INTERP_NEAREST = 1 

                nearest neighbor

            .. cpp:enumerator:: INTERP_LINEAR = 2 

                linear interpolation (trilinear for Lut3D)

            .. cpp:enumerator:: INTERP_TETRAHEDRAL = 3 

                tetrahedral interpolation (Lut3D only)

            .. cpp:enumerator:: INTERP_CUBIC = 4 

                cubic interpolation (not supported)

            .. cpp:enumerator:: INTERP_DEFAULT = 254 

                the default interpolation type

            .. cpp:enumerator:: INTERP_BEST = 255 

                the ‘best’ suitable interpolation type

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.Interpolation

            Members:

            * INTERP_UNKNOWN

            * INTERP_NEAREST

            * INTERP_LINEAR

            * INTERP_TETRAHEDRAL

            * INTERP_CUBIC

            * INTERP_DEFAULT

            * INTERP_BEST


BitDepth 
========

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::BitDepth 

            Used in a configuration file to indicate the bit-depth of a color
            space, and by the Processor to specify the input and output
            bit-depths of images to process. Note that Processor only supports:
            UINT8, UINT10, UINT12, UINT16, F16 and F32.

            *Values:*

            .. cpp:enumerator:: BIT_DEPTH_UNKNOWN = 0 

            .. cpp:enumerator:: BIT_DEPTH_UINT8 

            .. cpp:enumerator:: BIT_DEPTH_UINT10 

            .. cpp:enumerator:: BIT_DEPTH_UINT12 

            .. cpp:enumerator:: BIT_DEPTH_UINT14 

            .. cpp:enumerator:: BIT_DEPTH_UINT16 

            .. cpp:enumerator:: BIT_DEPTH_UINT32 

            .. cpp:enumerator:: BIT_DEPTH_F16 

            .. cpp:enumerator:: BIT_DEPTH_F32 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.BitDepth

            Members:

            * BIT_DEPTH_UNKNOWN

            * BIT_DEPTH_UINT8

            * BIT_DEPTH_UINT10

            * BIT_DEPTH_UINT12

            * BIT_DEPTH_UINT14

            * BIT_DEPTH_UINT16

            * BIT_DEPTH_UINT32

            * BIT_DEPTH_F16

            * BIT_DEPTH_F32

Lut1DHueAdjust 
==============

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::Lut1DHueAdjust 

            Used by :cpp:classLut1DTransform to control optional hue
            restoration algorithm.

            *Values:*

            .. cpp:enumerator:: HUE_NONE = 0 

            .. cpp:enumerator:: HUE_DW3 


    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.Lut1DHueAdjust

            Members:

            * HUE_NONE

            * HUE_DW3

ChannelOrdering 
===============

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::ChannelOrdering 

            Used by PackedImageDesc to indicate the channel ordering of the
            image to process.

            *Values:*

            .. cpp:enumerator:: CHANNEL_ORDERING_RGBA = 0 

            .. cpp:enumerator:: CHANNEL_ORDERING_BGRA 

            .. cpp:enumerator:: CHANNEL_ORDERING_ABGR 

            .. cpp:enumerator:: CHANNEL_ORDERING_RGB 

            .. cpp:enumerator:: CHANNEL_ORDERING_BGR 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.ChannelOrdering

            Members:

            * CHANNEL_ORDERING_RGBA

            * CHANNEL_ORDERING_BGRA

            * CHANNEL_ORDERING_ABGR

            * CHANNEL_ORDERING_RGB

            * CHANNEL_ORDERING_BGR

Allocation 
==========

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::Allocation 

            *Values:*

            .. cpp:enumerator:: ALLOCATION_UNKNOWN = 0 

            .. cpp:enumerator:: ALLOCATION_UNIFORM 

            .. cpp:enumerator:: ALLOCATION_LG2 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.Allocation

            Members:

            * ALLOCATION_UNKNOWN

            * ALLOCATION_UNIFORM

            * ALLOCATION_LG2

GpuLanguage 
===========

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::GpuLanguage 

            Used when there is a choice of hardware shader language.

            *Values:*

            .. cpp:enumerator:: GPU_LANGUAGE_UNKNOWN = 0 

            .. cpp:enumerator:: GPU_LANGUAGE_CG 

                Nvidia Cg shader.

            .. cpp:enumerator:: GPU_LANGUAGE_GLSL_1_0 

                OpenGL Shading Language.

            .. cpp:enumerator:: GPU_LANGUAGE_GLSL_1_3 

                OpenGL Shading Language.

            .. cpp:enumerator:: GPU_LANGUAGE_GLSL_4_0 

                OpenGL Shading Language.

            .. cpp:enumerator:: GPU_LANGUAGE_HLSL_DX11 

                DirectX Shading Language.

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.GpuLanguage

            Members:

            * GPU_LANGUAGE_UNKNOWN

            * GPU_LANGUAGE_CG

            * GPU_LANGUAGE_GLSL_1_0

            * GPU_LANGUAGE_GLSL_1_3

            * GPU_LANGUAGE_GLSL_4_0

            * GPU_LANGUAGE_HLSL_DX11

EnvironmentMode 
===============

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::EnvironmentMode 

            *Values:*

            .. cpp:enumerator:: ENV_ENVIRONMENT_UNKNOWN = 0 

            .. cpp:enumerator:: ENV_ENVIRONMENT_LOAD_PREDEFINED 

            .. cpp:enumerator:: ENV_ENVIRONMENT_LOAD_ALL 


    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.EnvironmentMode

            Members:

            * ENV_ENVIRONMENT_UNKNOWN

            * ENV_ENVIRONMENT_LOAD_PREDEFINED

            * ENV_ENVIRONMENT_LOAD_ALL

RangeStyle 
==========

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::RangeStyle 

            A RangeTransform may be set to clamp the values, or not.

            *Values:*

            .. cpp:enumerator:: RANGE_NO_CLAMP = 0 

            .. cpp:enumerator:: RANGE_CLAMP 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.RangeStyle

            Members:

            * RANGE_NO_CLAMP

            * RANGE_CLAMP

FixedFunctionStyle 
==================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::FixedFunctionStyle 

            Enumeration of the :cpp:class:FixedFunctionTransform transform
            algorithms.

            *Values:*

            .. cpp:enumerator:: FIXED_FUNCTION_ACES_RED_MOD_03 = 0 

                Red modifier (ACES 0.3/0.7)

            .. cpp:enumerator:: FIXED_FUNCTION_ACES_RED_MOD_10 

                Red modifier (ACES 1.0)

            .. cpp:enumerator:: FIXED_FUNCTION_ACES_GLOW_03 

                Glow function (ACES 0.3/0.7)

            .. cpp:enumerator:: FIXED_FUNCTION_ACES_GLOW_10 

                Glow function (ACES 1.0)

            .. cpp:enumerator:: FIXED_FUNCTION_ACES_DARK_TO_DIM_10 

                Dark to dim surround correction (ACES 1.0)

            .. cpp:enumerator:: FIXED_FUNCTION_REC2100_SURROUND 

                Rec.2100 surround correction (takes one double for the gamma
                param)

            .. cpp:enumerator:: FIXED_FUNCTION_RGB_TO_HSV 

                Classic RGB to HSV function.

            .. cpp:enumerator:: FIXED_FUNCTION_XYZ_TO_xyY 

                CIE XYZ to 1931 xy chromaticity coordinates.

            .. cpp:enumerator:: FIXED_FUNCTION_XYZ_TO_uvY 

                CIE XYZ to 1976 u’v’ chromaticity coordinates.

            .. cpp:enumerator:: FIXED_FUNCTION_XYZ_TO_LUV 

                CIE XYZ to 1976 CIELUV colour space (D65 white)

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.FixedFunctionStyle

            Members:

            * FIXED_FUNCTION_ACES_RED_MOD_03

            * FIXED_FUNCTION_ACES_RED_MOD_10

            * FIXED_FUNCTION_ACES_GLOW_03

            * FIXED_FUNCTION_ACES_GLOW_10

            * FIXED_FUNCTION_ACES_DARK_TO_DIM_10

            * FIXED_FUNCTION_REC2100_SURROUND

            * FIXED_FUNCTION_RGB_TO_HSV

            * FIXED_FUNCTION_XYZ_TO_xyY

            * FIXED_FUNCTION_XYZ_TO_uvY

            * FIXED_FUNCTION_XYZ_TO_LUV

ExposureContrastStyle 
=====================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::ExposureContrastStyle 

            Enumeration of the :cpp:class:ExposureContrastTransform
            transform algorithms.

            *Values:*

            .. cpp:enumerator:: EXPOSURE_CONTRAST_LINEAR = 0 

                E/C to be applied to a linear space image.

            .. cpp:enumerator:: EXPOSURE_CONTRAST_VIDEO 

                E/C to be applied to a video space image.

            .. cpp:enumerator:: EXPOSURE_CONTRAST_LOGARITHMIC 

                E/C to be applied to a log space image.

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.ExposureContrastStyle

            Members:

            * EXPOSURE_CONTRAST_LINEAR

            * EXPOSURE_CONTRAST_VIDEO

            * EXPOSURE_CONTRAST_LOGARITHMIC

CDLStyle 
========

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::CDLStyle 

            Enumeration of the :cpp:class:CDLTransform transform
            algorithms.

            **Note**
                The default for reading .cc/.ccc/.cdl files, config file YAML,
                and CDLTransform is no-clamp, since that is what is primarily
                desired in VFX. However, the CLF format default is ASC.

            *Values:*

            .. cpp:enumerator:: CDL_ASC = 0 

                ASC CDL specification v1.2.

            .. cpp:enumerator:: CDL_NO_CLAMP 

                CDL that does not clamp.

            .. cpp:enumerator:: CDL_TRANSFORM_DEFAULT = `CDL_NO_CLAMP`_ 

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.CDLStyle

            Members:

            * CDL_ASC

            * CDL_NO_CLAMP

            * CDL_TRANSFORM_DEFAULT

NegativeStyle
=============

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::NegativeStyle 

            Negative values handling style for ExponentTransform and
            ExponentWithLinearTransform transform algorithms.

            *Values:*

            .. cpp:enumerator:: NEGATIVE_CLAMP = 0 

                Clamp negative values.

            .. cpp:enumerator:: NEGATIVE_MIRROR 

                Positive curve is rotated 180 degrees around the origin to
                handle negatives.

            .. cpp:enumerator:: NEGATIVE_PASS_THRU 

                Negative values are passed through unchanged.

            .. cpp:enumerator:: NEGATIVE_LINEAR 

                Linearly extrapolate the curve for negative values.

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.NegativeStyle

            Members:

            * NEGATIVE_CLAMP

            * NEGATIVE_MIRROR

            * NEGATIVE_PASS_THRU

            * NEGATIVE_LINEAR

DynamicPropertyType 
===================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::DynamicPropertyType 

            *Values:*

            .. cpp:enumerator:: DYNAMIC_PROPERTY_EXPOSURE = 0 

                Image exposure value (double floating point value)

            .. cpp:enumerator:: DYNAMIC_PROPERTY_CONTRAST 

                Image contrast value (double floating point value)

            .. cpp:enumerator:: DYNAMIC_PROPERTY_GAMMA 

                Image gamma value (double floating point value)

    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.DynamicPropertyType

            Members:

            * DYNAMIC_PROPERTY_EXPOSURE

            * DYNAMIC_PROPERTY_CONTRAST

            * DYNAMIC_PROPERTY_GAMMA

OptimizationFlags 
=================

.. tabs::

    .. group-tab:: C++

        .. cpp:enum:: OpenColorIO::OptimizationFlags 

            Provides control over how the ops in a Processor are combined in
            order to improve performance.

            *Values:*

            .. cpp:enumerator:: OPTIMIZATION_NONE = 0x00000000 

                Do not optimize.

            .. cpp:enumerator:: OPTIMIZATION_IDENTITY = 0x00000001 

                Replace identity ops (other than gamma).

            .. cpp:enumerator:: OPTIMIZATION_IDENTITY_GAMMA = 0x00000002 

                Replace identity gamma ops.

            .. cpp:enumerator:: OPTIMIZATION_PAIR_IDENTITY_CDL = 0x00000004 

                Replace a pair of ops where one is the inverse of the other.

            .. cpp:enumerator:: OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST =
            0x00000008 

            .. cpp:enumerator:: OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION = 0x00000010
            

            .. cpp:enumerator:: OPTIMIZATION_PAIR_IDENTITY_GAMMA = 0x00000020 

            .. cpp:enumerator:: OPTIMIZATION_PAIR_IDENTITY_LUT1D = 0x00000040 

            .. cpp:enumerator:: OPTIMIZATION_PAIR_IDENTITY_LUT3D = 0x00000080 

            .. cpp:enumerator:: OPTIMIZATION_PAIR_IDENTITY_LOG = 0x00000100 

            .. cpp:enumerator:: OPTIMIZATION_COMP_EXPONENT = 0x00000200 

                Compose a pair of ops into a single op.

            .. cpp:enumerator:: OPTIMIZATION_COMP_GAMMA = 0x00000400 

            .. cpp:enumerator:: OPTIMIZATION_COMP_MATRIX = 0x00000800 

            .. cpp:enumerator:: OPTIMIZATION_COMP_LUT1D = 0x00001000 

            .. cpp:enumerator:: OPTIMIZATION_COMP_LUT3D = 0x00002000 

            .. cpp:enumerator:: OPTIMIZATION_COMP_RANGE = 0x00004000 

            .. cpp:enumerator:: OPTIMIZATION_COMP_SEPARABLE_PREFIX = 0x00008000 

                For integer and half bit-depths only, replace separable ops
                (i.e. no channel crosstalk ops) by a single 1D LUT of input
                bit-depth domain.

            .. cpp:enumerator:: OPTIMIZATION_LUT_INV_FAST = 0x00010000 

                Implement inverse Lut1D and Lut3D evaluations using a a forward
                LUT (faster but less accurate). Note that GPU evals always do
                FAST.

            .. cpp:enumerator:: OPTIMIZATION_NO_DYNAMIC_PROPERTIES = 0x00020000 

                Turn off dynamic control of any ops that offer adjustment of
                parameter values after finalization (e.g. ExposureContrast).

            .. cpp:enumerator:: OPTIMIZATION_ALL = 0xFFFFFFFF 

                Apply all possible optimizations.

            .. cpp:enumerator:: OPTIMIZATION_LOSSLESS = (`OPTIMIZATION_IDENTITY`_ | `OPTIMIZATION_IDENTITY_GAMMA`_ | `OPTIMIZATION_PAIR_IDENTITY_CDL`_ | `OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST`_ | `OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION`_ | `OPTIMIZATION_PAIR_IDENTITY_GAMMA`_ | `OPTIMIZATION_PAIR_IDENTITY_LOG`_ | `OPTIMIZATION_PAIR_IDENTITY_LUT1D`_ | `OPTIMIZATION_PAIR_IDENTITY_LUT3D`_ | `OPTIMIZATION_COMP_EXPONENT`_ | `OPTIMIZATION_COMP_GAMMA`_ | `OPTIMIZATION_COMP_MATRIX`_ | `OPTIMIZATION_COMP_RANGE`_) 

            .. cpp:enumerator:: OPTIMIZATION_VERY_GOOD = (`OPTIMIZATION_LOSSLESS`_ | `OPTIMIZATION_COMP_LUT1D`_ | `OPTIMIZATION_LUT_INV_FAST`_ | `OPTIMIZATION_COMP_SEPARABLE_PREFIX`_) 

            .. cpp:enumerator:: OPTIMIZATION_GOOD = `OPTIMIZATION_VERY_GOOD`_ | `OPTIMIZATION_COMP_LUT3D`_ 

            .. cpp:enumerator:: OPTIMIZATION_DRAFT = `OPTIMIZATION_ALL`_ 

                For quite lossy optimizations.

            .. cpp:enumerator:: OPTIMIZATION_DEFAULT = `OPTIMIZATION_VERY_GOOD`_ 


    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.OptimizationFlags

            Members:

            * OPTIMIZATION_NONE

            * OPTIMIZATION_IDENTITY

            * OPTIMIZATION_IDENTITY_GAMMA

            * OPTIMIZATION_PAIR_IDENTITY_CDL

            * OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST

            * OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION

            * OPTIMIZATION_PAIR_IDENTITY_GAMMA

            * OPTIMIZATION_PAIR_IDENTITY_LUT1D

            * OPTIMIZATION_PAIR_IDENTITY_LUT3D

            * OPTIMIZATION_PAIR_IDENTITY_LOG

            * OPTIMIZATION_COMP_EXPONENT

            * OPTIMIZATION_COMP_GAMMA

            * OPTIMIZATION_COMP_MATRIX

            * OPTIMIZATION_COMP_LUT1D

            * OPTIMIZATION_COMP_LUT3D

            * OPTIMIZATION_COMP_RANGE

            * OPTIMIZATION_COMP_SEPARABLE_PREFIX

            * OPTIMIZATION_LUT_INV_FAST

            * OPTIMIZATION_NO_DYNAMIC_PROPERTIES

            * OPTIMIZATION_ALL

            * OPTIMIZATION_LOSSLESS

            * OPTIMIZATION_VERY_GOOD

            * OPTIMIZATION_GOOD

            * OPTIMIZATION_DRAFT

            * OPTIMIZATION_DEFAULT
