// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYTRANSFORM_H
#define INCLUDED_OCIO_PYTRANSFORM_H

#include <typeinfo>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
    
// Subclasses
void bindPyAllocationTransform(py::module & m);
void bindPyBuiltinTransform(py::module & m);
void bindPyCDLTransform(py::module & m);
void bindPyColorSpaceTransform(py::module & m);
void bindPyDisplayViewTransform(py::module & m);
void bindPyExponentTransform(py::module & m);
void bindPyExponentWithLinearTransform(py::module & m);
void bindPyExposureContrastTransform(py::module & m);
void bindPyFileTransform(py::module & m);
void bindPyFixedFunctionTransform(py::module & m);
void bindPyGradingPrimaryTransform(py::module & m);
void bindPyGradingRGBCurveTransform(py::module & m);
void bindPyGradingToneTransform(py::module & m);
void bindPyGroupTransform(py::module & m);
void bindPyLogAffineTransform(py::module & m);
void bindPyLogCameraTransform(py::module & m);
void bindPyLogTransform(py::module & m);
void bindPyLookTransform(py::module & m);
void bindPyLut1DTransform(py::module & m);
void bindPyLut3DTransform(py::module & m);
void bindPyMatrixTransform(py::module & m);
void bindPyRangeTransform(py::module & m);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYTRANSFORM_H
