..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ViewTransform
=============

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_viewtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ViewTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ViewTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstViewTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ViewTransformRcPtr
