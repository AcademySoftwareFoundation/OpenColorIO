..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Transforms
==========

See also: :ref:`grading_transforms`

Transform
*********

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_transform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Transform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Transform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::TransformRcPtr

AllocationTransform
*******************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_allocationtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::AllocationTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const AllocationTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstAllocationTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::AllocationTransformRcPtr

BuiltinTransform
****************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_builtintransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::BuiltinTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const BuiltinTransform&) noexcept

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstBuiltinTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::BuiltinTransformRcPtr

BuiltinTransformRegistry
^^^^^^^^^^^^^^^^^^^^^^^^

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_builtintransformregistry.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::BuiltinTransformRegistry
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstBuiltinTransformRegistryRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::BuiltinTransformRegistryRcPtr

CDLTransform
************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_cdltransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::CDLTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const CDLTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstCDLTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::CDLTransformRcPtr

ColorSpaceTransform
*******************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_colorspacetransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ColorSpaceTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ColorSpaceTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstColorSpaceTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ColorSpaceTransformRcPtr

DisplayViewTransform
********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_displayviewtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::DisplayViewTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const DisplayViewTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstDisplayViewTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::DisplayViewTransformRcPtr

ExponentTransform
*****************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_exponenttransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ExponentTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ExponentTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstExponentTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ExponentTransformRcPtr

ExponentWithLinearTransform
***************************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_exponentwithlineartransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ExponentWithLinearTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ExponentWithLinearTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstExponentWithLinearTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ExponentWithLinearTransformRcPtr

ExposureContrastTransform
*************************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_exposurecontrasttransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::ExposureContrastTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const ExposureContrastTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstExposureContrastTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ExposureContrastTransformRcPtr

FileTransform
*************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_filetransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::FileTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const FileTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstFileTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::FileTransformRcPtr

FixedFunctionTransform
**********************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_fixedfunctiontransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::FixedFunctionTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const FixedFunctionTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstFixedFunctionTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::FixedFunctionTransformRcPtr

GroupTransform
**************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_grouptransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::GroupTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const GroupTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstGroupTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::GroupTransformRcPtr

LogAffineTransform
******************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_logaffinetransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::LogAffineTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const LogAffineTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLogAffineTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::LogAffineTransformRcPtr

LogCameraTransform
******************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_logcameratransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::LogCameraTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const LogCameraTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLogCameraTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::LogCameraTransformRcPtr

LogTransform
************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_logtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::LogTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const LogTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLogTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::LogTransformRcPtr

LookTransform
*************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_looktransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::LookTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const LookTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLookTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::LookTransformRcPtr

Lut1DTransform
**************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_lut1dtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Lut1DTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Lut1DTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLut1DTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::Lut1DTransformRcPtr

Lut3DTransform
**************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_lut3dtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Lut3DTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Lut3DTransform&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstLut3DTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::Lut3DTransformRcPtr

MatrixTransform
***************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_matrixtransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::MatrixTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const MatrixTransform&) noexcept

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstMatrixTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::MatrixTransformRcPtr

RangeTransform
**************

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_rangetransform.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::RangeTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const RangeTransform&) noexcept

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstRangeTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::RangeTransformRcPtr
