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


Display and View Aliases
************************

For Config Authors
++++++++++++++++++

Config authors may now define alias names for displays and views that will be recognized
in a ``DisplayViewTransform`` as equivalent to the canonical names. Similar to color space
aliases, this allows config authors to evolve naming of display and views over time while
still providing backwards compatibility for the older names.

Display aliasing is opt-in and the config author must set the new config-level attribute
``use_display_aliases: true``. With that enabled, the name or aliases of the display color
space for the display will be considered synonyms for that display. 

View aliasing is allowed via a new ``aliases`` attribute on a view or shared view. These are
always active, independent of whether ``use_display_aliases`` is enabled. Similar to other
Yaml lists, these are separated by a comma. Names that contain an embedded comma are 
enclosed in quotes to prevent it from being used as a separator.

Please note that the ``active_displays`` and ``active_views`` lists must use the canonical names
rather than aliases. Similarly, view aliases in a shared view may not be used when referring to
the shared view in a display's views. 

For virtual displays, aliases may be used with shared views but are 
not suppored for display-defined virtual views.

As an example, in the following config file excerpt, "srgb_rec709_display" could be used as
a display alias and "aces2_sdr_view" could be used as a view alias when creating a
``DisplayViewTransform``.

.. code-block:: yaml

    use_display_aliases: true
    
    shared_views:
      - !<View> {name: ACES 2.0 - SDR, view_transform: ACES 2.0 - SDR, 
                 display_colorspace: <USE_DISPLAY_NAME>, aliases: [aces2_sdr_view]}
    
    displays:
      sRGB - Display:
        - !<Views> [ACES 2.0 - SDR]
    
    display_colorspaces:
      - !<ColorSpace>
        name: sRGB - Display
        aliases: [srgb_rec709_display]

For Developers
++++++++++++++

If application code is currently calling ``Config::getDisplayViewColorSpaceName``, you will
probably want to change that to ``Config::getResolvedDisplayViewColorSpaceName`` so that it
will handle aliases. Note that this resolves the ``<USE_DISPLAY_NAME>`` token, as well.

Any existing calls to ``DisplayViewTransform`` should automatically work with aliases, without
any changes.

The new functions ``Config::getCanonicalDisplayName`` and ``Config::getCanonicalViewName`` may
be used to convert aliases back to the primary name used in the config.


Display Descriptions
********************

For Developers
++++++++++++++

On a related note, the new ``Config::getDisplayDescription`` allows applications to get a
description for a display. This is sourced from the description attribute of the display
color space that implements the display. (Views already have a description attribute
available for config authors to set.) This enables applications to provide tool-tips or
similar help text for both displays and views.


Color Interop ID Support
************************

For Developers
++++++++++++++

New functions have been added to assist developers in implementing support for the ASWF
Color Interop Forum's `Color Interop ID. <https://github.com/AcademySoftwareFoundation/ColorInterop/blob/main/Recommendations/03_ColorInteropID/ColorInteropID.md>`_
These allow applications to either find a color space for an ID or, conversely, find an ID
for a color space (even if its ``interop_id`` is missing).

* The function ``Config::findColorSpaceForID`` searches a config for the color space that
  should be used for a given interop ID. This implements the fallback rules defined in the
  ASWF CIF Recommendation.

* The function ``Config::LocateBuiltinColorSpace`` searches for a color space in a built-in
  config that is equivalent to a source color space. Since all color spaces in the recent
  built-in configs have interop IDs populated, it allows you to determine the interop ID
  for most color spaces, even if the interop ID is not populated in your source config.
  This function leverages the color space "finger-printing" technique developed for the
  config merging feature in OCIO 2.5.

* The function ``Config::generateLocalIDForColorSpace`` allows applications to generate
  an interop ID, even if the config does not contain one for that color space and one 
  cannot be found using LocateBuiltinColorSpace.


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
