..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ImageDesc
=========

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.ImageDesc
         :members:
         :undoc-members:
         :special-members: __init__

      .. py:data:: AutoStride
         :module: PyOpenColorIO
         :type: int

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ImageDesc
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ImageDesc&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstImageDescRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ImageDescRcPtr

      .. doxygenvariable:: ${OCIO_NAMESPACE}::AutoStride

PackedImageDesc
***************

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.PackedImageDesc
         :members:
         :undoc-members:
         :special-members: __init__
         :inherited-members:

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::PackedImageDesc
         :members:
         :undoc-members:

PlanarImageDesc
***************

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.PlanarImageDesc
         :members:
         :undoc-members:
         :special-members: __init__
         :inherited-members:

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::PlanarImageDesc
         :members:
         :undoc-members:
