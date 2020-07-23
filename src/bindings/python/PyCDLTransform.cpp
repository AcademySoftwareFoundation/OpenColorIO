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

    auto cls = py::class_<CDLTransform, 
                          CDLTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "CDLTransform", DS(CDLTransform))
        .def(py::init(&CDLTransform::Create), DS(CDLTransform, Create))
        .def(py::init([](const std::string & xml, TransformDirection dir) 
            {
                CDLTransformRcPtr p = CDLTransform::Create();
                if (!xml.empty()) { p->setXML(xml.c_str()); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             DS(CDLTransform, CDLTransform),
             "xml"_a,
             "direction"_a = DEFAULT->getDirection())
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
             DS(CDLTransform, CDLTransform, 2),
             "slope"_a = DEFAULT_SLOPE,
             "offset"_a = DEFAULT_OFFSET,
             "power"_a = DEFAULT_POWER,
             "sat"_a = DEFAULT->getSat(),
             "id"_a = DEFAULT->getID(),
             "description"_a = DEFAULT->getDescription(),
             "direction"_a = DEFAULT->getDirection())

        .def_static("CreateFromFile", &CDLTransform::CreateFromFile, DS(CDLTransform, CreateFromFile), "src"_a, "id"_a)

        .def("getFormatMetadata", 
             (FormatMetadata & (CDLTransform::*)()) &CDLTransform::getFormatMetadata,
             DS(CDLTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (CDLTransform::*)() const) &CDLTransform::getFormatMetadata,
             DS(CDLTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("setStyle", &CDLTransform::setStyle, DS(CDLTransform, setStyle), "style"_a)
        .def("equals", &CDLTransform::equals, DS(CDLTransform, equals), "other"_a)
        .def("getStyle", &CDLTransform::getStyle, DS(CDLTransform, getStyle))
        .def("setStyle", &CDLTransform::setStyle, DS(CDLTransform, setStyle), "style"_a)
        .def("getXML", &CDLTransform::getXML, DS(CDLTransform, getXML))
        .def("setXML", &CDLTransform::setXML, DS(CDLTransform, setXML), "xml"_a)
        .def("getSlope", [](ConstCDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getSlope(rgb.data());
                return rgb;
            },
             DS(CDLTransform, getSlope))
        .def("setSlope", [](CDLTransformRcPtr self, const std::array<double, 3> & rgb)
            { 
                self->setSlope(rgb.data());
            }, 
             DS(CDLTransform, setSlope),
             "rgb"_a)
        .def("getOffset", [](CDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getOffset(rgb.data());
                return rgb;
            },
             DS(CDLTransform, getOffset))
        .def("setOffset", [](CDLTransformRcPtr self, const std::array<double, 3> & rgb) 
            { 
                self->setOffset(rgb.data());
            }, 
             DS(CDLTransform, setOffset),
             "rgb"_a)
        .def("getPower", [](CDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getPower(rgb.data());
                return rgb;
            },
             DS(CDLTransform, getPower))
        .def("setPower", [](CDLTransformRcPtr self, const std::array<double, 3> & rgb) 
            { 
                self->setPower(rgb.data());
            }, 
             DS(CDLTransform, setPower),
             "rgb"_a)
        .def("getSOP", [](CDLTransformRcPtr self)
            {
                std::array<double, 9> vec9;
                self->getSOP(vec9.data());
                return vec9;
            },
             DS(CDLTransform, getSOP))
        .def("setSOP", [](CDLTransformRcPtr self, const std::array<double, 9> & vec9) 
            { 
                self->setSOP(vec9.data());
            }, 
             DS(CDLTransform, setSOP),
             "vec9"_a)
        .def("getSat", &CDLTransform::getSat, DS(CDLTransform, getSat))
        .def("setSat", &CDLTransform::setSat, DS(CDLTransform, setSat), "sat"_a)
        .def("getSatLumaCoefs", [](CDLTransformRcPtr self)
            {
                std::array<double, 3> rgb;
                self->getSatLumaCoefs(rgb.data());
                return rgb;
            },
             DS(CDLTransform, getSatLumaCoefs))
        .def("getID", &CDLTransform::getID, DS(CDLTransform, getID))
        .def("setID", &CDLTransform::setID, DS(CDLTransform, setID), "id"_a)
        .def("getDescription", &CDLTransform::getDescription, DS(CDLTransform, getDescription))
        .def("setDescription", &CDLTransform::setDescription,  DS(CDLTransform, setDescription), "description"_a);

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
