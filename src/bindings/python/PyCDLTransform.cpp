// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyCDLTransform(py::module & m)
{
    CDLTransformRcPtr DEFAULT = CDLTransform::Create();

    std::array<double, 3> DEFAULT_SLOPE;
    DEFAULT->getSlope(DEFAULT_SLOPE.data());

    std::array<double, 3> DEFAULT_OFFSET;
    DEFAULT->getOffset(DEFAULT_OFFSET.data());

    std::array<double, 3> DEFAULT_POWER;
    DEFAULT->getPower(DEFAULT_POWER.data());

    auto clsCDLTransform = 
        py::class_<CDLTransform, CDLTransformRcPtr /* holder */, Transform /* base */>(
            m, "CDLTransform", 
            DOC(CDLTransform))

        .def(py::init(&CDLTransform::Create), 
             DOC(CDLTransform, Create))
        .def(py::init([](const std::string & xml, TransformDirection dir) 
            {
                CDLTransformRcPtr p = CDLTransform::Create();
                if (!xml.empty()) { p->setXML(xml.c_str()); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "xml"_a,
             "direction"_a = DEFAULT->getDirection(),
             DOC(CDLTransform, Create))
        .def(py::init([](const std::array<double, 3> & slope,
                         const std::array<double, 3> & offset,
                         const std::array<double, 3> & power,
                         double sat,
                         const std::string & id,
                         const std::string & desc, 
                         TransformDirection dir) 
            {
                CDLTransformRcPtr p = CDLTransform::Create();
                p->setSlope(slope.data());
                p->setOffset(offset.data());
                p->setPower(power.data());
                p->setSat(sat);
                if (!id.empty())   { p->setID(id.c_str()); }
                if (!desc.empty()) { p->setDescription(desc.c_str()); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "slope"_a = DEFAULT_SLOPE,
             "offset"_a = DEFAULT_OFFSET,
             "power"_a = DEFAULT_POWER,
             "sat"_a = DEFAULT->getSat(),
             "id"_a = DEFAULT->getID(),
             "description"_a = DEFAULT->getDescription(),
             "direction"_a = DEFAULT->getDirection(),
             DOC(CDLTransform, Create))

        .def_static("CreateFromFile", &CDLTransform::CreateFromFile, "src"_a, "id"_a, 
                    DOC(CDLTransform, CreateFromFile))

        .def("getFormatMetadata", 
             (FormatMetadata & (CDLTransform::*)()) &CDLTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(CDLTransform, getFormatMetadata))
        .def("setStyle", &CDLTransform::setStyle, "style"_a, 
             DOC(CDLTransform, setStyle))
        .def("equals", &CDLTransform::equals, "other"_a, 
             DOC(CDLTransform, equals))
        .def("getStyle", &CDLTransform::getStyle, 
             DOC(CDLTransform, getStyle))
        .def("setStyle", &CDLTransform::setStyle, "style"_a, 
             DOC(CDLTransform, setStyle))
        .def("getXML", &CDLTransform::getXML, 
             DOC(CDLTransform, getXML))
        .def("setXML", &CDLTransform::setXML, "xml"_a, 
             DOC(CDLTransform, setXML))
        .def("getSlope", [](ConstCDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getSlope(rgb.data());
                return rgb;
            }, 
             DOC(CDLTransform, getSlope))
        .def("setSlope", [](CDLTransformRcPtr self, const std::array<double, 3> & rgb)
            { 
                self->setSlope(rgb.data());
            }, 
             "rgb"_a, 
             DOC(CDLTransform, setSlope))
        .def("getOffset", [](CDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getOffset(rgb.data());
                return rgb;
            }, 
             DOC(CDLTransform, getOffset))
        .def("setOffset", [](CDLTransformRcPtr self, const std::array<double, 3> & rgb) 
            { 
                self->setOffset(rgb.data());
            }, 
             "rgb"_a, 
             DOC(CDLTransform, setOffset))
        .def("getPower", [](CDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getPower(rgb.data());
                return rgb;
            }, 
             DOC(CDLTransform, getPower))
        .def("setPower", [](CDLTransformRcPtr self, const std::array<double, 3> & rgb) 
            { 
                self->setPower(rgb.data());
            }, 
             "rgb"_a, 
             DOC(CDLTransform, setPower))
        .def("getSOP", [](CDLTransformRcPtr self)
            {
                std::array<double, 9> vec9;
                self->getSOP(vec9.data());
                return vec9;
            }, 
             DOC(CDLTransform, getSOP))
        .def("setSOP", [](CDLTransformRcPtr self, const std::array<double, 9> & vec9) 
            { 
                self->setSOP(vec9.data());
            }, 
             "vec9"_a, 
             DOC(CDLTransform, setSOP))
        .def("getSat", &CDLTransform::getSat, 
             DOC(CDLTransform, getSat))
        .def("setSat", &CDLTransform::setSat, "sat"_a, 
             DOC(CDLTransform, setSat))
        .def("getSatLumaCoefs", [](CDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getSatLumaCoefs(rgb.data());
                return rgb;
            }, 
             DOC(CDLTransform, getSatLumaCoefs))
        .def("getID", &CDLTransform::getID, 
             DOC(CDLTransform, getID))
        .def("setID", &CDLTransform::setID, "id"_a, 
             DOC(CDLTransform, setID))
        .def("getDescription", &CDLTransform::getDescription, 
             DOC(CDLTransform, getDescription))
        .def("setDescription", &CDLTransform::setDescription,  "description"_a, 
             DOC(CDLTransform, setDescription));

    defStr(clsCDLTransform);
}

} // namespace OCIO_NAMESPACE
