..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ColorSpaceSet
*************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class ColorSpaceSet**

The *ColorSpaceSet* is a set of color spaces (i.e. no color space
duplication) which could be the result of
:cpp:func:```Config::getColorSpaces`_`` or built from scratch.

**Note**
The color spaces are decoupled from the config ones, i.e., any
changes to the set itself or to its color spaces do not affect
the original color spaces from the configuration. If needed, use
:cpp:func:```Config::addColorSpace`_`` to update the
configuration.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ColorSpaceSetRcPtr createEditableCopy() const

         Create a set containing a copy of all the color spaces.

      .. cpp:function:: bool operator==(const ColorSpaceSet &css) const

         Return true if the two sets are equal.

         **Note**
         The comparison is done on the color space names (not a deep comparison).

      .. cpp:function:: bool operator!=(const ColorSpaceSet &css) const

         Return true if the two sets are different.

      .. cpp:function:: int getNumColorSpaces() const

         Return the number of color spaces.

      .. cpp:function:: const char *getColorSpaceNameByIndex(int index) const

         Return the color space name using its index. This will be null
         if an invalid index is specified.

      .. cpp:function:: ConstColorSpaceRcPtr getColorSpaceByIndex(int index) const

         Return the color space using its index. This will be empty if an
         invalid index is specified.

      .. cpp:function:: ConstColorSpaceRcPtr getColorSpace(const char *name) const

         Will return null if the name is not found.

         **Note**
         Only accepts color space names (i.e. no role name).

      .. cpp:function:: int getColorSpaceIndex(const char *name) const

         Will return -1 if the name is not found.

         **Note**
         Only accepts color space names (i.e. no role name).

      .. cpp:function:: bool hasColorSpace(const char *name) const

         **Note**
         Only accepts color space names (i.e. no role name)

         **Return**
         true

         **Return**
         false

         **Parameters**
         * ``name``:

      .. cpp:function:: void addColorSpace(const ConstColorSpaceRcPtr &cs)

         Add color space(s).

         **Note**
         If another color space is already registered with the same
         name, this will overwrite it. This stores a copy of the
         specified color space(s).

      .. cpp:function:: void addColorSpaces(const ConstColorSpaceSetRcPtr &cs)

      .. cpp:function:: void removeColorSpace(const char *name)

         Remove color space(s) using color space names (i.e. no role
         name).

         **Note**
         The removal of a missing color space does nothing.

      .. cpp:function:: void removeColorSpaces(const ConstColorSpaceSetRcPtr &cs)

      .. cpp:function:: void clearColorSpaces()

         Clear all color spaces.

      .. cpp:function:: ~ColorSpaceSet()

      -[ Public Static Functions ]-

      .. cpp:function:: ColorSpaceSetRcPtr Create()

         Create an empty set of color spaces.

   .. group-tab:: Python

      .. py:class:: PyOpenColorIO.ColorSpaceSet

      .. py:class:: ColorSpaceIterator

      .. py:class:: ColorSpaceNameIterator

      .. py:method:: addColorSpace(self: PyOpenColorIO.ColorSpaceSet, colorSpace: PyOpenColorIO.ColorSpace) -> None

      .. py:method:: addColorSpaces(self: PyOpenColorIO.ColorSpaceSet, colorSpace: PyOpenColorIO.ColorSpaceSet) -> None

      .. py:method:: clearColorSpaces(self: PyOpenColorIO.ColorSpaceSet) -> None

      .. py:method:: getColorSpace(self: PyOpenColorIO.ColorSpaceSet, name: str) -> PyOpenColorIO.ColorSpace

      .. py:method:: getColorSpaceNames(self: PyOpenColorIO.ColorSpaceSet) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ColorSpaceSet>, 0>

      .. py:method:: getColorSpaces(self: PyOpenColorIO.ColorSpaceSet) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ColorSpaceSet>, 1>

      .. py:method:: removeColorSpace(self: PyOpenColorIO.ColorSpaceSet, colorSpace: str) -> None

      .. py:method:: removeColorSpaces(self: PyOpenColorIO.ColorSpaceSet, colorSpace: PyOpenColorIO.ColorSpaceSet) -> None
