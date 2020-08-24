..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

PlanarImageDesc
***************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class PlanarImageDesc : public OpenColorIO::`ImageDesc`_**

All the constructors expect pointers to the specified image planes
(i.e. rrrr gggg bbbb) starting at the first color channel of the
first pixel to process (which need not be the first pixel of the
image). Pass NULL for aData if no alpha exists (r/g/bData must not
be NULL).

**Note**
The methods assume the CPUProcessor bit-depth type for the
R/G/B/A data pointers.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: PlanarImageDesc(void *rData, void *gData, void *bData, void *aData, long width, long height)

      .. cpp:function:: PlanarImageDesc(void *rData, void *gData, void *bData, void *aData, long width, long height, BitDepth bitDepth, ptrdiff_t xStrideBytes, ptrdiff_t yStrideBytes)

         Note that although PlanarImageDesc is powerful enough to also
         describe all :cpp:class:``PackedImageDesc`` scenarios, it is
         recommended to use a PackedImageDesc where possible since that
         allows for additional optimizations.

      .. cpp:function:: ~PlanarImageDesc()

      .. cpp:function:: void *getRData() const override

         Get a pointer to the red channel of the first pixel.

      .. cpp:function:: void *getGData() const override

         Get a pointer to the green channel of the first pixel.

      .. cpp:function:: void *getBData() const override

         Get a pointer to the blue channel of the first pixel.

      .. cpp:function:: void *getAData() const override

         Get a pointer to the alpha channel of the first pixel or null as
         alpha channel is optional.

      .. cpp:function:: BitDepth getBitDepth() const override

         Get the bit-depth.

      .. cpp:function:: long getWidth() const override

         Get the width to process (where x position starts at 0 and ends
         at width-1).

      .. cpp:function:: long getHeight() const override

         Get the height to process (where y position starts at 0 and ends
         at height-1).

      .. cpp:function:: ptrdiff_t getXStrideBytes() const override

         Get the step in bytes to find the same color channel of the next
         pixel.

      .. cpp:function:: ptrdiff_t getYStrideBytes() const override

         Get the step in bytes to find the same color channel of the
         pixel at the same position in the next line.

      .. cpp:function:: bool isRGBAPacked() const override

         Is the image buffer in packed mode with the 4 color channels?
         (“Packed” here means that XStrideBytes is 4x the bytes per
         channel, so it is more specific than simply any
         PackedImageDesc.)

      .. cpp:function:: bool isFloat() const override

         Is the image buffer 32-bit float?


   .. group-tab:: Python

      .. py:method:: getAData(self: PyOpenColorIO.PlanarImageDesc) -> array

      .. py:method:: getBData(self: PyOpenColorIO.PlanarImageDesc) -> array

      .. py:method:: getGData(self: PyOpenColorIO.PlanarImageDesc) -> array

      .. py:method:: getRData(self: PyOpenColorIO.PlanarImageDesc) -> array
