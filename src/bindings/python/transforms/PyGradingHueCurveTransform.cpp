// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyGradingHueCurveTransform(py::module & m)
{
    GradingHueCurveTransformRcPtr DEFAULT = GradingHueCurveTransform::Create(GRADING_LOG);

    auto clsGradingHueCurveTransform = 
        py::class_<GradingHueCurveTransform, GradingHueCurveTransformRcPtr, Transform>(
            m.attr("GradingHueCurveTransform"))

        .def(py::init([](const ConstGradingHueCurveRcPtr & values,
                         GradingStyle style,
                         bool dynamic, 
                         TransformDirection dir) 
            {
                GradingHueCurveTransformRcPtr p = GradingHueCurveTransform::Create(style);
                p->setStyle(style);
                p->setValue(values);
                if (dynamic) { p->makeDynamic(); }
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "values"_a,
             "style"_a = DEFAULT->getStyle(),
             "dynamic"_a = DEFAULT->isDynamic(),
             "dir"_a = DEFAULT->getDirection(),
             DOC(GradingHueCurveTransform, Create))
        .def(py::init([](GradingStyle style, bool dynamic, TransformDirection dir)
            {
                GradingHueCurveTransformRcPtr p = GradingHueCurveTransform::Create(style);
                p->setStyle(style);
                if (dynamic)
                {
                    p->makeDynamic();
                }
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "style"_a = DEFAULT->getStyle(),
             "dynamic"_a = DEFAULT->isDynamic(),
             "dir"_a = DEFAULT->getDirection(),
             DOC(GradingHueCurveTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (GradingHueCurveTransform::*)()) 
             &GradingHueCurveTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(GradingHueCurveTransform, getFormatMetadata))
        .def("getStyle", &GradingHueCurveTransform::getStyle, 
             DOC(GradingHueCurveTransform, getStyle))
        .def("setStyle", &GradingHueCurveTransform::setStyle, "style"_a, 
             DOC(GradingHueCurveTransform, setStyle))
        .def("getValue", &GradingHueCurveTransform::getValue, 
             DOC(GradingHueCurveTransform, getValue))
        .def("setValue", &GradingHueCurveTransform::setValue, "values"_a, 
             DOC(GradingHueCurveTransform, setValue))
        .def("getSlope", &GradingHueCurveTransform::getSlope, "curve"_a, "index"_a,
             DOC(GradingHueCurveTransform, getSlope))
        .def("setSlope", &GradingHueCurveTransform::setSlope, "curve"_a, "index"_a, "slope"_a, 
             DOC(GradingHueCurveTransform, setSlope))
        .def("slopesAreDefault", &GradingHueCurveTransform::slopesAreDefault, "curve"_a,
             DOC(GradingHueCurveTransform, slopesAreDefault))
// TODO: The above follows what is in GradingRGBCurveTransform, but could add a more pythonic version?
//         .def("getSlopes", [](GradingHueCurveTransformRcPtr self, HueCurveType curveType)
//             {
//                 std::vector<float> slopes;
//                 GradingHueCurveRcPtr curve = self->getValue();
//                 const size_t numPts = self->getNumControlPoints();
//                 slopes.resize(numPts);
//                 for (size_t pt = 0; pt < numPts; ++pt)
//                 {
//                     slopes[pt] = self->getSlope(curveType, pt);
//                 }
//                 return slopes;
//             }, 
//              DOC(GradingHueCurveTransform, getSlopes))
//         .def("setSlopes", [](GradingHueCurveTransformRcPtr self, 
//                              HueCurveType curveType, 
//                              const std::vector<float> & slopes)
//             {
//                 const size_t numPts = self->getNumControlPoints();
//                 const auto size = slopes.size();
//                 if (size != numPts)
//                 {
//                     throw Exception("Length of slopes vector must match number of control points.");
//                 }
//                 for (size_t pt = 0; pt < numPts; ++pt)
//                 {
//                     self->setSlope(curveType, pt, slopes[pt]);
//                 }
//             },
//              DOC(GradingHueCurveTransform, getSlopes));
        .def("getRGBToHSY", &GradingHueCurveTransform::getRGBToHSY, 
             DOC(GradingHueCurveTransform, getRGBToHSY))
        .def("setRGBToHSY", &GradingHueCurveTransform::setRGBToHSY, "style"_a, 
             DOC(GradingHueCurveTransform, setRGBToHSY))
        .def("isDynamic", &GradingHueCurveTransform::isDynamic, 
             DOC(GradingHueCurveTransform, isDynamic))
        .def("makeDynamic", &GradingHueCurveTransform::makeDynamic, 
             DOC(GradingHueCurveTransform, makeDynamic))
        .def("makeNonDynamic", &GradingHueCurveTransform::makeNonDynamic, 
             DOC(GradingHueCurveTransform, makeNonDynamic));

    defRepr(clsGradingHueCurveTransform);
}

} // namespace OCIO_NAMESPACE
