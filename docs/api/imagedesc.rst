..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ImageDesc
=========

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_imagedesc.rst

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

      .. include:: python/${PYDIR}/pyopencolorio_packedimagedesc.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::PackedImageDesc
         :members:
         :undoc-members:

PlanarImageDesc
***************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_planarimagedesc.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::PlanarImageDesc
         :members:
         :undoc-members:
