// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyColorSpaceTransform(py::module & m)
{
    ColorSpaceTransformRcPtr DEFAULT = ColorSpaceTransform::Create();

    auto clsColorSpaceTransform = 
        py::class_<ColorSpaceTransform, ColorSpaceTransformRcPtr, Transform>(
            m.attr("ColorSpaceTransform"))

        .def(py::init(&ColorSpaceTransform::Create), 
             DOC(ColorSpaceTransform, Create))
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
             "direction"_a  = DEFAULT->getDirection(),
             "dataBypass"_a = DEFAULT->getDataBypass(), 
             DOC(ColorSpaceTransform, Create))

        .def("getSrc", &ColorSpaceTransform::getSrc, 
             DOC(ColorSpaceTransform, getSrc))
        .def("setSrc", &ColorSpaceTransform::setSrc, "src"_a.none(false), 
             DOC(ColorSpaceTransform, setSrc))
        .def("getDst", &ColorSpaceTransform::getDst, 
             DOC(ColorSpaceTransform, getDst))
        .def("setDst", &ColorSpaceTransform::setDst, "dst"_a.none(false), 
             DOC(ColorSpaceTransform, setDst))
        .def("getDataBypass", &ColorSpaceTransform::getDataBypass, 
             DOC(ColorSpaceTransform, getDataBypass))
        .def("setDataBypass", &ColorSpaceTransform::setDataBypass, "dataBypass"_a, 
             DOC(ColorSpaceTransform, setDataBypass));

    defRepr(clsColorSpaceTransform);
}

} // namespace OCIO_NAMESPACE
