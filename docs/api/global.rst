..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Global
======

Caching
*******

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.ClearAllCaches

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ClearAllCaches

Constants: :ref:`vars_caches`

Version
*******

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.GetVersion

      .. autofunction:: PyOpenColorIO.GetVersionHex

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetVersion

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetVersionHex

Logging
*******

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.GetLoggingLevel

      .. autofunction:: PyOpenColorIO.SetLoggingLevel

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetLoggingLevel

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetLoggingLevel

Logging Function
^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.SetLoggingFunction

      .. autofunction:: PyOpenColorIO.ResetToDefaultLoggingFunction

      .. autofunction:: PyOpenColorIO.LogMessage

   .. group-tab:: C++

      .. doxygentypedef:: ${OCIO_NAMESPACE}::LoggingFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetLoggingFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ResetToDefaultLoggingFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::LogMessage

Compute Hash Function
*********************

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.SetComputeHashFunction

      .. autofunction:: PyOpenColorIO.ResetComputeHashFunction

   .. group-tab:: C++

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ComputeHashFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetComputeHashFunction

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ResetComputeHashFunction

Environment Variables
*********************

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.GetEnvVariable

      .. autofunction:: PyOpenColorIO.SetEnvVariable

      .. autofunction:: PyOpenColorIO.UnsetEnvVariable

      .. autofunction:: PyOpenColorIO.IsEnvVariablePresent

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

      .. autofunction:: PyOpenColorIO.BoolToString

      .. autofunction:: PyOpenColorIO.BoolFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BoolToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::BoolFromString

.. _conversion_logging_level:

LoggingLevel
^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.LoggingLevelToString

      .. autofunction:: PyOpenColorIO.LoggingLevelFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::LoggingLevelToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::LoggingLevelFromString

.. _conversion_transform_direction:

TransformDirection
^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.TransformDirectionToString

      .. autofunction:: PyOpenColorIO.TransformDirectionFromString

      .. autofunction:: PyOpenColorIO.GetInverseTransformDirection

      .. autofunction:: PyOpenColorIO.CombineTransformDirections

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

      .. autofunction:: PyOpenColorIO.BitDepthToString

      .. autofunction:: PyOpenColorIO.BitDepthFromString

      .. autofunction:: PyOpenColorIO.BitDepthIsFloat

      .. autofunction:: PyOpenColorIO.BitDepthToInt

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

      .. autofunction:: PyOpenColorIO.AllocationToString

      .. autofunction:: PyOpenColorIO.AllocationFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::AllocationToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::AllocationFromString

.. _conversion_interpolation:

Interpolation
^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.InterpolationToString

      .. autofunction:: PyOpenColorIO.InterpolationFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::InterpolationToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::InterpolationFromString

.. _conversion_gpu_language:

GpuLanguage
^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.GpuLanguageToString

      .. autofunction:: PyOpenColorIO.GpuLanguageFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GpuLanguageToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GpuLanguageFromString

.. _conversion_environment_mode:

EnvironmentMode
^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.EnvironmentModeToString

      .. autofunction:: PyOpenColorIO.EnvironmentModeFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::EnvironmentModeToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::EnvironmentModeFromString

.. _conversion_cdl_style:

CDLStyle
^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.CDLStyleToString

      .. autofunction:: PyOpenColorIO.CDLStyleFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::CDLStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::CDLStyleFromString

.. _conversion_range_style:

RangeStyle
^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.RangeStyleToString

      .. autofunction:: PyOpenColorIO.RangeStyleFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::RangeStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::RangeStyleFromString

.. _conversion_fixed_function_style:

FixedFunctionStyle
^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.FixedFunctionStyleToString

      .. autofunction:: PyOpenColorIO.FixedFunctionStyleFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::FixedFunctionStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::FixedFunctionStyleFromString

.. _conversion_grading_style:

GradingStyle
^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.GradingStyleToString

      .. autofunction:: PyOpenColorIO.GradingStyleFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GradingStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GradingStyleFromString

.. _conversion_exposure_contrast_style:

ExposureContrastStyle
^^^^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.ExposureContrastStyleToString

      .. autofunction:: PyOpenColorIO.ExposureContrastStyleFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ExposureContrastStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::ExposureContrastStyleFromString

.. _conversion_negative_style:

NegativeStyle
^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autofunction:: PyOpenColorIO.NegativeStyleToString

      .. autofunction:: PyOpenColorIO.NegativeStyleFromString

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::NegativeStyleToString

      .. doxygenfunction:: ${OCIO_NAMESPACE}::NegativeStyleFromString
