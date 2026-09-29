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
