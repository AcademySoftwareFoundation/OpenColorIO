
ColorSpace
**********

.. class:: ColorSpace


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ColorSpaceRcPtr OpenColorIO::ColorSpace::Create()


      .. cpp:function:: ColorSpaceRcPtr OpenColorIO::ColorSpace::createEditableCopy() const

   .. group-tab:: Python

      .. py:method:: ColorSpace.__init__(*args, **kwargs)

         Overloaded function.

         1. __init__(self: PyOpenColorIO.ColorSpace) -> None

         2. __init__(self: PyOpenColorIO.ColorSpace, referenceSpace: PyOpenColorIO.ReferenceSpaceType) -> None

         3. __init__(self: PyOpenColorIO.ColorSpace, referenceSpace: PyOpenColorIO.ReferenceSpaceType = ReferenceSpaceType.REFERENCE_SPACE_SCENE, name: str = ‘’, family: str = ‘’, encoding: str = ‘’, equalityGroup: str = ‘’, description: str = ‘’, bitDepth: PyOpenColorIO.BitDepth = BitDepth.BIT_DEPTH_UNKNOWN, isData: bool = False, allocation: PyOpenColorIO.Allocation = Allocation.ALLOCATION_UNIFORM, allocationVars: List[float] = [], toReference: PyOpenColorIO.Transform = None, fromReference: PyOpenColorIO.Transform = None, categories: List[str] = []) -> None


Methods
=======

