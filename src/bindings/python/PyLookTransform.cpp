// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLookTransform(py::module & m)
{
    LookTransformRcPtr DEFAULT = LookTransform::Create();

    auto clsLookTransform = 
        py::class_<LookTransform, LookTransformRcPtr /* holder */, Transform /* base */>(
            m, "LookTransform", 
            DOC(LookTransform))

        .def(py::init(&LookTransform::Create),
             DOC(LookTransform, Create))
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
             "src"_a = DEFAULT->getSrc(),
             "dst"_a = DEFAULT->getDst(),
             "looks"_a = DEFAULT->getLooks(),
             "direction"_a = DEFAULT->getDirection(),
             "skipColorSpaceConversion"_a = DEFAULT->getSkipColorSpaceConversion(),
             DOC(LookTransform, Create))

        .def("getSrc", &LookTransform::getSrc,
             DOC(LookTransform, getSrc))
        .def("setSrc", &LookTransform::setSrc, "src"_a,
             DOC(LookTransform, setSrc))
        .def("getDst", &LookTransform::getDst,
             DOC(LookTransform, getDst))
        .def("setDst", &LookTransform::setDst, "dst"_a,
             DOC(LookTransform, setDst))
        .def("getLooks", &LookTransform::getLooks,
             DOC(LookTransform, getLooks))
        .def("setLooks", &LookTransform::setLooks, "looks"_a,
             DOC(LookTransform, setLooks))
        .def("getSkipColorSpaceConversion", &LookTransform::getSkipColorSpaceConversion,
             DOC(LookTransform, getSkipColorSpaceConversion))
        .def("setSkipColorSpaceConversion", &LookTransform::setSkipColorSpaceConversion,
             "skipColorSpaceConversion"_a,
             DOC(LookTransform, setSkipColorSpaceConversion));

    defStr(clsLookTransform);
}

} // namespace OCIO_NAMESPACE
