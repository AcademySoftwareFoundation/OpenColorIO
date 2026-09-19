// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

void bindPyConfigCompatibilityHelpers(py::module & m)
{
    auto mConfigCompatibilityHelpers = m.def_submodule("ConfigCompatibilityHelpers")
        .def("CheckCompatibility", &ConfigCompatibilityHelpers::CheckCompatibility,
             "config"_a.none(false),
             "compatibility"_a.none(false),
             DOC(ConfigCompatibilityHelpers, CheckCompatibility));
}

} // namespace OCIO_NAMESPACE
