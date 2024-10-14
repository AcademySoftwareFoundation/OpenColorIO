..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


OCIO 2.4 Release
================

Timeline
********

OpenColorIO 2.4 was delivered in September 2024 and is in the VFX Reference Platform for
calendar year 2025.


New Feature Guide
=================

ACES 2.0 Output Transforms (PREVIEW RELEASE)
********************************************

The Academy Color Encoding System has released a new major version with a completely new
set of Output Transforms (which are the transforms that convert from ACES2065-1 to a display
color space). These transforms are much more sophisticated than ACES 1 and include a
sophisticated gamut mapping algorithm that takes advantage of recent work in color appearance
modeling. The invertibility of the transforms is also improved.

**ACES 2 support in OCIO 2.4.0 is labelled a "Preview Release" to indicate that this is a 
work-in-progress that is not yet ready for production use. Both the processing results and
the API for it will likely change in OCIO 2.4.1 and future releases.**

The OCIO config files for ACES 2 are still under development, but prototype configs for
testing may be downloaded from `this PR on the OpenColorIO-Config-ACES repo. 
<https://github.com/AcademySoftwareFoundation/OpenColorIO-Config-ACES/pull/130>`_

A preliminary technical description of the ACES 2 Output Transform algorithm may be found 
`at this link. <https://draftdocs.acescentral.com/output-transforms/technical-details/>`_


Built-in Configs
****************

An updated set of built-in CG and Studio configs is included in the 2.4 release. These include 
the updates from the OCIO Configs Working Group over the past year. The updates include:

* Updated texture asset color space names, following the work of the ASWF Color Interop Forum.
  (The previous names for all active color spaces have been preserved via an alias.)
* Making the display color spaces active (allowing for easier display-to-display conversions).

Additional updates for the Studio config include:

* Addition of the Apple Log camera color space

Given that the ACES 2 support is still a work-in-progress, the built-in configs in 2.4.0
are based on ACES 1.3. Versions for ACES 2 are under development and are planned for the
OCIO 2.4.1 release.

For Users
+++++++++

The following URI strings may be provided anywhere you would normally provide a file path
to a config (e.g. as the OCIO environment variable):

To use the updated :ref:`aces_cg`, use this string for the config path:
    ocio://cg-config-v2.2.0_aces-v1.3_ocio-v2.4

To use the updated :ref:`aces_studio`, use this string for the config path:
    ocio://studio-config-v2.2.0_aces-v1.3_ocio-v2.4

This string will give you the current default config, which is the latest ACES CG Config:
    ocio://default

This string now points to this latest CG config:
    ocio://cg-config-latest

This string now points to this latest Studio config:
    ocio://studio-config-latest


Tool Enhancements
*****************

For Users
+++++++++

* There are now Python Wheels available for Python 3.13 (Python 3.7 support will be removed soon).

* There is a ``bitdepth`` argument to ``ocioconvert`` to control the processing and writing bit-depth.

* There is a ``ociocpuinfo`` utility that prints info about the CPU in your computer, especially
  which SIMD intrinsics it supports. As with the other command-line tools, this is available as part
  of an OCIO install when building from source or via PyPI.


API Enhancements
****************

For Developers
++++++++++++++

You may find the following additions to the OCIO API useful:

* There is now a ``hasAlias`` method on the ColorSpace and NamedTransform classes.

* The FileTransform class has a new static method ``IsFormatExtensionSupported`` that
  checks if a specific LUT format file extension is supported by OpenColorIO.


New Fixed Function Transforms
*****************************

For Config Authors
++++++++++++++++++

The following new styles are available for use with the FixedFunctionTransform:

* ``FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20`` -- This implements the part of the ACES 2.0 
  Output Transform that converts from ACES2065-1 to the intended linear values in the limiting
  primaries. (This requires further steps to get to the intended display colorimetry, typically
  a clamp, scale, and matrix.) It takes 9 parameters: peak_luminance (in nits), followed by 
  the chromaticity coordinates for the limiting gamut: [ red_x, red_y, green_x, green_y, blue_x,
  blue_y, white_x, white_y ]. **This is a work-in-progress and may change or be removed in 
  future releases.**

* ``FIXED_FUNCTION_ACES_RGB_TO_JMH_20`` -- This implements the conversion from a linear RGB
  space into the JMh (lightness, colorfulness, hue) color appearance model space that is used
  in the ACES 2 Output Transform algorithm. It takes 8 parameters which are the chromaticity
  coordinates of the linear RGB color space: [ red_x, red_y, green_x, green_y, blue_x,
  blue_y, white_x, white_y ]. **This is a work-in-progress and may change or be removed in 
  future releases.**

* ``FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20`` -- This implements the part of the ACES 2
  Output Transform algorithm that modify the tonescale and chroma of the image when mapping from
  the scene-referred to display-referred image state. It takes one parameter: the peak_luminance
  of the target display device (in nits). **This is a work-in-progress and may change or be 
  removed in future releases.**

* ``FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20`` -- This implements the part of the ACES 2 Output
  Transform algorithm that applies gamut compression to the target display gamut. It takes
  the same 9 parameters as ``FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20``. **This is a work-in-progress 
  and may change or be removed in future releases.**

