..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


OCIO 2.5 Release
================

Timeline
********

OpenColorIO 2.5 was delivered in September 2025 and is in the VFX Reference Platform for
calendar year 2026.


Breaking Changes
****************

Please be aware of the following changes when upgrading to OCIO 2.5.

For Users
+++++++++

* OCIOZ archive files created on Windows in prior releases should be regenerated due to an
  issue in a third-party library. Please see below for more details. 

* For applications that use the categories attribute of color spaces to filter the color
  space menus shown to users, the filtering will now include color spaces that do not have
  any category set. (Though applications may use a new API call to override this behavior.)

For Developers
++++++++++++++

* Applications that use the OCIO GPU renderer may need to make small adjustments to their
  code due to changes that were required in order to support the Khronos Vulkan graphics API.

* The Python functions getActiveDisplays and getActiveViews now return iterators, please 
  see below for details.

* Please see the section below about version updates to required third-party dependencies.


New Feature Guide
=================

Config Merging (PREVIEW RELEASE)
********************************

Config files may now be merged. This was a long-standing feature request and it opens up
interesting new possibilities for how configs are created, managed, and deployed. Because 
the workflow ramifications of this feature are far-reaching, this is being labelled a 
Preview Release and therefore the API and behavior may change as a result of more feedback 
and testing.

Five different merge strategy options are provided, along with an extensive set of parameters
to customize the process. The new feature handles the complexities inherent in the very 
flexible OCIO config format such as:

* Handling name or alias conflicts.

* Handling differences in reference connection spaces by adding compensating color space 
  conversions to transforms.

* Avoiding adding duplicate color spaces (even if they are named are differently). This 
  allows applications to avoid adding color spaces that may already be present in the user's
  config.

* Providing a mechanism to group added color spaces under a separate menu hierarchy.

For Users
+++++++++

End-users may use the new ``ociomergeconfigs`` command-line tool to merge configs. A new
OCIOM file format is provided to set the parameters that control how the merge is done.
For example, you may merge a specific set of color spaces or displays and views that are
needed for a given project into a general-purpose base config.

For Developers
++++++++++++++

Developers may use the new API to merge a set of color spaces that an application requires
into the user's config. (This is typically done on the fly, in memory, to avoid modifying
any config files on disk.) Applications may then publish their required color spaces as a
config file which makes the process transparent to color scientists or pipeline experts at
a facility who may use ``ociomergeconfigs`` to merge the application color spaces into their 
own configs and validate everything works as intended.


Built-in ACES 2.0 Configs
*************************

The ACES 2.0 library support was finalized in the OCIO 2.4.2 release. In the 2.5.0 release,
built-in ACES 2.0 versions of the Studio and CG config are now provided. This allows use of
the ACES 2.0 Output Transforms as OCIO views.

This `ASWF Open Source Days presentation <https://youtu.be/VZti3UztRE4?feature=shared>`_ 
from ILM's Alex Fry provides a great overview of the benefits of the new configs.

In addition, the new configs make expanded use of OCIO v2 features such as File Rules and
Viewing Rules and add a virtual display to support use with ICC monitor profiles.

Please take note that the display color spaces in the new configs pass through negative
values rather than clamping them (by mirroring the transfer function around the origin).
This provides better support for video workflows that require values below reference black
to be passed through.

Finally, there are a few other color space additions including a Gamma 2.2 Rec.709 display 
color space and a color space for DJI Log.

For Users
+++++++++

The following URI strings may be provided anywhere you would normally provide a file path
to a config (e.g. as the OCIO environment variable):

To use the updated :ref:`aces_cg`, use this string for the config path:
    ocio://cg-config-v4.0.0_aces-v2.0_ocio-v2.5

To use the updated :ref:`aces_studio`, use this string for the config path:
    ocio://studio-config-v4.0.0_aces-v2.0_ocio-v2.5

This string will give you the current default config, which is the latest ACES 2 CG Config:
    ocio://default

This string now points to this latest ACES 2 CG config:
    ocio://cg-config-latest

