..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Look
====

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_look.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Look
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Look&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLookRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::LookRcPtr
