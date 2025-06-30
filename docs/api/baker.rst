..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Baker
=====

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.Baker
         :members:
         :undoc-members:
         :special-members: __init__
         :exclude-members: FormatIterator

      .. autoclass:: PyOpenColorIO.Baker.FormatIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Baker
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstBakerRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::BakerRcPtr
