..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _grading_transforms:

Grading Transforms
==================

GradingPrimaryTransform
***********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingprimarytransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::GradingPrimaryTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingPrimaryTransform&) noexcept

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstGradingPrimaryTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::GradingPrimaryTransformRcPtr

GradingPrimary
^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingprimary.rst

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingPrimary
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingPrimary&)

GradingRGBM
^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingrgbm.rst

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingRGBM
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingRGBM&)

GradingRGBCurveTransform
************************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingrgbcurvetransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::GradingRGBCurveTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingRGBCurveTransform&) noexcept

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstGradingRGBCurveTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::GradingRGBCurveTransformRcPtr

GradingRGBCurve
^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingrgbcurve.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::GradingRGBCurve
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingRGBCurve&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstGradingRGBCurveRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::GradingRGBCurveRcPtr

GradingControlPoint
^^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingcontrolpoint.rst

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingControlPoint
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingControlPoint&)

GradingBSplineCurve
^^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingbsplinecurve.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::GradingBSplineCurve
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingBSplineCurve&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstGradingBSplineCurveRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::GradingBSplineCurveRcPtr

GradingToneTransform
********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingtonetransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::GradingToneTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingToneTransform&) noexcept

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstGradingToneTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::GradingToneTransformRcPtr

GradingTone
^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingtone.rst

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingTone
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingTone&)

GradingRGBMSW
^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_gradingrgbmsw.rst

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingRGBMSW
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingRGBMSW&)
