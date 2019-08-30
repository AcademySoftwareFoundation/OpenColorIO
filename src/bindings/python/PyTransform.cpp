// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

OCIO_NAMESPACE_ENTER
{

namespace 
{
    const std::vector<float> ALLOCATION_DEFAULT_ARGS = { 0.0f, 1.0f };
    const std::vector<PyTransform> GROUP_DEFAULT_TRANSFORMS = {};
}

void bindPyTransform(py::module & m)
{
    // Transform
    py::class_<PyTransform>(m, "Transform");

    // AllocaionTransform
    py::class_<PyAllocationTransform, PyTransform>(m, "AllocationTransform")
        .def(py::init<>())
        .def(py::init([](Allocation allocation, 
                         std::vector<float> vars, 
                         TransformDirection direction) 
            {
                PyAllocationTransform obj;
                AllocationTransformRcPtr ptr = obj.getTransformRcPtr();
                ptr->setAllocation(allocation);
                ptr->setVars(vars.size(), vars.data());
                ptr->setDirection(direction);
                return obj;
            }), 
            "allocation"_a = ALLOCATION_UNIFORM, 
            "vars"_a = ALLOCATION_DEFAULT_ARGS,
            "direction"_a = TRANSFORM_DIR_FORWARD)

        .def("__repr__", &PyAllocationTransform::repr)
        .def("isEditable", &PyAllocationTransform::isEditable)
        .def("createEditableCopy", &PyAllocationTransform::createEditableCopy)
        .def("validate", &PyAllocationTransform::validate)
        .def("getDirection", &PyAllocationTransform::getDirection)
        .def("setDirection", &PyAllocationTransform::setDirection, "direction"_a)

        .def("getAllocation", [](PyAllocationTransform & self) 
            {
                return self.getConstTransformRcPtr()->getAllocation();
            })
        .def("setAllocation", [](PyAllocationTransform & self, Allocation allocation) 
            {
                self.getTransformRcPtr()->setAllocation(allocation);
            }, 
            "allocation"_a)

        .def("getNumVars", [](PyAllocationTransform & self) 
            {
                return self.getConstTransformRcPtr()->getNumVars();
            })
        .def("getVars", [](PyAllocationTransform & self)
            {
                std::vector<float> vars;
                vars.resize(self.getConstTransformRcPtr()->getNumVars());
                self.getConstTransformRcPtr()->getVars(vars.data());
                return vars;
            })
        .def("setVars", [](PyAllocationTransform & self, std::vector<float> vars) 
            { 
                self.getTransformRcPtr()->setVars(vars.size(), vars.data());
            }, 
            "vars"_a);

    // GroupTransform
    py::class_<PyGroupTransform, PyTransform>(m, "GroupTransform")
        .def(py::init<>())
        .def(py::init([](std::vector<PyTransform> transforms, 
                         TransformDirection direction) 
            {
                PyGroupTransform obj;
                GroupTransformRcPtr ptr = obj.getTransformRcPtr();
                for(auto & t : transforms)
                {
                    ptr->push_back(t.getConstRcPtr());
                }
                ptr->setDirection(direction);
                return obj;
            }), 
            "transforms"_a = GROUP_DEFAULT_TRANSFORMS,
            "direction"_a = TRANSFORM_DIR_FORWARD)

        .def("__repr__", &PyGroupTransform::repr)
        .def("isEditable", &PyGroupTransform::isEditable)
        .def("createEditableCopy", &PyGroupTransform::createEditableCopy)
        .def("validate", &PyGroupTransform::validate)
        .def("getDirection", &PyGroupTransform::getDirection)
        .def("setDirection", &PyGroupTransform::setDirection, "direction"_a)

        .def("getTransform", [](PyGroupTransform & self, int index) 
            {
                return PyTransform(self.getConstTransformRcPtr()->getTransform(index));
            },
            "index"_a)
        .def("getTransforms", [](PyGroupTransform & self) 
            {
                ConstGroupTransformRcPtr ptr = self.getConstTransformRcPtr();
                std::vector<PyTransform> transforms;
                for(int i = 0; i < ptr->size(); i++)
                {
                    transforms.push_back(PyTransform(ptr->getTransform(i)));
                }
                return transforms;
            })
        .def("size", [](PyGroupTransform & self) 
            {
                return self.getConstTransformRcPtr()->size();
            })
        .def("push_back", [](PyGroupTransform & self, const PyTransform & transform)
            {
                self.getTransformRcPtr()->push_back(transform.getConstRcPtr());
            })
        .def("clear", [](PyGroupTransform & self) 
            {
                self.getTransformRcPtr()->empty();
            })
        .def("empty", [](PyGroupTransform & self) 
            {
                return self.getConstTransformRcPtr()->empty();
            });
}

}
OCIO_NAMESPACE_EXIT
