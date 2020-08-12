..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


ViewingRules
************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class ViewingRules**


.. tabs::

    .. group-tab:: C++   

        .. cpp:function:: ViewingRulesRcPtr createEditableCopy() const 

            The method clones the content decoupling the two instances.

        .. cpp:function:: size_t getNumEntries() const noexcept 

        .. cpp:function:: size_t getIndexForRule(const char *ruleName) const 

            Get the index from the rule name. Will throw if there is no rule
            named ruleName.

        .. cpp:function:: const char *getName(size_t ruleIndex) const 

            Get name of the rule. Will throw if ruleIndex is invalid.

        .. cpp:function:: size_t getNumColorSpaces(size_t ruleIndex) const 

            Get number of colorspaces. Will throw if ruleIndex is invalid.

        .. cpp:function:: const char *getColorSpace(size_t ruleIndex, size_t colorSpaceIndex) const 

            Get colorspace name. Will throw if ruleIndex or colorSpaceIndex
            is invalid.

        .. cpp:function:: void addColorSpace(size_t ruleIndex, const char *colorSpace) 

            Add colorspace name. Will throw if:

            * RuleIndex is invalid.

            * :cpp:func:```ViewingRules::getNumEncodings`_`` is not 0.

        .. cpp:function:: void removeColorSpace(size_t ruleIndex, size_t colorSpaceIndex) 

            Remove colorspace. Will throw if ruleIndex or colorSpaceIndex is
            invalid.

        .. cpp:function:: size_t getNumEncodings(size_t ruleIndex) const 

            Get number of encodings. Will throw if ruleIndex is invalid.

        .. cpp:function:: const char *getEncoding(size_t ruleIndex, size_t encodingIndex) const 

            Get encoding name. Will throw if ruleIndex or encodingIndex is
            invalid.

        .. cpp:function:: void addEncoding(size_t ruleIndex, const char *encoding) 

            Add encoding name. Will throw if:

            * RuleIndex is invalid.

            * :cpp:func:```ViewingRules::getNumColorSpaces`_`` is not 0.

        .. cpp:function:: void removeEncoding(size_t ruleIndex, size_t encodingIndex) 

            Remove encoding. Will throw if ruleIndex or encodingIndex is
            invalid.

        .. cpp:function:: size_t getNumCustomKeys(size_t ruleIndex) const 

            Get number of key/value pairs. Will throw if ruleIndex is
            invalid.

        .. cpp:function:: const char *getCustomKeyName(size_t ruleIndex, size_t keyIndex) const 

            Get name of key. Will throw if ruleIndex or keyIndex is invalid.

        .. cpp:function:: const char *getCustomKeyValue(size_t ruleIndex, size_t keyIndex) const 

            Get value for the key. Will throw if ruleIndex or keyIndex is
            invalid.

        .. cpp:function:: void setCustomKey(size_t ruleIndex, const char *key, const char *value) 

            Adds a key/value or replace value if key exists. Setting a NULL
            or an empty value will erase the key. Will throw if ruleIndex is
            invalid.

        .. cpp:function:: void insertRule(size_t ruleIndex, const char *ruleName) 

            Insert a rule at a given ruleIndex.

            Rule currently at ruleIndex will be pushed to index: ruleIndex +
            1. If ruleIndex is :cpp:func:``ViewingRules::getNumEntries`` new
            rule will be added at the end. Will throw if:

            * RuleIndex is invalid (must be less than or equal to
                cpp:func:`ViewingRules::getNumEntries`).

            * RuleName already exists.

        .. cpp:function:: void removeRule(size_t ruleIndex) 

            Remove a rule. Throws if ruleIndex is not valid.

        .. cpp:function:: ViewingRules(const ViewingRules&) = delete 

        .. cpp:function:: `ViewingRules`_ &operator=(const ViewingRules&) = delete 

        .. cpp:function:: ~ViewingRules() 

        -[ Public Static Functions ]-

        .. cpp:function:: ViewingRulesRcPtr Create() 

            Creates ViewingRules for a Config.

    .. group-tab:: Python

        .. py:class:: ViewingRuleColorSpaceIterator

        .. py:class:: ViewingRuleEncodingIterator

        .. py:method:: addColorSpace(self: PyOpenColorIO.ViewingRules, ruleIndex: int, colorSpaceName: str) -> None

        .. py:method:: addEncoding(self: PyOpenColorIO.ViewingRules, ruleIndex: int, encodingName: str) -> None

        .. py:method:: getColorSpaces(self: PyOpenColorIO.ViewingRules, ruleIndex: int) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ViewingRules>, 0, unsigned long>

        .. py:method:: getCustomKeyName(self: PyOpenColorIO.ViewingRules, ruleIndex: int, key: int) -> str

        .. py:method:: getCustomKeyValue(self: PyOpenColorIO.ViewingRules, ruleIndex: int, key: int) -> str

        .. py:method:: getEncodings(self: PyOpenColorIO.ViewingRules, ruleIndex: int) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ViewingRules>, 1, unsigned long>

        .. py:method:: getIndexForRule(self: PyOpenColorIO.ViewingRules, ruleName: str) -> int

        .. py:method:: getName(self: PyOpenColorIO.ViewingRules, ruleIndex: int) -> str

        .. py:method:: getNumCustomKeys(self: PyOpenColorIO.ViewingRules, ruleIndex: int) -> int

        .. py:method:: getNumEntries(self: PyOpenColorIO.ViewingRules) -> int

        .. py:method:: insertRule(self: PyOpenColorIO.ViewingRules, ruleIndex: int, name: str) -> None

        .. py:method:: removeColorSpace(self: PyOpenColorIO.ViewingRules, ruleIndex: int, colorSpaceIndex: int) -> None

        .. py:method:: removeEncoding(self: PyOpenColorIO.ViewingRules, ruleIndex: int, encodingIndex: int) -> None

        .. py:method:: removeRule(self: PyOpenColorIO.ViewingRules, ruleIndex: int) -> None

        .. py:method:: setCustomKey(self: PyOpenColorIO.ViewingRules, ruleIndex: int, key: str, value: str) -> None
