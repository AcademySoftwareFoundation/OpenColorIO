
Processor
*********

.. class:: Processor

   The *Processor* represents a specific color transformation which is the result of :cpp:func:`Config::getProcessor`. 


Methods
=======

.. tabs::

   .. group-tab:: C++


      Warning: doxygenfunction: Cannot find function “OpenColorIO::Processor::getWriteFormats” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: bool OpenColorIO::Processor::isNoOp() const

      .. cpp:function:: bool OpenColorIO::Processor::hasChannelCrosstalk() const

         True if the image transformation is non-separable. For example, if a change in red may also cause a change in green or blue. 

      .. cpp:function:: const char *OpenColorIO::Processor::getCacheID() const

      .. cpp:function:: ConstProcessorMetadataRcPtr OpenColorIO::Processor::getProcessorMetadata() const

         The ProcessorMetadata contains technical information such as the number of files and looks used in the processor. 

      .. cpp:function:: const `FormatMetadata`_ &OpenColorIO::Processor::getFormatMetadata() const

         Get a `FormatMetadata` containing the top level metadata for the processor. For a processor from a CLF file, this corresponds to the ProcessList metadata. 

      .. cpp:function:: const `FormatMetadata`_ &OpenColorIO::Processor::getTransformFormatMetadata(int index) const

         Get a `FormatMetadata` containing the metadata for a transform within the processor. For a processor from a CLF file, this corresponds to the metadata associated with an individual process node. 

      .. cpp:function:: GroupTransformRcPtr OpenColorIO::Processor::createGroupTransform() const

         Return a GroupTransform that contains a copy of the transforms that comprise the processor. (Changes to it will not modify the original processor.) 

      .. cpp:function:: void OpenColorIO::Processor::write(const char *formatName, std::ostream &os) const

         Write the transforms comprising the processor to the stream. Writing (as opposed to Baking) is a lossless process. An exception is thrown if the processor cannot be losslessly written to the specified file format. 

      .. cpp:function:: DynamicPropertyRcPtr OpenColorIO::Processor::getDynamicProperty(DynamicPropertyType type) const

      .. cpp:function:: bool OpenColorIO::Processor::hasDynamicProperty(DynamicPropertyType type) const

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Processor::getOptimizedProcessor” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - ConstProcessorRcPtr getOptimizedProcessor(BitDepth inBD, BitDepth outBD, OptimizationFlags oFlags) const
            - ConstProcessorRcPtr getOptimizedProcessor(OptimizationFlags oFlags) const

      **CPU**

      .. cpp:function:: ConstGPUProcessorRcPtr OpenColorIO::Processor::getDefaultGPUProcessor() const

         Get an optimized `GPUProcessor` instance. 

      .. cpp:function:: ConstGPUProcessorRcPtr OpenColorIO::Processor::getOptimizedGPUProcessor(OptimizationFlags oFlags) const

      **GPU**

      .. cpp:function:: ConstCPUProcessorRcPtr OpenColorIO::Processor::getDefaultCPUProcessor() const

         Get an optimized :cpp:class:`CPUProcessor` instance.

         ::

            OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

            OCIO::ConstProcessorRcPtr processor
               = config->getProcessor(colorSpace1, colorSpace2);

            OCIO::ConstCPUProcessorRcPtr cpuProcessor
               = processor->getDefaultCPUProcessor();

            OCIO::PackedImageDesc img(imgDataPtr, imgWidth, imgHeight, imgChannels);
            cpuProcessor->apply(img);

         

         **Note**
            This may provide higher fidelity than anticipated due to internal optimizations. For example, if the inputColorSpace and the outputColorSpace are members of the same family, no conversion will be applied, even though strictly speaking quantization should be added.

         **Note**
            The typical use case to apply color processing to an image is:

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Processor::getOptimizedCPUProcessor” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - ConstCPUProcessorRcPtr getOptimizedCPUProcessor(BitDepth inBitDepth, BitDepth outBitDepth, OptimizationFlags oFlags) const
            - ConstCPUProcessorRcPtr getOptimizedCPUProcessor(OptimizationFlags oFlags) const

   .. group-tab:: Python

      .. py:method:: Processor.getWriteFormats() -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Processor>, 0>

      .. py:method:: Processor.isNoOp(self: PyOpenColorIO.Processor) -> bool

      .. py:method:: Processor.hasChannelCrosstalk(self: PyOpenColorIO.Processor) -> bool

      .. py:method:: Processor.getCacheID(self: PyOpenColorIO.Processor) -> str

      .. py:method:: Processor.getProcessorMetadata(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::ProcessorMetadata

      .. py:method:: Processor.getFormatMetadata(self: PyOpenColorIO.Processor) -> PyOpenColorIO.FormatMetadata

      .. py:method:: Processor.getTransformFormatMetadata(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Processor>, 1>

      .. py:method:: Processor.createGroupTransform(self: PyOpenColorIO.Processor) -> `PyOpenColorIO.GroupTransform`

      .. py:method:: Processor.write(*args, **kwargs)

         Overloaded function.

         1. write(self: PyOpenColorIO.Processor, formatName: str, fileName: str) -> None

         2. write(self: PyOpenColorIO.Processor, formatName: str) -> str

      .. py:method:: Processor.getDynamicProperty(self: PyOpenColorIO.Processor, type: PyOpenColorIO.DynamicPropertyType) -> PyOpenColorIO.DynamicProperty

      .. py:method:: Processor.hasDynamicProperty(self: PyOpenColorIO.Processor, type: PyOpenColorIO.DynamicPropertyType) -> bool

      .. py:method:: Processor.getOptimizedProcessor(*args, **kwargs)

         Overloaded function.

         1. getOptimizedProcessor(self: PyOpenColorIO.Processor, oFlags: PyOpenColorIO.OptimizationFlags) -> PyOpenColorIO.Processor

         2. getOptimizedProcessor(self: PyOpenColorIO.Processor, inBitDepth: PyOpenColorIO.BitDepth, outBitDepth: PyOpenColorIO.BitDepth, oFlags: PyOpenColorIO.OptimizationFlags) -> PyOpenColorIO.Processor

      **CPU**

      .. py:method:: Processor.getDefaultGPUProcessor(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::GPUProcessor

      .. py:method:: Processor.getOptimizedGPUProcessor(self: PyOpenColorIO.Processor, oFlags: PyOpenColorIO.OptimizationFlags) -> OpenColorIO_v2_0dev::GPUProcessor

      **GPU**

      .. py:method:: Processor.getDefaultCPUProcessor(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::CPUProcessor

      .. py:method:: Processor.getOptimizedCPUProcessor(*args, **kwargs)

         Overloaded function.

         1. getOptimizedCPUProcessor(self: PyOpenColorIO.Processor, oFlags: PyOpenColorIO.OptimizationFlags) -> OpenColorIO_v2_0dev::CPUProcessor

         2. getOptimizedCPUProcessor(self: PyOpenColorIO.Processor, inBitDepth: PyOpenColorIO.BitDepth, outBitDepth: PyOpenColorIO.BitDepth, oFlags: PyOpenColorIO.OptimizationFlags) -> OpenColorIO_v2_0dev::CPUProcessor


ProcessorMetadata
*****************

.. class:: ProcessorMetadata

   This class contains meta information about the process that generated this processor. The results of these functions do not impact the pixel processing. 


Methods
=======

.. tabs::

   .. group-tab:: C++

      Warning: doxygenfunction: Cannot find function “OpenColorIO::ProcessorMetadata::getFiles” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::ProcessorMetadata::getLooks” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::ProcessorMetadata::addFile(const char *fname)

      .. cpp:function:: void OpenColorIO::ProcessorMetadata::addLook(const char *look)

   .. group-tab:: Python

      .. py:method:: ProcessorMetadata.getFiles(self: PyOpenColorIO.ProcessorMetadata) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ProcessorMetadata>, 0>

      .. py:method:: ProcessorMetadata.getLooks(self: PyOpenColorIO.ProcessorMetadata) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ProcessorMetadata>, 1>

      .. py:method:: ProcessorMetadata.addFile(self: PyOpenColorIO.ProcessorMetadata, fileName: str) -> None

      .. py:method:: ProcessorMetadata.addLook(self: PyOpenColorIO.ProcessorMetadata, look: str) -> None
