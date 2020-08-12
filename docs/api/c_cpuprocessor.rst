..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

CPUProcessor
************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class CPUProcessor**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: bool isNoOp() const

         The in and out bit-depths must be equal for isNoOp to be true.

      .. cpp:function:: bool isIdentity() const

         Equivalent to isNoOp from the underlying Processor, i.e., it
         ignores in/out bit-depth differences.

      .. cpp:function:: bool hasChannelCrosstalk() const

      .. cpp:function:: const char *getCacheID() const

      .. cpp:function:: BitDepth getInputBitDepth() const

         Bit-depth of the input pixel buffer.

      .. cpp:function:: BitDepth getOutputBitDepth() const

         Bit-depth of the output pixel buffer.

      .. cpp:function:: DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type)
      const**

         Refer to `GPUProcessor::getDynamicProperty`_.

      .. cpp:function:: void apply(ImageDesc &imgDesc) const

         Apply to an image with any kind of channel ordering while
         respecting the input and output bit-depths.

      .. cpp:function:: void apply(const ImageDesc &srcImgDesc, ImageDesc &dstImgDesc)
      const**

      .. cpp:function:: void applyRGB(float *pixel) const

         Apply to a single pixel respecting that the input and output
         bit-depths be 32-bit float and the image buffer be packed
         RGB/RGBA.

         **Note**
         This is not as efficient as applying to an entire image at
         once. If you are processing multiple pixels, and have the
         flexibility, use the above function instead.

      .. cpp:function:: void applyRGBA(float *pixel) const

      .. cpp:function:: ~CPUProcessor()**

   .. group-tab:: Python

      .. py:class:: PyOpenColorIO.CPUProcessor

      .. py:function:: apply(*args,**kwargs)

         Overloaded function.

         1. .. py:function:: apply(self: PyOpenColorIO.CPUProcessor, imgDesc: OpenColorIO_v2_0dev::PyImageDesc) -> None

         2. .. py:function:: apply(self: PyOpenColorIO.CPUProcessor, srcImgDesc: OpenColorIO_v2_0dev::PyImageDesc, dstImgDesc: OpenColorIO_v2_0dev::PyImageDesc) -> None

      .. py:function:: applyRGB(*args,**kwargs)

         Overloaded function.

         1. .. py:function:: applyRGB(self: PyOpenColorIO.CPUProcessor, pixel: buffer) buffer

         2. .. py:function:: applyRGB(self: PyOpenColorIO.CPUProcessor, pix List[float]) -> List[float]

      .. py:function:: applyRGBA(*args,**kwargs)

         Overloaded function.

         1. .. py:function:: applyRGBA(self: PyOpenColorIO.CPUProcessor, pixel: buffer) -> buffer

         2. .. py:function:: applyRGBA(self: PyOpenColorIO.CPUProcessor, pixel: List[float]) -> List[float]

      .. py:method:: getCacheID(self: PyOpenColorIO.CPUProcessor) -> str

      .. py:method:: getDynamicProperty(self: PyOpenColorIO.CPUProcessor, type: PyOpenColorIO.DynamicPropertyType) -> `PyOpenColorIO.DynamicProperty`_

      .. py:method:: getInputBitDepth(self: PyOpenColorIO.CPUProcessor) -> PyOpenColorIO.BitDepth

      .. py:method:: getOutputBitDepth(self: PyOpenColorIO.CPUProcessor) -> PyOpenColorIO.BitDepth

      .. py:method:: hasChannelCrosstalk(self: PyOpenColorIO.CPUProcessor) -> bool

      .. py:method:: isIdentity(self: PyOpenColorIO.CPUProcessor) -> bool

      .. py:method:: isNoOp(self: PyOpenColorIO.CPUProcessor) -> bool
