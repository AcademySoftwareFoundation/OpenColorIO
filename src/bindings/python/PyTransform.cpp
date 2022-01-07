// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyTransform(py::module & m)
{ 
    // Base class
    auto clsTransform = 
        py::class_<Transform, TransformRcPtr>(
            m.attr("Transform"))

        .def("__deepcopy__", [](const ConstTransformRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def("validate", &Transform::validate,
             DOC(Transform, validate))
        .def("getTransformType", &Transform::getTransformType,
             DOC(Transform, getTransformType))
        .def("getDirection", &Transform::getDirection,
             DOC(Transform, getDirection))
        .def("setDirection", &Transform::setDirection, "direction"_a,
             DOC(Transform, setDirection));

    defRepr(clsTransform);

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
    bindPyGradingPrimaryTransform(m);
    bindPyGradingRGBCurveTransform(m);
    bindPyGradingToneTransform(m);
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
