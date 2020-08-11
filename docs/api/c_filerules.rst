
FileRules
*********

.. class:: FileRules


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: FileRulesRcPtr OpenColorIO::FileRules::Create()

         Creates FileRules for a `Config`. File rules will contain the default rule using the default role. The default rule cannot be removed. 

      .. cpp:function:: FileRulesRcPtr OpenColorIO::FileRules::createEditableCopy() const

         The method clones the content decoupling the two instances. 

   .. group-tab:: Python

      .. py:method:: FileRules.__init__(self: PyOpenColorIO.FileRules) -> None


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const char *OpenColorIO::FileRules::getName(size_t ruleIndex) const

         Get name of the rule. 

      **Pattern**

      .. cpp:function:: const char *OpenColorIO::FileRules::getPattern(size_t ruleIndex) const

         Setting pattern will erase regex. 

      .. cpp:function:: void OpenColorIO::FileRules::setPattern(size_t ruleIndex, const char *pattern)

      **Extension**

      .. cpp:function:: const char *OpenColorIO::FileRules::getExtension(size_t ruleIndex) const

         Setting extension will erase regex. 

      .. cpp:function:: void OpenColorIO::FileRules::setExtension(size_t ruleIndex, const char *extension)

      **Regex**

      .. cpp:function:: const char *OpenColorIO::FileRules::getRegex(size_t ruleIndex) const

         Setting a regex will erase pattern & extension. 

      .. cpp:function:: void OpenColorIO::FileRules::setRegex(size_t ruleIndex, const char *regex)

      **ColorSpace**

      .. cpp:function:: const char *OpenColorIO::FileRules::getColorSpace(size_t ruleIndex) const

         Set the rule’s color space (may also be a role). 

      .. cpp:function:: void OpenColorIO::FileRules::setColorSpace(size_t ruleIndex, const char *colorSpace)

      .. cpp:function:: void OpenColorIO::FileRules::setDefaultRuleColorSpace(const char *colorSpace)

         Helper function to set the color space for the default rule. 

      **Keys**

      .. cpp:function:: size_t OpenColorIO::FileRules::getNumCustomKeys(size_t ruleIndex) const

         Get number of key/value pairs. 

      .. cpp:function:: const char *OpenColorIO::FileRules::getCustomKeyName(size_t ruleIndex, size_t key) const

         Get name of key. 

      .. cpp:function:: const char *OpenColorIO::FileRules::getCustomKeyValue(size_t ruleIndex, size_t key) const

         Get value for the key. 

      .. cpp:function:: void OpenColorIO::FileRules::setCustomKey(size_t ruleIndex, const char *key, const char *value)

         Adds a key/value or replace value if key exists. Setting a NULL or an empty value will erase the key. 

      **Rules**

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::FileRules::insertRule” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - void insertRule(size_t ruleIndex, const char *name, const char *colorSpace, const char *pattern, const char *extension)
            - void insertRule(size_t ruleIndex, const char *name, const char *colorSpace, const char *regex)

      .. cpp:function:: void OpenColorIO::FileRules::insertPathSearchRule(size_t ruleIndex)

         Helper function to insert a rule. 

         Uses Config:parseColorSpaceFromString to search the path for any of the color spaces named in the config (as per OCIO v1). 

      .. cpp:function:: void OpenColorIO::FileRules::removeRule(size_t ruleIndex)

         **Note**
            Default rule can’t be removed. Will throw if ruleIndex + 1 is not less than FileRules::getNumEntries . 

      .. cpp:function:: void OpenColorIO::FileRules::increaseRulePriority(size_t ruleIndex)

         Move a rule closer to the start of the list by one position. 

      .. cpp:function:: void OpenColorIO::FileRules::decreaseRulePriority(size_t ruleIndex)

         Move a rule closer to the end of the list by one position. 

      .. cpp:function:: size_t OpenColorIO::FileRules::getNumEntries() const noexcept

         Does include default rule. Result will be at least 1. 

      .. cpp:function:: size_t OpenColorIO::FileRules::getIndexForRule(const char *ruleName) const

         Get the index from the rule name. 

   .. group-tab:: Python

      .. py:method:: FileRules.getName(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      **Pattern**

      .. py:method:: FileRules.getPattern(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: FileRules.setPattern(self: PyOpenColorIO.FileRules, ruleIndex: int, pattern: str) -> None

      **Extension**

      .. py:method:: FileRules.getExtension(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: FileRules.setExtension(self: PyOpenColorIO.FileRules, ruleIndex: int, extension: str) -> None

      **Regex**

      .. py:method:: FileRules.getRegex(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: FileRules.setRegex(self: PyOpenColorIO.FileRules, ruleIndex: int, regex: str) -> None

      **ColorSpace**

      .. py:method:: FileRules.getColorSpace(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: FileRules.setColorSpace(self: PyOpenColorIO.FileRules, ruleIndex: int, colorSpace: str) -> None

      .. py:method:: FileRules.setDefaultRuleColorSpace(self: PyOpenColorIO.FileRules, colorSpace: str) -> None

      **Keys**

      .. py:method:: FileRules.getNumCustomKeys(self: PyOpenColorIO.FileRules, ruleIndex: int) -> int

      .. py:method:: FileRules.getCustomKeyName(self: PyOpenColorIO.FileRules, ruleIndex: int, key: int) -> str

      .. py:method:: FileRules.getCustomKeyValue(self: PyOpenColorIO.FileRules, ruleIndex: int, key: int) -> str

      .. py:method:: FileRules.setCustomKey(self: PyOpenColorIO.FileRules, ruleIndex: int, key: str, value: str) -> None

      **Rules**

      .. py:method:: FileRules.insertRule(*args, **kwargs)

         Overloaded function.

         1. insertRule(self: PyOpenColorIO.FileRules, ruleIndex: int, name: str, colorSpace: str, pattern: str, extension: str) -> None

         2. insertRule(self: PyOpenColorIO.FileRules, ruleIndex: int, name: str, colorSpace: str, regex: str) -> None

      .. py:method:: FileRules.insertPathSearchRule(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:method:: FileRules.removeRule(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:method:: FileRules.increaseRulePriority(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:method:: FileRules.decreaseRulePriority(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:method:: FileRules.getNumEntries(self: PyOpenColorIO.FileRules) -> int

      .. py:method:: FileRules.getIndexForRule(self: PyOpenColorIO.FileRules, ruleName: str) -> int
