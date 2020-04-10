// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyExposureContrastTransform(py::module & m)
{
    ExposureContrastTransformRcPtr DEFAULT = ExposureContrastTransform::Create();

    py::class_<ExposureContrastTransform, 
               ExposureContrastTransformRcPtr /* holder */, 
               Transform /* base */>(m, "ExposureContrastTransform")
        .def(py::init(&ExposureContrastTransform::Create))
        .def(py::init([](ExposureContrastStyle style,
                         double exposure,
                         double contrast,
                         double gamma,
                         double pivot,
                         double logExposureStep,
                         double logMidGray,
                         bool dynamicExposure,
                         bool dynamicContrast,
                         bool dynamicGamma,
                         TransformDirection dir) 
            {
                ExposureContrastTransformRcPtr p = ExposureContrastTransform::Create();
                p->setExposure(exposure);
                p->setContrast(contrast);
                p->setGamma(gamma);
                p->setPivot(pivot);
                p->setLogExposureStep(logExposureStep);
                p->setLogMidGray(logMidGray);
                if (dynamicExposure) { p->makeExposureDynamic(); }
                if (dynamicContrast) { p->makeContrastDynamic(); }
                if (dynamicGamma)    { p->makeGammaDynamic(); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "style"_a = DEFAULT->getStyle(),
             "exposure"_a = DEFAULT->getExposure(), 
             "contrast"_a = DEFAULT->getContrast(),
             "gamma"_a = DEFAULT->getGamma(),
             "pivot"_a = DEFAULT->getPivot(),
             "logExposureStep"_a = DEFAULT->getLogExposureStep(),
             "logMidGray"_a = DEFAULT->getLogMidGray(),
             "dynamicExposure"_a = DEFAULT->isExposureDynamic(),
             "dynamicContrast"_a = DEFAULT->isContrastDynamic(),
             "dynamicGamma"_a = DEFAULT->isGammaDynamic(),
             "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (ExposureContrastTransform::*)()) 
             &ExposureContrastTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (ExposureContrastTransform::*)() const) 
             &ExposureContrastTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &ExposureContrastTransform::equals, "other"_a)
        .def("getStyle", &ExposureContrastTransform::getStyle)
        .def("setStyle", &ExposureContrastTransform::setStyle, "style"_a)
        .def("getExposure", &ExposureContrastTransform::getExposure)
        .def("setExposure", &ExposureContrastTransform::setExposure, "exposure"_a)
        .def("isExposureDynamic", &ExposureContrastTransform::isExposureDynamic)
        .def("makeExposureDynamic", &ExposureContrastTransform::makeExposureDynamic)
        .def("getContrast", &ExposureContrastTransform::getContrast)
        .def("setContrast", &ExposureContrastTransform::setContrast, "contrast"_a)
        .def("isContrastDynamic", &ExposureContrastTransform::isContrastDynamic)
        .def("makeContrastDynamic", &ExposureContrastTransform::makeContrastDynamic)
        .def("getGamma", &ExposureContrastTransform::getGamma)
        .def("setGamma", &ExposureContrastTransform::setGamma, "gamma"_a)
        .def("isGammaDynamic", &ExposureContrastTransform::isGammaDynamic)
        .def("makeGammaDynamic", &ExposureContrastTransform::makeGammaDynamic)
        .def("getPivot", &ExposureContrastTransform::getPivot)
        .def("setPivot", &ExposureContrastTransform::setPivot, "pivot"_a)
        .def("getLogExposureStep", &ExposureContrastTransform::getLogExposureStep)
        .def("setLogExposureStep", &ExposureContrastTransform::setLogExposureStep, 
             "logExposureStep"_a)
        .def("getLogMidGray", &ExposureContrastTransform::getLogMidGray)
        .def("setLogMidGray", &ExposureContrastTransform::setLogMidGray, "logMidGray"_a);
}

} // namespace OCIO_NAMESPACE
