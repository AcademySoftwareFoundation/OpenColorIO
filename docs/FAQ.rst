FAQ
===

License?
********

New BSD.

You are welcome to include the OpenColorIO in commercial, or open source
applications. See the :doc:`License` for further details.


Terminology
***********

- Transform - a function that alters RGB(A) data (e.g transform an image from scene linear to sRGB)
- Reference space - a space that connects colorspaces
- Colorspace - a meaningful space that can be transferred to and from the reference space
- Display - a virtual or physical display device (e.g an sRGB display device)
- View - a meaningful view of the reference space on a Display (e.g a film emulation view on an sRGB display device)
- Role - abstract colorspace naming (e.g specify the "lnh" colorspace as the scene_linear role)


What LUT Formats are supported?
*******************************

=========  ===================================  ===============================
Ext        Details                              Notes
=========  ===================================  ===============================
3dl        Autodesk Apps: Lustre, Flame, etc.   Full support.
           Supports shaper LUT + 3D
csp        Cinespace (Rising Sun Research)      Full support. Shaper LUT is
           LUT. Spline-based shaper LUT, with   resampled into simple 1D LUT
           either 1D or 3D LUT.                 with 2^16 samples
cub        Truelight format. Shaper Lut + 3D    Full support.
cube       Iridas format. Either 1D or 3D Lut.  Supports arbitrary input domains.
                                                Full support.
hdl        Houdini. 1D Lut, 3D lut, 1D shaper   Only 'C' type is supported.
           Lut                                  Need to add R G B A RGB RGBA ALL.
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
   For very small shaper LUT sizes this may not be sufficient.
