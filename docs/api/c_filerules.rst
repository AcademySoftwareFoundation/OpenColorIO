..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

FileRules
*********

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.


The File Rules are a set of filepath to color space mappings that are evaluated
from first to last. The first rule to match is what determines which color space is
returned. There are four types of rules available. Each rule type has a name key that may
be used by applications to refer to that rule. Name values must be unique i.e. using a
case insensitive comparison. The other keys depend on the rule type:

Basic Rule: This is the basic rule type that uses Unix glob style pattern matching and
is thus very easy to use. It contains the keys:

* name: Name of the rule

* colorspace: Color space name to be returned.

* pattern: Glob pattern to be used for the main part of the name/path.

* extension: Glob pattern to be used for the file extension. Note that if glob tokens
  are not used, the extension will be used in a non-case-sensitive way by default.

Regex Rule: This is similar to the basic rule but allows additional capabilities for
power-users. It contains the keys:

* name: Name of the rule

* colorspace: Color space name to be returned.

* regex: Regular expression to be evaluated.

OCIO v1 style Rule: This rule allows the use of the OCIO v1 style, where the string
is searched for color space names from the config. This rule may occur 0 or 1 times
in the list. The position in the list prioritizes it with respect to the other rules.
StrictParsing is not used. If no color space is found in the path, the rule will not
match and the next rule will be considered.
See :cpp:func:`FileRules::insertPathSearchRule`.
It has the key:

* name: Must be "ColorSpaceNamePathSearch".

Default Rule: The file_rules must always end with this rule. If no prior rules match,
this rule specifies the color space applications will use.
See :cpp:func:`FileRules::setDefaultRuleColorSpace`.
It has the keys:

* name: must be "Default".

* colorspace : Color space name to be returned.

Custom string keys and associated string values may be used to convey app or
workflow-specific information, e.g. whether the color space should be left as is
or converted into a working space.

Getters and setters are using the rule position, they will throw if the position is not
valid. If the rule at the specified position does not implement the requested property
getter will return NULL and setter will throw.


