..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Named Transform
===============

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.NamedTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: NamedTransformCategoryIterator

      .. autoclass:: PyOpenColorIO.NamedTransform.NamedTransformCategoryIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::NamedTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const NamedTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstNamedTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::NamedTransformRcPtr
