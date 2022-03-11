// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

void bindPyFileRules(py::module & m)
{
    auto clsFileRules = 
        py::class_<FileRules, FileRulesRcPtr>(
            m.attr("FileRules"))

        .def(py::init(&FileRules::Create),
             DOC(FileRules, Create))

        .def("__deepcopy__", [](const ConstFileRulesRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def("getNumEntries", &FileRules::getNumEntries,
             DOC(FileRules, getNumEntries))
        .def("getIndexForRule", &FileRules::getIndexForRule, "ruleName"_a,
             DOC(FileRules, getIndexForRule))
        .def("getName", &FileRules::getName, "ruleIndex"_a,
             DOC(FileRules, getName))
        .def("getPattern", &FileRules::getPattern, "ruleIndex"_a,
             DOC(FileRules, getPattern))
        .def("setPattern", &FileRules::setPattern, "ruleIndex"_a, "pattern"_a,
             DOC(FileRules, setPattern))
        .def("getExtension", &FileRules::getExtension, "ruleIndex"_a,
             DOC(FileRules, getExtension))
        .def("setExtension", &FileRules::setExtension, "ruleIndex"_a, "extension"_a,
             DOC(FileRules, setExtension))
        .def("getRegex", &FileRules::getRegex, "ruleIndex"_a,
             DOC(FileRules, getRegex))
        .def("setRegex", &FileRules::setRegex, "ruleIndex"_a, "regex"_a,
             DOC(FileRules, setRegex))
        .def("getColorSpace", &FileRules::getColorSpace, "ruleIndex"_a,
             DOC(FileRules, getColorSpace))
        .def("setColorSpace", &FileRules::setColorSpace, "ruleIndex"_a, "colorSpace"_a,
             DOC(FileRules, setColorSpace))
        .def("getNumCustomKeys", &FileRules::getNumCustomKeys, "ruleIndex"_a,
             DOC(FileRules, getNumCustomKeys))
        .def("getCustomKeyName", &FileRules::getCustomKeyName, "ruleIndex"_a, "key"_a,
             DOC(FileRules, getCustomKeyName))
        .def("getCustomKeyValue", &FileRules::getCustomKeyValue, "ruleIndex"_a, "key"_a,
             DOC(FileRules, getCustomKeyValue))
        .def("setCustomKey", &FileRules::setCustomKey, "ruleIndex"_a, "key"_a, "value"_a,
             DOC(FileRules, setCustomKey))
        .def("insertRule",
             (void (FileRules::*)(size_t, const char *, const char *, const char *, const char *))
             &FileRules::insertRule,
             "ruleIndex"_a, "name"_a, "colorSpace"_a, "pattern"_a, "extension"_a,
             DOC(FileRules, insertRule))
        .def("insertRule",
             (void (FileRules::*)(size_t, const char *, const char *, const char *))
             &FileRules::insertRule,
             "ruleIndex"_a, "name"_a, "colorSpace"_a, "regex"_a,
             DOC(FileRules, insertRule, 2))
        .def("insertPathSearchRule", &FileRules::insertPathSearchRule, "ruleIndex"_a,
             DOC(FileRules, insertPathSearchRule))
        .def("setDefaultRuleColorSpace", &FileRules::setDefaultRuleColorSpace, "colorSpace"_a,
             DOC(FileRules, setDefaultRuleColorSpace))
        .def("removeRule", &FileRules::removeRule, "ruleIndex"_a,
             DOC(FileRules, removeRule))
        .def("increaseRulePriority", &FileRules::increaseRulePriority, "ruleIndex"_a,
             DOC(FileRules, increaseRulePriority))
        .def("decreaseRulePriority", &FileRules::decreaseRulePriority, "ruleIndex"_a,
             DOC(FileRules, decreaseRulePriority))
        .def("isDefault", &FileRules::isDefault, DOC(FileRules, isDefault));

    defRepr(clsFileRules);

    m.attr("DEFAULT_RULE_NAME") = FileRules::DefaultRuleName;
    m.attr("FILE_PATH_SEARCH_RULE_NAME") = FileRules::FilePathSearchRuleName;
}

} // namespace OCIO_NAMESPACE
