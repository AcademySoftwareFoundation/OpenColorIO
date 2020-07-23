// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

void bindPyFileRules(py::module & m)
{
    py::class_<FileRules, FileRulesRcPtr /* holder */>(m, "FileRules", DS(FileRules))
        .def(py::init(&FileRules::Create), DS(FileRules, Create))
                    
        .def("getNumEntries", &FileRules::getNumEntries, DS(FileRules, getNumEntries))
        .def("getIndexForRule", &FileRules::getIndexForRule, DS(FileRules, getIndexForRule), "ruleName"_a)
        .def("getName", &FileRules::getName, DS(FileRules, getName), "ruleIndex"_a)
        .def("getPattern", &FileRules::getPattern, DS(FileRules, getPattern), "ruleIndex"_a)
        .def("setPattern", &FileRules::setPattern, DS(FileRules, setPattern), "ruleIndex"_a, "pattern"_a)
        .def("getExtension", &FileRules::getExtension, DS(FileRules, getExtension), "ruleIndex"_a)
        .def("setExtension", &FileRules::setExtension, DS(FileRules, setExtension), "ruleIndex"_a, "extension"_a)
        .def("getRegex", &FileRules::getRegex, DS(FileRules, getRegex), "ruleIndex"_a)
        .def("setRegex", &FileRules::setRegex, DS(FileRules, setRegex), "ruleIndex"_a, "regex"_a)
        .def("getColorSpace", &FileRules::getColorSpace, DS(FileRules, getColorSpace), "ruleIndex"_a)
        .def("setColorSpace", &FileRules::setColorSpace, DS(FileRules, setColorSpace), "ruleIndex"_a, "colorSpace"_a)
        .def("getNumCustomKeys", &FileRules::getNumCustomKeys, DS(FileRules, getNumCustomKeys), "ruleIndex"_a)
        .def("getCustomKeyName", &FileRules::getCustomKeyName, DS(FileRules, getCustomKeyName), "ruleIndex"_a, "key"_a)
        .def("getCustomKeyValue", &FileRules::getCustomKeyValue, DS(FileRules, getCustomKeyValue), "ruleIndex"_a, "key"_a)
        .def("setCustomKey", &FileRules::setCustomKey, DS(FileRules, setCustomKey), "ruleIndex"_a, "key"_a, "value"_a)
        .def("insertRule", 
             (void (FileRules::*)(size_t, const char *, const char *, const char *, const char *))
             &FileRules::insertRule, 
             DS(FileRules, insertRule),
             "ruleIndex"_a, "name"_a, "colorSpace"_a, "pattern"_a, "extension"_a)
        .def("insertRule", 
             (void (FileRules::*)(size_t, const char *, const char *, const char *)) 
             &FileRules::insertRule, 
             DS(FileRules, insertRule),
             "ruleIndex"_a, "name"_a, "colorSpace"_a, "regex"_a)
        .def("insertPathSearchRule", &FileRules::insertPathSearchRule, DS(FileRules, insertPathSearchRule), "ruleIndex"_a)
        .def("setDefaultRuleColorSpace", &FileRules::setDefaultRuleColorSpace,
             DS(FileRules, setDefaultRuleColorSpace),
             "colorSpace"_a)
        .def("removeRule", &FileRules::removeRule, DS(FileRules, removeRule), "ruleIndex"_a)
        .def("increaseRulePriority", &FileRules::increaseRulePriority,
             DS(FileRules, increaseRulePriority),
             "ruleIndex"_a)
        .def("decreaseRulePriority", &FileRules::decreaseRulePriority,
             DS(FileRules, decreaseRulePriority),
             "ruleIndex"_a);
}

} // namespace OCIO_NAMESPACE
