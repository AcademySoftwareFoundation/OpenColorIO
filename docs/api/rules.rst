..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Rules
=====

FileRules
*********

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.FileRules
         :members:
         :undoc-members:
         :special-members: __init__

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

      .. autoclass:: PyOpenColorIO.ViewingRules
         :members:
         :undoc-members:
         :special-members: __init__
         :exclude-members: ViewingRuleColorSpaceIterator, ViewingRuleEncodingIterator

      .. autoclass:: PyOpenColorIO.ViewingRules.ViewingRuleColorSpaceIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.ViewingRules.ViewingRuleEncodingIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ViewingRules
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ViewingRules&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstViewingRulesRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ViewingRulesRcPtr
