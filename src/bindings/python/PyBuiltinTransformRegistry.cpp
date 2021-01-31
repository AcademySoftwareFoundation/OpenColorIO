// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyBuiltinTransformRegistry.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace 
{

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
    auto clsBuiltinTransformRegistry = 
        py::class_<PyBuiltinTransformRegistry>(
            m.attr("BuiltinTransformRegistry"));

    auto clsBuiltinStyleIterator = 
        py::class_<BuiltinStyleIterator>(
            clsBuiltinTransformRegistry, "BuiltinStyleIterator");

    auto clsBuiltinIterator = 
        py::class_<BuiltinIterator>(
            clsBuiltinTransformRegistry, "BuiltinIterator");

    clsBuiltinTransformRegistry
        .def(py::init<>(), DOC(BuiltinTransformRegistry, Get))

        .def("__iter__", [](PyBuiltinTransformRegistry & self)
            { 
                return BuiltinStyleIterator(self); 
            },
             DOC(BuiltinTransformRegistry, getBuiltinStyle))
        .def("__len__", [](PyBuiltinTransformRegistry & self) { return self.getNumBuiltins(); },
             DOC(BuiltinTransformRegistry, getNumBuiltins))
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
            },
             DOC(BuiltinTransformRegistry, getBuiltinDescription))
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

    clsBuiltinStyleIterator
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

    clsBuiltinIterator
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
