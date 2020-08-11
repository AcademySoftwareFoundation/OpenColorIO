
DynamicProperty
***************

.. class:: DynamicProperty

   Allows transform parameter values to be set on-the-fly (after finalization). For example, to modify the exposure in a viewport. 


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: DynamicPropertyType OpenColorIO::DynamicProperty::getType() const = 0

      .. cpp:function:: DynamicPropertyValueType OpenColorIO::DynamicProperty::getValueType() const = 0

      .. cpp:function:: double OpenColorIO::DynamicProperty::getDoubleValue() const = 0

      .. cpp:function:: void OpenColorIO::DynamicProperty::setValue(double value) = 0

      .. cpp:function:: bool OpenColorIO::DynamicProperty::isDynamic() const = 0

   .. group-tab:: Python

      .. py:method:: DynamicProperty.getType(self: PyOpenColorIO.DynamicProperty) -> PyOpenColorIO.DynamicPropertyType

      .. py:method:: DynamicProperty.getValueType(self: PyOpenColorIO.DynamicProperty) -> PyOpenColorIO.DynamicPropertyValueType

      .. py:method:: DynamicProperty.getDoubleValue(self: PyOpenColorIO.DynamicProperty) -> float

      .. py:method:: DynamicProperty.setValue(self: PyOpenColorIO.DynamicProperty, value: float) -> None

      .. py:method:: DynamicProperty.isDynamic(self: PyOpenColorIO.DynamicProperty) -> bool
