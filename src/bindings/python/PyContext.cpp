// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
namespace 
{

enum ContextIterator
{
    IT_STRING_VAR_NAME = 0,
    IT_STRING_VAR,
    IT_SEARCH_PATH
};

using StringVarNameIterator = PyIterator<ContextRcPtr, IT_STRING_VAR_NAME>;
using StringVarIterator     = PyIterator<ContextRcPtr, IT_STRING_VAR>;
using SearchPathIterator    = PyIterator<ContextRcPtr, IT_SEARCH_PATH>;

std::vector<std::string> getSearchPathsStdVec(const ContextRcPtr & p)
{
    std::vector<std::string> searchPaths;
    for (int i = 0; i < p->getNumSearchPaths(); i++)
    {
        searchPaths.push_back(std::string(p->getSearchPath(i)));
    }
    return searchPaths;
}

std::map<std::string, std::string> getStringVarsStdMap(const ContextRcPtr & p)
{
    std::map<std::string, std::string> stringVars;
    for (int i = 0; i < p->getNumStringVars(); i++)
    {
        const char * name = p->getStringVarNameByIndex(i);
        stringVars[std::string(name)] = std::string(p->getStringVar(name));
    }
    return stringVars;
}

} // namespace

void bindPyContext(py::module & m)
{
    ContextRcPtr DEFAULT = Context::Create();

    auto cls = py::class_<Context, ContextRcPtr /* holder */>(m, "Context")
        .def(py::init(&Context::Create))
        .def(py::init([](const std::string & workingDir,
                         const std::vector<std::string> & searchPaths,
                         const std::map<std::string, std::string> stringVars,
                         EnvironmentMode environmentMode) 
            {
                ContextRcPtr p = Context::Create();
                if (!workingDir.empty()) { p->setWorkingDir(workingDir.c_str()); }
                
                if (!searchPaths.empty())
                {
                    for (const auto & path : searchPaths)
                    {
                        p->addSearchPath(path.c_str());
                    }
                }

                if (!stringVars.empty())
                {
                    for (const auto & var: stringVars)
                    {
                        p->setStringVar(var.first.c_str(), var.second.c_str());
                    }
                }

                p->setEnvironmentMode(environmentMode);
                return p;
            }), 
             "workingDir"_a = DEFAULT->getWorkingDir(),
             "searchPaths"_a = getSearchPathsStdVec(DEFAULT),
             "stringVars"_a = getStringVarsStdMap(DEFAULT),
             "environmentMode"_a = DEFAULT->getEnvironmentMode())  

        .def("__iter__", [](ContextRcPtr & self) { return StringVarNameIterator(self); })
        .def("__len__", &Context::getNumStringVars)
        .def("__getitem__", [](ContextRcPtr & self, const std::string & name) -> bool
            { 
                for (int i = 0; i < self->getNumStringVars(); i++)
                {
                    const char * varName = self->getStringVarNameByIndex(i);
                    if (StringUtils::Compare(std::string(varName), name))
                    {
                        return self->getStringVar(name.c_str());
                    }
                }
                
                std::ostringstream os;
                os << "'" << name << "'";
                throw py::key_error(os.str().c_str());
            },
             "name"_a)
        .def("__setitem__", &Context::setStringVar, "name"_a, "value"_a)
        .def("__contains__", [](ContextRcPtr & self, const std::string & name) -> bool
            { 
                for (int i = 0; i < self->getNumStringVars(); i++)
                {
                    const char * varName = self->getStringVarNameByIndex(i);
                    if (StringUtils::Compare(std::string(varName), name))
                    {
                        return true;
                    }
                }
                return false;
            },
            "name"_a)

        .def("getCacheID", &Context::getCacheID)
        .def("getSearchPath", (const char * (Context::*)() const) &Context::getSearchPath)
        .def("setSearchPath", &Context::setSearchPath, "path"_a)
        .def("getSearchPaths", [](ContextRcPtr & self) { return SearchPathIterator(self); })
        .def("clearSearchPaths", &Context::clearSearchPaths)
        .def("addSearchPath", &Context::addSearchPath, "path"_a)
        .def("getWorkingDir", &Context::getWorkingDir)
        .def("setWorkingDir", &Context::setWorkingDir, "dirname"_a)
        .def("getStringVars", [](ContextRcPtr & self) { return StringVarIterator(self); })
        .def("clearStringVars", &Context::clearStringVars)
        .def("getEnvironmentMode", &Context::getEnvironmentMode)
        .def("setEnvironmentMode", &Context::setEnvironmentMode, "mode"_a)
        .def("loadEnvironment", &Context::loadEnvironment)
        .def("resolveStringVar", &Context::resolveStringVar, "val"_a)
        .def("resolveFileLocation", &Context::resolveFileLocation, "filename"_a);

    defStr(cls);

    py::class_<StringVarNameIterator>(cls, "StringVarNameIterator")
        .def("__len__", [](StringVarNameIterator & it) 
            { 
                return it.m_obj->getNumStringVars(); 
            })
        .def("__getitem__", [](StringVarNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumStringVars());
                return it.m_obj->getStringVarNameByIndex(i);
            })
        .def("__iter__", [](StringVarNameIterator & it) -> StringVarNameIterator & { return it; })
        .def("__next__", [](StringVarNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumStringVars());
                return it.m_obj->getStringVarNameByIndex(i);
            });

    py::class_<StringVarIterator>(cls, "StringVarIterator")
        .def("__len__", [](StringVarIterator & it) { return it.m_obj->getNumStringVars(); })
        .def("__getitem__", [](StringVarIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumStringVars());
                const char * name = it.m_obj->getStringVarNameByIndex(i);
                return py::make_tuple(name, it.m_obj->getStringVar(name));
            })
        .def("__iter__", [](StringVarIterator & it) -> StringVarIterator & { return it; })
        .def("__next__", [](StringVarIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumStringVars());
                const char * name = it.m_obj->getStringVarNameByIndex(i);
                return py::make_tuple(name, it.m_obj->getStringVar(name));
            });

    py::class_<SearchPathIterator>(cls, "SearchPathIterator")
        .def("__len__", [](SearchPathIterator & it) { return it.m_obj->getNumSearchPaths(); })
        .def("__getitem__", [](SearchPathIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumSearchPaths());
                return it.m_obj->getSearchPath(i);
            })
        .def("__iter__", [](SearchPathIterator & it) -> SearchPathIterator & { return it; })
        .def("__next__", [](SearchPathIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumSearchPaths());
                return it.m_obj->getSearchPath(i);
            });
}

} // namespace OCIO_NAMESPACE
