.. _config-spipipeline:

Sony Pictures Imageworks Color Pipeline
=======================================

This document describes a high-level overview on how to emulate the current color management practice at Sony Imageworks. It applies equally to all profiles used at Imageworks, including both the VFX and Animation profiles.  It's by no means a requirement to follow this workflow at your own facility, this is merely a guideline on how we choose to work.

General Pipeline Observations
*****************************

* All images, on disk, contain colorspace information as a substring withing the filename.  This is obeyed by all applications that load image, write images, or view images.  File extensions and metadata are ignored with regards to color processing.

 Example::

      colorimage_lnf.exr  : lnf
      dataimage_ncf.exr : ncf
      plate_lg10.dpx : lg10
      texture_dt8.tif: dt8

 Note: fileformat extension does NOT imply a color space.  Not all dpx files are lg10. Note all tif images are dt8.

* The common file formats we use are exr, tif, dpx.

 * render outputs: exr
 * render inputs (mipmapped-textures): exr, tif (txtif)
 * photographic plates (scans): dpx
 * composite outputs: dpx, exr
 * on-set reference: (camera raw) NEF, CR2, etc.
 * painted textures: psd, tif
 * output proxies: jpg

* All pipelines that need to be colorspace aware rely on ``Config.parseColorSpaceFromString``.

* Color configurations are show specific. The $OCIO environment variable is set as part of a 'setshot' process, before other applications are launched.   Artists are not allowed to work across different shows without using a fresh shell + setshot.

* While the list of colorspaces can be show specific, care is taken to maintain similar naming to the greatest extent feasible. This reduces artist confusion.  Even if two color spaces are not identical across shows, if they serve a similar purpose they are named the same.  Example: We label 10-bit scanned film negatives as lg10. Even if two different shows use different acquisition film stocks, and rely on different linearization curves, they are both labelled lg10.

* There is no explicit guarantee that image assets copied across shows will be transferable in a color-correct manner. For example, in the above film scan example, one would not expect that the linearized versions of scans processed on different shows to match. In practice, this is not a problematic issue as the colorspaces which are convenient to copy (such as texture assets) happen to be similarly defined across show profiles.


Rendering
*********

* Rendering and shading occurs in a scene-linear floating point space, typically named "ln".  Half-float (16-bit) images are labelled lnh, full float images (32-bit) are labelled lnf.


* All image inputs should be converted to ln prior to render-time.  Typically, this is done when textures are published. (See below) 

* Renderer outputs are always floating-point.  Color outputs are typically stored as lnh (16-bit half float).  Data outputs (normals, depth data, etc) are stored as ncf ("not color" data, 32-bit full float). Lossy compression is never utilized.

* Render outputs are always viewed with an OCIO compatible image viewer.   Thus, for typical color imagery the lnf display transform will be applied.  In Nuke, this can be emulated using the OCIODisplay node.  A standalone image viewer, ociodisplay, is also included with  OpenColorIO src/example.


.. _config-spipipeline-texture:

Texture Painting / Matte Painting
*********************************

* Textures are painted in non-OCIO color-managed environment. (Photoshop, etc).

* At texture publish time, before mipmaps are generated, all color processing is applied.  Internally at SPI we use a modified version of OpenImageIO's maketx that also links to OpenColorIO.  We intend to make this code available as soon as possible.  In the meantime, the OpenColorIO 'ocioconvert' script included in examples can be relied upon.  Color processing (linearization) is applied before mipmap generation in order to assure energy preservation in the render.  If the opposite processing order were used, (mipmap in the original space, color convert in the shader), the apparent intensity of texture values would change as the object approached to receeded to the camera.

* The original texture filenames contain the colorspace information as a substring, to signify processing intent.

 * Textures that contain data (bump maps, opacity maps, blend maps, etc) are labelled with the nc colorspaces according to their bitdepth.  Ex: an 8-bit opacity map -> skin_opacity_nc8.tif

 * Painted textures that are intended to modulate diffuse color components are labelled dt (standing for "diffuse texture").  The dt8 colorspace is designed such that, when linearized, values will not extend above 1.0.  At texture publishing time these are converted to lnh mipmapped tiffs/exr.  Note that as linear textures have greater allocation requirements, a bit depth promotion is required in this case.  I.e., even if the original texture as painted was only 8-bits, the mipmapped texture will be stored as a 16-bit float image.

 * Painted environment maps, which may be emissive as labelled vd (standing for 'video').   These values, when linearized, have the potential to generate specular information well above 1.0.   Note that in the current vd linearization curves, the top code values may be very "sensitive". I.e., very small changes in the initial code value (such as 254->255) may actually result in very large differences in the estimated scene-linear intensity.   All environment maps are store as lnh mipmapped tiffs/exr. The same bit-depth promotion as in the dt8 case is required here.


Compositing
***********

* The majority of compositing operations happen in scene-linear, lnf, colorspace.
* All image inputs are linearized to lnf as they are loaded.  Customized input nodes make this processing convenient.  Rendered elements, which are stored in linear already, do not require processing.  Photographic plates will typically be linearized according to their source type, (lg10 for film scans, gn10 for genesis sources, etc).
* All output images are de-linearized from lnf when they are written. A customized output node makes this convenient.
* On occasion log data is required for certain processing operations.  (Plate resizing, pulling keys, degrain, etc).  For each show, a colorspace is specified as appropriate for this operation.  The artist does not have to keep track of which colorspace is appropriate to use; the OCIOLogConvert node is always intended for this purpose.  (Within the OCIO profile, this is specified using the 'compositing_log' role).
