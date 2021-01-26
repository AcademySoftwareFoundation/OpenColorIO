// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLookTransform(py::module & m)
{
    LookTransformRcPtr DEFAULT = LookTransform::Create();

    auto clsLookTransform = 
        py::class_<LookTransform, LookTransformRcPtr, Transform>(
            m.attr("LookTransform"))

        .def(py::init(&LookTransform::Create), 
             DOC(LookTransform, Create))
        .def(py::init([](const std::string & src,
                         const std::string & dst,
                         const std::string & looks,
                         bool skipColorSpaceConversion,
                         TransformDirection dir)
            {
                LookTransformRcPtr p = LookTransform::Create();
                if (!src.empty())   { p->setSrc(src.c_str()); }
                if (!dst.empty())   { p->setDst(dst.c_str()); }
                if (!looks.empty()) { p->setLooks(looks.c_str()); }
                p->setSkipColorSpaceConversion(skipColorSpaceConversion);
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "src"_a.none(false),
             "dst"_a.none(false),
             "looks"_a.none(false) = DEFAULT->getLooks(),
             "skipColorSpaceConversion"_a = DEFAULT->getSkipColorSpaceConversion(),
             "direction"_a = DEFAULT->getDirection(),
             DOC(LookTransform, Create))

        .def("getSrc", &LookTransform::getSrc,
             DOC(LookTransform, getSrc))
        .def("setSrc", &LookTransform::setSrc, "src"_a.none(false),
             DOC(LookTransform, setSrc))
        .def("getDst", &LookTransform::getDst,
             DOC(LookTransform, getDst))
        .def("setDst", &LookTransform::setDst, "dst"_a.none(false),
             DOC(LookTransform, setDst))
        .def("getLooks", &LookTransform::getLooks,
             DOC(LookTransform, getLooks))
        .def("setLooks", &LookTransform::setLooks, "looks"_a.none(false),
             DOC(LookTransform, setLooks))
        .def("getSkipColorSpaceConversion", &LookTransform::getSkipColorSpaceConversion,
             DOC(LookTransform, getSkipColorSpaceConversion))
        .def("setSkipColorSpaceConversion", &LookTransform::setSkipColorSpaceConversion,
             "skipColorSpaceConversion"_a,
             DOC(LookTransform, setSkipColorSpaceConversion));

    defRepr(clsLookTransform);
}

} // namespace OCIO_NAMESPACE
