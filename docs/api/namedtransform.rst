..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Named Transform
===============

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_namedtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::NamedTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const NamedTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstNamedTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::NamedTransformRcPtr
