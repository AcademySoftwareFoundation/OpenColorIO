// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

PYBIND11_MODULE(PyOpenColorIO, m)
{
    // Exceptions
    py::register_exception<Exception>(m, "Exception");
    py::register_exception<ExceptionMissingFile>(m, "ExceptionMissingFile");

    // Global
    m.def("ClearAllCaches", &ClearAllCaches, DS(ClearAllCaches));
    m.def("GetVersion", &GetVersion, DS(GetVersion));
    m.def("GetVersionHex", &GetVersionHex, DS(GetVersionHex));
    m.def("GetLoggingLevel", &GetLoggingLevel, DS(GetLoggingLevel));
    m.def("SetLoggingLevel", &SetLoggingLevel, DS(SetLoggingLevel), "level"_a);
    m.def("SetLoggingFunction", &SetLoggingFunction, DS(SetLoggingFunction), "logFunction"_a);
    m.def("ResetToDefaultLoggingFunction", &ResetToDefaultLoggingFunction, DS(ResetToDefaultLoggingFunction));
    m.def("LogMessage", &LogMessage, DS(LogMessage), "level"_a, "message"_a);
    m.def("GetEnvVariable", &GetEnvVariable, DS(GetEnvVariable), "name"_a);
    m.def("SetEnvVariable", &SetEnvVariable, DS(SetEnvVariable), "name"_a, "value"_a);

    bindPyTypes(m);
    bindPyTransform(m);
    bindPyConfig(m);
    bindPyFileRules(m);
    bindPyColorSpace(m);
    bindPyColorSpaceSet(m);
    bindPyLook(m);
    bindPyViewTransform(m);
    bindPyProcessor(m);
    bindPyCPUProcessor(m);
    bindPyGPUProcessor(m);
    bindPyProcessorMetadata(m);
    bindPyBaker(m);
    bindPyImageDesc(m);
    bindPyGpuShaderCreator(m);
    bindPyContext(m);
    bindPyViewingRules(m);
}

} // namespace OCIO_NAMESPACE
