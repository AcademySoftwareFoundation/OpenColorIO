..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Exceptions
==========

.. tabs::

   .. group-tab:: Python

      .. include:: python/${PYDIR}/pyopencolorio_exception.rst

      .. include:: python/${PYDIR}/pyopencolorio_exceptionmissingfile.rst

   .. group-tab:: C++

      .. doxygenclass:: ${OCIO_NAMESPACE}::Exception
         :members:
         :undoc-members:

      .. doxygenclass:: ${OCIO_NAMESPACE}::ExceptionMissingFile
         :members:
         :undoc-members:
