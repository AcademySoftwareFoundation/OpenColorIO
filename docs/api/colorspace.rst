..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ColorSpace
==========

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_colorspace.rst

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

      .. include:: python/${PYDIR}/pyopencolorio_colorspaceset.rst

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
