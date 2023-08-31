..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


OCIO 2.1 Release
================

Timeline
********

OpenColorIO 2.1 was delivered in August 2021 and is in the VFX Reference Platform for
calendar year 2022.


New Feature Guide
=================

ACES 1.3 Gamut Compression
**************************

ACES 1.3 introduced a scene-referred gamut compression algorithm that compresses
colors captured by a camera into the AP1 gamut.  This avoids artifacts that may
occur downstream (such as in the ACES Output Transforms) for scene elements such 
as colored lights.  This supersedes the earlier "Blue Light Artifact Fix" LMT.

`ACES RGC User Guide 
<https://www.dropbox.com/scl/fi/acuu6e626s14zlitpu2w8/ACES-1.3-Reference-Gamut-Compression-User-Guide.paper?dl=0&rlkey=bbbhneq62f6r85cptpcywu08n>`_

`ACES RGC Implementation Guide 
<https://www.dropbox.com/scl/fi/cousf6tispmegu4fpmpr2/ACES-Gamut-Compression-Implementation-Guide.paper?dl=0&rlkey=ul38rayi7imiiy4wjo3cektb2>`_

The implementation includes a BuiltinTransform for the ACES Reference Gamut Compression.
Here's what that looks like in Config YAML:: 

    !<BuiltinTransform> {style: ACES-LMT - ACES 1.3 Reference Gamut Compression}

There is also a FixedFunctionTransform for the underlying gamut compression algorithm.
As described in the links above, it takes seven parameter values:: 

    [ Limit_Cyan, Limit_Magenta, Limit_Yellow, Threshold_Cyan, Threshold_Magenta, Threshold_Yellow, Roll-off ].

Here is how that looks in Config YAML with the parameters set for the ACES 1.3 Reference Gamut
Compression::

    !<FixedFunctionTransform> {style: ACES_GamutComp13, params: [1.147, 1.264, 1.312, 0.815, 0.803, 0.88, 1.2]}

And here's how it looks in a CTF file::

    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="GamutComp13Fwd" params="1.147 1.264 1.312 0.815 0.803 0.88 1.2" />

**Config authors**: These new transforms may be used in configs with version 2.1 or higher 
and CTF files with version 2.1 or higher.


OpenFX OCIO plug-ins
********************

A framework to support OpenFX plug-ins has been added and example source code may be found 
in the vendor/openfx directory.  The initial set consists of two plug-ins, one for applying
a ColorSpaceTransform and another for applying a DisplayViewTransform.

Some screenshots of the UI may be found in `PR #1371. 
<https://github.com/AcademySoftwareFoundation/OpenColorIO/pull/1371>`_

**End users**: There are currently no pre-built executables available, so you will need to
compile these yourself.  

**Developers**: Set the CMake variable ``-D OCIO_BUILD_OPENFX=ON`` to build the plug-ins.


Support for PyPI (pip install)
******************************

Python wheel generation has been added and support for the `Python Package Index (PyPI). 
<https://pypi.org/project/opencolorio/>`_
This allows you to easily install the OCIO Python bindings without needing to build from
source or use one of the OS-specific package managers.  The command is simply::

    pip install opencolorio


Support for emitting Open Shading Language (OSL)
************************************************

The OCIO GPU Renderer may now emit shaders in `Open Shading Language 
<https://github.com/AcademySoftwareFoundation/OpenShadingLanguage>`_ format.  There is a 
new GpuLanguage enum: ``LANGUAGE_OSL_1``.  

Note that the OCIO library does not need to be compiled against the OSL library but running 
the OSL unit tests does require OSL and OIIO.  These will be built automatically if CMake is 
able to find them or you set ``-D OSL_ROOT`` to indicate the installed location.

Currently, Lut1D and Lut3D Transforms are not supported and will throw an exception if used.
All other transforms are supported.  However, dynamic properties are currently locked to 
their initial values and may not be adjusted.


Support for Apple Metal Shading Language and OpenGL ES
******************************************************

The OCIO GPU Renderer may now emit shaders in these other new languages.  The new 
GpuLanguage enums are: 

* ``GPU_LANGUAGE_MSL_2_0``
* ``GPU_LANGUAGE_GLSL_ES_3_0``
* ``GPU_LANGUAGE_GLSL_ES_1_0``


Test suite for the Academy/ASC Common LUT Format (CLF)
******************************************************

The set of CLF test files was expanded and a set of Python scripts was added under 
``share/clf`` which were developed by the ACES Common LUT Format Implementation Working Group.
The scripts allow use of the OpenColorIO renderers to process a sample image through each
CLF file in the test suite and then compare the reference images to a set of corresponding
images from another CLF implementation to see if the differences are within the tolerance
allowed by the `CLF Implementation Guide. <https://docs.acescentral.com/guides/clf/>`_


Imath 3 support
***************

OCIO and its CI system now support Imath 3 for the Half data type dependency.


Noteworthy API updates
**********************

getColorSpaceFromFilePath
+++++++++++++++++++++++++

``Config::getColorSpaceFromFilepath()`` is now the proper way to extract a color space from
a file path, regardless of whether the config is version 1, or version 2 or higher.  It 
works regardless of whether the config has FileRules defined.  Therefore, the older
``Config::parseColorSpaceFromString()`` method (which does not work with FileRules) is 
officially deprecated.

Note that ``Config::getColorSpaceFromFilepath()`` always returns a color space, regardless
of whether the config's ``strictparsing`` attribute is true or false.   This was a request from 
app developers that always need a default (which is a common scenario).  Please see 
`Issue #1398 <https://github.com/AcademySoftwareFoundation/OpenColorIO/issues/1398>`_ 
for more details on how to take special action if ``strictparsing`` is true.


getDefaultView
++++++++++++++
There is a new ``Config::getDefaultView(display, colorspaceName)`` method that takes
advantage of the ViewingRules feature that was introduced in OCIO 2.0.  It returns the
default view that is most appropriate for a given color space.  If the color space is
known, developers should call this rather than ``Config::getDefaultView(display)``.

The motivation is that the best view transform to use varies with the color space.  For
example, a scene-referred color space typically wants some kind of tone-map such as an
ACES Output Transform.  However, a display-referred color space will look wrong with such
a view transform since it has already been tone-mapped.

One scenario where this new method is ideal is when an application needs to generate 
thumbnails for a large number of images that are in a variety of color spaces.  Note that
the config must have the Viewing Rules set in order to return different views, but if the
rules are not set, it will still return the same result as ``Config::getDefaultView(display)``,
so it may be used with either v1 or v2 configs.


Release Notes
=============

For more detail, please see the GitHub release pages:

`OCIO 2.1.0 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.1.0>`_

`OCIO 2.1.1 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.1.1>`_

`OCIO 2.1.2 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.1.2>`_

`OCIO 2.1.3 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.1.3>`_
