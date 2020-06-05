// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

void bindPyLook(py::module & m)
{
    LookRcPtr DEFAULT = Look::Create();

    auto cls = py::class_<Look, LookRcPtr /* holder */>(m, "Look")
        .def(py::init(&Look::Create))
        .def(py::init([](const std::string & name,
                         const std::string & processSpace,
                         const TransformRcPtr & transform,
                         const TransformRcPtr & inverseTransform,
                         const std::string & description)
            {
                LookRcPtr p = Look::Create();
                if (!name.empty())         { p->setName(name.c_str()); }
                if (!processSpace.empty()) { p->setProcessSpace(processSpace.c_str()); }
                if (transform)             { p->setTransform(transform); }
                if (inverseTransform)      { p->setInverseTransform(inverseTransform); }
                if (!description.empty())  { p->setDescription(description.c_str()); }
                return p;
            }),
             "name"_a = DEFAULT->getName(),
             "processSpace"_a = DEFAULT->getProcessSpace(),
             "transform"_a = DEFAULT->getTransform(),
             "inverseTransform"_a = DEFAULT->getInverseTransform(),
             "description"_a = DEFAULT->getDescription())

        .def("getName", &Look::getName)
        .def("setName", &Look::setName, "name"_a.none(false))
        .def("getProcessSpace", &Look::getProcessSpace)
        .def("setProcessSpace", &Look::setProcessSpace, "processSpace"_a.none(false))
        .def("getTransform", &Look::getTransform)
        .def("setTransform", &Look::setTransform, "transform"_a)
        .def("getInverseTransform", &Look::getInverseTransform)
        .def("setInverseTransform", &Look::setInverseTransform, "transform"_a)
        .def("getDescription", &Look::getDescription)
        .def("setDescription", &Look::setDescription, "description"_a.none(false));

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
