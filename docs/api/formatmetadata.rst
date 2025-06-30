..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

FormatMetadata
==============

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.FormatMetadata
         :members:
         :undoc-members:
         :special-members: __init__, 
                           __iter__, 
                           __len__, 
                           __getitem__, 
                           __setitem__, 
                           __contains__
         :exclude-members: AttributeNameIterator,
                           AttributeIterator,
                           ConstChildElementIterator,
                           ChildElementIterator

      .. autoclass:: PyOpenColorIO.FormatMetadata.AttributeNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.FormatMetadata.AttributeIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.FormatMetadata.ConstChildElementIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.FormatMetadata.ChildElementIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::FormatMetadata
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const FormatMetadata&)

Constants: :ref:`vars_format_metadata`
