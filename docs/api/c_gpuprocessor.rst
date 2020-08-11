
GPUProcessor
************

.. class:: GPUProcessor


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: bool OpenColorIO::GPUProcessor::isNoOp() const

      .. cpp:function:: bool OpenColorIO::GPUProcessor::hasChannelCrosstalk() const

      .. cpp:function:: const char *OpenColorIO::GPUProcessor::getCacheID() const

      .. cpp:function:: DynamicPropertyRcPtr OpenColorIO::GPUProcessor::getDynamicProperty(DynamicPropertyType type) const

         The returned pointer may be used to set the value of any dynamic properties of the requested type. Throws if the requested property is not found. Note that if the processor contains several ops that support the requested property, only ones for which dynamic has been enabled will be controlled.

         **Note**
            The dynamic properties in this object are decoupled from the ones in the :cpp:class:`Processor` it was generated from. 

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::GPUProcessor::extractGpuShaderInfo” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - void extractGpuShaderInfo(GpuShaderCreatorRcPtr &shaderCreator) const
            - void extractGpuShaderInfo(GpuShaderDescRcPtr &shaderDesc) const

   .. group-tab:: Python

      .. py:method:: GPUProcessor.isNoOp(self: PyOpenColorIO.GPUProcessor) -> bool

      .. py:method:: GPUProcessor.hasChannelCrosstalk(self: PyOpenColorIO.GPUProcessor) -> bool

      .. py:method:: GPUProcessor.getCacheID(self: PyOpenColorIO.GPUProcessor) -> str

      .. py:method:: GPUProcessor.getDynamicProperty(self: PyOpenColorIO.GPUProcessor, type: PyOpenColorIO.DynamicPropertyType) -> PyOpenColorIO.DynamicProperty

      .. py:method:: GPUProcessor.extractGpuShaderInfo(*args, **kwargs)

         Overloaded function.

         1. extractGpuShaderInfo(self: PyOpenColorIO.GPUProcessor, shaderDesc: OpenColorIO_v2_0dev::GpuShaderDesc) -> None

         2. extractGpuShaderInfo(self: PyOpenColorIO.GPUProcessor, shaderCreator: OpenColorIO_v2_0dev::GpuShaderCreator) -> None
