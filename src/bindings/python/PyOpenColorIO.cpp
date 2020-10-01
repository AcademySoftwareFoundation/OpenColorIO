// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

PYBIND11_MODULE(PyOpenColorIO, m)
{
    m.doc() = DOC(PyOpenColorIO);

    bindPyTypes(m);

    // Exceptions
    py::register_exception<Exception>(m, "Exception").doc() 
      = DOC(Exception);
    py::register_exception<ExceptionMissingFile>(m, "ExceptionMissingFile").doc() 
      = DOC(ExceptionMissingFile);

    // Global
    m.def("ClearAllCaches", &ClearAllCaches,
          DOC(PyOpenColorIO, ClearAllCaches));
    m.def("GetVersion", &GetVersion,
          DOC(PyOpenColorIO, GetVersion));
    m.def("GetVersionHex", &GetVersionHex,
          DOC(PyOpenColorIO, GetVersionHex));
    m.def("GetLoggingLevel", &GetLoggingLevel,
          DOC(PyOpenColorIO, GetLoggingLevel));
    m.def("SetLoggingLevel", &SetLoggingLevel, "level"_a,
          DOC(PyOpenColorIO, SetLoggingLevel));
    m.def("SetLoggingFunction", &SetLoggingFunction, "logFunction"_a,
          DOC(PyOpenColorIO, SetLoggingFunction));
    m.def("ResetToDefaultLoggingFunction", &ResetToDefaultLoggingFunction,
          DOC(PyOpenColorIO, ResetToDefaultLoggingFunction));
    m.def("LogMessage", &LogMessage, "level"_a, "message"_a,
          DOC(PyOpenColorIO, LogMessage));
    m.def("GetEnvVariable", &GetEnvVariable, "name"_a,
          DOC(PyOpenColorIO, GetEnvVariable));
    m.def("SetEnvVariable", &SetEnvVariable, "name"_a, "value"_a,
          DOC(PyOpenColorIO, SetEnvVariable));
    m.def("UnsetEnvVariable", &UnsetEnvVariable, "name"_a,
          DOC(PyOpenColorIO, UnsetEnvVariable));
    m.def("IsEnvVariablePresent", &IsEnvVariablePresent, "name"_a,
          DOC(PyOpenColorIO, IsEnvVariablePresent));

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
    bindPySystemMonitors(m);
    bindPyGradingData(m);
    bindPyGradingPrimaryTransform(m);
    bindPyGradingRGBCurveTransform(m);
    bindPyGradingToneTransform(m);
}

} // namespace OCIO_NAMESPACE
