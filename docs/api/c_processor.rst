..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Processor
*********

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class Processor**

The *Processor* represents a specific color transformation which is
the result of :cpp:func:``Config::getProcessor``.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: bool isNoOp() const

      .. cpp:function:: bool hasChannelCrosstalk() const

         True if the image transformation is non-separable. For example,
         if a change in red may also cause a change in green or blue.

      .. cpp:function:: const char *getCacheID() const

      .. cpp:function:: ConstProcessorMetadataRcPtr getProcessorMetadata() const

         The ProcessorMetadata contains technical information such as the
         number of files and looks used in the processor.

      .. cpp:function:: const `FormatMetadata`_ &getFormatMetadata() const

         Get a FormatMetadata containing the top level metadata for the
         processor. For a processor from a CLF file, this corresponds to
         the ProcessList metadata.

      .. cpp:function:: int getNumTransforms() const

         Get the number of transforms that comprise the processor. Each
         transform has a (potentially empty) FormatMetadata.

      .. cpp:function:: const `FormatMetadata`_ &getTransformFormatMetadata(int index) const

         Get a FormatMetadata containing the metadata for a transform
         within the processor. For a processor from a CLF file, this
         corresponds to the metadata associated with an individual
         process node.

      .. cpp:function:: GroupTransformRcPtr createGroupTransform() const

         Return a GroupTransform that contains a copy of the transforms
         that comprise the processor. (Changes to it will not modify the
         original processor.)

      .. cpp:function:: void write(const char *formatName, std::ostream &os) const

         Write the transforms comprising the processor to the stream.
         Writing (as opposed to Baking) is a lossless process. An
         exception is thrown if the processor cannot be losslessly
         written to the specified file format.

      .. cpp:function:: DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const

      .. cpp:function:: bool hasDynamicProperty(DynamicPropertyType type) const

      .. cpp:function:: ConstProcessorRcPtr getOptimizedProcessor(OptimizationFlags oFlags) const

         Run the optimizer on a Processor to create a new
         :cpp:class:``Processor``. It is usually not necessary to call
         this since getting a CPUProcessor or GPUProcessor will also
         optimize. However if you need both, calling this method first
         makes getting a CPU and GPU Processor faster since the
         optimization is effectively only done once.

      .. cpp:function:: ConstProcessorRcPtr getOptimizedProcessor(BitDepth inBD, BitDepth outBD, OptimizationFlags oFlags) const

         Create a :cpp:class:``Processor`` that is optimized for a
         specific in and out bit-depth (as CPUProcessor would do). This
         method is provided primarily for diagnostic purposes.

      .. cpp:function:: ConstGPUProcessorRcPtr getDefaultGPUProcessor() const

         Get an optimized GPUProcessor instance.

      .. cpp:function:: ConstGPUProcessorRcPtr getOptimizedGPUProcessor(OptimizationFlags oFlags) const

      .. cpp:function:: ConstCPUProcessorRcPtr getDefaultCPUProcessor() const

         Get an optimized :cpp:class:``CPUProcessor`` instance.

         ::

            OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

            OCIO::ConstProcessorRcPtr processor
            = config->getProcessor(colorSpace1, colorSpace2);

            OCIO::ConstCPUProcessorRcPtr cpuProcessor
            = processor->getDefaultCPUProcessor();

            OCIO::PackedImageDesc img(imgDataPtr, imgWidth, imgHeight, imgChannels);
            cpuProcessor->apply(img);


         **Note**
         This may provide higher fidelity than anticipated due to
         internal optimizations. For example, if the inputColorSpace
         and the outputColorSpace are members of the same family, no
         conversion will be applied, even though strictly speaking
         quantization should be added.

         **Note**
         The typical use case to apply color processing to an image
         is:

      .. cpp:function:: ConstCPUProcessorRcPtr getOptimizedCPUProcessor(OptimizationFlags oFlags) const

      .. cpp:function:: ConstCPUProcessorRcPtr getOptimizedCPUProcessor(BitDepth inBitDepth, BitDepth outBitDepth, OptimizationFlags oFlags) const
      

      .. cpp:function:: ~Processor()

      -[ Public Static Functions ]-

      .. cpp:function:: int getNumWriteFormats()

         Get the number of writers.

      .. cpp:function:: const char *getFormatNameByIndex(int index)

         Get the writer at index, return empty string if an invalid index
         is specified.

      .. cpp:function:: const char *getFormatExtensionByIndex(int index)

   .. group-tab:: Python

      .. py:class:: TransformFormatMetadataIterator

      .. py:class:: WriteFormatIterator

      .. py:method:: createGroupTransform(self: PyOpenColorIO.Processor) -> `PyOpenColorIO.GroupTransform`_

      .. py:method:: getCacheID(self: PyOpenColorIO.Processor) -> str

      .. py:method:: getDefaultCPUProcessor(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::CPUProcessor

      .. py:method:: getDefaultGPUProcessor(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::GPUProcessor

      .. py:method:: getDynamicProperty(self: PyOpenColorIO.Processor, type: PyOpenColorIO.DynamicPropertyType) -> `PyOpenColorIO.DynamicProperty`_

      .. py:method:: getFormatMetadata(self: PyOpenColorIO.Processor) -> `PyOpenColorIO.FormatMetadata`_

      .. py:method:: getOptimizedCPUProcessor(*args,**kwargs)

         Overloaded function.

         1. ..py:method:: getOptimizedCPUProcessor(self: PyOpenColorIO.Processor, oFlags: PyOpenColorIO.OptimizationFlags) -> OpenColorIO_v2_0dev::CPUProcessor

         2. ..py:method:: getOptimizedCPUProcessor(self: PyOpenColorIO.Processor, inBitDepth: PyOpenColorIO.BitDepth, outBitDepth: PyOpenColorIO.BitDepth, oFlags: PyOpenColorIO.OptimizationFlags) -> OpenColorIO_v2_0dev::CPUProcessor

      .. py:method:: getOptimizedGPUProcessor(self: PyOpenColorIO.Processor, oFlags: PyOpenColorIO.OptimizationFlags) -> OpenColorIO_v2_0dev::GPUProcessor

      .. py:method:: getOptimizedProcessor(*args,**kwargs)

         Overloaded function.

         1. ..py:method:: getOptimizedProcessor(self: PyOpenColorIO.Processor, oFlags: PyOpenColorIO.OptimizationFlags) -> PyOpenColorIO.Processor

         2. ..py:method:: getOptimizedProcessor(self: PyOpenColorIO.Processor, inBitDepth: PyOpenColorIO.BitDepth, outBitDepth: PyOpenColorIO.BitDepth, oFlags: PyOpenColorIO.OptimizationFlags) -> PyOpenColorIO.Processor

      .. py:method:: getProcessorMetadata(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::ProcessorMetadata

      .. py:method:: getTransformFormatMetadata(self: PyOpenColorIO.Processor) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Processor>, 1>

      .. py:method:: static getWriteFormats() -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Processor>, 0>

      .. py:method:: hasChannelCrosstalk(self: PyOpenColorIO.Processor) -> bool

      .. py:method:: hasDynamicProperty(self: PyOpenColorIO.Processor, type: PyOpenColorIO.DynamicPropertyType) -> bool

      .. py:method:: isNoOp(self: PyOpenColorIO.Processor) -> bool

      .. py:method:: write(*args,**kwargs)

         Overloaded function.

         1. ..py:method:: write(self: PyOpenColorIO.Processor, formatName: str,
         fileName: str) -> None

         2. ..py:method:: write(self: PyOpenColorIO.Processor, formatName: str) -> str


   ProcessorMetadata
   *****************

   **class ProcessorMetadata**

   This class contains meta information about the process that
   generated this processor. The results of these functions do not
   impact the pixel processing.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: int getNumFiles() const

      .. cpp:function:: const char *getFile(int index) const

      .. cpp:function:: int getNumLooks() const

      .. cpp:function:: const char *getLook(int index) const

      .. cpp:function:: void addFile(const char *fname)

      .. cpp:function:: void addLook(const char *look)

      .. cpp:function:: ~ProcessorMetadata()

      .. cpp:function:: ProcessorMetadataRcPtr Create()

   .. group-tab:: Python

      .. py:class:: FileIterator

      .. py:class:: LookIterator

      .. py:method:: addFile(self: PyOpenColorIO.ProcessorMetadata, fileName: str) -> None

      .. py:method:: addLook(self: PyOpenColorIO.ProcessorMetadata, look: str) -> None

      .. py:method:: getFiles(self: PyOpenColorIO.ProcessorMetadata) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ProcessorMetadata>, 0>

      .. py:method:: getLooks(self: PyOpenColorIO.ProcessorMetadata) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ProcessorMetadata>, 1>
