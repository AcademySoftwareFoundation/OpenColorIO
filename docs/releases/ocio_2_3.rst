..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


OCIO 2.3 Release
================

Timeline
********

OpenColorIO 2.3 was delivered in August 2023 and is in the VFX Reference Platform for
calendar year 2024.


New Feature Guide
=================

New config editor GUI app ``ocioview``
**************************************

There is a new GUI app for inspecting and editing OCIO configs called ``ocioview``.  As of
the 2.3.0 release, ``ocioview`` is currently in an alpha release (please see the README in
the apps/ocioview directory for details).  Documentation and tutorials will be forthcoming
and we are looking for contributors to help with this and other tasks.


Built-in Configs
****************

An updated set of built-in CG and Studio configs is included in the 2.3 release. These include 
the updates from the OCIO Configs Working Group over the past year. The updates include:

* Addition of Apple's Display P3 as both a texture space and a display
* A ``texture`` category to allow filtering of color spaces typically used for textures
* More appropriate color space assignments for the ``matte_paint`` and ``texture_paint`` roles

Additional updates for the Studio config include:

* Support for CanonLog2 CinemaGamut color space (in addition to the existing CanonLog3)
* Addition of ACES Transform IDs to better support the ACES Metadata Format (AMF)

For Users
+++++++++

The following URI strings may be provided anywhere you would normally provide a file path
to a config (e.g. as the OCIO environment variable):

To use the updated :ref:`aces_cg`, use this string for the config path::
    ocio://cg-config-v2.1.0_aces-v1.3_ocio-v2.3

To use the updated :ref:`aces_studio`, use this string for the config path::
    ocio://studio-config-v2.1.0_aces-v1.3_ocio-v2.3

This string will give you the current default config, which is the latest ACES CG Config::
    ocio://default

Because there are now several built-in configs, there is now a short-cut to referring to
the latest versions:

This string will always give you the latest CG config::
    ocio://cg-config-latest

This string will always give you the latest Studio config::
    ocio://studio-config-latest

For Developers
++++++++++++++

As the built-in config collection evolves, special names such as ``ocio://default`` and 
``ocio://studio-config-latest`` will point to newer versions of those configs. Therefore, it is 
recommended that application developers not save those strings and instead save the string that 
refers to the current version of that config. That way, it's guaranteed that there will be no 
change of behavior in the future. For example, as of OCIO 2.3, ``ocio://default`` should be saved as
``ocio://cg-config-v2.1.0_aces-v1.3_ocio-v2.3``.

The method for doing this is to use the new ResolveConfigPath function. For example:
``ResolveConfigPath("ocio://default")``

And if you intend to present a list of built-in configs to users, be sure to filter
the list of configs returned by the ``BuiltinConfigRegistry`` and only show the ones
where ``isBuiltinConfigRecommended`` is True so that you are only presenting the most 
recent versions of each of the built-in configs.


Improved Lut1D and Lut3D performance via AVX SIMD intrinsics
************************************************************

In some cases, this gives more than a 2x speed-up in CPU Renderer performance.

For Developers
++++++++++++++

The library now contains multiple Lut1D and Lut3D evaluation routines and the appropriate
one is selected at runtime based on which SIMD instruction set is supported by the chip.
Please let us know if you encounter any issues as you test on various flavors of x86 hardware.


Improved support for macOS ARM chips
************************************

For Developers
++++++++++++++

SSE SIMD intrinsics are now converted to the equivalent Neon intrinsics when building for
the ``arm64`` platform.  

In addition, the build process for generating macOS Universal Binaries has been
improved.


OCIO command-line tools are now available via PyPI
**************************************************

For Users
+++++++++

Doing a ``pip install opencolorio`` now gives you the 
`command-line tools <https://opencolorio.readthedocs.io/en/latest/guides/using_ocio/using_ocio.html#tool-overview>`_
such as ``ociocheck``, ``ociochecklut``, and ``ocioconvert``.


Enhanced methods to find an equivalent color space between configs
******************************************************************

In OCIO 2.2, several methods were introduced that used heuristics to identify a color space
in a user's config that matches one of the known color spaces in the default built-in config.
In OCIO 2.3, these methods were improved and extended as follows:

- ``IdentifyBuiltinColorSpace`` Searches a user config to find the name of an equivalent 
  color space to a color space in one of the built-in configs. For example, "What is the 
  name in this config for the ``sRGB - Texture`` color space from the default built-in config?"

- ``IdentifyInterchangeSpace`` Identifies the pair of color space names to be used as the 
  interchange space to allow conversion between color spaces in a user config and one of the 
  built-in configs. This is now the fundamental function that contains the heuristics for trying 
  to identify known color spaces and is used by IdentifyBuiltinColorSpace and in the 
  GetProcessorFrom/ToBuiltinColorSpace methods.

This improves upon and extends upon PR #1710. One piece of feedback on that PR is that it 
would be helpful to be able to use the heuristics to extract the names of the interchange 
color spaces rather than a Processor. This gives the application more flexibility in how 
that information is persisted and may reduce the number of times that any heuristics (that 
search through a config's color spaces) need to be run.

Finally, an issue was fixed in ``GetProcessorFromConfigs`` so that the returned Processor is 
a no-op if either of the color spaces in the two configs is a data space.

Note that if the official interchange role(s) are present (as is required in configs with
``ocio_profile_version`` 2.2 or higher), the above methods simply use those rather than
relying on any heuristics.


Breaking API Change
*******************

For Developers
++++++++++++++

For developers that have customized their GPU Renderer by overriding the GpuShaderCreator
class, there is a minor breaking change in the API. The addTexture and getTexture virtual
methods now take a ``TextureDimensions`` parameter. This was necessary to give client apps full
control over whether 1D or 2D textures are used for Lut1D ops. Please see 
`PR #1762 <https://github.com/AcademySoftwareFoundation/OpenColorIO/pull/1762>`_
for an example of how to update your implementation of these modified virtual methods.


API Enhancements
****************

For Developers
++++++++++++++

You may find the following additions to the OCIO API useful:

- There is now a ``clearProcessorCache`` method to clear the Processor, CPUProcessor, and
  GPUProcessor contents from the cache present in a Config instance. This is useful if a
  LUT file on disk has changed since the Config instance was created.

- The ``setAllowTexture1D`` method on the GpuShaderCreator/GpuShaderDesc class allows your
  GPU Renderer implementation to always use 2D textures rather than 1D textures for Lut1D ops.

- The Config class now has ``GetProcessorFromConfigs`` variants that allow converting from
  a source color space in one config to a display/view in another config.

- The Config class now has an ``isInactiveColorSpace`` method and ``getRoleColorSpace`` method.

- The CPUProcessor now has the ``isDynamic`` and ``hasDynamic`` methods that were present on 
  the Processor class.

- The ``ResolveConfigPath`` function converts one of the built-in config short-cut strings
  into the unambiguous full URI for the current config.

- The new ``IdentifyBuiltinColorSpace`` and ``IdentifyInterchangeSpace`` methods are 
  described above.


New Built-in Transforms
***********************

For Config Authors
++++++++++++++++++

In config files with ``ocio_profile_version`` set to 2.3 or higher, config authors may take
advantage of the following new transform:

* ``DISPLAY - CIE-XYZ-D65_to_DisplayP3`` To implement a display for the Apple Display P3 color space


Release Notes
=============

For more detail, please see the GitHub release pages:

`OCIO 2.3.0 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.3.0>`_
