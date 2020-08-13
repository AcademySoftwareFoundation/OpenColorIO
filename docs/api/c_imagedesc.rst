..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ImageDesc
*********

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class ImageDesc**

This is a light-weight wrapper around an image, that provides a
context for pixel access. This does NOT claim ownership of the
pixels or copy image data.

Subclassed by OpenColorIO::PackedImageDesc,
OpenColorIO::PlanarImageDesc


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ImageDesc()

      .. cpp:function:: ~ImageDesc()

      .. cpp:function:: void *getRData() const = 0

         Get a pointer to the red channel of the first pixel.

      .. cpp:function:: void *getGData() const = 0

         Get a pointer to the green channel of the first pixel.

      .. cpp:function:: void *getBData() const = 0

         Get a pointer to the blue channel of the first pixel.

      .. cpp:function:: void *getAData() const = 0

         Get a pointer to the alpha channel of the first pixel or null as
         alpha channel is optional.

      .. cpp:function:: BitDepth getBitDepth() const = 0

         Get the bit-depth.

      .. cpp:function:: long getWidth() const = 0

         Get the width to process (where x position starts at 0 and ends
         at width-1).

      .. cpp:function:: long getHeight() const = 0

         Get the height to process (where y position starts at 0 and ends
         at height-1).

      .. cpp:function:: ptrdiff_t getXStrideBytes() const = 0

         Get the step in bytes to find the same color channel of the next
         pixel.

      .. cpp:function:: ptrdiff_t getYStrideBytes() const = 0

         Get the step in bytes to find the same color channel of the
         pixel at the same position in the next line.

      .. cpp:function:: bool isRGBAPacked() const = 0

         Is the image buffer in packed mode with the 4 color channels?
         (“Packed” here means that XStrideBytes is 4x the bytes per
         channel, so it is more specific than simply any
         PackedImageDesc.)

      .. cpp:function:: bool isFloat() const = 0

         Is the image buffer 32-bit float?


   .. group-tab:: Python

      .. py:method:: getBitDepth(self: PyOpenColorIO.ImageDesc) -> PyOpenColorIO.BitDepth

      .. py:method:: getHeight(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: getWidth(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: getXStrideBytes(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: getYStrideBytes(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: isFloat(self: PyOpenColorIO.ImageDesc) -> bool

      .. py:method:: isRGBAPacked(self: PyOpenColorIO.ImageDesc) -> bool
