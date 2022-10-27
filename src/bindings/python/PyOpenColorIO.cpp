// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

PYBIND11_MODULE(PyOpenColorIO, m)
{
    m.doc() = DOC(PyOpenColorIO);

    // OpenColorIOTypes
    bindPyTypes(m);

    // Exceptions
    auto clsException = 
        py::register_exception<Exception>(
            m, "Exception");
            
    auto clsExceptionMissingFile = 
        py::register_exception<ExceptionMissingFile>(
            m, "ExceptionMissingFile");

#if PY_VERSION_MAJOR >= 3
    // __doc__ is not writable after class creation in Python 2
    clsException.doc() = DOC(Exception);
    clsExceptionMissingFile.doc() = DOC(ExceptionMissingFile);
#endif

    // Global variables
    m.attr("__author__")    = "OpenColorIO Contributors";
    m.attr("__email__")     = "ocio-dev@lists.aswf.io";
    m.attr("__license__")   = "SPDX-License-Identifier: BSD-3-Clause";
    m.attr("__copyright__") = "Copyright Contributors to the OpenColorIO Project";
    m.attr("__version__")   = OCIO_VERSION_FULL_STR;
    m.attr("__status__")    = std::string(OCIO_VERSION_STATUS_STR).empty() ? "Production" : OCIO_VERSION_STATUS_STR;
    m.attr("__doc__")       = "OpenColorIO (OCIO) is a complete color management solution geared towards motion picture production";

    // Global functions
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
    m.def("SetComputeHashFunction", &SetComputeHashFunction, "hashFunction"_a,
          DOC(PyOpenColorIO, SetComputeHashFunction));
    m.def("ResetComputeHashFunction", &ResetComputeHashFunction,
          DOC(PyOpenColorIO, ResetComputeHashFunction));
    m.def("GetEnvVariable", &GetEnvVariable, "name"_a,
          DOC(PyOpenColorIO, GetEnvVariable));
    m.def("SetEnvVariable", &SetEnvVariable, "name"_a, "value"_a,
          DOC(PyOpenColorIO, SetEnvVariable));
    m.def("UnsetEnvVariable", &UnsetEnvVariable, "name"_a,
          DOC(PyOpenColorIO, UnsetEnvVariable));
    m.def("IsEnvVariablePresent", &IsEnvVariablePresent, "name"_a,
          DOC(PyOpenColorIO, IsEnvVariablePresent));

    // OpenColorIO
    bindPyBaker(m);
    bindPyBuiltinConfigRegistry(m);
    bindPyColorSpace(m);
    bindPyColorSpaceSet(m);
    bindPyConfig(m);
    bindPyContext(m);
    bindPyConfigIOProxy(m);
    bindPyCPUProcessor(m);
    bindPyFileRules(m);
    bindPyGPUProcessor(m);
    bindPyGpuShaderCreator(m);
    bindPyImageDesc(m);
    bindPyLook(m);
    bindPyNamedTransform(m);
    bindPyProcessor(m);
    bindPyProcessorMetadata(m);
    bindPySystemMonitors(m);
    bindPyViewingRules(m);
    bindPyViewTransform(m);

    // OpenColorIOTransforms
    bindPyBuiltinTransformRegistry(m);
    bindPyDynamicProperty(m);
    bindPyFormatMetadata(m);
    bindPyGradingData(m);
    bindPyTransform(m);

    // OpenColorIOAppHelpers
    bindPyColorSpaceMenuHelpers(m);
    bindPyDisplayViewHelpers(m);
    bindPyLegacyViewingPipeline(m);
    bindPyMixingHelpers(m);
}

} // namespace OCIO_NAMESPACE