**class FileRules**

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: FileRulesRcPtr createEditableCopy() const

         The method clones the content decoupling the two instances.

      .. cpp:function:: size_t getNumEntries() const noexcept

         Does include default rule. Result will be at least 1.

      .. cpp:function:: size_t getIndexForRule(const char *ruleName) const

         Get the index from the rule name.

      .. cpp:function:: const char *getName(size_t ruleIndex) const

         Get name of the rule.

      .. cpp:function:: const char *getPattern(size_t ruleIndex) const

         Setting pattern will erase regex.

      .. cpp:function:: void setPattern(size_t ruleIndex, const char *pattern)

      .. cpp:function:: const char *getExtension(size_t ruleIndex) const

         Setting extension will erase regex.

      .. cpp:function:: void setExtension(size_t ruleIndex, const char *extension)

      .. cpp:function:: const char *getRegex(size_t ruleIndex) const

         Setting a regex will erase pattern & extension.

      .. cpp:function:: void setRegex(size_t ruleIndex, const char *regex)

      .. cpp:function:: const char *getColorSpace(size_t ruleIndex) const

         Set the rule’s color space (may also be a role).

      .. cpp:function:: void setColorSpace(size_t ruleIndex, const char *colorSpace)

      .. cpp:function:: size_t getNumCustomKeys(size_t ruleIndex) const

         Get number of key/value pairs.

      .. cpp:function:: const char *getCustomKeyName(size_t ruleIndex, size_t key) const
      

         Get name of key.

      .. cpp:function:: const char *getCustomKeyValue(size_t ruleIndex, size_t key) const
      

         Get value for the key.

      .. cpp:function:: void setCustomKey(size_t ruleIndex, const char *key, const char *value)

         Adds a key/value or replace value if key exists. Setting a NULL
         or an empty value will erase the key.

      .. cpp:function:: void insertRule(size_t ruleIndex, const char *name, const char *colorSpace, const char *pattern, const char *extension)

         Insert a rule at a given ruleIndex.

         Rule currently at ruleIndex will be pushed to index: ruleIndex +
         1. Name must be unique.

         * ”Default” is a reserved name for the default rule. The
         default rule is automatically added and can’t be removed.
         (see `FileRules::setDefaultRuleColorSpace`_ ).

         * ”ColorSpaceNamePathSearch” is also a reserved name (see
         `FileRules::insertPathSearchRule`_ ).

         Will throw if ruleIndex is not less than
         `FileRules::getNumEntries`_ .

      .. cpp:function:: void insertRule(size_t ruleIndex, const char *name, const char *colorSpace, const char *regex)

      .. cpp:function:: void insertPathSearchRule(size_t ruleIndex)

         Helper function to insert a rule.

         Uses Config:parseColorSpaceFromString to search the path for any
         of the color spaces named in the config (as per OCIO v1).

      .. cpp:function:: void setDefaultRuleColorSpace(const char *colorSpace)

         Helper function to set the color space for the default rule.

      .. cpp:function:: void removeRule(size_t ruleIndex)

      .. cpp:function:: Note
         Default rule can’t be removed. Will throw if ruleIndex + 1 is
         not less than `FileRules::getNumEntries`_ .

      .. cpp:function:: void increaseRulePriority(size_t ruleIndex)

         Move a rule closer to the start of the list by one position.

      .. cpp:function:: void decreaseRulePriority(size_t ruleIndex)

         Move a rule closer to the end of the list by one position.

      .. cpp:function:: FileRules(const FileRules&) = delete

      .. cpp:function:: `FileRules`_ &operator=(const FileRules&) = delete

      .. cpp:function:: ~FileRules()

      -[ Public Static Functions ]-

      .. cpp:function:: FileRulesRcPtr Create()

         Creates FileRules for a Config. File rules will contain the
         default rule using the default role. The default rule cannot be
         removed.


   .. group-tab:: Python

      .. py:method:: decreaseRulePriority(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:method:: getColorSpace(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: getCustomKeyName(self: PyOpenColorIO.FileRules, ruleIndex: int, key: int) -> str

      .. py:method:: getCustomKeyValue(self: PyOpenColorIO.FileRules, ruleIndex: int, key: int) -> str

      .. py:method:: getExtension(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: getIndexForRule(self: PyOpenColorIO.FileRules, ruleName: str) -> int

      .. py:method:: getName(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: getNumCustomKeys(self: PyOpenColorIO.FileRules, ruleIndex: int) -> int

      .. py:method:: getNumEntries(self: PyOpenColorIO.FileRules) -> int

      .. py:method:: getPattern(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: getRegex(self: PyOpenColorIO.FileRules, ruleIndex: int) -> str

      .. py:method:: increaseRulePriority(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:method:: insertPathSearchRule(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:function:: insertRule(*args,**kwargs)

         Overloaded function.

         1. .. py:function:: insertRule(self: PyOpenColorIO.FileRules, ruleIndex: int, name: str, colorSpace: str, pattern: str, extension: str) -> None

         2. .. py:function:: insertRule(self: PyOpenColorIO.FileRules, ruleIndex: int, name: str, colorSpace: str, regex: str) -> None

      .. py:method:: removeRule(self: PyOpenColorIO.FileRules, ruleIndex: int) -> None

      .. py:method:: setColorSpace(self: PyOpenColorIO.FileRules, ruleIndex: int, colorSpace: str) -> None

      .. py:method:: setCustomKey(self: PyOpenColorIO.FileRules, ruleIndex: int, key: str, value: str) -> None

      .. py:method:: setDefaultRuleColorSpace(self: PyOpenColorIO.FileRules, colorSpace: str) -> None

      .. py:method:: setExtension(self: PyOpenColorIO.FileRules, ruleIndex: int, extension: str) -> None

      .. py:method:: setPattern(self: PyOpenColorIO.FileRules, ruleIndex: int, pattern: str) -> None

      .. py:method:: setRegex(self: PyOpenColorIO.FileRules, ruleIndex: int, regex: str) -> None
