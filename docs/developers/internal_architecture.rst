Internal Architecture Overview
==============================

External API
************

Configs
+++++++

At the highest level, we have OCIO::Configs. This represents the entirety of the
current color "universe".  Configs are serialized as .ocio files, read at runtime,
and are often used in a 'read-only' context.

Config are loaded at runtime to allow for customized color handling in a show-
dependent manner.

Example Configs:

* ACES (Acacdemy's standard color workflow)
* spi-vfx (Used on some Imageworks VFX shows such as spiderman, etc).
* and others


ColorSpaces
+++++++++++

The meat of an OCIO::Config is a list of named ColorSpaces. ColorSpace often
correspond to input image states, output image states, or image states used for
internal processing.

Example ColorSpaces (from ACES configuration):

* aces (HDR, scene-linear)
* adx10 (log-like density encoding space)
* slogf35 (sony F35 slog camera encoding)
* rrt_srgb (baked in display transform, suitable for srgb display)
* rrt_p3dci (baked in display transform, suitable for dcip3 display)


Transforms
++++++++++

ColorSpaces contain an ordered list of transforms, which define the conversion
to and from the Config's "reference" space.

Transforms are the atomic units available to the designer in order to specify a
color conversion.

Examples of OCIO::Transforms are:

* File-based transforms (1d lut, 3d lut, mtx... anything, really.)
* Math functions (gamma, log, mtx)
* The 'meta' GroupTransform, which contains itself an ordered lists of transforms
* The 'meta' LookTransform, which contains an ordered lists of transforms


For example, the adx10 ColorSpace (in one particular ACES configuration)
-Transform FROM adx, to our reference ColorSpace:

#. Apply FileTransform adx_adx10_to_cdd.spimtx
#. Apply FileTransform adx_cdd_to_cid.spimtx
#. Apply FileTransform adx_cid_to_rle.spi1d
#. Apply LogTransform base 10 (inverse)
#. Apply FileTransform adx_exp_to_aces.spimtx


If we have an image in the reference ColorSpace (unnamed), we can convert TO
adx by applying each in the inverse direction:

#. Apply FileTransform adx_exp_to_aces.spimtx (inverse)
#. Apply LogTransform base 10 (forward)
#. Apply FileTransform adx_cid_to_rle.spi1d (inverse)
#. Apply FileTransform adx_cdd_to_cid.spimtx (inverse)
#. Apply FileTransform adx_adx10_to_cdd.spimtx (inverse)


Note that this isn't possible in all cases (what if a lut or matrix is not 
invertible?), but conceptually it's a simple way to think about the design.


Summary
+++++++

Configs and ColorSpaces are just a bookkeeping device used to get and ordered
lists of Transforms corresponding to image color transformation.

Transforms are visible to the person AUTHORING the OCIO config, but are
NOT visible to the client applications. Client apps need only concern themselves
with Configs and Processors.


OCIO::Processors
++++++++++++++++

A processor corresponds to a 'baked' color transformation. You specify two arguments
when querying a processor: the :ref:`colorspace_section` you are coming from,
and the :ref:`colorspace_section` you are going to.

Once you have the processor, you can apply the color transformation using the
"apply" function.  For the CPU veseion, first wrap your image in an
ImageDesc class, and then call apply to process in place.

Example:

.. code-block:: cpp

   #include <OpenColorIO/OpenColorIO.h>
   namespace OCIO = OCIO_NAMESPACE;
   
   try
   {
      // Get the global OpenColorIO config
      // This will auto-initialize (using $OCIO) on first use
      OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
      
      // Get the processor corresponding to this transform.
      // These strings, in this example, are specific to the above
      // example. ColorSpace names should NEVER be hard-coded into client
      // software, but should be dynamically queried at runtime from the library
      OCIO::ConstProcessorRcPtr processor = config->getProcessor("adx10", "aces");
      
      // Wrap the image in a light-weight ImageDescription
      OCIO::PackedImageDesc img(imageData, w, h, 4);
      
      // Apply the color transformation (in place)
      processor->apply(img);
   }
   catch(OCIO::Exception & exception)
   {
      std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
   }


The GPU code path is similar.  You get the processor from the config, and then
query the shaderText and the lut3d.  The client loads these to the GPU themselves,
and then makes the appropriate calls to the newly defined function.

See `src/apps/ociodisplay` for an example.


Internal API
************


The Op Abstraction
++++++++++++++++++

It is a useful abstraction, both for code-reuse and optimization, to not relying
on the transforms to do pixel processing themselves.

