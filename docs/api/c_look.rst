
Look
****

.. class:: Look


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: LookRcPtr OpenColorIO::`Look`_::Create()

      .. cpp:function:: LookRcPtr OpenColorIO::`Look`_::createEditableCopy() const

   .. group-tab:: Python

      .. py:method:: Look.__init__(*args, **kwargs)

         Overloaded function.

         1. __init__(self: PyOpenColorIO.Look) -> None

         2. __init__(self: PyOpenColorIO.Look, name: str = ‘’, processSpace: str = ‘’, transform: PyOpenColorIO.Transform = None, inverseTransform: PyOpenColorIO.Transform = None, description: str = ‘’) -> None


Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const char *OpenColorIO::`Look`_::getName() const

      .. cpp:function:: void OpenColorIO::`Look`_::setName(const char *name)

      .. cpp:function:: const char *OpenColorIO::`Look`_::getProcessSpace() const

      .. cpp:function:: void OpenColorIO::`Look`_::setProcessSpace(const char *processSpace)

      .. cpp:function:: ConstTransformRcPtr OpenColorIO::`Look`_::getTransform() const

      .. cpp:function:: void OpenColorIO::`Look`_::setTransform(const ConstTransformRcPtr &transform)

         Setting a transform to a non-null call makes it allowed. 

      .. cpp:function:: ConstTransformRcPtr OpenColorIO::`Look`_::getInverseTransform() const

      .. cpp:function:: void OpenColorIO::`Look`_::setInverseTransform(const ConstTransformRcPtr &transform)

         Setting a transform to a non-null call makes it allowed. 

      .. cpp:function:: const char *OpenColorIO::`Look`_::getDescription() const

      .. cpp:function:: void OpenColorIO::`Look`_::setDescription(const char *description)

   .. group-tab:: Python

      .. py:method:: Look.getName(self: PyOpenColorIO.Look) -> str

      .. py:method:: Look.setName(self: PyOpenColorIO.Look, name: str) -> None

      .. py:method:: Look.getProcessSpace(self: PyOpenColorIO.Look) -> str

      .. py:method:: Look.setProcessSpace(self: PyOpenColorIO.Look, processSpace: str) -> None

      .. py:method:: Look.getTransform(self: PyOpenColorIO.Look) -> `PyOpenColorIO.Transform`_

      .. py:method:: Look.setTransform(self: PyOpenColorIO.Look, transform: PyOpenColorIO.Transform) -> None

      .. py:method:: Look.getInverseTransform(self: PyOpenColorIO.Look) -> `PyOpenColorIO.Transform`_

      .. py:method:: Look.setInverseTransform(self: PyOpenColorIO.Look, transform: PyOpenColorIO.Transform) -> None

      .. py:method:: Look.getDescription(self: PyOpenColorIO.Look) -> str

      .. py:method:: Look.setDescription(self: PyOpenColorIO.Look, description: str) -> None
