..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

DynamicProperty
***************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class DynamicProperty**

Allows transform parameter values to be set on-the-fly (after
finalization). For example, to modify the exposure in a viewport.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: DynamicPropertyType getType() const = 0

      .. cpp:function:: DynamicPropertyValueType getValueType() const = 0

      .. cpp:function:: double getDoubleValue() const = 0

      .. cpp:function:: void setValue(double value) = 0

      .. cpp:function:: bool isDynamic() const = 0

      .. cpp:function:: DynamicProperty &operator=(const DynamicProperty&) = delete

      .. cpp:function:: ~DynamicProperty()

   .. group-tab:: Python

      .. py:class:: PyOpenColorIO.DynamicProperty

      .. py:method:: getDoubleValue(self: PyOpenColorIO.DynamicProperty) -> float

      .. py:method:: getType(self: PyOpenColorIO.DynamicProperty) -> PyOpenColorIO.DynamicPropertyType

      .. py:method:: getValueType(self: PyOpenColorIO.DynamicProperty) -> PyOpenColorIO.DynamicPropertyValueType

      .. py:method:: isDynamic(self: PyOpenColorIO.DynamicProperty) -> bool

      .. py:method:: setValue(self: PyOpenColorIO.DynamicProperty, value: float) -> None
