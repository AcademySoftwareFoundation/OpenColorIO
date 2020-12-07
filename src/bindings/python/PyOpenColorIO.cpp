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

    // Global functions
    m.def("ClearAllCaches", &ClearAllCaches);
    m.def("GetVersion", &GetVersion);
    m.def("GetVersionHex", &GetVersionHex);
    m.def("GetLoggingLevel", &GetLoggingLevel);
    m.def("SetLoggingLevel", &SetLoggingLevel, "level"_a);
    m.def("SetLoggingFunction", &SetLoggingFunction, "logFunction"_a);
    m.def("ResetToDefaultLoggingFunction", &ResetToDefaultLoggingFunction);
    m.def("LogMessage", &LogMessage, "level"_a, "message"_a);
    m.def("GetEnvVariable", &GetEnvVariable, "name"_a);
    m.def("SetEnvVariable", &SetEnvVariable, "name"_a, "value"_a);
    m.def("UnsetEnvVariable", &UnsetEnvVariable, "name"_a);
    m.def("IsEnvVariablePresent", &IsEnvVariablePresent, "name"_a);

    // Global variables
    m.attr("__author__")    = "OpenColorIO Contributors";
    m.attr("__email__")     = "ocio-dev@lists.aswf.io";
    m.attr("__license__")   = "SPDX-License-Identifier: BSD-3-Clause";
    m.attr("__copyright__") = "Copyright Contributors to the OpenColorIO Project";
    m.attr("__version__")   = OCIO_VERSION_FULL_STR;
    m.attr("__status__")    = std::string(OCIO_VERSION_STATUS_STR).empty() ? "Production" : OCIO_VERSION_STATUS_STR;
    m.attr("__doc__")       = "OpenColorIO (OCIO) is a complete color management solution geared towards motion picture production";

    // Classes
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
    bindPySystemMonitors(m);
    bindPyGradingData(m);
    bindPyGradingPrimaryTransform(m);
    bindPyGradingRGBCurveTransform(m);
    bindPyGradingToneTransform(m);
    bindPyNamedTransform(m);
}

} // namespace OCIO_NAMESPACE
