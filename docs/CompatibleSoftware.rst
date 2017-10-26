.. _compatiblesoftware:

Compatible Software
===================

The following sofware all supports OpenColorIO (to varying degrees).

If you are interested in getting OCIO support for an application not listed
here, please contact the ocio-dev mailing list.

If you are a developer and would like assistance integration OCIO into your
application, please contant ocio-dev as well.


After Effects
*************

`Compositor - Adobe <http://www.adobe.com/products/aftereffects.html>`__

An OpenColorIO plugin is available for use in After Effects.

See `src/aftereffects
<http://github.com/imageworks/OpenColorIO/tree/master/src/aftereffects>`__
if you are interested in building your own OCIO plugins.

Pre-built binaries are also available for `download
<http://www.fnordware.com/OpenColorIO>`__, courtesy of 
`fnordware <http://www.fnordware.com>`__.


Blender
*******
`Open Source 3D Application <http://www.blender.org/>`__

In `version 2.64
<http://wiki.blender.org/index.php/Dev:Ref/Release_Notes/2.64>`__,
Blender has integrated OCIO as part it's redesigned `color management
system
<http://wiki.blender.org/index.php/Dev:Ref/Release_Notes/2.64/Color_Management>`__.


Krita
*****

`2D Paint - Open Source <http://www.krita.org/>`__

Krita now support OpenColorIO for image viewing, allowing for the accurate
painting of float32/OpenEXR imagery.

See `krita.org 
<http://www.krita.org/item/113-krita-starts-supporting-opencolorio>`__
for details.


Silhouette
**********

`Roto, Paint, Keying - SilhouetteFX <http://www.silhouettefx.com/silhouette>`__

OCIO is natively integrated in
`4.5+ <http://www.silhouettefx.com/silhouette/silhouette-4.5-WhatsNew.pdf>`__
Full support is provide for both image import/export, as well as image display.



Nuke
****

`Compositor - The Foundry <http://www.thefoundry.co.uk/products/nuke>`__

Nuke 6.3v7+ ships with native support for OpenColorIO. The OCIO configuration
is selectable in the user preferences.

OCIO Nodes: OCIOCDLTransform, OCIOColorSpace, OCIODisplay, OCIOFileTransform,
OCIOLookConvert, OCIOLogConvert

The OCIODisplay node is suitable for use in the Viewer as an input process (IP),
and a register function is provides to add viewer options for each display upon
launch.

The OCIO config "nuke-default" is provided, which matches the built-in Nuke
color processing. This profile is useful for those who want to mirror the native
nuke color processing in other applications.  (The underlying equations are
also provided as python code in the config as well).

A `video demonstration <http://vimeo.com/38773736>`__ of the Nuke OCIO workflow.

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

It comes with "nuke-default" OCIO config by default, so the Hiero viewer
matches when sending files to Nuke for rendering.


Photoshop
*********

OpenColorIO display luts can be exported as ICC profiles for use in
photoshop.  The core idea is to create an .icc profile, with a valid
description, and then to save it to the proper OS icc directory. (On
OSX, ``~/Library/ColorSync/Profiles/``). Upon a Photoshop relaunch,
Edit->Assign Profile, and then select your new OCIO lut.

See the the OCIO user guide `for details on baking ICC profiles for Photoshop
<userguide-bakelut-photoshop>`__

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

The core OpenColorIO API is avaiable for use in C++. See the `export
directory
<http://github.com/imageworks/OpenColorIO/tree/master/export/OpenColorIO>`__
for the C++ API headers.  Minimal code examples are also available in
the source code distribution. Of particular note are
`src/apps/ocioconvert/
<https://github.com/imageworks/OpenColorIO/tree/master/src/apps/ocioconvert>`__
and `src/apps/ociodisplay/
<https://github.com/imageworks/OpenColorIO/tree/master/src/apps/ociodisplay>`__

Also see the :ref:`developer-guide`

Python
******

The OpenColorIO API is available for use in python. See the "pyglue" directory
in the codebase.

See the devleoper guide for `usage examples
<developers-usageexamples>`__ and API documentation on the PYthon
bindings

Vegas Pro
*********

`Video editing - Sony <http://www.sonycreativesoftware.com/vegaspro>`__


Vegas Pro 12 uses OpenColorIO, supporting workflows such as S-log
footage via the ACES colorspace.

Apps w/icc or luts
******************
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


RV
*********

`Playback Tool - Tweak Software <http://www.tweaksoftware.com>`__

RV has native OCIO support in version 4 onwards. For more details, see
the OpenColorIO section of the `RV User Manual
<http://www.tweaksoftware.com/static/documentation/rv/current/html/rv_manual.html#OpenColorIO>`__.

Java (Beta)
***********
The OpenColorIO API is available for use in Java. See the `jniglue directory
<http://github.com/imageworks/OpenColorIO/tree/master/src/jniglue>`__
in the codebase.

This integration is currently considered a work in progress, and should not be
relied upon for critical production work.


Gaffer
******
`Open Source VFX Platform <http://gafferhw.org>`__

Gaffer is a node based application for use in CG and VFX production, with a
particular focus on lighting and look development.


Natron
******
'Open Source Compositing Softare <http://natron.fr>'


CryEngine3 (Beta)
*****************

`Game Engine - Crytek (Cinema Sandbox) <http://mycryengine.com/index.php?conid=59>`__

CryENGINE is a real-time game engine, targeting applications in the
motion-picture market. While we don't know many details about the CryEngine
OpenColorIO integration, we're looking forward to learning more as information
becomes available.

