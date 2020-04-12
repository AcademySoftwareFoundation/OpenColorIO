// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyTransform(py::module & m)
{ 
    auto cls = py::class_<Transform, TransformRcPtr /* holder */>(m, "Transform")
        .def("validate", &Transform::validate)
        .def("getDirection", &Transform::getDirection)
        .def("setDirection", &Transform::setDirection, "dir"_a);

    defStr(cls);

    bindPyFormatMetadata(m);
    bindPyDynamicProperty(m);

    // Subclasses
    bindPyAllocationTransform(m);
    bindPyCDLTransform(m);
    bindPyColorSpaceTransform(m);
    bindPyDisplayTransform(m);
    bindPyExponentTransform(m);
    bindPyExponentWithLinearTransform(m);
    bindPyExposureContrastTransform(m);
    bindPyFileTransform(m);
    bindPyFixedFunctionTransform(m);
    bindPyGroupTransform(m);
    bindPyLogAffineTransform(m);
    bindPyLogCameraTransform(m);
    bindPyLogTransform(m);
    bindPyLookTransform(m);
    bindPyLut1DTransform(m);
    bindPyLut3DTransform(m);
    bindPyMatrixTransform(m);
    bindPyRangeTransform(m);
}

} // namespace OCIO_NAMESPACE
