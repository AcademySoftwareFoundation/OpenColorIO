..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Context
=======

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.Context
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: StringVarNameIterator, 
                           StringVarIterator, 
                           SearchPathIterator

      .. autoclass:: PyOpenColorIO.Context.StringVarNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Context.StringVarIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Context.SearchPathIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Context
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Context&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstContextRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ContextRcPtr
