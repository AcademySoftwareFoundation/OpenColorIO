..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Config
======

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.Config
         :members:
         :undoc-members:
         :special-members: __init__, __str__
         :exclude-members: EnvironmentVarNameIterator, 
                           SearchPathIterator, 
                           ColorSpaceNameIterator, 
                           ColorSpaceIterator, 
                           ActiveColorSpaceNameIterator, 
                           ActiveColorSpaceIterator, 
                           RoleNameIterator, 
                           RoleColorSpaceIterator, 
                           DisplayIterator, 
                           SharedViewIterator, 
                           ViewIterator, 
                           ViewForColorSpaceIterator, 
                           LookNameIterator, 
                           LookIterator, 
                           ViewTransformNameIterator, 
                           ViewTransformIterator,
                           NamedTransformNameIterator,
                           NamedTransformIterator,
                           ActiveNamedTransformNameIterator,
                           ActiveNamedTransformIterator

      .. autoclass:: PyOpenColorIO.Config.EnvironmentVarNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.SearchPathIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ColorSpaceNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ColorSpaceIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ActiveColorSpaceNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ActiveColorSpaceIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.RoleNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.RoleColorSpaceIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.DisplayIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.SharedViewIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ViewIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ViewForColorSpaceIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.LookNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.LookIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ViewTransformNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ViewTransformIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.NamedTransformNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.NamedTransformIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ActiveNamedTransformNameIterator
         :special-members: __getitem__, __iter__, __len__, __next__

      .. autoclass:: PyOpenColorIO.Config.ActiveNamedTransformIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenfunction:: ${OCIO_NAMESPACE}::GetCurrentConfig

      .. doxygenfunction:: ${OCIO_NAMESPACE}::SetCurrentConfig

      .. doxygenclass:: ${OCIO_NAMESPACE}::Config
         :members:
         :undoc-members:

      .. doxygenfunction:: ${OCIO_NAMESPACE}::operator<<(std::ostream&, const Config&)

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstConfigRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConfigRcPtr

Constants: :ref:`vars_roles`, :ref:`vars_shared_view`
