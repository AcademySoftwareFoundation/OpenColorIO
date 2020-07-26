// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYOPENCOLORIO_OGLAPPHELPERS_H
#define INCLUDED_OCIO_PYOPENCOLORIO_OGLAPPHELPERS_H

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

#include <OpenColorIO/OpenColorIO.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace OCIO_NAMESPACE
{

void bindPyGLSL(py::module & m);
void bindPyOglApp(py::module & m);
void bindPyScreenApp(py::module & m);

#ifdef OCIO_HEADLESS_ENABLED
void bindPyHeadlessApp(py::module & m);
#endif

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYOPENCOLORIO_OGLAPPHELPERS_H
