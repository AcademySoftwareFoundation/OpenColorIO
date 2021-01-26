..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Rules
=====

FileRules
*********

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_filerules.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::FileRules
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const FileRules&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstFileRulesRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::FileRulesRcPtr

ViewingRules
************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_viewingrules.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ViewingRules
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ViewingRules&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstViewingRulesRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ViewingRulesRcPtr
