
FormatMetadata
**************

.. class:: FormatMetadata

   The FormatMetadata class is intended to be a generic container to hold metadata from various file formats.

   This class provides a hierarchical metadata container. A metadata object is similar to an element in XML. It contains:

   * A name string (e.g. “Description”).

   * A value string (e.g. “updated viewing LUT”).

   * A list of attributes (name, value) string pairs (e.g. “version”, “1.5”).

   * And a list of child sub-elements, which are also objects implementing FormatMetadata. 


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const char *OpenColorIO::FormatMetadata::getName() const = 0

      .. cpp:function:: void OpenColorIO::FormatMetadata::setName(const char*) = 0

      .. cpp:function:: const char *OpenColorIO::FormatMetadata::getValue() const = 0

      .. cpp:function:: void OpenColorIO::FormatMetadata::setValue(const char*) = 0

      Warning: doxygenfunction: Cannot find function “OpenColorIO::FormatMetadata::getAttributes” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::FormatMetadata::getChildElements” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: FormatMetadata &OpenColorIO::FormatMetadata::addChildElement(const char *name, const char *value) = 0

         Add a child element with a given name and value. Name has to be non-empty. Value may be empty, particularly if this element will have children. Return a reference to the added element. 

   .. group-tab:: Python

      .. py:method:: FormatMetadata.getName(self: PyOpenColorIO.FormatMetadata) -> str

      .. py:method:: FormatMetadata.setName(self: PyOpenColorIO.FormatMetadata, name: str) -> None

      .. py:method:: FormatMetadata.getValue(self: PyOpenColorIO.FormatMetadata) -> str

      .. py:method:: FormatMetadata.setValue(self: PyOpenColorIO.FormatMetadata, value: str) -> None

      .. py:method:: FormatMetadata.getAttributes(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata const&, 1>

      .. py:method:: FormatMetadata.getChildElements(*args, **kwargs)**

         Overloaded function.

         1. getChildElements(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata const&, 2>

         2. getChildElements(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata&, 3>

      .. py:method:: FormatMetadata.addChildElement(self: PyOpenColorIO.FormatMetadata, name: str, value: str) -> PyOpenColorIO.FormatMetadata
