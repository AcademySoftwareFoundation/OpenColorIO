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

      .. autoclass:: PyOpenColorIO.GradingPrimaryTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.GradingPrimary
         :members:
         :undoc-members:
         :special-members: __init__, __str__

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingPrimary
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingPrimary&)

GradingRGBM
^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.GradingRGBM
         :members:
         :undoc-members:
         :special-members: __init__, __str__

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingRGBM
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingRGBM&)

GradingRGBCurveTransform
************************

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.GradingRGBCurveTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.GradingRGBCurve
         :members:
         :undoc-members:
         :special-members: __init__, __str__

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

      .. autoclass:: PyOpenColorIO.GradingControlPoint
         :members:
         :undoc-members:
         :special-members: __init__, __str__

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingControlPoint
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingControlPoint&)

GradingBSplineCurve
^^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.GradingBSplineCurve
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: GradingControlPointIterator

      .. autoclass:: PyOpenColorIO.GradingBSplineCurve.GradingControlPointIterator
         :special-members: __getitem__, __setitem__, __iter__, __len__, __next__

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

      .. autoclass:: PyOpenColorIO.GradingToneTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.GradingTone
         :members:
         :undoc-members:
         :special-members: __init__, __str__

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingTone
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingTone&)

GradingRGBMSW
^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.GradingRGBMSW
         :members:
         :undoc-members:
         :special-members: __init__, __str__

   .. group-tab:: C++

      .. doxygenstruct:: ${OCIO_NAMESPACE}::GradingRGBMSW
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GradingRGBMSW&)
