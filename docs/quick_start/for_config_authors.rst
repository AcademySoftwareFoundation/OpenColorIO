..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _quick_start_config_authors:

Quick Start for Config Authors
==============================

As a config author, you'll want access to the OCIO command-line tools such as 
``ociocheck``.  Please see the :ref:`installation` instructions for the various
options, for example Homebrew on macOS and Vcpkg on Windows.  On Linux you may
need to build from source.

Grab the available configuration files (and the sample images, if you want) from
:ref:`downloads` so you'll have some examples to study.

Try using the command-line tool :ref:`overview-ociochecklut` to send an RGB through 
a file in the "luts" sub-directory of one of the configs you downloaded.  Try using 
the :ref:`overview-ocioconvert` tool to process an image from one color space to another.
Try using :ref:`overview-ociodisplay` to view an image through a color space conversion.

Look through the :ref:`concepts_overview` to get a sense of the big picture.

Study the descriptions in the :ref:`configurations` section of the example configs.

Check out :ref:`upgrading_to_v2` for an overview of the new features in OCIO v2.
