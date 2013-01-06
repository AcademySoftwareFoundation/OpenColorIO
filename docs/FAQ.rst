.. _faq:

FAQ
===

License?
********

New BSD.

You are welcome to include the OpenColorIO in commercial, or open source
applications. See the :ref:`license` for further details.


.. _faq-terminology:

Terminology
***********

- Transform - a function that alters RGB(A) data (e.g transform an image from scene linear to sRGB)
- Reference space - a space that connects colorspaces
- Colorspace - a meaningful space that can be transferred to and from the reference space
- Display - a virtual or physical display device (e.g an sRGB display device)
- View - a meaningful view of the reference space on a Display (e.g a film emulation view on an sRGB display device)
- Role - abstract colorspace naming (e.g specify the "lnh" colorspace as the scene_linear role, or the color-picker UI uses color_picking role)
- Look - a color transform which applies a creative look (for example a per-shot netural grade to remove color-casts from a sequence of film scans, or a DI look)

.. _faq-supportedlut:

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
look       IRIDAS .look                         Read baked 3D LUT embedded in file.
                                                No mask support.
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


Can you query a color space by name (like "Rec709") and get back XYZ coordinates of its primaries and whitepoint?
*****************************************************************************************************************

Not currently.

OCIO is a color configuration 'playback' tool that tries to be as flexible as possible;
color information such as this is often only needed / relevant at configuration authoring time.
Making primaries / whitepoint required would limit the pipeline OCIO could service. In the
strictest sense, we would consider OCIO to be a 'baked' representation of color processes,
similar to how Alembic files do not store animation rig data, but rather only the baked geometry.

Also, remember that not all colorspaces using in visual effects even have strongly
defined color space definitions. For example, scanned film negatives,  when linearized with
1d transfer curves (the historical norm in vfx), do not have defined primaries/white point.
Each rgb value could of course individually be tied to a specific color, but if you were to
do a sweep  of the pure 'red channel', for example, you'd find that it creates a curves in
chromaticity space, not a single point.  (This is due to the 1d linearization not attempting
to undo the subtractive processes that created the pixels in the first place.

But many color spaces in OCIO *do* have very strongly defined white points/chromaticities.
On the display side, for example, we have very  precise information on this.

Perhaps OCIO should include optional metadata to tag outputs?  We are looking at this as
as a OCIO 1.2 feature.

Can you convert XYZ <-> named color space RGB values?
*****************************************************

OCIO includes a MatrixTransform, so the processing capability is there. But there is no convenience
function to compute this matrix for you. (We do include other Matrix convenience functions though,
so it already has a place to be added. See MatrixTransform in export/OpenColorTransforms.h)

There's talk of extended OCIO 1.2 to have a plugin api where colorspaces could be dynamically
added at runtime (such as after reading exr  chromaticity header metadata).  This would
necessitate adding such a feature.


What are the differences between Nuke's Vectorfield and OCIOFileTransform?
**************************************************************************

(All tests done with Nuke 6.3)

=========  =============================================   ===============================
Ext        Details                                         Notes
=========  =============================================   ===============================
3dl        Matched Results
ccc        n/a
cc         n/a
csp        *Difference*                                    Gain error. Believe OCIO is correct, but need to verify.
cub        Matched Results                                 Note: Nuke's .cub exporter is broken (gain error)
cube       Matched Results
hdl        n/a
mga/m3d    n/a
spi1d      n/a
spi3d      n/a
spimtx     n/a
vf         *Difference*                                     Gain error. Believe OCIO is correct, but need to verify.
=========  =============================================   ===============================

All gain differences are due to a common 'gotcha' when interpolating 3d luts, related to
internal index computation. If you have a 32x32x32 3dlut, when sampling values from (0,1)
do you internally scale by 31.0 or 32.0?  This is typically well-defined for each format,
(in this case the answer is usually 31.0) but when incorrectly handled in an application,
you occationally see gain errors that differ by this amount. (In the case of a 32-sized
3dlut, 32/31 = ~3% error)


What do ColorSpace::setAllocation() and ColorSpace::setAllocationVars() do?
***************************************************************************

These hints only come into play during GPU processing, and are used to determine proper
colorspace allocation handling for 3D LUTs. See this page :ref:`allocationvars` for
further information.

