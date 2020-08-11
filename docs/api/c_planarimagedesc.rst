
PlanarImageDesc
***************

.. class:: PlanarImageDesc : public OpenColorIO::`ImageDesc`

   All the constructors expect pointers to the specified image planes (i.e. rrrr gggg bbbb) starting at the first color channel of the first pixel to process (which need not be the first pixel of the image). Pass NULL for aData if no alpha exists (r/g/bData must not be NULL).

   **Note**
      The methods assume the `CPUProcessor` bit-depth type for the R/G/B/A data pointers. 


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: OpenColorIO::PlanarImageDesc::PlanarImageDesc()

   .. group-tab:: Python

      .. py:method:: PlanarImageDesc.__init__(*args, **kwargs)

         Overloaded function.

         1. __init__(self: PyOpenColorIO.PlanarImageDesc, rData: buffer, gData: buffer, bData: buffer, width: int, height: int) -> None

         2. __init__(self: PyOpenColorIO.PlanarImageDesc, rData: buffer, gData: buffer, bData: buffer, aData: buffer, width: int, height: int) -> None

         3. __init__(self: PyOpenColorIO.PlanarImageDesc, rData: buffer, gData: buffer, bData: buffer, width: int, height: int, bitDepth: PyOpenColorIO.BitDepth, xStrideBytes: int, yStrideBytes: int) -> None

         4. __init__(self: PyOpenColorIO.PlanarImageDesc, rData: buffer, gData: buffer, bData: buffer, aData: buffer, width: int, height: int, bitDepth: PyOpenColorIO.BitDepth, xStrideBytes: int, yStrideBytes: int) -> None


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: void *OpenColorIO::PlanarImageDesc::getRData() const override

         Get a pointer to the red channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::PlanarImageDesc::getGData() const override

         Get a pointer to the green channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::PlanarImageDesc::getBData() const override

         Get a pointer to the blue channel of the first pixel. 

      .. cpp:function:: void *OpenColorIO::PlanarImageDesc::getAData() const override

         Get a pointer to the alpha channel of the first pixel or null as alpha channel is optional. 

      .. cpp:function:: BitDepth OpenColorIO::PlanarImageDesc::getBitDepth() const override

         Get the bit-depth. 

      .. cpp:function:: long OpenColorIO::PlanarImageDesc::getWidth() const override

         Get the width to process (where x position starts at 0 and ends at width-1). 

      .. cpp:function:: long OpenColorIO::PlanarImageDesc::getHeight() const override

         Get the height to process (where y position starts at 0 and ends at height-1). 

      .. cpp:function:: ptrdiff_t OpenColorIO::PlanarImageDesc::getXStrideBytes() const override

         Get the step in bytes to find the same color channel of the next pixel. 

      .. cpp:function:: ptrdiff_t OpenColorIO::PlanarImageDesc::getYStrideBytes() const override

         Get the step in bytes to find the same color channel of the pixel at the same position in the next line. 

      .. cpp:function:: bool OpenColorIO::PlanarImageDesc::isRGBAPacked() const override

         Is the image buffer in packed mode with the 4 color channels? (“Packed” here means that XStrideBytes is 4x the bytes per channel, so it is more specific than simply any `PackedImageDesc`_.) 

      .. cpp:function:: bool OpenColorIO::PlanarImageDesc::isFloat() const override

         Is the image buffer 32-bit float? 

   .. group-tab:: Python

      .. py:method:: PlanarImageDesc.getRData(self: PyOpenColorIO.PlanarImageDesc) -> array

      .. py:method:: PlanarImageDesc.getGData(self: PyOpenColorIO.PlanarImageDesc) -> array

      .. py:method:: PlanarImageDesc.getBData(self: PyOpenColorIO.PlanarImageDesc) -> array

      .. py:method:: PlanarImageDesc.getAData(self: PyOpenColorIO.PlanarImageDesc) -> array