This string now points to this latest ACES 2 Studio config:
    ocio://studio-config-latest


Hue Curve Transform
*******************

A new ``GradingHueCurveTransform`` is added to allow a type of selective color correction
often available in grading tools. A set of eight spline-based curve adjustments are provided:

* Hue-Hue: Map input hue to output hue (where a diagonal line is the identity).
* Hue-Sat: Adjust saturation as a function of hue (a value of 1.0 is the identity).
* Hue-Lum: Adjust luma as a function of hue (a value of 1.0 is the identity).
* Lum-Sat: Adjust saturation as a function of luma (a value of 1.0 is the identity).
* Sat-Sat: Adjust saturation as a function of saturation (a diagonal is the identity).
* Lum-Lum: Adjust luma as a function of luma, maintaining hue & sat (diagonal is identity).
* Sat-Lum: Adjust luma as a function of saturation (a value of 1.0 is the identity).
* Hue-FX : Map input hue to delta output hue (a value of 0.0 is the identity).

Unlike the typical implementation of tools of this type, this transform is invertible and
has been developed to work well on video, log, and scene-linear color encodings.

The parameters may be adjusted via OCIO's dynamic parameter mechanism which allows 
adjustments to be made without having to recreate a Processor object. This facilitates
real-time adjustment in applications.


Vulkan Support
**************

For Developers
++++++++++++++

Support has been added for the Khronos Vulkan graphics API standard. The OCIO GPU renderer may
now be used within applications that use Vulkan to get the most out of the latest GPUs.

This has required a small modification to the existing GPU API. Developers that use the
GPU interface may need to modify their code when updating to OCIO 2.5. The API for the 
following functions have been modified: ``addUniform``, ``addTexture``, ``add3DTexture``,
and ``createShaderText``. The ``addToDeclareShaderCode`` function has been replaced by the
more specific ``addToParameterDeclareShaderCode`` and ``addToTextureDeclareShaderCode``.
In addition new functions have been added to get the descriptor set and texture binding 
indices. The ``GPU_LANGUAGE_GLSL_VK_4_6`` option is now available for the GpuLanguage enum.


New Color Space Attributes
**************************

For Config Authors
++++++++++++++++++

