// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

void bindPyBuiltinTransformRegistry(py::module & m)
{
    // Wrapper to preserve the BuiltinTransformRegistry singleton.
    class Wrapper
    {
    public:
        Wrapper() = default;
        Wrapper(const Wrapper &) = delete;
        Wrapper & operator=(const Wrapper &) = delete;
        ~Wrapper() = default;

        size_t getNumBuiltins() const noexcept
        {
        	return BuiltinTransformRegistry::Get()->getNumBuiltins();
        }

        const char * getBuiltinStyle(size_t idx) const
        {
        	return BuiltinTransformRegistry::Get()->getBuiltinStyle(idx);
        }

        const char * getBuiltinDescription(size_t idx) const
        {
        	return BuiltinTransformRegistry::Get()->getBuiltinDescription(idx);
        }
    };

    py::class_<Wrapper>(m, "BuiltinTransformRegistry")
        .def(py::init<>())

        .def("getNumBuiltins",        &Wrapper::getNumBuiltins)
        .def("getBuiltinStyle",       &Wrapper::getBuiltinStyle)
        .def("getBuiltinDescription", &Wrapper::getBuiltinDescription);
}

} // namespace OCIO_NAMESPACE