* ``FIXED_FUNCTION_LIN_TO_PQ`` -- This is the curve for SMPTE ST-2084 (PQ). It is scaled with 100 nits 
  at 1.0. Negative values are mirrored around the origin. It takes no parameters. Note that this
  implementation is slower (especially on the CPU) than the Built-in Transform that implements
  the same function via a LUT1D rather than via a formula: ``CURVE - LINEAR_to_ST-2084``. The
  only advantage of the Fixed Function is that it does not use texture memory on the GPU.

* ``FIXED_FUNCTION_LIN_TO_GAMMA_LOG`` -- This curve contains a parametrized gamma and log segment.
  It takes 10 parameters and is capable of implementing both the Apple Log curve and the OETF for
  Hybrid Log Gamma (HLG). The parameters are: 
  [ mirrorPt, breakPt, gammaSeg_power, gammaSeg_slope, gammaSeg_off, logSeg_base, logSeg_logSlope,
  logSeg_logOff, logSeg_linSlope, logSeg_linOff ].
  
  The exact formula may be found in the renderer code in `ops/fixedfunction/FixedFunctionOpCPU.cpp
  <https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/2f5ae2c568a4c1fb87e548716e2a12ac9e74d861/src/OpenColorIO/ops/fixedfunction/FixedFunctionOpCPU.cpp#L1908>`_
  
  For implementing the Apple Log or HLG OETF curves, it is recommended to use the Built-in Transforms
  ``CURVE - APPLE_LOG_to_LINEAR`` and ``CURVE - HLG-OETF`` as they are faster (especially on the CPU)
  since they are implemented via a LUT1D rather than a formula.

* ``FIXED_FUNCTION_LIN_TO_DOUBLE_LOG`` -- This curve contains two parameterized LogAffineTransforms 
  with an optional middle linear segment. It takes 13 parameters and is capable of implementing the
  Canon Log 2 and Canon Log 3 curves. The parameters are: 
  [ base, break1, break2, logSeg1_logSlope, logSeg1_logOff, logSeg1_linSlope, logSeg1_linOff
  logSeg2_logSlope, logSeg2_logOff, logSeg2_linSlope, logSeg2_linOff, linSeg_slope, linSeg_off ]
  
  The exact formula may be found in the renderer code in `ops/fixedfunction/FixedFunctionOpCPU.cpp
  <https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/2f5ae2c568a4c1fb87e548716e2a12ac9e74d861/src/OpenColorIO/ops/fixedfunction/FixedFunctionOpCPU.cpp#L2006>`_
  
  For implementing the Canon Log 2 or 3 curves, it is recommended to use the Built-in Transforms
  ``CURVE - CANON_CLOG2_to_LINEAR`` and ``CURVE - CANON_CLOG3_to_LINEAR`` as they are faster
  (especially on the CPU) since they are implemented via a LUT1D rather than a formula.


New Built-in Transforms
***********************

For Config Authors
++++++++++++++++++

In config files with ``ocio_profile_version`` set to 2.4 or higher, config authors may take
advantage of the following new BuiltinTransform styles:

* ``APPLE_LOG_to_ACES2065-1`` -- Converts Apple Log to ACES2065-1.

* ``CURVE - APPLE_LOG_to_LINEAR`` -- Applies only the EOTF curve for Apple Log.

* ``CURVE - HLG-OETF`` -- Applies the OETF curve for Hybrid Log Gamma.

* ``CURVE - HLG-OETF-INVERSE`` -- Applies the inverse-OETF curve for Hybrid Log Gamma.

* ``DISPLAY - CIE-XYZ-D65_to_DCDM-D65`` -- Implements a display color space to produce a
  Digital Cinema Distribution Master (DCDM) with a D65 creative white point. This uses the
  traditional 2.6 gamma SDR transfer function.

* ``DISPLAY - CIE-XYZ-D65_to_ST2084-DCDM-D65`` -- Implements a display color space to produce an
  HDR Digital Cinema Distribution Master (DCDM) with a D65 creative white point. This implements
  the ST-2084 transfer function for HDR with an input Y value of 1.0 mapping to 100 nits.

The following implement various View Transforms used by the ACES 2 Output Transforms:

* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-REC709_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-108nit-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-300nit-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-500nit-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-1000nit-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-2000nit-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-500nit-REC2020_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-1000nit-REC2020_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-2000nit-REC2020_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-REC2020_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-REC709-D60-in-REC709-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-REC709-D60-in-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-REC709-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-P3-D60-in-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-P3-D60-in-XYZ-E_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-108nit-P3-D60-in-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-300nit-P3-D60-in-XYZ-E_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-500nit-P3-D60-in-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-1000nit-P3-D60-in-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-2000nit-P3-D60-in-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D60-in-P3-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-500nit-P3-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-1000nit-P3-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-2000nit-P3-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-500nit-REC2020-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-1000nit-REC2020-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-2000nit-REC2020-D60-in-REC2020-D65_2.0`` 
* ``ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-REC2020-D60-in-REC2020-D65_2.0`` 


Release Notes
=============

For additional details, please see the GitHub release page:

`OCIO 2.4.0 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.4.0>`_
