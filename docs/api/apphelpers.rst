..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Applications Helpers
====================

ColorSpaceMenuHelpers
*********************

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.ColorSpaceHelpers.AddColorSpace

      .. autoclass:: PyOpenColorIO.ColorSpaceMenuParameters
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: AddedColorSpaceIterator

      .. autoclass:: PyOpenColorIO.ColorSpaceMenuHelper
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: ColorSpaceLevelIterator

      .. autoclass:: PyOpenColorIO.ColorSpaceMenuHelper.ColorSpaceLevelIterator
         :special-members: __getitem__, __iter__, __len__, __next__

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

      .. autofunction:: PyOpenColorIO.DisplayViewHelpers.GetProcessor
      .. autofunction:: PyOpenColorIO.DisplayViewHelpers.GetIdentityProcessor
      .. autofunction:: PyOpenColorIO.DisplayViewHelpers.AddDisplayView
      .. autofunction:: PyOpenColorIO.DisplayViewHelpers.RemoveDisplayView

   .. group-tab:: C++

      .. doxygennamespace:: ${OCIO_NAMESPACE}::DisplayViewHelpers
         :members:
         :undoc-members:

LegacyViewingPipeline
*********************

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.LegacyViewingPipeline
         :members:
         :undoc-members:
         :special-members: __init__, __str__

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

      .. autoclass:: PyOpenColorIO.MixingSlider
         :members:
         :undoc-members:
         :special-members: __init__, __str__

      .. autoclass:: PyOpenColorIO.MixingColorSpaceManager
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: MixingSpaceIterator, MixingEncodingIterator

      .. autoclass:: PyOpenColorIO.MixingColorSpaceManager.MixingSpaceIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.MixingColorSpaceManager.MixingEncodingIterator
         :special-members: __getitem__, __iter__, __len__, __next__

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
