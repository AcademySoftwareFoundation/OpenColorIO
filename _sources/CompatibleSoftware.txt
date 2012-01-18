.. _compatiblesoftware:

Compatible Software
===================

The following sofware all supports OpenColorIO (to varying degrees).

If you are interested in getting OCIO support for an application not listed
here, please contact the ocio-dev mailing list.

If you are a developer and would like assistance integration OCIO into your
application, please contant ocio-dev as well.

Silhouette
**********

`Roto, Paint, Keying - SilhouetteFX <http://www.silhouettefx.com/silhouette>`__

OCIO is natively integrated in
`4.5+ <http://www.silhouettefx.com/silhouette/silhouette-4.5-WhatsNew.pdf>`__
Full support is provide for both image import/export, as well as image display.

Nuke
****

`Compositor - The Foundry <http://www.thefoundry.co.uk/products/nuke>`__

OCIO Nuke nodes currently require compiling by the user, but are expected to
ship in a commercial Nuke release sometime soon.

OCIO Nodes: OCIOCDLTransform, OCIOColorSpace, OCIODisplay, OCIOFileTransform,
OCIOLookConvert, OCIOLogConvert

The OCIODisplay node is suitable for use in the Viewer as an input process (IP),
and a register function is provides to add viewer options for each display upon
launch.

The OCIO config "nuke-default" is provided, which matches the built-in Nuke
color processing. This profile is useful for those who want to mirror the native
nuke color processing in other applications.  (The underlying equations are
also provided as python code in the config as well).

Mari
****

`3D Paint - The Foundry <http://www.thefoundry.co.uk/products/mari>`__

Mari 1.4v1+ ships with native support for OpenColorIO in their display toolbar.

A `video demonstration <http://vimeo.com/32909648>`__ of the Mari OCIO workflow.

Katana
******

`CG Pipeline / Lighting Tool - The Foundry <http://www.thefoundry.co.uk/products/katana>`__

Color management in Katana (all versions) natively relies on OCIO.

2D Nodes: OCIODisplay, OCIOColorSpace, OCIOCDLTransform
Monitor Panel: Full OCIO Support

Hiero
*****

`Conform & Review - The Foundry <http://www.thefoundry.co.uk/products/hiero>`__

Hiero 1.0 will ship with native support for OCIO in the display and the
equivalent of Nuke's OCIOColorSpace in the Read nodes.

It comes with “nuke-default” OCIO config by default, so the Hiero viewer
matches when sending files to Nuke for rendering.


OpenImageIO
***********

`Open Source Image Library / Renderer Texture Engine <http://openimageio.org>`__

Available in the current code trunk. Integration is with makecolortx (allowing
for color space conversion during mipmap generation), and also through the
public header `src/include/color.h <http://github.com/OpenImageIO/oiio/blob/master/src/include/color.h>`__ .

Remaining integration tasks include
`color conversion at runtime <http://github.com/OpenImageIO/oiio/issues/193>`__ .

C++
***
The core OpenColorIO API is avaiable for use in C++. See the `export directory
<http://github.com/imageworks/OpenColorIO/tree/master/export/OpenColorIO>`__
for the C++ API headers.  Minimal code examples are also available in the source
code distribution. Of particular note are apps/ocioconvert and apps/ociodisplay.

Python
******
The OpenColorIO API is available for use in python. See the "pyglue" directory
in the codebase.


Apps w/icc or luts
**************************************************
flame (.3dl), lustre (.3dl), cinespace (.csp), houdini (.lut), iridas_itx (.itx)
photoshop (.icc)

