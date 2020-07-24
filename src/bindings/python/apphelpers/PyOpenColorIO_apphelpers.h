// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYOPENCOLORIO_APPHELPERS_H
#define INCLUDED_OCIO_PYOPENCOLORIO_APPHELPERS_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

#include <OpenColorIO/OpenColorIO.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace OCIO_NAMESPACE
{

void bindPyCategoryHelpers(py::module & m);
void bindPyColorSpaceHelpers(py::module & m);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYOPENCOLORIO_APPHELPERS_H
