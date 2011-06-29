.. _faq:

FAQ
===

License?
********

New BSD.

You are welcome to include the OpenColorIO in commercial, or open source
applications. See the :ref:`license` for further details.


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
3dl        Autodesk Apps: Lustre, Flame, etc.   Read + Write Support.
           Supports shaper LUT + 3D
ccc        ASC CDL ColorCorrectionCollection    Full read support.
cc         ASC CDL ColorCorrection              Full read support.
csp        Cinespace (Rising Sun Research)      Read + Write Support.  Shaper is
           LUT. Spline-based shaper LUT, with   resampled into simple 1D LUT
           either 1D or 3D LUT.                 with 2^16 samples.
cub        Truelight format. Shaper Lut + 3D    Full read support.
cube       Iridas format. Either 1D or 3D Lut.  Full read support
hdl        Houdini. 1D Lut, 3D lut, 1D shaper   Only 'C' type is supported.
           Lut                                  Need to add R G B A RGB RGBA ALL.
                                                No support for Sampling tag.
                                                Header fields must be in strict order.
mga/m3d    Pandora 3D lut                       Full read support.
spi1d      1D format. Imageworks native 1D      Full read support.
           lut format.  HDR friendly, supports
           arbitrary input and output domains
spi3d      3D format. Imageworks native 3D      Full read support.
           lut format.
spimtx     3x3 matrix + color offset.           Full read support.
           Imageworks native color matrix
           format
vf         Inventor 3d lut.                     Read support for 3d lut data
                                                and global_transform element
=========  ===================================  ===============================

.. note::
   Shaper LUT application in OCIO currently only supports linear interpolation.
   For very small shaper LUT sizes this may not be sufficient. (CSP shaper luts
   excluded; they do use spline interpolation at load-time).
