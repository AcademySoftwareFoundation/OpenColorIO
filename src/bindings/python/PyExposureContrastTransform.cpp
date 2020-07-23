// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyExposureContrastTransform(py::module & m)
{
    ExposureContrastTransformRcPtr DEFAULT = ExposureContrastTransform::Create();

    auto cls = py::class_<ExposureContrastTransform, 
                          ExposureContrastTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "ExposureContrastTransform", DS(ExposureContrastStyle))
        .def(py::init(&ExposureContrastTransform::Create), DS(ExposureContrastStyle, Create))
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
             DS(ExposureContrastStyle, ExposureContrastStyle),
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
             "direction"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (ExposureContrastTransform::*)()) 
             &ExposureContrastTransform::getFormatMetadata,
             DS(ExposureContrastStyle, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (ExposureContrastTransform::*)() const) 
             &ExposureContrastTransform::getFormatMetadata,
             DS(ExposureContrastStyle, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("equals", &ExposureContrastTransform::equals, DS(ExposureContrastStyle, equals), "other"_a)
        .def("getStyle", &ExposureContrastTransform::getStyle, DS(ExposureContrastStyle, getStyle))
        .def("setStyle", &ExposureContrastTransform::setStyle, DS(ExposureContrastStyle, setStyle), "style"_a)
        .def("getExposure", &ExposureContrastTransform::getExposure, DS(ExposureContrastStyle, getExposure))
        .def("setExposure", &ExposureContrastTransform::setExposure, DS(ExposureContrastStyle, setExposure), "exposure"_a)
        .def("isExposureDynamic", &ExposureContrastTransform::isExposureDynamic,
             DS(ExposureContrastStyle, isExposureDynamic))
        .def("makeExposureDynamic", &ExposureContrastTransform::makeExposureDynamic,
             DS(ExposureContrastStyle, makeExposureDynamic))
        .def("getContrast", &ExposureContrastTransform::getContrast, DS(ExposureContrastStyle, getContrast))
        .def("setContrast", &ExposureContrastTransform::setContrast, 
             DS(ExposureContrastStyle, setContrast),
             "contrast"_a)
        .def("isContrastDynamic", &ExposureContrastTransform::isContrastDynamic,
             DS(ExposureContrastStyle, isContrastDynamic))
        .def("makeContrastDynamic", &ExposureContrastTransform::makeContrastDynamic,
             DS(ExposureContrastStyle, makeContrastDynamic))
        .def("getGamma", &ExposureContrastTransform::getGamma,
             DS(ExposureContrastStyle, getGamma))
        .def("setGamma", &ExposureContrastTransform::setGamma,
             DS(ExposureContrastStyle, setGamma),
             "gamma"_a)
        .def("isGammaDynamic", &ExposureContrastTransform::isGammaDynamic,
             DS(ExposureContrastStyle, isGammaDynamic))
        .def("makeGammaDynamic", &ExposureContrastTransform::makeGammaDynamic,
             DS(ExposureContrastStyle, makeGammaDynamic))
        .def("getPivot", &ExposureContrastTransform::getPivot, DS(ExposureContrastStyle, getPivot))
        .def("setPivot", &ExposureContrastTransform::setPivot, DS(ExposureContrastStyle, setPivot), "pivot"_a)
        .def("getLogExposureStep", &ExposureContrastTransform::getLogExposureStep,
             DS(ExposureContrastStyle, getLogExposureStep))
        .def("setLogExposureStep", &ExposureContrastTransform::setLogExposureStep, 
             DS(ExposureContrastStyle, setLogExposureStep),
             "logExposureStep"_a)
        .def("getLogMidGray", &ExposureContrastTransform::getLogMidGray, DS(ExposureContrastStyle, getLogMidGray))
        .def("setLogMidGray", &ExposureContrastTransform::setLogMidGray,
             DS(ExposureContrastStyle, setLogMidGray),
             "logMidGray"_a);

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
