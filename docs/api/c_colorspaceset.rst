
ColorSpaceSet
*************

.. class:: ColorSpaceSet

   The *ColorSpaceSet* is a set of color spaces (i.e. no color space duplication) which could be the result of :cpp:func:`Config::getColorSpaces` or built from scratch.

   **Note**
      The color spaces are decoupled from the config ones, i.e., any changes to the set itself or to its color spaces do not affect the original color spaces from the configuration. If needed, use :cpp:func:`Config::addColorSpace` to update the configuration. 


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ColorSpaceSetRcPtr OpenColorIO::ColorSpaceSet::Create()

         Create an empty set of color spaces. 

      .. cpp:function:: ColorSpaceSetRcPtr OpenColorIO::ColorSpaceSet::createEditableCopy() const

         Create a set containing a copy of all the color spaces. 

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

      Warning: doxygenfunction: Cannot find function “OpenColorIO::ColorSpaceSet::getColorSpaceNames” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::ColorSpaceSet::getColorSpaces” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: ConstColorSpaceRcPtr OpenColorIO::ColorSpaceSet::getColorSpace(const char *name) const

         Will return null if the name is not found. 

         **Note**
            Only accepts color space names (i.e. no role name).

      .. cpp:function:: void OpenColorIO::ColorSpaceSet::addColorSpace(const ConstColorSpaceRcPtr &cs)

         Add color space(s). 

         **Note**
            If another color space is already registered with the same name, this will overwrite it. This stores a copy of the specified color space(s). 

      .. cpp:function:: void OpenColorIO::ColorSpaceSet::removeColorSpace(const char *name)

         Remove color space(s) using color space names (i.e. no role name). 

         **Note**
            The removal of a missing color space does nothing. 

      .. cpp:function:: void OpenColorIO::ColorSpaceSet::removeColorSpaces(const ConstColorSpaceSetRcPtr &cs)

      .. cpp:function:: void OpenColorIO::ColorSpaceSet::clearColorSpaces()

         Clear all color spaces. 


