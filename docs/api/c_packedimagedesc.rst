..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

PackedImageDesc
***************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class PackedImageDesc : public OpenColorIO::`ImageDesc`_**

All the constructors expect a pointer to packed image data (such as
rgbrgbrgb or rgbargbargba) starting at the first color channel of
the first pixel to process (which does not need to be the first
pixel of the image). The number of channels must be greater than or
equal to 3. If a 4th channel is specified, it is assumed to be
alpha information. Channels > 4 will be ignored.

**Note**
The methods assume the CPUProcessor bit-depth type for the data
pointer.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: PackedImageDesc(void *data, long width, long height, long numChannels)

         **Note**
         numChannels must be 3 (RGB) or 4 (RGBA).

      .. cpp:function:: PackedImageDesc(void *data, long width, long height, long numChannels, BitDepth bitDepth, ptrdiff_t chanStrideBytes, ptrdiff_t xStrideBytes, ptrdiff_t yStrideBytes)

         **Note**
         numChannels must be 3 (RGB) or 4 (RGBA).

      .. cpp:function:: PackedImageDesc(void *data, long width, long height, ChannelOrdering chanOrder)

      .. cpp:function:: PackedImageDesc(void *data, long width, long height, ChannelOrdering chanOrder, BitDepth bitDepth, ptrdiff_t chanStrideBytes, ptrdiff_t xStrideBytes, ptrdiff_t yStrideBytes)

      .. cpp:function:: ~PackedImageDesc()

      .. cpp:function:: ChannelOrdering getChannelOrder() const

         Get the channel ordering of all the pixels.

      .. cpp:function:: BitDepth getBitDepth() const override

         Get the bit-depth.

      .. cpp:function:: void *getData() const

         Get a pointer to the first color channel of the first pixel.

      .. cpp:function:: void *getRData() const override

         Get a pointer to the red channel of the first pixel.

      .. cpp:function:: void *getGData() const override

         Get a pointer to the green channel of the first pixel.

      .. cpp:function:: void *getBData() const override

         Get a pointer to the blue channel of the first pixel.

      .. cpp:function:: void *getAData() const override

         Get a pointer to the alpha channel of the first pixel or null as
         alpha channel is optional.

      .. cpp:function:: long getWidth() const override

         Get the width to process (where x position starts at 0 and ends
         at width-1).

      .. cpp:function:: long getHeight() const override

         Get the height to process (where y position starts at 0 and ends
         at height-1).

      .. cpp:function:: long getNumChannels() const

      .. cpp:function:: ptrdiff_t getChanStrideBytes() const

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

      .. py:method:: getChanStrideBytes(self: PyOpenColorIO.PackedImageDesc) -> int

      .. py:method:: getChannelOrder(self: PyOpenColorIO.PackedImageDesc) -> PyOpenColorIO.ChannelOrdering

      .. py:method:: getData(self: PyOpenColorIO.PackedImageDesc) -> array

      .. py:method:: getNumChannels(self: PyOpenColorIO.PackedImageDesc) -> int
