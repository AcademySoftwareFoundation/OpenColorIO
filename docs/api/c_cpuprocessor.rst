
CPUProcessor
************

.. class:: CPUProcessor


Methods
=======

.. tabs::

   .. group-tab:: C++

      **Metadata**

      .. cpp:function:: bool OpenColorIO::CPUProcessor::isNoOp() const

         The in and out bit-depths must be equal for isNoOp to be true. 

      .. cpp:function:: bool OpenColorIO::CPUProcessor::isIdentity() const

         Equivalent to isNoOp from the underlying `Processor`, i.e., it ignores in/out bit-depth differences. 

      .. cpp:function:: bool OpenColorIO::CPUProcessor::hasChannelCrosstalk() const

      .. cpp:function:: const char *OpenColorIO::CPUProcessor::getCacheID() const

      .. cpp:function:: BitDepth OpenColorIO::CPUProcessor::getInputBitDepth() const

         Bit-depth of the input pixel buffer. 

      .. cpp:function:: BitDepth OpenColorIO::CPUProcessor::getOutputBitDepth() const

         Bit-depth of the output pixel buffer. 

      .. cpp:function:: DynamicPropertyRcPtr OpenColorIO::CPUProcessor::getDynamicProperty(DynamicPropertyType type) const

         Refer to `GPUProcessor::getDynamicProperty`. 

      **Apply**

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::CPUProcessor::apply” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - void apply(ImageDesc &imgDesc) const
            - void apply(const ImageDesc &srcImgDesc, ImageDesc &dstImgDesc) const

      .. cpp:function:: void OpenColorIO::CPUProcessor::applyRGB(float *pixel) const

         Apply to a single pixel respecting that the input and output bit-depths be 32-bit float and the image buffer be packed RGB/RGBA.

         **Note**
            This is not as efficient as applying to an entire image at once. If you are processing multiple pixels, and have the flexibility, use the above function instead. 

      .. cpp:function:: void OpenColorIO::CPUProcessor::applyRGBA(float *pixel) const

   .. group-tab:: Python

      **Metadata**

      .. py:method:: CPUProcessor.isNoOp(self: PyOpenColorIO.CPUProcessor) -> bool

      .. py:method:: CPUProcessor.isIdentity(self: PyOpenColorIO.CPUProcessor) -> bool

      .. py:method:: CPUProcessor.hasChannelCrosstalk(self: PyOpenColorIO.CPUProcessor) -> bool

      .. py:method:: CPUProcessor.getCacheID(self: PyOpenColorIO.CPUProcessor) -> str

      .. py:method:: CPUProcessor.getInputBitDepth(self: PyOpenColorIO.CPUProcessor) -> PyOpenColorIO.BitDepth

      .. py:method:: CPUProcessor.getOutputBitDepth(self: PyOpenColorIO.CPUProcessor) -> PyOpenColorIO.BitDepth

      .. py:method:: CPUProcessor.getDynamicProperty(self: PyOpenColorIO.CPUProcessor, type: PyOpenColorIO.DynamicPropertyType) -> PyOpenColorIO.DynamicProperty

      **Apply**

      .. py:method:: CPUProcessor.apply(*args, **kwargs)

         Overloaded function.

         1. apply(self: PyOpenColorIO.CPUProcessor, imgDesc: OpenColorIO_v2_0dev::PyImageDesc) -> None

         2. apply(self: PyOpenColorIO.CPUProcessor, srcImgDesc: OpenColorIO_v2_0dev::PyImageDesc, dstImgDesc: OpenColorIO_v2_0dev::PyImageDesc) -> None

      .. py:method:: CPUProcessor.applyRGB(*args, **kwargs)

         Overloaded function.

         1. applyRGB(self: PyOpenColorIO.CPUProcessor, pixel: buffer) -> buffer

         2. applyRGB(self: PyOpenColorIO.CPUProcessor, pixel: List[float]) -> List[float]

      .. py:method:: CPUProcessor.applyRGBA(*args, **kwargs)

         Overloaded function.

         1. applyRGBA(self: PyOpenColorIO.CPUProcessor, pixel: buffer) -> buffer

         2. applyRGBA(self: PyOpenColorIO.CPUProcessor, pixel: List[float]) -> List[float]
