// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

OCIO_NAMESPACE_ENTER
{

namespace 
{
    const std::vector<float> DEFAULT_ARGS = { 0.0f, 1.0f };
}

void bindPyTransform(py::module & m)
{
    py::class_<PyTransform>(m, "Transform")
        .def(py::init([](py::object transform) 
        {
            if(!py::is<PyTransform>(transform))
            {
                return py::cast<py::none>(Py_None);
            }

            PyTransform pyptr;

            if(py::isinstance<PyAllocationTransform>(transform))
            {
                pyptr = py::cast<PyAllocationTransform>(transform);
            }

            return pyptr;
        }),
        "transform"_a);

    py::class_<PyAllocationTransform, PyTransform>(m, "AllocationTransform")
        .def(py::init<>())
        .def(py::init([](Allocation allocation, 
                         std::vector<float> vars, 
                         TransformDirection direction) 
            {
                PyAllocationTransform pyptr;
                AllocationTransformRcPtr eptr = pyptr.getRcPtr();
                eptr->setAllocation(allocation);
                eptr->setVars(vars.size(), vars.data());
                eptr->setDirection(direction);
                return pyptr;
            }), 
            "allocation"_a = ALLOCATION_UNIFORM, 
            "vars"_a = DEFAULT_ARGS,
            "direction"_a = TRANSFORM_DIR_FORWARD)

        .def("__repr__", &PyAllocationTransform::repr)
        .def("createEditableCopy", &PyAllocationTransform::createEditableCopy)

        .def("validate", [](PyAllocationTransform & self) 
            {
                self.getConstRcPtr()->validate();
            })

        .def("getDirection", [](PyAllocationTransform & self) 
            {
                return self.getConstRcPtr()->getDirection();
            })
        .def("setDirection", [](PyAllocationTransform & self, TransformDirection direction) 
            {
                self.getRcPtr()->setDirection(direction);
            }, 
            "direction"_a)

        .def("getAllocation", [](PyAllocationTransform & self) 
            {
                return self.getConstRcPtr()->getAllocation();
            })
        .def("setAllocation", [](PyAllocationTransform & self, Allocation allocation) 
            {
                self.getRcPtr()->setAllocation(allocation);
            }, 
            "allocation"_a)

        .def("getNumVars", [](PyAllocationTransform & self) 
            {
                return self.getConstRcPtr()->getNumVars();
            })
        .def("getVars", [](PyAllocationTransform & self)
            {
                std::vector<float> vars;
                vars.resize(self.getConstRcPtr()->getNumVars());
                self.getConstRcPtr()->getVars(vars.data());
                return vars;
            })
        .def("setVars", [](PyAllocationTransform & self, std::vector<float> vars) 
            { 
                self.getRcPtr()->setVars(vars.size(), vars.data());
            }, 
            "vars"_a);
}

}
OCIO_NAMESPACE_EXIT
