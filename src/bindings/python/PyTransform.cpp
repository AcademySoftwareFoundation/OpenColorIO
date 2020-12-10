// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyTransform(py::module & m)
{ 
    auto clsTransform = 
        py::class_<Transform, TransformRcPtr /* holder */>(
            m, "Transform", 
            DOC(Transform))

        .def("validate", &Transform::validate,
             DOC(Transform, validate))
        .def("getTransformType", &Transform::getTransformType,
             DOC(Transform, getTransformType))
        .def("getDirection", &Transform::getDirection,
             DOC(Transform, getDirection))
        .def("setDirection", &Transform::setDirection, "direction"_a,
             DOC(Transform, setDirection));

    defStr(clsTransform);

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
