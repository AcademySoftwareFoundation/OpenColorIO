
PackedImageDesc
***************

.. class:: PackedImageDesc : public OpenColorIO::`ImageDesc`

   All the constructors expect a pointer to packed image data (such as rgbrgbrgb or rgbargbargba) starting at the first color channel of the first pixel to process (which does not need to be the first pixel of the image). The number of channels must be greater than or equal to 3. If a 4th channel is specified, it is assumed to be alpha information. Channels > 4 will be ignored.

   **Note**
      The methods assume the `CPUProcessor`_ bit-depth type for the data pointer. 


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: OpenColorIO::PackedImageDesc::PackedImageDesc()

   .. group-tab:: Python

      .. py:method:: PackedImageDesc.__init__(*args, **kwargs)**

         Overloaded function.

         1. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, numChannels: int) -> None

         2. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, numChannels: int, bitDepth: PyOpenColorIO.BitDepth, chanStrideBytes: int, xStrideBytes: int, yStrideBytes: int) -> None

         3. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, chanOrder: PyOpenColorIO.ChannelOrdering) -> None

         4. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, chanOrder: PyOpenColorIO.ChannelOrdering, bitDepth: PyOpenColorIO.BitDepth, chanStrideBytes: int, xStrideBytes: int, yStrideBytes: int) -> None


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ChannelOrdering OpenColorIO::PackedImageDesc::getChannelOrder() const

         Get the channel ordering of all the pixels. 

      .. cpp:function:: BitDepth OpenColorIO::PackedImageDesc::getBitDepth() const override

         Get the bit-depth. 

      .. cpp:function:: void *OpenColorIO::PackedImageDesc::getRData() const override

         Get a pointer to the red channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::PackedImageDesc::getGData() const override

         Get a pointer to the green channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::PackedImageDesc::getBData() const override

         Get a pointer to the blue channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::PackedImageDesc::getAData() const override

         Get a pointer to the alpha channel of the first pixel or null as alpha channel is optional. 

      .. cpp:function:: long OpenColorIO::PackedImageDesc::getWidth() const override

         Get the width to process (where x position starts at 0 and ends at width-1). 

      .. cpp:function:: long OpenColorIO::PackedImageDesc::getHeight() const override

         Get the height to process (where y position starts at 0 and ends at height-1). 

      .. cpp:function:: long OpenColorIO::PackedImageDesc::getNumChannels() const

      .. cpp:function:: ptrdiff_t OpenColorIO::PackedImageDesc::getChanStrideBytes() const

      .. cpp:function:: ptrdiff_t OpenColorIO::PackedImageDesc::getXStrideBytes() const override

         Get the step in bytes to find the same color channel of the next pixel. 

      Warning: doxygenfunction: Cannot find function “OpenColorIO::PackedImageDesc::getYStrideBtytes” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: bool OpenColorIO::PackedImageDesc::isRGBAPacked() const override

         Is the image buffer in packed mode with the 4 color channels? (“Packed” here means that XStrideBytes is 4x the bytes per channel, so it is more specific than simply any PackedImageDesc.) 

      .. cpp:function:: bool OpenColorIO::PackedImageDesc::isFloat() const override

         Is the image buffer 32-bit float? 

   .. group-tab:: Python

      .. py:method:: PackedImageDesc.__init__(*args, **kwargs)

         Overloaded function.

         1. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, numChannels: int) -> None

         2. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, numChannels: int, bitDepth: PyOpenColorIO.BitDepth, chanStrideBytes: int, xStrideBytes: int, yStrideBytes: int) -> None

         3. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, chanOrder: PyOpenColorIO.ChannelOrdering) -> None

         4. __init__(self: PyOpenColorIO.PackedImageDesc, data: buffer, width: int, height: int, chanOrder: PyOpenColorIO.ChannelOrdering, bitDepth: PyOpenColorIO.BitDepth, chanStrideBytes: int, xStrideBytes: int, yStrideBytes: int) -> None

      .. py:method:: PackedImageDesc.getChannelOrder(self: PyOpenColorIO.PackedImageDesc) -> PyOpenColorIO.ChannelOrdering

      .. py:method:: PackedImageDesc.getNumChannels(self: PyOpenColorIO.PackedImageDesc) -> int

      .. py:method:: PackedImageDesc.getChanStrideBytes(self: PyOpenColorIO.PackedImageDesc) -> int
