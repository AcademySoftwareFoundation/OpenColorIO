..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

ColorSpace
**********

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

The *ColorSpace* is the state of an image with respect to colorimetry
and color encoding. Transforming images between different
*ColorSpaces* is the primary motivation for this library.

While a complete discussion of color spaces is beyond the scope of
header documentation, traditional uses would be to have *ColorSpaces*
corresponding to: physical capture devices (known cameras, scanners),
and internal 'convenience' spaces (such as scene linear, logarithmic).

*ColorSpaces* are specific to a particular image precision (float32,
uint8, etc.), and the set of ColorSpaces that provide equivalent mappings
(at different precisions) are referred to as a 'family'.

**class ColorSpace**

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ColorSpaceRcPtr createEditableCopy() const

      .. cpp:function:: const char *getName() const noexcept

      .. cpp:function:: void setName(const char *name)

      .. cpp:function:: const char *getFamily() const noexcept

         Get the family, for use in user interfaces (optional) The family
         string could use a ‘/’ separator to indicate levels to be used
         by hierarchical menus.

      .. cpp:function:: void setFamily(const char *family)

         Set the family, for use in user interfaces (optional)

      .. cpp:function:: const char *getEqualityGroup() const noexcept

         Get the ColorSpace group name (used for equality comparisons)
         This allows no-op transforms between different colorspaces. If
         an equalityGroup is not defined (an empty string), it will be
         considered unique (i.e., it will not compare as equal to other
         ColorSpaces with an empty equality group). This is often, though
         not always, set to the same value as ‘family’.

      .. cpp:function:: void setEqualityGroup(const char *equalityGroup)

      .. cpp:function:: const char *getDescription() const noexcept

      .. cpp:function:: void setDescription(const char *description)

      .. cpp:function:: BitDepth getBitDepth() const noexcept

      .. cpp:function:: void setBitDepth(BitDepth bitDepth)

      .. cpp:function:: ReferenceSpaceType getReferenceSpaceType() const noexcept

         A display color space will use the display-referred reference
         space.

      .. cpp:function:: bool hasCategory(const char *category) const

         Return true if the category is present.

      .. cpp:function:: void addCategory(const char *category)

         Add a single category.

         **Note**
         Will do nothing if the category already exists.

      .. cpp:function:: void removeCategory(const char *category)

         Remove a category.

         **Note**
         Will do nothing if the category is missing.

      .. cpp:function:: int getNumCategories() const

         Get the number of categories.

      .. cpp:function:: const char *getCategory(int index) const

         Return the category name using its index.

         **Note**
         Will be null if the index is invalid.

      .. cpp:function:: void clearCategories()

         Clear all the categories.

      .. cpp:function:: const char *getEncoding() const noexcept

      .. cpp:function:: void setEncoding(const char *encoding)

      .. cpp:function:: bool isData() const noexcept

      .. cpp:function:: void setIsData(bool isData) noexcept

      .. cpp:function:: Allocation getAllocation() const noexcept

      .. cpp:function:: void setAllocation(Allocation allocation) noexcept

      .. cpp:function:: int getAllocationNumVars() const

      .. cpp:function:: void getAllocationVars(float *vars) const

      .. cpp:function:: void setAllocationVars(int numvars, const float *vars)

      .. cpp:function:: ConstTransformRcPtr getTransform(ColorSpaceDirection dir) const

         If a transform in the specified direction has been specified,
         return it. Otherwise return a null ConstTransformRcPtr

      .. cpp:function:: void setTransform(const ConstTransformRcPtr &transform, ColorSpaceDirection dir)

         Specify the transform for the appropriate direction. Setting the
         transform to null will clear it.

      .. cpp:function:: ColorSpace(const ColorSpace&) = delete

      .. cpp:function:: ColorSpace &operator=(const ColorSpace&) = delete

      .. cpp:function:: ~ColorSpace()

      -[ Public Static Functions ]-

      .. cpp:function:: ColorSpaceRcPtr Create()

      .. cpp:function:: ColorSpaceRcPtr Create(ReferenceSpaceType referenceSpace)

   .. group-tab:: Python

      .. py:class:: PyOpenColorIO.ColorSpace

      .. py:class:: ColorSpaceCategoryIterator

      .. py:method:: addCategory(self: PyOpenColorIO.ColorSpace, category: str) -> None

      .. py:method:: clearCategories(self: PyOpenColorIO.ColorSpace) -> None

      .. py:method:: getAllocation(self: PyOpenColorIO.ColorSpace) -> PyOpenColorIO.Allocation

      .. py:method:: getAllocationVars(self: PyOpenColorIO.ColorSpace) -> List[float]

      .. py:method:: getBitDepth(self: PyOpenColorIO.ColorSpace) -> PyOpenColorIO.BitDepth

      .. py:method:: getCategories(self: PyOpenColorIO.ColorSpace) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ColorSpace>, 0>

      .. py:method:: getDescription(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: getEncoding(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: getEqualityGroup(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: getFamily(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: getName(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: getReferenceSpaceType(self: PyOpenColorIO.ColorSpace) -> PyOpenColorIO.ReferenceSpaceType

      .. py:method:: getTransform(self: PyOpenColorIO.ColorSpace, direction: PyOpenColorIO.ColorSpaceDirection) -> PyOpenColorIO.Transform

      .. py:method:: hasCategory(self: PyOpenColorIO.ColorSpace, category: str) -> bool

      .. py:method:: isData(self: PyOpenColorIO.ColorSpace) -> bool

      .. py:method:: removeCategory(self: PyOpenColorIO.ColorSpace, category: str) -> None

      .. py:method:: setAllocation(self: PyOpenColorIO.ColorSpace, allocation: PyOpenColorIO.Allocation) -> None

      .. py:method:: setAllocationVars(self: PyOpenColorIO.ColorSpace, vars: List[float]) -> None

      .. py:method:: setBitDepth(self: PyOpenColorIO.ColorSpace, bitDepth: PyOpenColorIO.BitDepth) -> None

      .. py:method:: setDescription(self: PyOpenColorIO.ColorSpace, description: str) -> None

      .. py:method:: setEncoding(self: PyOpenColorIO.ColorSpace, encoding: str) -> None

      .. py:method:: setEqualityGroup(self: PyOpenColorIO.ColorSpace, equalityGroup: str) -> None

      .. py:method:: setFamily(self: PyOpenColorIO.ColorSpace, family: str) -> None

      .. py:method:: setIsData(self: PyOpenColorIO.ColorSpace, isData: bool) -> None

      .. py:method:: setName(self: PyOpenColorIO.ColorSpace, name: str) -> None

      .. py:method:: setTransform(self: PyOpenColorIO.ColorSpace, transform: PyOpenColorIO.Transform, direction: PyOpenColorIO.ColorSpaceDirection) -> None
