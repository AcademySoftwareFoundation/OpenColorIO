// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLookTransform(py::module & m)
{
    LookTransformRcPtr DEFAULT = LookTransform::Create();

    py::class_<LookTransform, 
               LookTransformRcPtr /* holder */, 
               Transform /* base */>(m, "LookTransform")
        .def(py::init(&LookTransform::Create))
        .def(py::init([](const std::string & src, 
                         const std::string & dst, 
                         const std::string & looks, 
                         TransformDirection dir) 
            {
                LookTransformRcPtr p = LookTransform::Create();
                if (!src.empty())   { p->setSrc(src.c_str()); }
                if (!dst.empty())   { p->setDst(dst.c_str()); }
                if (!looks.empty()) { p->setLooks(looks.c_str()); }
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "src"_a = DEFAULT->getSrc(),
             "dst"_a = DEFAULT->getDst(),
             "looks"_a = DEFAULT->getLooks(),
             "dir"_a = DEFAULT->getDirection())

        .def("getSrc", &LookTransform::getSrc)
        .def("setSrc", &LookTransform::setSrc, "src"_a)
        .def("getDst", &LookTransform::getDst)
        .def("setDst", &LookTransform::setDst, "dst"_a)
        .def("getLooks", &LookTransform::getLooks)
        .def("setLooks", &LookTransform::setLooks, "looks"_a);
}

} // namespace OCIO_NAMESPACE
