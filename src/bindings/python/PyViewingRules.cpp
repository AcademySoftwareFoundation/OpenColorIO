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
    auto clsViewingRules = 
        py::class_<ViewingRules, ViewingRulesRcPtr>(
            m.attr("ViewingRules"));

    auto clsViewingRuleColorSpaceIterator = 
        py::class_<ViewingRuleColorSpaceIterator>(
            clsViewingRules, "ViewingRuleColorSpaceIterator");

    auto clsViewingRuleEncodingIterator = 
        py::class_<ViewingRuleEncodingIterator>(
            clsViewingRules, "ViewingRuleEncodingIterator");

    clsViewingRules
        .def(py::init(&ViewingRules::Create),
             DOC(ViewingRules, Create))

        .def("__deepcopy__", [](const ConstViewingRulesRcPtr & self, py::dict)
            {
                 return self->createEditableCopy();
            },
            "memo"_a)

        .def("getNumEntries", &ViewingRules::getNumEntries,
             DOC(ViewingRules, getNumEntries))
        .def("getIndexForRule", &ViewingRules::getIndexForRule, "ruleName"_a,
             DOC(ViewingRules, getIndexForRule))
        .def("getName", &ViewingRules::getName, "ruleIndex"_a,
             DOC(ViewingRules, getName))
        .def("getColorSpaces",
             [](ViewingRulesRcPtr & self, size_t ruleIndex)
             {
                 return ViewingRuleColorSpaceIterator(self, ruleIndex);
             },
            "ruleIndex"_a)
        .def("addColorSpace", &ViewingRules::addColorSpace, "ruleIndex"_a, "colorSpaceName"_a,
             DOC(ViewingRules, addColorSpace))
        .def("removeColorSpace", &ViewingRules::removeColorSpace, 
             "ruleIndex"_a, "colorSpaceIndex"_a,
             DOC(ViewingRules, removeColorSpace))
        .def("getEncodings",
             [](ViewingRulesRcPtr & self, size_t ruleIndex)
             {
                 return ViewingRuleEncodingIterator(self, ruleIndex);
             },
            "ruleIndex"_a)
        .def("addEncoding", &ViewingRules::addEncoding, "ruleIndex"_a, "encodingName"_a,
             DOC(ViewingRules, addEncoding))
        .def("removeEncoding", &ViewingRules::removeEncoding, "ruleIndex"_a, "encodingIndex"_a,
             DOC(ViewingRules, removeEncoding))
        .def("getNumCustomKeys", &ViewingRules::getNumCustomKeys, "ruleIndex"_a,
             DOC(ViewingRules, getNumCustomKeys))
        .def("getCustomKeyName", &ViewingRules::getCustomKeyName, "ruleIndex"_a, "key"_a,
             DOC(ViewingRules, getCustomKeyName))
        .def("getCustomKeyValue", &ViewingRules::getCustomKeyValue, "ruleIndex"_a, "key"_a,
             DOC(ViewingRules, getCustomKeyValue))
        .def("setCustomKey", &ViewingRules::setCustomKey, "ruleIndex"_a, "key"_a, "value"_a,
             DOC(ViewingRules, setCustomKey))
        .def("insertRule", 
             (void (ViewingRules::*)(size_t, const char *)) &ViewingRules::insertRule,
             "ruleIndex"_a, "name"_a,
             DOC(ViewingRules, insertRule))
        .def("removeRule", &ViewingRules::removeRule, "ruleIndex"_a,
             DOC(ViewingRules, removeRule));

    defRepr(clsViewingRules);

    clsViewingRuleColorSpaceIterator
        .def("__len__", [](ViewingRuleColorSpaceIterator & it) 
            { 
                return it.m_obj->getNumColorSpaces(std::get<0>(it.m_args)); 
            })
        .def("__getitem__", [](ViewingRuleColorSpaceIterator & it, size_t i)
            {
                size_t ruleIndex = std::get<0>(it.m_args);
                it.checkIndex(static_cast<int>(i),
                              static_cast<int>(it.m_obj->getNumColorSpaces(ruleIndex)));
                return it.m_obj->getColorSpace(std::get<0>(it.m_args), i);
            })
        .def("__iter__", [](ViewingRuleColorSpaceIterator & it) -> ViewingRuleColorSpaceIterator &
            { 
                return it; 
            })
        .def("__next__", [](ViewingRuleColorSpaceIterator & it)
            {
                size_t ruleIndex = std::get<0>(it.m_args);
                int i = it.nextIndex(static_cast<int>(it.m_obj->getNumColorSpaces(ruleIndex)));
                return it.m_obj->getColorSpace(std::get<0>(it.m_args), i);
            });

    clsViewingRuleEncodingIterator
        .def("__len__", [](ViewingRuleEncodingIterator & it) 
            { 
                return it.m_obj->getNumEncodings(std::get<0>(it.m_args)); 
            })
        .def("__getitem__", [](ViewingRuleEncodingIterator & it, size_t i)
            { 
                size_t ruleIndex = std::get<0>(it.m_args);
                it.checkIndex(static_cast<int>(i),
                              static_cast<int>(it.m_obj->getNumEncodings(ruleIndex)));
                return it.m_obj->getEncoding(std::get<0>(it.m_args), i);
            })
        .def("__iter__", [](ViewingRuleEncodingIterator & it) -> ViewingRuleEncodingIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ViewingRuleEncodingIterator & it)
            {
                size_t ruleIndex = std::get<0>(it.m_args);
                int i = it.nextIndex(static_cast<int>(it.m_obj->getNumEncodings(ruleIndex)));
                return it.m_obj->getEncoding(std::get<0>(it.m_args), i);
            });
}

} // namespace OCIO_NAMESPACE
