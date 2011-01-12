FAQ
===

**License?**
-------------------------------------------------------------------------------

New BSD.

You are welcome to include the OpenColorIO in commercial, or open source
applications. See the :doc:`License` for further details.

**What LUT Formats are supported?**
-------------------------------------------------------------------------------

=========  ===================================  ===============================
Ext        Details                              Notes
=========  ===================================  ===============================
3dl        Autodesk Apps: Lustre, Flame, etc.   Full support.
           Supports shaper LUT + 3D
csp        Cinespace (Rising Sun Research) LUT  Shaper Lut not yet supported.
cub        Truelight format. Shaper Lut + 3D    Full support.
cube       Iridas format. Either 1D or 3D Lut.  Supports arbitrary input domains.
                                                Full support.
hdl        Houdini. 1D Lut, 3D lut, 1D shaper   Only 'C' type is supported.
           Lut                                  need to add R G B A RGB RGBA ALL.
                                                No support for Sampling tag.
                                                Header fields must be in strict order.
spi1d      1D format. Imageworks native 1D      Full support.
           lut format.  HDR friendly, supports
           arbitrary input and output domains
spi3d      3D format. Imageworks native 3D      Full support.
           lut format.
spimtx     3x3 matrix + color offset.           Full support.
           Imageworks native color matrix
           format
=========  ===================================  ===============================

.. note::
   Shaper LUT application in OCIO currently only supports linear interpolation.
   For very small shaper lut sizes this may not be sufficient.
