..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Processors
==========

Processor
*********

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.Processor
         :members:
         :undoc-members:
         :special-members: __init__
         :exclude-members: TransformFormatMetadataIterator, WriteFormatIterator

      .. autoclass:: PyOpenColorIO.Processor.TransformFormatMetadataIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Processor
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstProcessorRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ProcessorRcPtr

CPUProcessor
************

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.CPUProcessor
         :members:
         :undoc-members:
         :special-members: __init__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::CPUProcessor
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstCPUProcessorRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::CPUProcessorRcPtr

GPUProcessor
************

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.GPUProcessor
         :members:
         :undoc-members:
         :special-members: __init__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::GPUProcessor
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstGPUProcessorRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::GPUProcessorRcPtr

ProcessorMetadata
*****************

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.ProcessorMetadata
         :members:
         :undoc-members:
         :special-members: __init__
         :exclude-members: FileIterator, LookIterator

      .. autoclass:: PyOpenColorIO.ProcessorMetadata.FileIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.ProcessorMetadata.LookIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ProcessorMetadata
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstProcessorMetadataRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ProcessorMetadataRcPtr
