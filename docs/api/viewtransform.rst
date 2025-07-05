..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ViewTransform
=============

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.ViewTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: ViewTransformCategoryIterator

      .. autoclass:: PyOpenColorIO.ViewTransform.ViewTransformCategoryIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ViewTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ViewTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstViewTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ViewTransformRcPtr