Export capabilities through ociobakelut::

    $ ociobakelut -- create a new LUT or icc profile from an OCIO config or lut file(s)
    $ 
    $ usage:  ociobakelut [options] <OUTPUTFILE.LUT>
    $ 
    $ example:  ociobakelut --inputspace lg10 --outputspace srgb8 --format flame lg_to_srgb.3dl
    $ example:  ociobakelut --lut filmlut.3dl --lut calibration.3dl --format flame display.3dl
    $ example:  ociobakelut --lut look.3dl --offset 0.01 -0.02 0.03 --lut display.3dl --format flame display_with_look.3dl
    $ example:  ociobakelut --inputspace lg10 --outputspace srgb8 --format icc ~/Library/ColorSync/Profiles/test.icc
    $ example:  ociobakelut --lut filmlut.3dl --lut calibration.3dl --format icc ~/Library/ColorSync/Profiles/test.icc
    $ 
    $ 
    $ Using Existing OCIO Configurations
    $     --inputspace %s      Input OCIO ColorSpace (or Role)
    $     --outputspace %s     Output OCIO ColorSpace (or Role)
    $     --shaperspace %s     the OCIO ColorSpace or Role, for the shaper
    $     --iconfig %s         Input .ocio configuration file (default: $OCIO)
    $ 
    $ Config-Free LUT Baking
    $     (all options can be specified multiple times, each is applied in order)
    $     --lut %s             Specify a LUT (forward direction)
    $     --invlut %s          Specify a LUT (inverse direction)
    $     --slope %f %f %f     slope
    $     --offset %f %f %f    offset (float)
    $     --offset10 %f %f %f  offset (10-bit)
    $     --power %f %f %f     power
    $     --sat %f             saturation (ASC-CDL luma coefficients)
    $ 
    $ Baking Options
    $     --format %s          the lut format to bake: flame (.3dl), lustre (.3dl),
    $                          cinespace (.csp), houdini (.lut), iridas_itx (.itx), icc (.icc)
    $     --shapersize %d      size of the shaper (default: format specific)
    $     --cubesize %d        size of the cube (default: format specific)
    $     --stdout             Write to stdout (rather than file)
    $     --v                  Verbose
    $     --help               Print help message
    $ 
    $ ICC Options
    $     --whitepoint %d      whitepoint for the profile (default: 6505)
    $     --displayicc %s      an icc profile which matches the OCIO profiles target display
    $     --description %s     a meaningful description, this will show up in UI like photoshop
    $     --copyright %s       a copyright field
    


See this `ocio-dev thread 
<http://groups.google.com/group/ocio-dev/browse_thread/thread/56fd58e60d98e0f6#>`__
for additional usage discussions.

When exporting an ICC Profile, you will be asked to specify your monitor’s
profile (it will be selected for you by default). This is because ICC Profile
are not LUTs per se. An ICC Profile describes a color space and then needs a
destination profile to calculate the transformation. So if you have an operation
working and looking good on the monitor you’re using (and maybe its
profile has been properly measured using a spectrophotometer), then choose your
display. If the transform was approved on a different monitor, then maybe you
should choose its profile instead.



RV (Beta)
*********************************

`Playback Tool - Tweak Software <http://www.tweaksoftware.com>`__

`OCIO support in RV <https://github.com/imageworks/OpenColorIO/tree/master/src/rv>`__
is currently being developed by Ben Dickson (dbr).

See this `email thread <http://groups.google.com/group/ocio-dev/browse_thread/thread/955fc6271f89a28e>`__
for additional details.

This integration is currently considered a work in progress, and should not be
relied upon for critical production work.


After Effects (Beta)
*********************************

`Compositor - Adobe <http://www.adobe.com/products/aftereffects.html>`__

OCIO support through `this plugin <http://www.fnordware.com/OpenColorIO>`__.

This code has not yet been rolled into the OCIO source tree, though we hope to
add support in the near future.

See this `email thread <http://groups.google.com/group/ocio-dev/browse_thread/thread/5b37c04e2d743759>`__
for additional details.

This plugin is currently considered a work in progress, and should not be
relied upon for critical production work.


Java (Beta)
************************
The OpenColorIO API is available for use in Java. See the `jniglue directory
<http://github.com/imageworks/OpenColorIO/tree/master/src/jniglue>`__
in the codebase.

This integration is currently considered a work in progress, and should not be
relied upon for critical production work.

java.com


Blender (Beta)
***************************
`Open Source 3D Application <http://www.blender.org/>`__

Xaview Thomas has begun the `Blender OCIO integration <http://github.com/thmxv/blender-ocio>`__ .
Currently undergoing development.

`Blender Developers Meeting Notes July 31, 2011
<http://www.blendernation.com/2011/08/01/blender-developers-meeting-notes-july-31-2011>`__

`YouTube Blender Example
<http://www.youtube.com/watch?v=O43ItUVvcks>`__

Ramen (Beta)
*************************
`Open Source Compositor <http://ramencomp.blogspot.com>`__

Under development, with native OCIO color managment.

