..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Look
====

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.Look
         :members:
         :undoc-members:
         :special-members: __init__, __str__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Look
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Look&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLookRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::LookRcPtr
