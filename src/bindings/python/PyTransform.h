// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYTRANSFORM_H
#define INCLUDED_OCIO_PYTRANSFORM_H

#include "OpenColorABI.h"

#include <pybind11/pybind11.h>


namespace OCIO_NAMESPACE
{
    
// Subclasses
void bindPyAllocationTransform(pybind11::module & m);
void bindPyBuiltinTransform(pybind11::module & m);
void bindPyCDLTransform(pybind11::module & m);
void bindPyColorSpaceTransform(pybind11::module & m);
void bindPyDisplayViewTransform(pybind11::module & m);
void bindPyExponentTransform(pybind11::module & m);
void bindPyExponentWithLinearTransform(pybind11::module & m);
void bindPyExposureContrastTransform(pybind11::module & m);
void bindPyFileTransform(pybind11::module & m);
void bindPyFixedFunctionTransform(pybind11::module & m);
void bindPyGradingPrimaryTransform(pybind11::module & m);
void bindPyGradingRGBCurveTransform(pybind11::module & m);
void bindPyGradingHueCurveTransform(pybind11::module & m);
void bindPyGradingToneTransform(pybind11::module & m);
void bindPyGroupTransform(pybind11::module & m);
void bindPyLogAffineTransform(pybind11::module & m);
void bindPyLogCameraTransform(pybind11::module & m);
void bindPyLogTransform(pybind11::module & m);
void bindPyLookTransform(pybind11::module & m);
void bindPyLut1DTransform(pybind11::module & m);
void bindPyLut3DTransform(pybind11::module & m);
void bindPyMatrixTransform(pybind11::module & m);
void bindPyRangeTransform(pybind11::module & m);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYTRANSFORM_H
