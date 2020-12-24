..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Global
======

Caching
*******

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_clearallcaches.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ClearAllCaches

Constants: :ref:`vars_caches`

Version
*******

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_getversion.rst

      .. include:: python/${PYDIR}/pyopencolorio_getversionhex.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetVersion

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetVersionHex

Logging
*******

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_getlogginglevel.rst

      .. include:: python/${PYDIR}/pyopencolorio_setlogginglevel.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetLoggingLevel

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetLoggingLevel

Logging Function
^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_setloggingfunction.rst

      .. include:: python/${PYDIR}/pyopencolorio_resettodefaultloggingfunction.rst

      .. include:: python/${PYDIR}/pyopencolorio_logmessage.rst

   .. group-tab:: C++

      .. doxygentypedef:: ${OCIO_NAMESPACE}::LoggingFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetLoggingFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ResetToDefaultLoggingFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::LogMessage

Compute Hash Function
*********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_setcomputehashfunction.rst

      .. include:: python/${PYDIR}/pyopencolorio_resetcomputehashfunction.rst

   .. group-tab:: C++

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ComputeHashFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetComputeHashFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ResetComputeHashFunction

Environment Variables
*********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_getenvvariable.rst

      .. include:: python/${PYDIR}/pyopencolorio_setenvvariable.rst

      .. include:: python/${PYDIR}/pyopencolorio_unsetenvvariable.rst

      .. include:: python/${PYDIR}/pyopencolorio_isenvvariablepresent.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetEnvVariable

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetEnvVariable

      .. doxygenfunction:: ${OCIO_NAMESPACE}::UnsetEnvVariable

      .. doxygenfunction:: ${OCIO_NAMESPACE}::IsEnvVariablePresent

Constants: :ref:`vars_envvar`

Casting
*******

.. tabs::

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::DynamicPtrCast

Conversions
***********

Bool
^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_booltostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_boolfromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BoolToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BoolFromString

.. _conversion_logging_level:

LoggingLevel
^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_loggingleveltostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_logginglevelfromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::LoggingLevelToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::LoggingLevelFromString

.. _conversion_transform_direction:

TransformDirection
^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_transformdirectiontostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_transformdirectionfromstring.rst

      .. include:: python/${PYDIR}/pyopencolorio_getinversetransformdirection.rst

      .. include:: python/${PYDIR}/pyopencolorio_combinetransformdirections.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::TransformDirectionToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::TransformDirectionFromString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetInverseTransformDirection

      .. doxygenfunction:: ${OCIO_NAMESPACE}::CombineTransformDirections

.. _conversion_bit_depth:

BitDepth
^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_bitdepthtostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_bitdepthfromstring.rst

      .. include:: python/${PYDIR}/pyopencolorio_bitdepthisfloat.rst

      .. include:: python/${PYDIR}/pyopencolorio_bitdepthtoint.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BitDepthToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BitDepthFromString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BitDepthIsFloat

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BitDepthToInt

.. _conversion_allocation:

Allocation
^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_allocationtostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_allocationfromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::AllocationToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::AllocationFromString

.. _conversion_interpolation:

Interpolation
^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_interpolationtostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_interpolationfromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::InterpolationToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::InterpolationFromString

.. _conversion_gpu_language:

GpuLanguage
^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gpulanguagetostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_gpulanguagefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GpuLanguageToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GpuLanguageFromString

.. _conversion_environment_mode:

EnvironmentMode
^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_environmentmodetostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_environmentmodefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::EnvironmentModeToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::EnvironmentModeFromString

.. _conversion_cdl_style:

CDLStyle
^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_cdlstyletostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_cdlstylefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::CDLStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::CDLStyleFromString

.. _conversion_range_style:

RangeStyle
^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_rangestyletostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_rangestylefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::RangeStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::RangeStyleFromString

.. _conversion_fixed_function_style:

FixedFunctionStyle
^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_fixedfunctionstyletostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_fixedfunctionstylefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::FixedFunctionStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::FixedFunctionStyleFromString

.. _conversion_grading_style:

GradingStyle
^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingstyletostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_gradingstylefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GradingStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GradingStyleFromString

.. _conversion_exposure_contrast_style:

ExposureContrastStyle
^^^^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_exposurecontraststyletostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_exposurecontraststylefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ExposureContrastStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ExposureContrastStyleFromString

.. _conversion_negative_style:

NegativeStyle
^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_negativestyletostring.rst

      .. include:: python/${PYDIR}/pyopencolorio_negativestylefromstring.rst

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::NegativeStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::NegativeStyleFromString
