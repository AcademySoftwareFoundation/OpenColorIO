..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _quick_start_config_authors:

Quick Start for Config Authors
==============================

Get started by following the :ref:`installation` instructions.  Note that if
you want to try out the new OpenColorIO v2 features, you'll need to build
from source.  You will want to make sure to build the command-line tools as
well as the library itself, so you should install OpenImageIO or OpenEXR before
building (OCIO is now able to build the latter itself).

Grab the available configuration files (and the sample images, if you want) from
:ref:`downloads` so you'll have some examples to study.

Try using the command-line tool :ref:`overview-ociochecklut` to send an RGB through 
a file in the "luts" sub-directory of one of the configs you downloaded.  Try using 
the :ref:`overview-ocioconvert` tool to process an image from one color space to another.
Try using :ref:`overview-ociodisplay` to view an image through a color space conversion.

Look through the :ref:`concepts_overview` to get a sense of the big picture.

Study the descriptions in the :ref:`configurations` section of the example configs.

Check out :ref:`upgrading_to_v2` for an overview of the new features in OCIO v2.
