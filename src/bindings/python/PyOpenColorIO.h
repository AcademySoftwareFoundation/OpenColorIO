// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYOPENCOLORIO_H
#define INCLUDED_OCIO_PYOPENCOLORIO_H

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include <OpenColorIO/OpenColorIO.h>

#include "utils/StringUtils.h"

// Generated at build time: '${PROJECT_BINARY_DIR}/docs/_doxygen/docstrings.h'
#include "docstrings.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace OCIO_NAMESPACE
{

// OpenColorIOTypes
void bindPyTypes(py::module & m);

// OpenColorIO
void bindPyBaker(py::module & m);
void bindPyBuiltinConfigRegistry(py::module & m);
void bindPyColorSpace(py::module & m);
void bindPyColorSpaceSet(py::module & m);
void bindPyConfig(py::module & m);
void bindPyContext(py::module & m);
void bindPyConfigIOProxy(py::module & m);
void bindPyCPUProcessor(py::module & m);
void bindPyFileRules(py::module & m);
void bindPyGPUProcessor(py::module & m);
void bindPyGpuShaderCreator(py::module & m);
void bindPyImageDesc(py::module & m);
void bindPyLook(py::module & m);
void bindPyNamedTransform(py::module & m);
void bindPyProcessor(py::module & m);
void bindPyProcessorMetadata(py::module & m);
void bindPySystemMonitors(py::module & m);
void bindPyViewingRules(py::module & m);
void bindPyViewTransform(py::module & m);

// OpenColorIOTransforms
void bindPyBuiltinTransformRegistry(py::module & m);
void bindPyDynamicProperty(py::module & m);
void bindPyFormatMetadata(py::module & m);
void bindPyGradingData(py::module & m);
void bindPyTransform(py::module & m);

// OpenColorIOAppHelpers
void bindPyColorSpaceMenuHelpers(py::module & m);
void bindPyDisplayViewHelpers(py::module & m);
void bindPyLegacyViewingPipeline(py::module & m);
void bindPyMixingHelpers(py::module & m);

} // namespace OCIO_NAMESPACE

// Transform polymorphism is not detected by pybind11 outright. Custom automatic downcasting 
// is needed to return Transform subclass types from the OCIO API. See:
//   https://pybind11.readthedocs.io/en/stable/advanced/classes.html#custom-automatic-downcasters

namespace OCIO = OCIO_NAMESPACE;

namespace pybind11 
{

template<> 
struct polymorphic_type_hook<OCIO::ImageDesc> {
    static const void *get(const OCIO::ImageDesc *const src, const std::type_info*& type) {
        // Note: src may be nullptr
        if (src)
        {
            if(dynamic_cast<const OCIO::PackedImageDesc*>(src))
            {
                type = &typeid(OCIO::PackedImageDesc);
            }
            else if(dynamic_cast<const OCIO::PlanarImageDesc*>(src))
            {
                type = &typeid(OCIO::PlanarImageDesc);
            }
        }
        return src;
    }
};

template<> 
struct polymorphic_type_hook<OCIO::GpuShaderCreator> {
    static const void *get(const OCIO::GpuShaderCreator *const src, const std::type_info*& type) {
        // Note: src may be nullptr
        if (src)
        {
            if(dynamic_cast<const OCIO::GpuShaderDesc*>(src))
            {
                type = &typeid(OCIO::GpuShaderDesc);
            }
        }
        return src;
    }
};

template<> 
struct polymorphic_type_hook<OCIO::Transform> {
    static const void *get(const OCIO::Transform *const src, const std::type_info*& type) {
        // Note: src may be nullptr
        if (src)
        {
            if(dynamic_cast<const OCIO::AllocationTransform*>(src))
            {
                type = &typeid(OCIO::AllocationTransform);
            }
            if(dynamic_cast<const OCIO::BuiltinTransform*>(src))
            {
                type = &typeid(OCIO::BuiltinTransform);
            }
            else if(dynamic_cast<const OCIO::CDLTransform*>(src))
            {
                type = &typeid(OCIO::CDLTransform);
            }
            else if(dynamic_cast<const OCIO::ColorSpaceTransform*>(src))
            {
                type = &typeid(OCIO::ColorSpaceTransform);
            }
            else if(dynamic_cast<const OCIO::DisplayViewTransform*>(src))
            {
                type = &typeid(OCIO::DisplayViewTransform);
            }
            else if(dynamic_cast<const OCIO::ExponentTransform*>(src))
            {
                type = &typeid(OCIO::ExponentTransform);
            }
            else if(dynamic_cast<const OCIO::ExponentWithLinearTransform*>(src))
            {
                type = &typeid(OCIO::ExponentWithLinearTransform);
            }
            else if(dynamic_cast<const OCIO::ExposureContrastTransform*>(src))
            {
                type = &typeid(OCIO::ExposureContrastTransform);
            }
            else if(dynamic_cast<const OCIO::FileTransform*>(src))
            {
                type = &typeid(OCIO::FileTransform);
            }
            else if(dynamic_cast<const OCIO::FixedFunctionTransform*>(src))
            {
                type = &typeid(OCIO::FixedFunctionTransform);
            }
            else if (dynamic_cast<const OCIO::GradingPrimaryTransform*>(src))
            {
                type = &typeid(OCIO::GradingPrimaryTransform);
            }
            else if (dynamic_cast<const OCIO::GradingRGBCurveTransform*>(src))
            {
                type = &typeid(OCIO::GradingRGBCurveTransform);
            }
            if(dynamic_cast<const OCIO::GradingToneTransform*>(src))
            {
                type = &typeid(OCIO::GradingToneTransform);
            }
            else if(dynamic_cast<const OCIO::GroupTransform*>(src))
            {
                type = &typeid(OCIO::GroupTransform);
            }
            else if(dynamic_cast<const OCIO::LogAffineTransform*>(src))
            {
                type = &typeid(OCIO::LogAffineTransform);
            }
            else if(dynamic_cast<const OCIO::LogCameraTransform*>(src))
            {
                type = &typeid(OCIO::LogCameraTransform);
            }
            else if(dynamic_cast<const OCIO::LogTransform*>(src))
            {
                type = &typeid(OCIO::LogTransform);
            }
            else if(dynamic_cast<const OCIO::LookTransform*>(src))
            {
                type = &typeid(OCIO::LookTransform);
            }
            else if(dynamic_cast<const OCIO::Lut1DTransform*>(src))
            {
                type = &typeid(OCIO::Lut1DTransform);
            }
            else if(dynamic_cast<const OCIO::Lut3DTransform*>(src))
            {
                type = &typeid(OCIO::Lut3DTransform);
            }
            else if(dynamic_cast<const OCIO::MatrixTransform*>(src))
            {
                type = &typeid(OCIO::MatrixTransform);
            }
            else if(dynamic_cast<const OCIO::RangeTransform*>(src))
            {
                type = &typeid(OCIO::RangeTransform);
            }
        }
        return src;
    }
};

} // namespace pybind11

#endif // INCLUDED_OCIO_PYOPENCOLORIO_H
