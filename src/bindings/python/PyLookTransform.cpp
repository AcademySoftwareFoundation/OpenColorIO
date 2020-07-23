// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLookTransform(py::module & m)
{
    LookTransformRcPtr DEFAULT = LookTransform::Create();

    auto cls = py::class_<LookTransform, 
                          LookTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "LookTransform", DS(LookTransform))
        .def(py::init(&LookTransform::Create), DS(LookTransform, Create))
        .def(py::init([](const std::string & src,
                         const std::string & dst,
                         const std::string & looks,
                         TransformDirection dir,
                         bool skipColorSpaceConversion)
            {
                LookTransformRcPtr p = LookTransform::Create();
                if (!src.empty())   { p->setSrc(src.c_str()); }
                if (!dst.empty())   { p->setDst(dst.c_str()); }
                if (!looks.empty()) { p->setLooks(looks.c_str()); }
                p->setDirection(dir);
                p->setSkipColorSpaceConversion(skipColorSpaceConversion);
                p->validate();
                return p;
            }),
             DS(LookTransform, LookTransform),
             "src"_a = DEFAULT->getSrc(),
             "dst"_a = DEFAULT->getDst(),
             "looks"_a = DEFAULT->getLooks(),
             "direction"_a = DEFAULT->getDirection(),
             "skipColorSpaceConversion"_a = DEFAULT->getSkipColorSpaceConversion())

        .def("getSrc", &LookTransform::getSrc, DS(LookTransform, getSrc))
        .def("setSrc", &LookTransform::setSrc, DS(LookTransform, setSrc), "src"_a)
        .def("getDst", &LookTransform::getDst, DS(LookTransform, getDst))
        .def("setDst", &LookTransform::setDst, DS(LookTransform, setDst), "dst"_a)
        .def("getLooks", &LookTransform::getLooks, DS(LookTransform, getLooks))
        .def("setLooks", &LookTransform::setLooks, DS(LookTransform, setLooks), "looks"_a)
        .def("getSkipColorSpaceConversion", &LookTransform::getSkipColorSpaceConversion,
             DS(LookTransform, getSkipColorSpaceConversion))
        .def("setSkipColorSpaceConversion", &LookTransform::setSkipColorSpaceConversion,
             DS(LookTransform, setSkipColorSpaceConversion),
             "skipColorSpaceConversion"_a);

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
