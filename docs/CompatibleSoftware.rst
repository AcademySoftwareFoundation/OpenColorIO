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
Roto, Paint, Keying - SilhouetteFX
http://www.silhouettefx.com/silhouette/

OCIO is natively integrated in 4.5+
Full support is provide for both image import/export, as well as image display.

http://www.silhouettefx.com/silhouette/silhouette-4.5-WhatsNew.pdf

Nuke
****
Compositor - The Foundry
http://www.thefoundry.co.uk/products/nuke/

Ships with 6.3v7+

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
3D Paint - The Foundry
http://www.thefoundry.co.uk/products/mari/

Mari 1.4v1+ ships with native support for OCIO in the display.
A video demonstration of the OCIO workflow:
http://vimeo.com/32909648

Katana
******
CG Pipeline / Lighting Tool - The Foundry
http://www.thefoundry.co.uk/products/katana/

Color management in Katana (all version) natively relies on OCIO.

2D Nodes: OCIODisplay, OCIOColorSpace, OCIOCDLTransform
Monitor Panel: Full OCIO Support

OpenImageIO
***********
Open Source Image Library / Renderer Texture Engine
openimageio.org

Available in the current code trunk. Integration is with makecolortx (allowing
for color space conversion during mipmap generation), and also through the
public header src/include/color.h.  Remaining integration tasks include color
conversion at runtime (see https://github.com/OpenImageIO/oiio/issues/193 )

C++
***
The core OpenColorIO API is avaiable for use in C++. See the "export" directory
for the C++ API headers.  Minimal code examples are also available in the source
code distribution. Of particular note are apps/ocioconvert and apps/ociodisplay.

Python
******
The OpenColorIO API is available for use in python. See the "pyglue" directory
in the codebase.


Apps w/icc (Photoshop, etc.)
**************************************************
Export capabilities through ocio2icc.

See 
http://groups.google.com/group/ocio-dev/browse_thread/thread/56fd58e60d98e0f6#

When exporting an ICC Profile, you will be asked to specify your monitor’s
profile (it will be selected for you by default). This is because ICC Profile
are not LUTs per se. An ICC Profile describes a color space and then needs a
destination profile to calculate the transformation. So if you have an operation
working and looking good on the monitor you’re using (and maybe its
profile has been properly measured using a spectrophotometer), then choose your
display. If the transform was approved on a different monitor, then maybe you
should choose its profile instead.

Apps w/lut (Flame, Lustre, etc.)
***************************
Truelight, Iridas, Autodesk (Flame, lustre, etc)

Export capabilities through ociobakelut


After Effects (Beta)
*********************************
Compositor - Adobe
http://www.adobe.com/products/aftereffects.html

Native OCIO support through a plugin:
http://www.fnordware.com/OpenColorIO/

This code has not yet been rolled into the OCIO source tree, though we hope to
add support in the near future.

See this email thread for additional details:
http://groups.google.com/group/ocio-dev/browse_thread/thread/5b37c04e2d743759

This plugins is currently considered a work in progress, and should not be
relied upon for critical production work.


Java (Beta)
************************
The OpenColorIO API is available for use in Java. See the "jni" directory
in the codebase.

This integration is currently considered a work in progress, and should not be
relied upon for critical production work.

java.com


Blender (Beta)
***************************
Open Source 3D Application

A user has begun a Blender integration. Not sure of the current state of the
project.

https://github.com/thmxv/blender-ocio


Ramen (Beta)
*************************
Open Source Compositor
http://ramencomp.blogspot.com/

Has native OCIO integration.

