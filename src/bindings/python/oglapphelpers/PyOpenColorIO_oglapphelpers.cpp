// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO_oglapphelpers.h"

namespace OCIO_NAMESPACE
{

PYBIND11_MODULE(PyOpenColorIO_oglapphelpers, m)
{
    bindPyGLSL(m);
    bindPyOglApp(m);
    bindPyScreenApp(m);

#ifdef OCIO_HEADLESS_ENABLED
    bindPyHeadlessApp(m);
#endif
}

} // namespace OCIO_NAMESPACE

