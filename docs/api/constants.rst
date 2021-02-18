..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Constants
=========

.. _vars_envvar:

Environment Variables
*********************

These environmental variables are used by the OpenColorIO library
(i.e. these variables are defined in src/OpenColorIO/Config.cpp).

.. tabs::

   .. group-tab:: Python

      .. data:: PyOpenColorIO.OCIO_CONFIG_ENVVAR

         The envvar 'OCIO' provides a path to the config file used by 
         :ref:`Config.CreateFromEnv`

      .. data:: PyOpenColorIO.OCIO_ACTIVE_DISPLAYS_ENVVAR

          The envvar 'OCIO_ACTIVE_DISPLAYS' provides a list of displays 
          overriding the 'active_displays' list from the config file.

      .. data:: PyOpenColorIO.OCIO_ACTIVE_VIEWS_ENVVAR

         The envvar 'OCIO_ACTIVE_VIEWS' provides a list of views overriding the 
         'active_views' list from the config file.

      .. data:: PyOpenColorIO.OCIO_INACTIVE_COLORSPACES_ENVVAR

         The envvar 'OCIO_INACTIVE_COLORSPACES' provides a list of inactive 
         color spaces overriding the 'inactive_color_spaces' list from the 
         config file.

      .. data:: PyOpenColorIO.OCIO_OPTIMIZATION_FLAGS_ENVVAR

         The envvar 'OCIO_OPTIMIZATION_FLAGS' provides a way to force a given 
         optimization level. Remove the variable or set the value to empty to 
         not use it. Set the value of the variable to the desired optimization 
         level as either an integer or hexadecimal value. 
         Ex: OCIO_OPTIMIZATION_FLAGS="20479" or "0x4FFF" for 
         OPTIMIZATION_LOSSLESS.

   .. group-tab:: C++

      .. doxygengroup:: VarsEnvvar
         :content-only:
         :members:
         :undoc-members:

.. _vars_roles:

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

.. tabs::

   .. group-tab:: Python

      .. data:: PyOpenColorIO.ROLE_DEFAULT

         "default"

      .. data:: PyOpenColorIO.ROLE_REFERENCE

         "reference"

      .. data:: PyOpenColorIO.ROLE_DATA

         "data"

      .. data:: PyOpenColorIO.ROLE_COLOR_PICKING

         "color_picking"

      .. data:: PyOpenColorIO.ROLE_SCENE_LINEAR

         "scene_linear"

      .. data:: PyOpenColorIO.ROLE_COMPOSITING_LOG

         "compositing_log"

      .. data:: PyOpenColorIO.ROLE_COLOR_TIMING

         "color_timing"

      .. data:: PyOpenColorIO.ROLE_TEXTURE_PAINT

         This role defines the transform for painting textures. In some 
         workflows this is just a inverse display gamma with some limits.

      .. data:: PyOpenColorIO.ROLE_MATTE_PAINT

         This role defines the transform for matte painting. In some workflows 
         this is a 1D HDR to LDR allocation. It is normally combined with 
         another display transform in the host app for preview.

      .. data:: PyOpenColorIO.ROLE_RENDERING

         The rendering role may be used to identify a specific color space to be 
         used by CGI renderers.  This is typically a scene-linear space but the 
         primaries also matter since they influence the resulting color, especially 
         in areas of indirect illumination.

      .. data:: PyOpenColorIO.ROLE_INTERCHANGE_SCENE

         The aces_interchange role is used to specify which color space in the 
         config implements the standard ACES2065-1 color space (SMPTE ST2065-1). 
         This may be used when converting scene-referred colors from one config 
         to another.

      .. data:: PyOpenColorIO.ROLE_INTERCHANGE_DISPLAY

         The cie_xyz_d65_interchange role is used to specify which color space in 
         the config implements CIE XYZ colorimetry with the neutral axis at D65. 
         This may be used when converting display-referred colors from one config 
         to another.

   .. group-tab:: C++

      .. doxygengroup:: VarsRoles
         :content-only:
         :members:
         :undoc-members:

.. _vars_shared_view:

Shared View
***********

.. tabs::

   .. group-tab:: Python

      .. data:: PyOpenColorIO.OCIO_VIEW_USE_DISPLAY_NAME

         A shared view using this for the color space name will use a display 
         color space that has the same name as the display the shared view is 
         used by.

   .. group-tab:: C++

      .. doxygengroup:: VarsSharedView
         :content-only:
         :members:
         :undoc-members:

.. _vars_format_metadata:

FormatMetadata
**************

These constants describe various types of rich metadata. They are used with 
FormatMetadata objects as the "name" part of a (name, value) pair. All of these 
types of metadata are supported in the CLF/CTF file formats whereas other 
formats support some or none of them.

Although the string constants used here match those used in the CLF/CTF 
formats, the concepts are generic, so the goal is for other file formats to 
reuse the same constants within a FormatMetadata object (even if the syntax 
used in a given format is somewhat different).

.. tabs::

   .. group-tab:: Python

      .. data:: PyOpenColorIO.METADATA_DESCRIPTION

         A description string -- used as the "Description" element in CLF/CTF 
         and CDL, and to hold comments for other LUT formats when baking.

      .. data:: PyOpenColorIO.METADATA_INFO

         A block of informative metadata such as the "Info" element in CLF/CTF. 
         Usually contains child elements.

      .. data:: PyOpenColorIO.METADATA_INPUT_DESCRIPTOR

         A string describing the expected input color space -- used as the 
         "InputDescriptor" element in CLF/CTF and the "InputDescription" in 
         CDL.

      .. data:: PyOpenColorIO.METADATA_OUTPUT_DESCRIPTOR

         A string describing the output color space -- used as the 
         "OutputDescriptor" element in CLF/CTF and the "OutputDescription" in 
         CDL.

      .. data:: PyOpenColorIO.METADATA_NAME

         A name string -- used as a "name" attribute in CLF/CTF elements. Use 
         on a GroupTransform to get/set the name for the CLF/CTF ProcessList. 
         Use on an individual Transform (i.e. MatrixTransform, etc.) to get/set 
         the name of the corresponding process node.

      .. data:: PyOpenColorIO.METADATA_ID

         An ID string -- used as an "id" attribute in CLF/CTF elements. Use on 
         a GroupTransform to get/set the id for the CLF/CTF ProcessList. Use 
         on an individual Transform (i.e. MatrixTransform, etc.) to get/set the 
         id of the corresponding process node.

   .. group-tab:: C++

      .. doxygengroup:: VarsFormatMetadata
         :content-only:
         :members:
         :undoc-members:

.. _vars_caches:

Caches
******

.. tabs::

   .. group-tab:: Python

      .. data:: PyOpenColorIO.OCIO_DISABLE_ALL_CACHES

         Disable all caches, including for FileTransforms and Optimized/CPU/GPU 
         Processors. (Provided only to facilitate developer investigations.)

      .. data:: PyOpenColorIO.OCIO_DISABLE_PROCESSOR_CACHES

         Disable only the Optimized, CPU, and GPU Processor caches. (Provided 
         only to facilitate developer investigations.)

      .. data:: PyOpenColorIO.OCIO_DISABLE_CACHE_FALLBACK

         By default the processor caches check for identical color 
         transformations when cache keys do not match. That fallback introduces 
         a major performance hit in some cases so there is an env. variable to 
         disable the fallback.

   .. group-tab:: C++

      .. doxygengroup:: VarsCaches
         :content-only:
         :members:
         :undoc-members:
