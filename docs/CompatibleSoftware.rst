..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _compatiblesoftware:

Compatible Software
===================

The following software all supports OpenColorIO (to varying degrees).

If you are interested in getting OCIO support for an application not listed
here, please contact the ocio-dev mailing list.

If you are a developer and would like assistance integration OCIO into your
application, please contact ocio-dev as well.


After Effects
*************

Website: `<http://www.adobe.com/products/aftereffects.html>`__

Documentation :

- See `src/aftereffects <http://github.com/AcademySoftwareFoundation/OpenColorIO/tree/master/src/aftereffects>`__ if you are interested in building your own OCIO plugins.

- Pre-built binaries are also available for `download <http://www.fnordware.com/OpenColorIO>`__, courtesy of `fnordware <http://www.fnordware.com>`__.


- Profile has been properly measured using a spectrophotometer), then choose your display. If the transform was approved on a different monitor, then maybe you


Arnold (Autodesk)
*****************

Website : `<https://docs.arnoldrenderer.com/display/A5AFMUG/Color+Management>`__


Blender
*******

Blender has integrated OCIO as part of it's redesigned color management system.

Website: `Open Source 3D Application <http://www.blender.org/>`__

Supported Version: >= 2.64

Documentation:

- `<http://wiki.blender.org/index.php/Dev:Ref/Release_Notes/2.64>`__,

- `<http://wiki.blender.org/index.php/Dev:Ref/Release_Notes/2.64/Color_Management>`__.


C++
***

The core OpenColorIO API is available for use in C++. Minimal code examples are also available in the source code distribution. 

Website: `<http://github.com/AcademySoftwareFoundation/OpenColorIO/tree/master/export/OpenColorIO>`__

Documentation: 

Of particular note are

- `<https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/master/src/apps/ocioconvert>`__
- `<https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/master/src/apps/ociodisplay>`__


Clarisse (Isotropix)
********************

Website: `<https://www.isotropix.com/learn/tutorials/managing-looks-with-opencolorio-ocio>`__


CryEngine (CryTek)
*******************

CryEngine is a real-time game engine, targeting applications in the motion-picture market. While we don't know many details about the CryEngine OpenColorIO integration, we're looking forward to learning more as information becomes available.

Website: `<https://www.cryengine.com/>`__


DJV
***

Website: `<https://darbyjohnston.github.io/DJV/>`__


Gaffer
******

Gaffer is a node based application for use in CG and VFX production, with a particular focus on lighting and look development.

Website: `<http://www.gafferhq.org/>`__


Houdini (SideFX)
****************

Website: `<https://www.sidefx.com>`__

Supported Version: >= 16

Documentation:


- `<https://www.sidefx.com/docs/houdini/io/ocio.html>`__

- `<https://www.sidefx.com/filmtv/whats-new-h16/>`__


Java (Beta)
***********

The OpenColorIO API is available for use in Java. This integration is currently considered a work in progress, and should not be relied upon for critical production work.

Website: `<https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/master/src/bindings/java>`__


Katana (Foundry)
****************

Website: `<http://www.thefoundry.co.uk/products/katana>`__

Documentation:

- 2D Nodes: OCIODisplay, OCIOColorSpace, OCIOCDLTransform

- Monitor Panel: Full OCIO Support


Krita
*****

Krita now support OpenColorIO for image viewing, allowing for the accurate painting of float32/OpenEXR imagery.

Website: `<http://www.krita.org/>`__

Documentation :

- See `krita.org <http://www.krita.org/item/113-krita-starts-supporting-opencolorio>`__ for details.


Mari (Foundry)
**************

Mari 1.4v1+ ships with native support for OpenColorIO in their display toolbar.

Website: `<http://www.thefoundry.co.uk/products/mari>`__

Documentation:

- A `video demonstration <http://vimeo.com/32909648>`__ of the Mari OCIO workflow.


Maya (Autodesk)
***************

Autodesk Maya supports OCIO since version 2016 (I believe Autodesk SynColor color management system can read OCIO configurations?), May 2016.

Website: `<https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2016/ENU/Maya/files/GUID-C22F815A-8390-405B-BA50-74FEC42C75E0-htm.html>`__


Mocha Pro 2020 (Boris FX)
*************************

Website: `<https://borisfx.com/videos/opencolorio-mocha-pro-2020/>`__


Modo (Foundry)
**************

Website: `<https://learn.foundry.com/modo/content/help/pages/rendering/color_management.html>`__


mrViewer
********

mrViewer is a professional flipbook player, hdri viewer and video/audio playback tool. It supports OCIO input color spaces in images as well as display/view color spaces.

Website: `<https://mrviewer.sourceforge.io>`__

Documentation:

- `mrViewer Features <https://mrviewer.sourceforge.io/features.html>`__


Natron
******

Website : `<http://natron.fr>`__

Documentation :

- `Open Source Compositing Software <https://natrongithub.github.io/>`__


Nuke (Foundry)
**************

Nuke 6.3v7+ ships with native support for OpenColorIO. The OCIO configuration
is selectable in the user preferences.

OCIO Nodes : OCIOCDLTransform, OCIOColorSpace, OCIODisplay, OCIOFileTransform,
OCIOLookConvert, OCIOLogConvert

The OCIODisplay node is suitable for use in the Viewer as an input process (IP),
and a register function is provides to add viewer options for each display upon
launch.

The OCIO config "nuke-default" is provided, which matches the built-in Nuke
color processing. This profile is useful for those who want to mirror the native
nuke color processing in other applications.  (The underlying equations are
also provided as python code in the config as well).

Website : `<http://www.thefoundry.co.uk/products/nuke>`__

Supported Version : >= 6.3

Documentation :

- A `video demonstration <http://vimeo.com/38773736>`__ of the Nuke OCIO workflow.


OpenImageIO
***********

OIIO's C++ and Python bindings include several methods for applying color transforms to whole images, notably functions in the ImageBufAlgo namespace including **colorconvert()**, **ociolook()**, **ociodisplay()**, **ociofiletransform()**. These are also available as part of the *oiiotool* command line utility (--colorconvert, --ociolook, --ociodisplay, --ociofiletransform) and the *maketx* utility for preparing textures also supports **--colorconvert**. From C++, there is additional low-level functionality in the header **OpenImageIO/color.h** that are wrappers for accessing underlying OCIO color configurations and doing color processing on individual pixel values.

Website : `Open Source Image Library / Renderer Texture Engine <http://openimageio.org>`__


PhotoFlow
*********

It supports OCIO via a dedicated tool that can load a given configuration and apply the available color transforms.

Website : `<https://github.com/aferrero2707/PhotoFlow>`__

Documentation : 

- So far the tool has been tested it with the `Filmic <https://github.com/sobotka/filmic-blender>`__ and `ACES <https://opencolorio.org/configurations/aces_1.0.3.html>`__ configs.


Photoshop
*********

OpenColorIO display luts can be exported as ICC profiles for use in photoshop. The core idea is to create an .icc profile, with a valid description, and then to save it to the proper OS icc directory. (On OSX, ``~/Library/ColorSync/Profiles/``). Upon a Photoshop relaunch, Edit->Assign Profile, and then select your new OCIO lut.

Website : `<https://www.adobe.com/products/photoshop.html?promoid=PC1PQQ5T&mv=other>`__


Python
******

The OpenColorIO API is available for use in python. See the "pyglue" directory
in the codebase.

Documentation :

- See the developer guide for `usage examples <_developers-usageexamples>`__ and API documentation on the Python bindings.


RV (Autodesk)
*************

Website : `<http://www.tweaksoftware.com>`__

Supported Version:  >= 4

Documentation : 

- For more details, see the OpenColorIO section of the `RV User Manual <http://www.tweaksoftware.com/static/documentation/rv/current/html/rv_manual.html#OpenColorIO>`__.


Silhouette (Boris FX)
*********************

Website : `<http://www.silhouettefx.com/silhouette>`__

Supported Version: >= 4.5

Documentation : 

- OCIO is natively integrated in `<http://www.silhouettefx.com/silhouette/silhouette-4.5-WhatsNew.pdf>`__. 

- Full support is provided for both image import/export, as well as image display.


Substance Designer (Adobe)
**************************

Website : `<https://magazine.substance3d.com/substance-designer-winter-2019-color-management-with-opencolorio/>`__


Unreal Engine (Epic Games)
**************************

Website : `<https://unrealengine.com>`_

Supported Version : >= 4.22

Documentation :

- `OCIO Plugin API <https://docs.unrealengine.com/en-US/API/Plugins/OpenColorIO/index.html>`_
- `Unreal Engine 4.22 Release Notes <https://docs.unrealengine.com/en-US/Support/Builds/ReleaseNotes/4_22/index.html>`_


Vegas Pro (Magix)
*****************

Vegas Pro 12 uses OpenColorIO, supporting workflows such as S-log footage via the ACES colorspace.

Website : `<http://www.sonycreativesoftware.com/vegaspro>`__


V-Ray (Chaos Group)
*******************

Website : `<https://chaosgroup.com>`__

Documentation :

- `OpenColorIO support <https://docs.chaosgroup.com/display/VRAY4MAX/OpenColorIO+Support>`__

- `VRayTexOCIO <https://docs.chaosgroup.com/display/VRAY4MAYA/VRayTexOCIO>`__


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
<https://lists.aswf.io/g/ocio-dev/topic/30498585>`__
for additional usage discussions.

When exporting an ICC Profile, you will be asked to specify your monitor’s
profile (it will be selected for you by default). This is because ICC Profile
are not LUTs per se. An ICC Profile describes a color space and then needs a
destination profile to calculate the transformation. So if you have an operation
working and looking good on the monitor you’re using (and maybe its
should choose its profile instead.


Compatible Software (Deprecated)
--------------------------------

Hiero (Foundry)
***************

Hiero 1.0 will ship with native support for OCIO in the display and the equivalent of Nuke's OCIOColorSpace in the Read nodes. It comes with "nuke-default" OCIO config by default, so the Hiero viewer
matches when sending files to Nuke for rendering.

Website : `<http://www.thefoundry.co.uk/products/hiero>`__
