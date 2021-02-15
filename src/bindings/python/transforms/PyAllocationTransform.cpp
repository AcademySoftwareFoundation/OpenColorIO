// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{
namespace 
{

std::vector<float> getVarsStdVec(const AllocationTransformRcPtr & p) 
{
    std::vector<float> vars;
    vars.resize(p->getNumVars());
    p->getVars(vars.data());
    return vars;
}

void setVars(AllocationTransformRcPtr & p, const std::vector<float> & vars)
{
    if (vars.size() < 2 || vars.size() > 3)
    {
        throw Exception("vars must be a float array, size 2 or 3");
    }
    p->setVars((int)vars.size(), vars.data());
}

} // namespace

void bindPyAllocationTransform(py::module & m)
{
    AllocationTransformRcPtr DEFAULT = AllocationTransform::Create();

    auto clsAllocationTransform = 
        py::class_<AllocationTransform, AllocationTransformRcPtr, Transform>(
            m.attr("AllocationTransform"))

        .def(py::init(&AllocationTransform::Create), 
             DOC(AllocationTransform, Create))
        .def(py::init([](Allocation allocation, 
                         const std::vector<float> & vars, 
                         TransformDirection dir) 
            {
                AllocationTransformRcPtr p = AllocationTransform::Create();
                p->setAllocation(allocation);
                if (!vars.empty()) { setVars(p, vars); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "allocation"_a = DEFAULT->getAllocation(), 
             "vars"_a = getVarsStdVec(DEFAULT),
             "direction"_a = DEFAULT->getDirection(),
             DOC(AllocationTransform, Create))

        .def("getAllocation", &AllocationTransform::getAllocation, 
             DOC(AllocationTransform, getAllocation))
        .def("setAllocation", &AllocationTransform::setAllocation,
             "allocation"_a,
             DOC(AllocationTransform, setAllocation))
        .def("getVars", [](AllocationTransformRcPtr self)
            {
                return getVarsStdVec(self);
            },
             DOC(AllocationTransform, getVars))
        .def("setVars", [](AllocationTransformRcPtr self, const std::vector<float> & vars)
            { 
                setVars(self, vars);
            }, 
             "vars"_a,
             DOC(AllocationTransform, setVars));

    defRepr(clsAllocationTransform);
}

} // namespace OCIO_NAMESPACE
