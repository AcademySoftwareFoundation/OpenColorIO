..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

FormatMetadata
**************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class FormatMetadata**

The FormatMetadata class is intended to be a generic container to
hold metadata from various file formats.

This class provides a hierarchical metadata container. A metadata
object is similar to an element in XML. It contains:

* A name string (e.g. “Description”).

* A value string (e.g. “updated viewing LUT”).

* A list of attributes (name, value) string pairs (e.g. “version”,
“1.5”).

* And a list of child sub-elements, which are also objects
implementing FormatMetadata.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const char *getName() const = 0

      .. cpp:function:: void setName(const char*) = 0

      .. cpp:function:: const char *getValue() const = 0

      .. cpp:function:: void setValue(const char*) = 0

      .. cpp:function:: int getNumAttributes() const = 0

      .. cpp:function:: const char *getAttributeName(int i) const = 0

      .. cpp:function:: const char *getAttributeValue(int i) const = 0

      .. cpp:function:: void addAttribute(const char *name, const char *value) = 0

         Add an attribute with a given name and value. If an attribute
         with the same name already exists, the value is replaced.

      .. cpp:function:: int getNumChildrenElements() const = 0

      .. cpp:function:: const `FormatMetadata`_ &getChildElement(int i) const = 0

      .. cpp:function:: `FormatMetadata`_ &getChildElement(int i) = 0

      .. cpp:function:: `FormatMetadata`_ &addChildElement(const char *name, const char *value) = 0

         Add a child element with a given name and value. Name has to be
         non-empty. Value may be empty, particularly if this element will
         have children. Return a reference to the added element.

      .. cpp:function:: void clear() = 0

      .. cpp:function:: `FormatMetadata`_ &operator=(const FormatMetadata &rhs) = 0

      .. cpp:function:: FormatMetadata(const FormatMetadata &rhs) = delete

      .. cpp:function:: ~FormatMetadata()

   .. group-tab:: Python


      .. py:class:: PyOpenColorIO.FormatMetadata

      .. py:class:: AttributeIterator

      .. py:class:: AttributeNameIterator

      .. py:class:: ChildElementIterator

      .. py:class:: ConstChildElementIterator

      .. py:method:: addChildElement(self: PyOpenColorIO.FormatMetadata, name: str, value: str) -> `PyOpenColorIO.FormatMetadata`_

      .. py:method:: clear(self: PyOpenColorIO.FormatMetadata) -> None

      .. py:method:: getAttributes(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata const&, 1>

      .. py:function:: getChildElements(*args,**kwargs)

         Overloaded function.

         1. .. py:function:: getChildElements(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata const&, 2>

         2. .. py:function:: getChildElements(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata&, 3>

      .. py:method:: getName(self: PyOpenColorIO.FormatMetadata) -> str

      .. py:method:: getValue(self: PyOpenColorIO.FormatMetadata) -> str

      .. py:method:: setName(self: PyOpenColorIO.FormatMetadata, name: str) -> None

      .. py:method:: setValue(self: PyOpenColorIO.FormatMetadata, value: str) -> None
