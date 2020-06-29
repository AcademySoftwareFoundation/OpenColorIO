// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

enum ViewingRuleIterator
{
    IT_VIEWING_RULE_COLOR_SPACE = 0,
    IT_VIEWING_RULE_ENCODING
};

using ViewingRuleColorSpaceIterator = PyIterator<ViewingRulesRcPtr, IT_VIEWING_RULE_COLOR_SPACE, size_t>;
using ViewingRuleEncodingIterator = PyIterator<ViewingRulesRcPtr, IT_VIEWING_RULE_ENCODING, size_t>;

void bindPyViewingRules(py::module & m)
{
    auto cls = py::class_<ViewingRules, ViewingRulesRcPtr /* holder */>(m, "ViewingRules")
        .def(py::init(&ViewingRules::Create))
                    
        .def("getNumEntries", &ViewingRules::getNumEntries)
        .def("getIndexForRule", &ViewingRules::getIndexForRule, "ruleName"_a)
        .def("getName", &ViewingRules::getName, "ruleIndex"_a)
        .def("getColorSpaces",
             [](ViewingRulesRcPtr & self, size_t ruleIndex)
             {
                 return ViewingRuleColorSpaceIterator(self, ruleIndex);
             },
            "ruleIndex"_a)
        .def("addColorSpace", &ViewingRules::addColorSpace, "ruleIndex"_a, "colorSpaceName"_a)
        .def("removeColorSpace", &ViewingRules::removeColorSpace, "ruleIndex"_a, "colorSpaceIndex"_a)
        .def("getEncodings",
             [](ViewingRulesRcPtr & self, size_t ruleIndex)
             {
                 return ViewingRuleEncodingIterator(self, ruleIndex);
             },
            "ruleIndex"_a)
        .def("addEncoding", &ViewingRules::addEncoding, "ruleIndex"_a, "encodingName"_a)
        .def("removeEncoding", &ViewingRules::removeEncoding, "ruleIndex"_a, "encodingIndex"_a)
        .def("getNumCustomKeys", &ViewingRules::getNumCustomKeys, "ruleIndex"_a)
        .def("getCustomKeyName", &ViewingRules::getCustomKeyName, "ruleIndex"_a, "key"_a)
        .def("getCustomKeyValue", &ViewingRules::getCustomKeyValue, "ruleIndex"_a, "key"_a)
        .def("setCustomKey", &ViewingRules::setCustomKey, "ruleIndex"_a, "key"_a, "value"_a)
        .def("insertRule", 
             (void (ViewingRules::*)(size_t, const char *)) &ViewingRules::insertRule,
             "ruleIndex"_a, "name"_a)
        .def("removeRule", &ViewingRules::removeRule, "ruleIndex"_a);

    py::class_<ViewingRuleColorSpaceIterator>(cls, "ViewingRuleColorSpaceIterator")
        .def("__len__", [](ViewingRuleColorSpaceIterator & it) { return it.m_obj->getNumColorSpaces(std::get<0>(it.m_args)); })
        .def("__getitem__", [](ViewingRuleColorSpaceIterator & it, size_t i)
            { 
                it.checkIndex(static_cast<int>(i),
                              static_cast<int>(it.m_obj->getNumColorSpaces(std::get<0>(it.m_args))));
                return it.m_obj->getColorSpace(std::get<0>(it.m_args), i);
            })
        .def("__iter__", [](ViewingRuleColorSpaceIterator & it) -> ViewingRuleColorSpaceIterator & { return it; })
        .def("__next__", [](ViewingRuleColorSpaceIterator & it)
            {
                int i = it.nextIndex(static_cast<int>(it.m_obj->getNumColorSpaces(std::get<0>(it.m_args))));
                return it.m_obj->getColorSpace(std::get<0>(it.m_args), i);
            });

    py::class_<ViewingRuleEncodingIterator>(cls, "ViewingRuleEncodingIterator")
        .def("__len__", [](ViewingRuleEncodingIterator & it) { return it.m_obj->getNumEncodings(std::get<0>(it.m_args)); })
        .def("__getitem__", [](ViewingRuleEncodingIterator & it, size_t i)
            { 
                it.checkIndex(static_cast<int>(i),
                              static_cast<int>(it.m_obj->getNumEncodings(std::get<0>(it.m_args))));
                return it.m_obj->getEncoding(std::get<0>(it.m_args), i);
            })
        .def("__iter__", [](ViewingRuleEncodingIterator & it) -> ViewingRuleEncodingIterator & { return it; })
        .def("__next__", [](ViewingRuleEncodingIterator & it)
            {
                int i = it.nextIndex(static_cast<int>(it.m_obj->getNumEncodings(std::get<0>(it.m_args))));
                return it.m_obj->getEncoding(std::get<0>(it.m_args), i);
            });
}

} // namespace OCIO_NAMESPACE
