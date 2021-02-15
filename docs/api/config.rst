..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Config
======

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_config.rst

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
