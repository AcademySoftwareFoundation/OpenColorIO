..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _quick_start_devs:

Quick Start for Developers
==========================

Get started by following the :ref:`installation` instructions to build OCIO from
source.  In addition to the library itself, you will want to make sure to build the 
command-line tools, including ``ocioconvert``.

Note that active development of OCIO v2 is happening in the main branch.

Watch the OCIO SIGGRAPH course listed in :ref:`publications`.

Grab the available configuration files (and the sample images, if you want) from
:ref:`downloads`.

Try using the command-line tool :ref:`overview-ociochecklut` to send an RGB through 
a file in the "luts" sub-directory of one of the configs you downloaded.  Try using 
the :ref:`overview-ocioconvert` tool to process an image from one color space to another.
Try using :ref:`overview-ociodisplay` to view an image through a color space conversion.

Take a look at the example code in the :ref:`developers-usageexamples` and then study
the code for the command-line tools, which is under ``src/apps``.  

There are helper classes under ``src/libutils/apphelpers`` for common application tasks
such as generating color space menus, building a viewing pipeline for a viewport,
and writing a color picker for scene-linear color spaces.

Look through the :ref:`concepts_overview` to get a sense of the big picture.

Check out :ref:`upgrading_to_v2` for an overview of the new features in OCIO v2.

Join the #dev and #ux channel on :ref:`slack`.  From the #ux channel, you may want to
get involved with the OCIO User Experience (UX) working group which discusses best
practices for implementing OCIO support in applications.
