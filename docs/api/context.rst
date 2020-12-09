..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Context
=======

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_context.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Context
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Context&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstContextRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ContextRcPtr
