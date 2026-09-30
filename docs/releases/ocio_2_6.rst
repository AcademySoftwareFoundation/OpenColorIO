..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


OCIO 2.6 Release
================

Timeline
********

OpenColorIO 2.6 was delivered in September 2026 and is in the VFX Reference Platform for
calendar year 2027.

New Feature Guide
=================

Built-in ACES 2.1 Configs
*************************

Built-in ACES 2.1 versions of the Studio and CG config are now provided. Please note that
the Output Transforms were not changed in the 2.1 release, only the AMF transform IDs.
The AMF transform IDs in the OCIO 2.6.0 built-in configs are preliminary and will change
in the OCIO 2.6.1 release, after ACES 2.1 is officially released.

In addition, these configs include the following updates:

* Adds "Apple Log 2" and "Linear Apple Wide Gamut" color spaces.

* The "P3-D65 - Display" display and display color space are renamed "Gamma 2.6 P3-D65 - Display".
  Backwards compatibility is provided by the new display aliasing feature in OCIO 2.6, but may
  require updates on the part of application developers in some situations.

* The new interop ID ``g24_rec709_scene`` is added, though the previous ID ``ocio:g24_rec709_scene``
  is preserved as an alias.

* The family attribute of Sony Venice color spaces is now "Input/Sony/Legacy" to lower their
  placement in hierarchical menus, following the guidance from Sony that the Venice color spaces are
  no longer recommended. These may be made inactive or be removed in future versions of the configs.

For Users
+++++++++

The following URI strings may be provided anywhere you would normally provide a file path
to a config (e.g. as the OCIO environment variable):

To use the updated :ref:`aces_cg`, use this string for the config path:
    ocio://cg-config-v5.0.0_aces-v2.1_ocio-v2.6

To use the updated :ref:`aces_studio`, use this string for the config path:
    ocio://studio-config-v5.0.0_aces-v2.1_ocio-v2.6

This string will give you the current default config, which is the latest ACES 2.1 CG Config:
    ocio://default

This string now points to this latest ACES 2.1 CG config:
    ocio://cg-config-latest

This string now points to this latest ACES 2.1 Studio config:
    ocio://studio-config-latest


New Fixed Function Transforms
*****************************

For Config Authors
++++++++++++++++++

The following new styles are available for use with FixedFunctionTransforms in config
files with ``ocio_profile_version`` set to 2.6 or higher. They implement a conversion from
a linear RGB space with customizable primaries to the JMh (lightness, colorfulness, hue)
color appearance space used in the ACES 2.0 Output Transforms. The ordering is hue,
colorfulness, lightness, rather than JMh since that is the order already established by
the built-in HSV and HSY Fixed Functions and it conforms to the order expected by the
GradingHueCurveTransform. The scaling is {h/360, M/200, J/100} to allow the resulting
images to be easier to work with in DCCs. It takes the following eight parameters to
describe the primaries and white point of the RGB space:
[ red_x, red_y, green_x, green_y, blue_x, blue_y, white_x, white_y ].

* ``FIXED_FUNCTION_ACES_RGB_TO_HMJ_20``


New Built-in Transforms
***********************

For Config Authors
++++++++++++++++++

In config files with ``ocio_profile_version`` set to 2.6 or higher, config authors may take
advantage of the following new BuiltinTransform styles:

* ``APPLE_LOG-APPLEWG_to_ACES2065-1``


Release Notes
=============

For additional details, please see the GitHub release page:

`OCIO 2.6.0 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.6.0>`_
