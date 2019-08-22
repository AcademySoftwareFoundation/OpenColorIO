// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

OCIO_NAMESPACE_ENTER
{

PYBIND11_MODULE(PyOpenColorIO, m)
{
    bindPyStatic(m);
    bindPyTransform(m);
}

}
OCIO_NAMESPACE_EXIT
