..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ColorSpace
==========

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.ColorSpace
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: ColorSpaceCategoryIterator, ColorSpaceAliasIterator

      .. autoclass:: PyOpenColorIO.ColorSpace.ColorSpaceCategoryIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.ColorSpace.ColorSpaceAliasIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ColorSpace
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ColorSpace&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstColorSpaceRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ColorSpaceRcPtr

ColorSpaceSet
=============

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.ColorSpaceSet
         :members:
         :undoc-members:
         :special-members: __init__, __eq__, __ne__, __sub__, __or__, __and__
         :exclude-members: ColorSpaceNameIterator, ColorSpaceIterator

      .. autoclass:: PyOpenColorIO.ColorSpaceSet.ColorSpaceNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.ColorSpaceSet.ColorSpaceIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ColorSpaceSet
         :members:
         :undoc-members:

      .. doxygengroup:: ColorSpaceSetOperators
         :content-only:
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstColorSpaceSetRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ColorSpaceSetRcPtr
