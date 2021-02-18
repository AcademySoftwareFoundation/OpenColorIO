// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYGPUSHADERCREATOR_H
#define INCLUDED_OCIO_PYGPUSHADERCREATOR_H

#include "PyOpenColorIO.h"
#include "PyUtils.h"
namespace OCIO_NAMESPACE
{

void bindPyGpuShaderDesc(py::module & m);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYGPUSHADERCREATOR_H
