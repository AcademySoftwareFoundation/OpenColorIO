// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyTransform(py::module & m)
{ 
    auto cls = py::class_<Transform, TransformRcPtr /* holder */>(m, "Transform", DS(Transform))
        .def("validate", &Transform::validate, DS(Transform, validate))
        .def("getDirection", &Transform::getDirection, DS(Transform, getDirection))
        .def("setDirection", &Transform::setDirection, DS(Transform, setDirection), "direction"_a);

    defStr(cls);

    bindPyBuiltinTransformRegistry(m);
    bindPyFormatMetadata(m);
    bindPyDynamicProperty(m);

    // Subclasses
    bindPyAllocationTransform(m);
    bindPyBuiltinTransform(m);
    bindPyCDLTransform(m);
    bindPyColorSpaceTransform(m);
    bindPyDisplayViewTransform(m);
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
