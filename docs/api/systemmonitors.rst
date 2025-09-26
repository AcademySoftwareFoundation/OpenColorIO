..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

SystemMonitors
==============

.. tabs::

   .. group-tab:: Python

      .. autoclass:: PyOpenColorIO.SystemMonitors
         :members:
         :undoc-members:
         :special-members: __init__
         :exclude-members: MonitorIterator

      .. autoclass:: PyOpenColorIO.SystemMonitors.MonitorIterator
         :special-members: __getitem__, __iter__, __len__, __next__

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::SystemMonitors
         :members:
         :undoc-members:

      .. doxygentypedef:: ${OCIO_NAMESPACE}::ConstSystemMonitorsRcPtr
      .. doxygentypedef:: ${OCIO_NAMESPACE}::SystemMonitorsRcPtr
