..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Look
****

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

The *Look* is an 'artistic' image modification, in a specified image state.

The processSpace defines the ColorSpace the image is required to be in, for the math to apply correctly.

**class Look**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: LookRcPtr createEditableCopy() const

      .. cpp:function:: const char *getName() const

      .. cpp:function:: void setName(const char *name)

      .. cpp:function:: const char *getProcessSpace() const

      .. cpp:function:: void setProcessSpace(const char *processSpace)

      .. cpp:function:: ConstTransformRcPtr getTransform() const

      .. cpp:function:: void setTransform(const ConstTransformRcPtr &transform)

         Setting a transform to a non-null call makes it allowed.

      .. cpp:function:: ConstTransformRcPtr getInverseTransform() const

      .. cpp:function:: void setInverseTransform(const ConstTransformRcPtr &transform)

         Setting a transform to a non-null call makes it allowed.

      .. cpp:function:: const char *getDescription() const

      .. cpp:function:: void setDescription(const char *description)

      .. cpp:function:: ~Look()

      .. cpp:function:: LookRcPtr Create()


   .. group-tab:: Python

      .. py:method:: getDescription(self: PyOpenColorIO.Look) -> str

      .. py:method:: getInverseTransform(self: PyOpenColorIO.Look) -> `PyOpenColorIO.Transform`_

      .. py:method:: getName(self: PyOpenColorIO.Look) -> str

      .. py:method:: getProcessSpace(self: PyOpenColorIO.Look) -> str

      .. py:method:: getTransform(self: PyOpenColorIO.Look) -> `PyOpenColorIO.Transform`_

      .. py:method:: setDescription(self: PyOpenColorIO.Look, description: str) -> None

      .. py:method:: setInverseTransform(self: PyOpenColorIO.Look, transform: PyOpenColorIO.Transform) -> None

      .. py:method:: setName(self: PyOpenColorIO.Look, name: str) -> None

      .. py:method:: setProcessSpace(self: PyOpenColorIO.Look, processSpace: str) -> None

      .. py:method:: setTransform(self: PyOpenColorIO.Look, transform: PyOpenColorIO.Transform) -> None
