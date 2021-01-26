..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Applications Helpers
====================

ColorSpaceMenuHelpers
*********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_colorspacehelpers.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ColorSpaceMenuParameters
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ColorSpaceMenuParameters&)

      .. doxygenclass:: ${OCIO_NAMESPACE}::ColorSpaceMenuHelper
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ColorSpaceMenuHelper&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstColorSpaceMenuHelperRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ColorSpaceMenuHelperRcPtr

DisplayViewHelpers
******************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_displayviewhelpers.rst

   .. group-tab:: C++

      .. doxygennamespace:: ${OCIO_NAMESPACE}::DisplayViewHelpers
         :members:
         :undoc-members:

LegacyViewingPipeline
*********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_legacyviewingpipeline.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::LegacyViewingPipeline
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const LegacyViewingPipeline&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLegacyViewingPipelineRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::LegacyViewingPipelineRcPtr

MixingHelpers
*************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_mixinghelpers.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::MixingSlider
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const MixingSlider&)

      .. doxygenclass:: ${OCIO_NAMESPACE}::MixingColorSpaceManager
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const MixingColorSpaceManager&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstMixingColorSpaceManagerRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::MixingColorSpaceManagerRcPtr
