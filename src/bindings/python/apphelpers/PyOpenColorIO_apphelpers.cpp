// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO_apphelpers.h"

namespace OCIO_NAMESPACE
{

PYBIND11_MODULE(PyOpenColorIO_apphelpers, m)
{
    bindPyCategoryHelpers(m);
    bindPyColorSpaceHelpers(m);
}

} // namespace OCIO_NAMESPACE
