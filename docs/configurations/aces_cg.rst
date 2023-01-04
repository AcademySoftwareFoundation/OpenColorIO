..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _aces_cg:

ACES CG Config
==============

The ACES Computer Graphics (CG) config is a simple, lightweight config intended for use
in typical digital content creation (DCC) apps that need robust choices for texture and
rendering spaces and a basic selection of display and view transforms.

Users who need a more extensive set of color spaces, including digital cinema camera
color spaces and a wider set of displays and view should look at the :ref:`aces_studio`.

The latest version of this config may be downloaded from the Releases page of its GitHub
`repo. <https://github.com/AcademySoftwareFoundation/OpenColorIO-Config-ACES/releases>`_

The ACES CG Config leverages the high quality ACES implementation built into OCIO itself
and so requires no external LUT files.  In fact, even the config file is built into OCIO
and users may access it from any application that uses OCIO 2.2 or higher by using the
following string in place of the config path::
    ocio://cg-config-v1.0.0_aces-v1.3_ocio-v2.1

The default built-in config is currently the ACES CG Config, so the even simpler:
``ocio://default`` may be used instead.  Note however, that the value of the default
config may evolve over time.

The OCIO Configs Working Group collected input from the community and simplified the
naming scheme relative to the earlier OCIO v1 ACES configs.  However, aliases have been 
added so that the original color space names continue to work (if there is an equivalent
space in the new config).

Please note that with OCIO v2 we are trying to be more rigorous about what constitutes a 
"color space". For this reason, the new configs do not bake view transforms or looks into 
the display color spaces.  Therefore, it is necessary to use a DisplayViewTransform rather 
than a ColorSpaceTransform if you want to bake in an ACES Output Transform.  This is not 
only more rigorous from a color management point of view, it also helps clarify to end-users 
the important role of a view transform in the process.  Baking in a view transform is a 
fundamentally different process than just converting between color space encodings, and it 
should be perceived as such by users.
