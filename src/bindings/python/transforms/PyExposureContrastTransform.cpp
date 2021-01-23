// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyExposureContrastTransform(py::module & m)
{
    ExposureContrastTransformRcPtr DEFAULT = ExposureContrastTransform::Create();

    auto clsExposureContrastTransform = 
        py::class_<ExposureContrastTransform, ExposureContrastTransformRcPtr, Transform>(
            m.attr("ExposureContrastTransform"))

        .def(py::init(&ExposureContrastTransform::Create), 
             DOC(ExposureContrastTransform, Create))
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
                p->setStyle(style);
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
             "direction"_a = DEFAULT->getDirection(), 
             DOC(ExposureContrastTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (ExposureContrastTransform::*)()) 
             &ExposureContrastTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(ExposureContrastTransform, getFormatMetadata))
        .def("equals", &ExposureContrastTransform::equals, "other"_a, 
             DOC(ExposureContrastTransform, equals))
        .def("getStyle", &ExposureContrastTransform::getStyle, 
             DOC(ExposureContrastTransform, getStyle))
        .def("setStyle", &ExposureContrastTransform::setStyle, "style"_a, 
             DOC(ExposureContrastTransform, setStyle))
        .def("getExposure", &ExposureContrastTransform::getExposure, 
             DOC(ExposureContrastTransform, getExposure))
        .def("setExposure", &ExposureContrastTransform::setExposure, "exposure"_a, 
             DOC(ExposureContrastTransform, setExposure))
        .def("isExposureDynamic", &ExposureContrastTransform::isExposureDynamic, 
             DOC(ExposureContrastTransform, isExposureDynamic))
        .def("makeExposureDynamic", &ExposureContrastTransform::makeExposureDynamic, 
             DOC(ExposureContrastTransform, makeExposureDynamic))
        .def("makeExposureNonDynamic", &ExposureContrastTransform::makeExposureNonDynamic, 
             DOC(ExposureContrastTransform, makeExposureNonDynamic))
        .def("getContrast", &ExposureContrastTransform::getContrast, 
             DOC(ExposureContrastTransform, getContrast))
        .def("setContrast", &ExposureContrastTransform::setContrast, "contrast"_a, 
             DOC(ExposureContrastTransform, setContrast))
        .def("isContrastDynamic", &ExposureContrastTransform::isContrastDynamic, 
             DOC(ExposureContrastTransform, isContrastDynamic))
        .def("makeContrastDynamic", &ExposureContrastTransform::makeContrastDynamic, 
             DOC(ExposureContrastTransform, makeContrastDynamic))
        .def("makeContrastNonDynamic", &ExposureContrastTransform::makeContrastNonDynamic, 
             DOC(ExposureContrastTransform, makeContrastNonDynamic))
        .def("getGamma", &ExposureContrastTransform::getGamma, 
             DOC(ExposureContrastTransform, getGamma))
        .def("setGamma", &ExposureContrastTransform::setGamma, "gamma"_a, 
             DOC(ExposureContrastTransform, setGamma))
        .def("isGammaDynamic", &ExposureContrastTransform::isGammaDynamic, 
             DOC(ExposureContrastTransform, isGammaDynamic))
        .def("makeGammaDynamic", &ExposureContrastTransform::makeGammaDynamic, 
             DOC(ExposureContrastTransform, makeGammaDynamic))
        .def("makeGammaNonDynamic", &ExposureContrastTransform::makeGammaNonDynamic, 
             DOC(ExposureContrastTransform, makeGammaNonDynamic))
        .def("getPivot", &ExposureContrastTransform::getPivot, 
             DOC(ExposureContrastTransform, getPivot))
        .def("setPivot", &ExposureContrastTransform::setPivot, "pivot"_a, 
             DOC(ExposureContrastTransform, setPivot))
        .def("getLogExposureStep", &ExposureContrastTransform::getLogExposureStep, 
             DOC(ExposureContrastTransform, getLogExposureStep))
        .def("setLogExposureStep", &ExposureContrastTransform::setLogExposureStep, 
             "logExposureStep"_a, 
             DOC(ExposureContrastTransform, setLogExposureStep))
        .def("getLogMidGray", &ExposureContrastTransform::getLogMidGray, 
             DOC(ExposureContrastTransform, getLogMidGray))
        .def("setLogMidGray", &ExposureContrastTransform::setLogMidGray, "logMidGray"_a, 
             DOC(ExposureContrastTransform, setLogMidGray));

    defRepr(clsExposureContrastTransform);
}

} // namespace OCIO_NAMESPACE
