..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

GPUProcessor
************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class GPUProcessor**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: bool isNoOp() const

      .. cpp:function:: bool hasChannelCrosstalk() const

      .. cpp:function:: const char *getCacheID() const

      .. cpp:function:: DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const

         The returned pointer may be used to set the value of any dynamic
         properties of the requested type. Throws if the requested
         property is not found. Note that if the processor contains
         several ops that support the requested property, only ones for
         which dynamic has been enabled will be controlled.

         **Note**
         The dynamic properties in this object are decoupled from the
         ones in the :cpp:class:``Processor`` it was generated from.

      .. cpp:function:: void extractGpuShaderInfo(GpuShaderDescRcPtr &shaderDesc) const
      

         Extract & Store the shader information to implement the color
         processing.

      .. cpp:function:: void extractGpuShaderInfo(GpuShaderCreatorRcPtr &shaderCreator) const

         Extract the shader information using a custom GpuShaderCreator
         class.

      .. cpp:function:: ~GPUProcessor()


   .. group-tab:: Python


      .. py:class:: PyOpenColorIO.GPUProcessor

      .. py:function:: extractGpuShaderInfo(*args,**kwargs)

         Overloaded function.

         1. .. py:function:: extractGpuShaderInfo(self: PyOpenColorIO.GPUProcessor, shaderDesc: OpenColorIO_v2_0dev::GpuShaderDesc) -> None

         2. .. py:function:: extractGpuShaderInfo(self: PyOpenColorIO.GPUProcessor, shaderCreator: OpenColorIO_v2_0dev::GpuShaderCreator) -> None

      .. py:function:: getCacheID(self: PyOpenColorIO.GPUProcessor) -> str

      .. py:function:: getDynamicProperty(self: PyOpenColorIO.GPUProcessor, type: PyOpenColorIO.DynamicPropertyType) -> `PyOpenColorIO.DynamicProperty`_

      .. py:function:: hasChannelCrosstalk(self: PyOpenColorIO.GPUProcessor) -> bool

      .. py:function:: isNoOp(self: PyOpenColorIO.GPUProcessor) -> bool