New attributes are provided to allow OCIO to better interoperate with other color standards.
OCIO now supports the Color Interop ID developed by the Color Interop Forum. This allows
config authors to set an ID on the color spaces in a config to unambiguously identify them
as conforming to the recommendations of the ASWF Color Interop Forum. These IDs may then be
used in file formats including OpenEXR and OpenUSD. (Please note that the IDs are for use in
file formats but the color space's name attribute is still what should be used in a UI.)

The new attributes are:

* ``interop_id``: Holds an ID string intended to be used across configs and file formats.

* ``icc_profile_name``: Holds the name of an associated ICC profile.

* ``amf_transform_ids``: Holds the ID strings associated with the ACES Metadata Format (AMF).

Please note that the drafts of various Color Interop Forum recommendations on this topic are
currently being finalized and should be `published on GitHub <https://github.com/AcademySoftwareFoundation/ColorInterop>`_ 
over the next few months.


New "edr-video" Encoding
************************

For Config Authors
++++++++++++++++++

Color spaces currently have an encoding attribute that is used to give applications a general
sense of how colors are distributed within the numerical encoding. There are currently two
encodings that are recommended for use with video: `sdr-video` and `hdr-video`, where the 
latter is intended for color spaces such as Rec.2100-PQ and Rec.2100-HLG. However, there are
increasingly a set of color spaces (such as "Display P3 HDR" in the built-in configs) that
use what is essentially an SDR encoding that is extended so that it may be used in cases
where there is a mix of both HDR and SDR content. For these color spaces, a new encoding is
being introduced: `edr-video`.


Version Updates
***************

The latest versions of all third-party dependencies have been reviewed for compatibility.
In some cases, support for older versions has been dropped.

For Users
+++++++++

* Support for minizip-ng 3.x has been dropped. Due to a bug in previous versions of that
  library, any OCIOZ files created on Windows would not have been cross-platform, are not
  fully supported in OCIO 2.5, and should be regenerated. You may use standard Zip 
  decompressors to expand the .ocioz files on Windows and use the `ocioarchive` command-line 
  tool to recompress them. The new files will be cross-platform and will be compatible with
  both OCIO 2.5 and previous releases.

For Developers
++++++++++++++

* Support for CMake 4 has been added.
* Support for more recent versions of all dependences has been added.
* Support for C++ 11 and 14 has been dropped. OCIO now requires at least a C++ 17 compiler.
* Support for Python 3.8 has been dropped.
* Support for Expat prior to 2.6.0 has been dropped.
* Support for OpenEXR prior to 3.2.0 has been dropped.
* Support for Yaml-cpp prior to 0.8.0 has been dropped.
* Continuous Integration testing has added VFX Platform 2025 and dropped 2021.
* Miscellaneous other modifications and improvements have been made.


API Enhancements
****************

For Developers
++++++++++++++

The previous "frozen docs" system for generating Python API documentation for ReadTheDocs
has been replaced. This reduces the burden on developers when adding new API functions and
simplifies the maintenance of the project.

The previous API for the active display and view lists returned one long string that 
developers would need to manually parse into tokens, which could be error-prone (as commas 
are both separators and may be used in display and view names). A new set of API calls has
been provided to get and set active displays and views individually. In the case of Python,
the ``getActiveDisplays`` and ``getActiveViews`` functions now return iterators, which is 
the only change that requires a modification to existing code.

In addition, here are some other new API functions:

* The ``setDisplayTemporary`` function may be used to serialize displays created using an ICC 
  profile and a config's virtual display. This is helpful if a host application needs to serialize
  a config that includes those displays for use by plug-ins such as OpenFX.

* The ``setTreatNoCategoryAsAny`` function may be used on the ColorSpaceMenuParameters class
  to control the menu filtering behavior for color spaces that do not have the categories 
  attribute set. The default is to include those color spaces when doing category filtering 
  since this is the least surprising behavior for end-users that are editing their own configs.

* The following convenience methods have been added to the Config class for working with various
  types of views: ``isViewShared``, ``clearSharedViews``, ``AreViewsEqual``, ``hasView``, 
  ``hasVirtualView``, ``isVirtualViewShared``, ``AreVirtualViewsEqual``.

* The ``setStringVar`` function is now available on the Context class in Python.

* A ``removeNamedTransform`` function is now available.

* The previously deprecated function ``getDefaultBuiltinConfigName`` has now been removed.
  Please use ``ResolveConfigPath("ocio://default")`` instead.


New Fixed Function Transforms
*****************************

For Config Authors
++++++++++++++++++

The following new styles are available for use with FixedFunctionTransforms in config files
with ``ocio_profile_version`` set to 2.5 or higher. They implement a conversion from an RGB
color space encoding to a hue, saturation, and luma space that is used by the new hue curve
transform. Three conversions are provided that are optimized for scene-linear, logarithmic,
and video color spaces:

* ``FIXED_FUNCTION_RGB_TO_HSY_LIN``
* ``FIXED_FUNCTION_RGB_TO_HSY_LOG``
* ``FIXED_FUNCTION_RGB_TO_HSY_VID``


New Built-in Transforms
***********************

For Config Authors
++++++++++++++++++

In config files with ``ocio_profile_version`` set to 2.5 or higher, config authors may take
advantage of the following new BuiltinTransform styles that mirror the transfer function
around the origin to pass negative values rather than clamp them:

* ``DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.709 - MIRROR NEGS``
* ``DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.2020 - MIRROR NEGS``
* ``DISPLAY - CIE-XYZ-D65_to_G2.2-REC.709 - MIRROR NEGS``
* ``DISPLAY - CIE-XYZ-D65_to_sRGB - MIRROR NEGS``
* ``DISPLAY - CIE-XYZ-D65_to_G2.6-P3-D65 - MIRROR NEGS``


Release Notes
=============

For additional details, please see the GitHub release page:

`OCIO 2.5.0 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.5.0>`_
