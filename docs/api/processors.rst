..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Processors
==========

Processor
*********

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_processor.rst

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

      .. include:: python/${PYDIR}/pyopencolorio_cpuprocessor.rst

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

      .. include:: python/${PYDIR}/pyopencolorio_gpuprocessor.rst

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

      .. include:: python/${PYDIR}/pyopencolorio_processormetadata.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ProcessorMetadata
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstProcessorMetadataRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ProcessorMetadataRcPtr
