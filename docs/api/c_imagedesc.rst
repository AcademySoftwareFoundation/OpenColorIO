
ImageDesc
*********

.. class:: ImageDesc

   This is a light-weight wrapper around an image, that provides a context for pixel access. This does NOT claim ownership of the pixels or copy image data. 

   Subclassed by `OpenColorIO::PackedImageDesc`, `OpenColorIO::PlanarImageDesc`


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: OpenColorIO::ImageDesc::ImageDesc()

   .. group-tab:: Python

      .. py:method:: PyOpenColorIO.ImageDesc.__init__(self: PyOpenColorIO.ImageDesc) -> None


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: void *OpenColorIO::ImageDesc::getRData() const = 0

         Get a pointer to the red channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::ImageDesc::getGData() const = 0

         Get a pointer to the green channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::ImageDesc::getBData() const = 0

         Get a pointer to the blue channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::ImageDesc::getAData() const = 0

         Get a pointer to the alpha channel of the first pixel or null as alpha channel is optional. 

      .. cpp:function:: BitDepth OpenColorIO::ImageDesc::getBitDepth() const = 0

         Get the bit-depth. 

      .. cpp:function:: long OpenColorIO::ImageDesc::getWidth() const = 0

         Get the width to process (where x position starts at 0 and ends at width-1). 

      .. cpp:function:: long OpenColorIO::ImageDesc::getHeight() const = 0

         Get the height to process (where y position starts at 0 and ends at height-1). 

      .. cpp:function:: ptrdiff_t OpenColorIO::ImageDesc::getXStrideBytes() const = 0

         Get the step in bytes to find the same color channel of the next pixel. 

      .. cpp:function:: ptrdiff_t OpenColorIO::ImageDesc::getYStrideBytes() const = 0

         Get the step in bytes to find the same color channel of the pixel at the same position in the next line. 

      .. cpp:function:: bool OpenColorIO::ImageDesc::isRGBAPacked() const = 0

         Is the image buffer in packed mode with the 4 color channels? (“Packed” here means that XStrideBytes is 4x the bytes per channel, so it is more specific than simply any `PackedImageDesc`.) 

      .. cpp:function:: bool OpenColorIO::ImageDesc::isFloat() const = 0

         Is the image buffer 32-bit float? 

   .. group-tab:: Python

      .. py:method:: PyOpenColorIO.ImageDesc.getBitDepth(self: PyOpenColorIO.ImageDesc) -> PyOpenColorIO.BitDepth

      .. py:method:: PyOpenColorIO.ImageDesc.getWidth(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: PyOpenColorIO.ImageDesc.getHeight(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: PyOpenColorIO.ImageDesc.getXStrideBytes(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: PyOpenColorIO.ImageDesc.getYStrideBytes(self: PyOpenColorIO.ImageDesc) -> int

      .. py:method:: PyOpenColorIO.ImageDesc.isRGBAPacked(self: PyOpenColorIO.ImageDesc) -> bool

      .. py:method:: PyOpenColorIO.ImageDesc.isFloat(self: PyOpenColorIO.ImageDesc) -> bool
