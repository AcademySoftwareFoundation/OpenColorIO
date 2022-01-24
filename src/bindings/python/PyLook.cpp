// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

void bindPyLook(py::module & m)
{
    LookRcPtr DEFAULT = Look::Create();

    auto clsLook = 
        py::class_<Look, LookRcPtr>(
            m.attr("Look"))

        .def(py::init(&Look::Create), 
             DOC(Look, Create))
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
             "description"_a = DEFAULT->getDescription(), 
             DOC(Look, Create))

        .def("__deepcopy__", [](const ConstLookRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def("getName", &Look::getName, 
             DOC(Look, getName))
        .def("setName", &Look::setName, "name"_a.none(false), 
             DOC(Look, setName))
        .def("getProcessSpace", &Look::getProcessSpace, 
             DOC(Look, getProcessSpace))
        .def("setProcessSpace", &Look::setProcessSpace, "processSpace"_a.none(false), 
             DOC(Look, setProcessSpace))
        .def("getTransform", &Look::getTransform, 
             DOC(Look, getTransform))
        .def("setTransform", &Look::setTransform, "transform"_a, 
             DOC(Look, setTransform))
        .def("getInverseTransform", &Look::getInverseTransform, 
             DOC(Look, getInverseTransform))
        .def("setInverseTransform", &Look::setInverseTransform, "transform"_a, 
             DOC(Look, setInverseTransform))
        .def("getDescription", &Look::getDescription, 
             DOC(Look, getDescription))
        .def("setDescription", &Look::setDescription, "description"_a.none(false), 
             DOC(Look, setDescription));

    defRepr(clsLook);
}

} // namespace OCIO_NAMESPACE
