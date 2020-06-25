// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace 
{

// Wrapper to preserve the BuiltinTransformRegistry singleton.
class PyBuiltinTransformRegistry
{
public:
    PyBuiltinTransformRegistry() = default;
    ~PyBuiltinTransformRegistry() = default;

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

enum BuiltinTransformRegistryIterator
{
    IT_BUILTIN_STYLE = 0,
    IT_BUILTIN
};

using BuiltinStyleIterator = PyIterator<PyBuiltinTransformRegistry, IT_BUILTIN_STYLE>;
using BuiltinIterator = PyIterator<PyBuiltinTransformRegistry, IT_BUILTIN>;

} // namespace

void bindPyBuiltinTransformRegistry(py::module & m)
{
    auto cls = py::class_<PyBuiltinTransformRegistry>(m, "BuiltinTransformRegistry")
        .def(py::init<>())

        .def("__iter__", [](PyBuiltinTransformRegistry & self)
            { 
                return BuiltinStyleIterator(self); 
            })
        .def("__len__", [](PyBuiltinTransformRegistry & self) { return self.getNumBuiltins(); })
        .def("__getitem__", [](PyBuiltinTransformRegistry & self, const std::string & style) 
            { 
                for (size_t i = 0; i < self.getNumBuiltins(); i++)
                {
                    const char * thisStyle = self.getBuiltinStyle(i);
                    if (StringUtils::Compare(std::string(thisStyle), style))
                    {
                        return self.getBuiltinDescription(i);
                    }
                }
                
                std::ostringstream os;
                os << "'" << style << "'";
                throw py::key_error(os.str().c_str());
            })
        .def("__contains__", [](PyBuiltinTransformRegistry & self, const std::string & style) 
            { 
                for (size_t i = 0; i < self.getNumBuiltins(); i++)
                {
                    const char * thisStyle = self.getBuiltinStyle(i);
                    if (StringUtils::Compare(std::string(thisStyle), style))
                    {
                        return true;
                    }
                }
                return false;
            })

        .def("getBuiltins", [](PyBuiltinTransformRegistry & self) 
            {
                return BuiltinIterator(self);
            });

    py::class_<BuiltinStyleIterator>(cls, "BuiltinStyleIterator")
        .def("__len__", [](BuiltinStyleIterator & it) { return it.m_obj.getNumBuiltins(); })
        .def("__getitem__", [](BuiltinStyleIterator & it, int i) 
            { 
                // BuiltinTransformRegistry provides index check with exception
                return it.m_obj.getBuiltinStyle(i);
            })
        .def("__iter__", [](BuiltinStyleIterator & it) -> BuiltinStyleIterator & { return it; })
        .def("__next__", [](BuiltinStyleIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj.getNumBuiltins());
                return it.m_obj.getBuiltinStyle(i);
            });

    py::class_<BuiltinIterator>(cls, "BuiltinIterator")
        .def("__len__", [](BuiltinIterator & it) { return it.m_obj.getNumBuiltins(); })
        .def("__getitem__", [](BuiltinIterator & it, int i) 
            { 
                // BuiltinTransformRegistry provides index check with exception
                return py::make_tuple(it.m_obj.getBuiltinStyle(i), 
                                      it.m_obj.getBuiltinDescription(i));
            })
        .def("__iter__", [](BuiltinIterator & it) -> BuiltinIterator & { return it; })
        .def("__next__", [](BuiltinIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj.getNumBuiltins());
                return py::make_tuple(it.m_obj.getBuiltinStyle(i), 
                                      it.m_obj.getBuiltinDescription(i));
            });
}

} // namespace OCIO_NAMESPACE
