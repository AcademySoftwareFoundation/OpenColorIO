// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYOPENCOLORIO_H
#define INCLUDED_OCIO_PYOPENCOLORIO_H

#include <typeinfo>

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

// OpenColorIOTypes
void bindPyTypes(pybind11::module & m);

// OpenColorIO
void bindPyBaker(pybind11::module & m);
void bindPyBuiltinConfigRegistry(pybind11::module & m);
void bindPyColorSpace(pybind11::module & m);
void bindPyColorSpaceSet(pybind11::module & m);
void bindPyConfig(pybind11::module & m);
void bindPyContext(pybind11::module & m);
void bindPyConfigIOProxy(pybind11::module & m);
void bindPyCPUProcessor(pybind11::module & m);
void bindPyFileRules(pybind11::module & m);
void bindPyGPUProcessor(pybind11::module & m);
void bindPyGpuShaderCreator(pybind11::module & m);
void bindPyImageDesc(pybind11::module & m);
void bindPyLook(pybind11::module & m);
void bindPyNamedTransform(pybind11::module & m);
void bindPyProcessor(pybind11::module & m);
void bindPyProcessorMetadata(pybind11::module & m);
void bindPySystemMonitors(pybind11::module & m);
void bindPyViewingRules(pybind11::module & m);
void bindPyViewTransform(pybind11::module & m);

// OpenColorIOTransforms
void bindPyBuiltinTransformRegistry(pybind11::module & m);
void bindPyDynamicProperty(pybind11::module & m);
void bindPyFormatMetadata(pybind11::module & m);
void bindPyGradingData(pybind11::module & m);
void bindPyTransform(pybind11::module & m);

// OpenColorIOAppHelpers
void bindPyColorSpaceMenuHelpers(pybind11::module & m);
void bindPyConfigMergingHelpers(pybind11::module & m);
void bindPyDisplayViewHelpers(pybind11::module & m);
void bindPyLegacyViewingPipeline(pybind11::module & m);
void bindPyMixingHelpers(pybind11::module & m);

} // namespace OCIO_NAMESPACE

// Transform polymorphism is not detected by pybind11 outright. Custom automatic downcasting 
// is needed to return Transform subclass types from the OCIO API. See:
//   https://pybind11.readthedocs.io/en/stable/advanced/classes.html#custom-automatic-downcasters

namespace PYBIND11_NAMESPACE 
{

template<> 
struct polymorphic_type_hook<OCIO_NAMESPACE::ImageDesc> {
    static const void *get(const OCIO_NAMESPACE::ImageDesc *const src, const std::type_info*& type) {
        // Note: src may be nullptr
        if (src)
        {
            if(dynamic_cast<const OCIO_NAMESPACE::PackedImageDesc*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::PackedImageDesc);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::PlanarImageDesc*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::PlanarImageDesc);
            }
        }
        return src;
    }
};

template<> 
struct polymorphic_type_hook<OCIO_NAMESPACE::GpuShaderCreator> {
    static const void *get(const OCIO_NAMESPACE::GpuShaderCreator *const src, const std::type_info*& type) {
        // Note: src may be nullptr
        if (src)
        {
            if(dynamic_cast<const OCIO_NAMESPACE::GpuShaderDesc*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::GpuShaderDesc);
            }
        }
        return src;
    }
};

template<> 
struct polymorphic_type_hook<OCIO_NAMESPACE::Transform> {
    static const void *get(const OCIO_NAMESPACE::Transform *const src, const std::type_info*& type) {
        // Note: src may be nullptr
        if (src)
        {
            if(dynamic_cast<const OCIO_NAMESPACE::AllocationTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::AllocationTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::BuiltinTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::BuiltinTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::CDLTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::CDLTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::ColorSpaceTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::ColorSpaceTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::DisplayViewTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::DisplayViewTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::ExponentTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::ExponentTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::ExponentWithLinearTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::ExponentWithLinearTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::ExposureContrastTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::ExposureContrastTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::FileTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::FileTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::FixedFunctionTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::FixedFunctionTransform);
            }
            else if (dynamic_cast<const OCIO_NAMESPACE::GradingPrimaryTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::GradingPrimaryTransform);
            }
            else if (dynamic_cast<const OCIO_NAMESPACE::GradingRGBCurveTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::GradingRGBCurveTransform);
            }
            else if (dynamic_cast<const OCIO_NAMESPACE::GradingHueCurveTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::GradingHueCurveTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::GradingToneTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::GradingToneTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::GroupTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::GroupTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::LogAffineTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::LogAffineTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::LogCameraTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::LogCameraTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::LogTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::LogTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::LookTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::LookTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::Lut1DTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::Lut1DTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::Lut3DTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::Lut3DTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::MatrixTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::MatrixTransform);
            }
            else if(dynamic_cast<const OCIO_NAMESPACE::RangeTransform*>(src))
            {
                type = &typeid(OCIO_NAMESPACE::RangeTransform);
            }
        }
        return src;
    }
};

} // namespace pybind11

#endif // INCLUDED_OCIO_PYOPENCOLORIO_H
