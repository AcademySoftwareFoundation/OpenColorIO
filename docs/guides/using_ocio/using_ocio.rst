..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _using_ocio:

Using OCIO
==========

.. _using_env_vars:

Environment Variables
=====================

The following environment variables may be used with OpenColorIO:

.. envvar:: OCIO

   Specifies the file path of the config file to be used.  (This is 
   necessary for some of the command-line tools but most applications
   provide a way of setting this from the user interface.)

.. envvar:: OCIO_ACTIVE_DISPLAYS

   Overrides the :ref:`active-displays` list from the config file and reorders them.
   Colon-separated list of displays, e.g ``sRGB:P3``

.. envvar:: OCIO_ACTIVE_VIEWS

   Overrides the :ref:`active-views` list from the config file and reorders them.
   Colon-separated list of view names, e.g ``internal:client:DI``

.. envvar:: OCIO_INACTIVE_COLORSPACES

   Overrides the :ref:`inactive_colorspaces` list from the config file.
   Colon-separated list of color spaces, e.g ``previousColorSpace:tempSpace``

.. envvar:: OCIO_LOGGING_LEVEL

    Configures OCIO's internal logging level. Valid values are
    ``none``, ``warning``, ``info``, or ``debug`` (or their respective
    numeric values ``0``, ``1``, ``2``, or ``3`` can be used)

    Logging output is sent to STDERR output by default.

.. envvar:: OCIO_OPTIMIZATION_FLAGS

   Overrides the optimization settings being used by an application, for 
   troubleshooting purposes.  The complete list of flags is in OpenColorTypes.h.


.. include:: tool_overview.rst


.. _faq-supportedlut:

Supported LUT Formats
=====================

========== ===================================  =========================================
Ext        Details                              Notes
========== ===================================  =========================================
3dl        Discreet legacy 3D-LUT               Read + Write Support.
                                                Supports shaper + 3D-LUT
ccc        ASC CDL ColorCorrectionCollection    Full read support.
cc         ASC CDL ColorCorrection              Full read support.
cdl        ASC CDL ColorCorrection              Full read support.
clf        Academy/ASC Common LUT Format        Full read + write support.
ctf        Autodesk Color Transform Format      Full read + write support.
                                                Provides lossless serialization
                                                of arbitrary OCIO transforms.
csp        Cinespace (Rising Sun Research)      Read + Write Support.  Shaper is
           LUT. Spline-based shaper LUT, with   resampled into simple 1D-LUT
           either 1D or 3D-LUT.                 with 2^16 samples.
cub        Truelight format. Shaper Lut + 3D    Full read support.
cube       Iridas format. Either 1D or 3D Lut.  Read support.
cube       Resolve format.                      Read support.
lut        Discreet legacy 1D-LUT               Read support.
hdl        Houdini. 1D-LUT, 3D-LUT, 1D shaper   Only 'C' type is supported.
           Lut                                  Need to add R G B A RGB RGBA ALL.
                                                No support for Sampling tag.
                                                Header fields must be in strict order.
icc/icm/pf ICC profile format                   Read support for basic monitor profiles.
look       IRIDAS .look                         Read baked 3D-LUT embedded in file.
                                                No mask support.
mga/m3d    Pandora 3D-LUT                       Full read support.
spi1d      1D-LUT format. Imageworks native     Full read support.
           format.  HDR friendly, supports
           arbitrary input and output domains
spi3d      3D-LUT format. Imageworks native     Full read support.
           format.
spimtx     3x3 matrix + color offset.           Full read support.
           Imageworks native matrix format.
vf         Inventor 3D-LUT.                     Read support for 3D-LUT data
                                                and global_transform element
========== ===================================  =========================================

.. note::
   Shaper LUT application in OCIO currently only supports linear interpolation.
   For very small shaper LUT sizes this may not be sufficient. (CSP shaper luts
   excluded; they do use spline interpolation at load-time).


.. include:: compatible_software.rst

.. include:: faq.rst