.. tabs::

   .. group-tab:: C++

      **Name**

      .. cpp:function:: const char *OpenColorIO::ColorSpace::getName() const noexcept


      .. cpp:function:: void OpenColorIO::ColorSpace::setName(const char *name)


      **Family**

      .. cpp:function:: const char *OpenColorIO::ColorSpace::getFamily() const noexcept

         Get the family, for use in user interfaces (optional) The family string could use a ‘/’ separator to indicate levels to be used by hierarchical menus. 

      .. cpp:function:: void OpenColorIO::ColorSpace::setFamily(const char *family)

         Set the family, for use in user interfaces (optional) 

      **Equality Group**

      .. cpp:function:: const char *OpenColorIO::ColorSpace::getEqualityGroup() const noexcept

         Get the ColorSpace group name (used for equality comparisons) This allows no-op transforms between different colorspaces. If an equalityGroup is not defined (an empty string), it will be considered unique (i.e., it will not compare as equal to other ColorSpaces with an empty equality group). This is often, though not always, set to the same value as ‘family’. 

      .. cpp:function:: void OpenColorIO::ColorSpace::setEqualityGroup(const char *equalityGroup)


      **Description**

      .. cpp:function:: const char *OpenColorIO::ColorSpace::getDescription() const noexcept


      .. cpp:function:: void OpenColorIO::ColorSpace::setDescription(const char *description)


      **Bit Depth**

      .. cpp:function:: BitDepth OpenColorIO::ColorSpace::getBitDepth() const noexcept


      .. cpp:function:: void OpenColorIO::ColorSpace::setBitDepth(BitDepth bitDepth)


      **Categories**

      .. cpp:function:: bool OpenColorIO::ColorSpace::hasCategory(const char *category) const


         Return true if the category is present. 

      .. cpp:function:: void OpenColorIO::ColorSpace::addCategory(const char *category)


         Add a single category. 

         **Note**
            Will do nothing if the category already exists. 

      .. cpp:function:: void OpenColorIO::ColorSpace::removeCategory(const char *category)

         Remove a category. 

         **Note**
            Will do nothing if the category is missing. 

      .. cpp:function:: void OpenColorIO::ColorSpace::clearCategories()


         Clear all the categories. 

      **Data**

      .. cpp:function:: bool OpenColorIO::ColorSpace::isData() const noexcept


      .. cpp:function:: void OpenColorIO::ColorSpace::setIsData(bool isData) noexcept


      **Reference Space**

      .. cpp:function:: ReferenceSpaceType OpenColorIO::ColorSpace::getReferenceSpaceType() const noexcept

         A display color space will use the display-referred reference space. 

      **Allocation**

      .. cpp:function:: Allocation OpenColorIO::ColorSpace::getAllocation() const noexcept


      .. cpp:function:: void OpenColorIO::ColorSpace::setAllocation(Allocation allocation) noexcept


      .. cpp:function:: void OpenColorIO::ColorSpace::getAllocationVars(float *vars) const


      .. cpp:function:: void OpenColorIO::ColorSpace::setAllocationVars(int numvars, const float *vars)


      **Transform**

      .. cpp:function:: ConstTransformRcPtr OpenColorIO::ColorSpace::getTransform(ColorSpaceDirection dir) const

         If a transform in the specified direction has been specified, return it. Otherwise return a null ConstTransformRcPtr 

      .. cpp:function:: void OpenColorIO::ColorSpace::setTransform(const ConstTransformRcPtr &transform, ColorSpaceDirection dir)

         Specify the transform for the appropriate direction. Setting the transform to null will clear it.

   .. group-tab:: Python

      **Name**

      .. py:method:: ColorSpace.getName(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: ColorSpace.setName(self: PyOpenColorIO.ColorSpace, name: str) -> None

      **Family**

      .. py:method:: ColorSpace.getFamily(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: ColorSpace.setFamily(self: PyOpenColorIO.ColorSpace, family: str) -> None

      **Equality Group**

      .. py:method:: ColorSpace.getEqualityGroup(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: ColorSpace.setEqualityGroup(self: PyOpenColorIO.ColorSpace, equalityGroup: str) -> None

      **Description**

      .. py:method:: ColorSpace.getDescription(self: PyOpenColorIO.ColorSpace) -> str

      .. py:method:: ColorSpace.setDescription(self: PyOpenColorIO.ColorSpace, description: str) -> None

      **Bit Depth**

      .. py:method:: ColorSpace.getBitDepth(self: PyOpenColorIO.ColorSpace) -> PyOpenColorIO.BitDepth

      .. py:method:: ColorSpace.setBitDepth(self: PyOpenColorIO.ColorSpace, bitDepth: PyOpenColorIO.BitDepth) -> None

      **Categories**

      .. py:method:: ColorSpace.hasCategory(self: PyOpenColorIO.ColorSpace, category: str) -> bool

      .. py:method:: ColorSpace.addCategory(self: PyOpenColorIO.ColorSpace, category: str) -> None

      .. py:method:: ColorSpace.removeCategory(self: PyOpenColorIO.ColorSpace, category: str) -> None

      .. py:method:: ColorSpace.getCategories(self: PyOpenColorIO.ColorSpace) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::ColorSpace>, 0>

      .. py:method:: ColorSpace.clearCategories(self: PyOpenColorIO.ColorSpace) -> None

      **Data**

      .. py:method:: ColorSpace.isData(self: PyOpenColorIO.ColorSpace) -> bool

      .. py:method:: ColorSpace.setIsData(self: PyOpenColorIO.ColorSpace, isData: bool) -> None

      **Reference Space**

      .. py:method:: ColorSpace.getReferenceSpaceType(self: PyOpenColorIO.ColorSpace) -> PyOpenColorIO.ReferenceSpaceType

      **Allocation**

      .. py:method:: ColorSpace.getAllocation(self: PyOpenColorIO.ColorSpace) -> PyOpenColorIO.Allocation

      .. py:method:: ColorSpace.setAllocation(self: PyOpenColorIO.ColorSpace, allocation: PyOpenColorIO.Allocation) -> None

      .. py:method:: ColorSpace.getAllocationVars(self: PyOpenColorIO.ColorSpace) -> List[float]

      .. py:method:: ColorSpace.setAllocationVars(self: PyOpenColorIO.ColorSpace, vars: List[float]) -> None

      **Transform**

      .. py:method:: ColorSpace.getTransform(self: PyOpenColorIO.ColorSpace, direction: PyOpenColorIO.ColorSpaceDirection) -> PyOpenColorIO.Transform

      .. py:method:: ColorSpace.setTransform(self: PyOpenColorIO.ColorSpace, transform: PyOpenColorIO.Transform, direction: PyOpenColorIO.ColorSpaceDirection) -> None
