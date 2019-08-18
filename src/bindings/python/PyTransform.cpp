// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

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
        .def("__str__", [](PyTransform & self) 
            {
                std::ostringstream os;
                // os << (*self.getRcPtr());
                return os.str();
            })
        .def("validate", [](PyTransform & self) 
            {
                // self.getRcPtr()->validate();
            });

    py::class_<PyAllocationTransform, PyTransform>(m, "AllocationTransform")
        .def(py::init<>())
        .def(py::init([](Allocation allocation, 
                         std::vector<float> vars, 
                         TransformDirection direction) 
            {
                PyAllocationTransform pyptr;
                AllocationTransformRcPtr rcptr = pyptr.getRcPtr();
                rcptr->setAllocation(allocation);
                rcptr->setVars(vars.size(), vars.data());
                rcptr->setDirection(direction);
                return pyptr;
            }), 
            "allocation"_a = ALLOCATION_UNIFORM, 
            "vars"_a = DEFAULT_ARGS,
            "direction"_a = TRANSFORM_DIR_FORWARD)
        .def("__str__", [](PyAllocationTransform & self) 
            {
                std::ostringstream os;
                os << (*self.getRcPtr());
                return os.str();
            })
        .def("createEditableCopy", [](PyAllocationTransform & self) 
            {
                return self.getRcPtr()->createEditableCopy();
            })
        .def("validate", [](PyAllocationTransform & self) 
            {
                self.getRcPtr()->validate();
            })
        .def("getDirection", [](PyAllocationTransform & self) 
            {
                return self.getRcPtr()->getDirection();
            })
        .def("setDirection", [](PyAllocationTransform & self, TransformDirection direction) 
            {
                self.getRcPtr()->setDirection(direction);
            }, 
            "direction"_a)
        .def("getAllocation", [](PyAllocationTransform & self) 
            {
                return self.getRcPtr()->getAllocation();
            })
        .def("setAllocation", [](PyAllocationTransform & self, Allocation allocation) 
            {
                self.getRcPtr()->setAllocation(allocation);
            }, 
            "allocation"_a)
        .def("getNumVars", [](PyAllocationTransform & self) 
            {
                return self.getRcPtr()->getNumVars();
            })
        .def("getVars", [](PyAllocationTransform & self)
            {
                std::vector<float> vars;
                vars.resize(self.getRcPtr()->getNumVars());
                self.getRcPtr()->getVars(vars.data());
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
