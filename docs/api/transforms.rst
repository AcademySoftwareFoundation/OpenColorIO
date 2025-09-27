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

      .. autoclass:: PyOpenColorIO.Transform
         :members:
         :undoc-members:
         :special-members: __init__, __str__

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

      .. autoclass:: PyOpenColorIO.AllocationTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.BuiltinTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.BuiltinTransformRegistry
         :members:
         :undoc-members:
         :special-members: __init__
         :exclude-members: BuiltinStyleIterator, BuiltinIterator

      .. autoclass:: PyOpenColorIO.BuiltinTransformRegistry.BuiltinStyleIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.BuiltinTransformRegistry.BuiltinIterator
         :special-members: __getitem__, __iter__, __len__, __next__

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

      .. autoclass:: PyOpenColorIO.CDLTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.ColorSpaceTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.DisplayViewTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.ExponentTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.ExponentWithLinearTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.ExposureContrastTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.FileTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:
         :exclude-members: FormatIterator

      .. autoclass:: PyOpenColorIO.FileTransform.FormatIterator
         :special-members: __getitem__, __iter__, __len__, __next__

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

      .. autoclass:: PyOpenColorIO.FixedFunctionTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.GroupTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:
         :exclude-members: TransformIterator, WriteFormatIterator

      .. autoclass:: PyOpenColorIO.GroupTransform.WriteFormatIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.GroupTransform.TransformIterator
         :special-members: __getitem__, __iter__, __len__, __next__

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

      .. autoclass:: PyOpenColorIO.LogAffineTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.LogCameraTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.LogTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.LookTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.Lut1DTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.Lut3DTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.MatrixTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

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

      .. autoclass:: PyOpenColorIO.RangeTransform
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :inherited-members:

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::RangeTransform
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const RangeTransform&) noexcept

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstRangeTransformRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::RangeTransformRcPtr
