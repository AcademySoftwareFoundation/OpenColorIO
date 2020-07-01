// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyColorSpaceTransform(py::module & m)
{
    ColorSpaceTransformRcPtr DEFAULT = ColorSpaceTransform::Create();

    auto cls = py::class_<ColorSpaceTransform, 
                          ColorSpaceTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "ColorSpaceTransform")
        .def(py::init(&ColorSpaceTransform::Create))
        .def(py::init([](const std::string & src,
                         const std::string & dst,
                         TransformDirection dir,
                         bool dataBypass)
            {
                ColorSpaceTransformRcPtr p = ColorSpaceTransform::Create();
                if (!src.empty()) { p->setSrc(src.c_str()); }
                if (!dst.empty()) { p->setDst(dst.c_str()); }
                p->setDirection(dir);
                p->setDataBypass(dataBypass);
                p->validate();
                return p;
            }), 
             "src"_a        = DEFAULT->getSrc(), 
             "dst"_a        = DEFAULT->getDst(),
             "dir"_a        = DEFAULT->getDirection(),
             "dataBypass"_a = DEFAULT->getDataBypass())

        .def("getSrc",        &ColorSpaceTransform::getSrc)
        .def("setSrc",        &ColorSpaceTransform::setSrc,        "src"_a)
        .def("getDst",        &ColorSpaceTransform::getDst)
        .def("setDst",        &ColorSpaceTransform::setDst,        "dst"_a)
        .def("getDataBypass", &ColorSpaceTransform::getDataBypass)
        .def("setDataBypass", &ColorSpaceTransform::setDataBypass, "dataBypass"_a);

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