Consider that the FileTransform represents a wide-range of image processing
operations (basically all of em), many of which are really complex.  For example,
the houdini lut format in a single file may contain a log convert, a 1d lut, and
then a 3d lut; all of which need to be applied in a row!  If we dont want the
FileTransform to know how to process all possible pixel operations, it's much
simpler to make light-weight processing operations, which the transforms can
create to do the dirty work as needed.

All image processing operations (ops) are a class that present the same
interface, and it's rather simple:

.. code-block:: cpp

   virtual void apply(float* rgbaBuffer, long numPixels)

Basically, given a packed float array with the specified number of pixels, process em.

Examples of ops include Lut1DOp, Lut3DOp, MtxOffsetOp, LogOp, etc.

Thus, the job of a transform becomes much simpler and they're only responsible
for converting themselves to a list of ops.  A simple FileTransform that only has
a single 1D lut internally may just generate a single Lut1DOp, but a
FileTransform that references a more complex format (such as the houdini lut case
referenced above) may generate a few ops:

.. code-block:: cpp

   void FileFormatHDL::BuildFileOps(OpRcPtrVec & ops,
                            const Config& /*config*/,
                            const ConstContextRcPtr & /*context*/,
                            CachedFileRcPtr untypedCachedFile,
                            const FileTransform& fileTransform,
                            TransformDirection dir) const {
   
   // Code omitted which loads the lut file into the file cache...
   
   CreateLut1DOp(ops, cachedFile->lut1D,
                      fileTransform.getInterpolation(), dir);
   CreateLut3DOp(ops, cachedFile->lut3D,
                      fileTransform.getInterpolation(), dir);

See (``src/core/*Ops.h``) for the available ops.

Note that while compositors often have complex, branching trees of image processing
operations, we just have a linear list of ops, lending itself very well to
optimization.

Before the ops are run, they are optimized. (Collapsed with appropriate neighbors, etc).


An Example
++++++++++

Let us consider the internal steps when getProcessor() is called to convert from ColorSpace
'adx10' to ColorSpace 'aces':

* The first step is to turn this ColorSpace conversion into an ordered list of transforms.
We do this by creating a single of the conversions from 'adx10' to reference, and then
adding the transforms required to go from reference to 'aces'.
* The Transform list is then converted into a list of ops.  It is during this stage luts,
are loaded, etc.


CPU CODE PATH
+++++++++++++

The master list of ops is then optimized, and stored internally in the processor.

.. code-block:: cpp

   FinalizeOpVec(m_cpuOps);


During Processor::apply(...), a subunit of pixels in the image are formatted into a sequential rgba block.  (Block size is optimized for computational (SSE) simplicity and performance, and is typically similar in size to an image scanline)

.. code-block:: cpp

   float * rgbaBuffer = 0;
   long numPixels = 0;
   while(true) {
      scanlineHelper.prepRGBAScanline(&rgbaBuffer, &numPixels);
      ...


Then for each op, op->apply is called in-place.

.. code-block:: cpp

   for(OpRcPtrVec::size_type i=0, size = m_cpuOps.size(); i<size; ++i)
   {
      m_cpuOps[i]->apply(rgbaBuffer, numPixels);
   }


After all ops have been applied, the results are copied back to the source

.. code-block:: cpp

   scanlineHelper.finishRGBAScanline();


GPU CODE PATH
+++++++++++++

#. The master list of ops is partitioned into 3 ordered lists:

- As many ops as possible from the BEGINNING of the op-list that can be done
  analytically in shader text. (called gpu-preops)
- As many ops as possible from the END of the op-list that can be done
  analytically in shader text. (called gpu-postops)
- The left-over ops in the middle that cannot support shader text, and thus
  will be baked into a 3dlut. (called gpu-lattice)


#. Between the first an the second lists (gpu-preops, and gpu-latticeops), we
anaylze the op-stream metadata and determine the appropriate allocation to use.
(to minimize clamping, quantization, etc). This is accounted for here by
interserting a forward allocation to the end of the pre-ops, and the inverse
allocation to the start of the lattice ops.

See https://github.com/imageworks/OpenColorIO/blob/master/src/core/NoOps.cpp#L183

#. The 3 lists of ops are then optimized individually, and stored on the processor.
The Lut3d is computed by applying the gpu-lattice ops, on the CPU, to a lut3d
image.

The shader text is computed by calculating the shader for the gpu-preops, adding
a sampling function of the 3d lut, and then calculating the shader for the gpu
post ops.

