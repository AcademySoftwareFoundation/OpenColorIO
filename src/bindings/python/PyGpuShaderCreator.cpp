// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyGpuShaderCreator.h"

namespace OCIO_NAMESPACE
{

namespace
{

// This trampoline class allows GpuShaderCreator to be subclassed in Python. Methods with 
// PYBIND11_OVERLOAD_PURE are abstract and need to be overridden. Methods with 
// PYBIND11_OVERLOAD have a default implementation.
// 
// Note: When overriding __init__ in Python, GpuShaderCreator.__init__(self) must be 
// called instead of super(<class name>, self).__init__(). See: 
//   https://pybind11.readthedocs.io/en/stable/advanced/classes.html#overriding-virtual-functions-in-python

class PyGpuShaderCreator : public GpuShaderCreator
{
public:
    using GpuShaderCreator::GpuShaderCreator;

    GpuShaderCreatorRcPtr clone() const override
    {
        PYBIND11_OVERLOAD_PURE(GpuShaderCreatorRcPtr, GpuShaderCreator, clone);
    }

    const char * getCacheID() const noexcept override
    {
        PYBIND11_OVERLOAD(const char *, GpuShaderCreator, getCacheID);
    }

    void begin(const char * uid) override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, begin, uid);
    }

    void end() override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, end);
    }

    void setTextureMaxWidth(unsigned maxWidth) override
    {
        PYBIND11_OVERLOAD_PURE(void, GpuShaderCreator, setTextureMaxWidth, maxWidth );
    }

    unsigned getTextureMaxWidth() const noexcept override
    {
        PYBIND11_OVERLOAD_PURE(unsigned, GpuShaderCreator, getTextureMaxWidth);
    }

    bool addUniform(const char * name, const GpuShaderCreator::DoubleGetter & getter) override
    {
        PYBIND11_OVERLOAD_PURE(bool, GpuShaderCreator, addUniform, name, getter);
    }

    bool addUniform(const char * name, const GpuShaderCreator::BoolGetter & getBool) override
    {
        PYBIND11_OVERLOAD_PURE(bool, GpuShaderCreator, addUniform, name, getBool);
    }

    bool addUniform(const char * name, const GpuShaderCreator::Float3Getter & getFloat3) override
    {
        PYBIND11_OVERLOAD_PURE(bool, GpuShaderCreator, addUniform, name, getFloat3);
    }

    bool addUniform(const char * name,
                    const GpuShaderCreator::SizeGetter & getSize,
                    const GpuShaderCreator::VectorFloatGetter & getVectorFloat) override
    {
        PYBIND11_OVERLOAD_PURE(bool, GpuShaderCreator, addUniform, name, getSize, getVectorFloat);
    }

    bool addUniform(const char * name,
                    const GpuShaderCreator::SizeGetter & getSize,
                    const GpuShaderCreator::VectorIntGetter & getVectorInt) override
    {
        PYBIND11_OVERLOAD_PURE(bool, GpuShaderCreator, addUniform, name, getSize, getVectorInt);
    }

    void addTexture(const char * textureName,
                    const char * samplerName,
                    unsigned width, unsigned height,
                    TextureType channel,
                    Interpolation interpolation,
                    const float * values) override
    {
        PYBIND11_OVERLOAD_PURE(
            void,
            GpuShaderCreator,
            addTexture,
            textureName, samplerName, width, height, channel, interpolation, values
        );
    }

    void add3DTexture(const char * textureName,
                      const char * samplerName,
                      unsigned edgelen,
                      Interpolation interpolation,
                      const float * values) override
    {
        PYBIND11_OVERLOAD_PURE(
            void,
            GpuShaderCreator,
            addTexture,
            textureName, samplerName, edgelen, interpolation, values
        );
    }

    void addToDeclareShaderCode(const char * shaderCode) override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, addToDeclareShaderCode, shaderCode);
    }

    void addToHelperShaderCode(const char * shaderCode) override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, addToHelperShaderCode, shaderCode);
    }

    void addToFunctionHeaderShaderCode(const char * shaderCode) override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, addToFunctionHeaderShaderCode, shaderCode);
    }

    void addToFunctionShaderCode(const char * shaderCode) override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, addToFunctionShaderCode, shaderCode);
    }

    void addToFunctionFooterShaderCode(const char * shaderCode) override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, addToFunctionFooterShaderCode, shaderCode);
    }

    void createShaderText(const char * shaderDeclarations,
                          const char * shaderHelperMethods,
                          const char * shaderFunctionHeader,
                          const char * shaderFunctionBody,
                          const char * shaderFunctionFooter) override
    {
        PYBIND11_OVERLOAD(
            void,
            GpuShaderCreator,
            createShaderText,
            shaderDeclarations, 
            shaderHelperMethods, 
            shaderFunctionHeader, 
            shaderFunctionBody, 
            shaderFunctionFooter
        );
    }

    void finalize() override
    {
        PYBIND11_OVERLOAD(void, GpuShaderCreator, finalize);
    }
};

enum GpuShaderCreatorIterator
{
    IT_DYNAMIC_PROPERTY = 0,
};

using DynamicPropertyIterator = PyIterator<GpuShaderCreatorRcPtr, IT_DYNAMIC_PROPERTY>;

} // namespace

void bindPyGpuShaderCreator(py::module & m)
{
    auto cls = py::class_<GpuShaderCreator, 
                          GpuShaderCreatorRcPtr /* holder */, 
                          PyGpuShaderCreator /* trampoline */>(m, "GpuShaderCreator")
        .def(py::init<>())

        .def("clone", &GpuShaderCreator::clone)
        .def("getUniqueID", &GpuShaderCreator::getUniqueID)
        .def("setUniqueID", &GpuShaderCreator::setUniqueID, "uid"_a)
        .def("getLanguage", &GpuShaderCreator::getLanguage)
        .def("setLanguage", &GpuShaderCreator::setLanguage, "language"_a)
        .def("getFunctionName", &GpuShaderCreator::getFunctionName)
        .def("setFunctionName", &GpuShaderCreator::setFunctionName, "name"_a)
        .def("getPixelName", &GpuShaderCreator::getPixelName)
        .def("setPixelName", &GpuShaderCreator::setPixelName, "name"_a)
        .def("getResourcePrefix", &GpuShaderCreator::getResourcePrefix)
        .def("setResourcePrefix", &GpuShaderCreator::setResourcePrefix, "prefix"_a)
        .def("getCacheID", &GpuShaderCreator::getCacheID)
        .def("begin", &GpuShaderCreator::begin, "uid"_a)
        .def("end", &GpuShaderCreator::end)
        .def("getTextureMaxWidth", &GpuShaderCreator::getTextureMaxWidth)
        .def("setTextureMaxWidth", &GpuShaderCreator::setTextureMaxWidth, "maxWidth"_a)
        .def("getNextResourceIndex", &GpuShaderCreator::getNextResourceIndex)
        .def("addTexture", &GpuShaderCreator::addTexture, 
             "textureName"_a, "samplerName"_a, "width"_a, "height"_a, "channel"_a, 
             "interpolation"_a, "values"_a)
        .def("add3DTexture", &GpuShaderCreator::add3DTexture,
             "textureName"_a, "samplerName"_a, "edgeLen"_a, "interpolation"_a, "values"_a)

        // Dynamic properties.
        .def("hasDynamicProperty", &GpuShaderCreator::hasDynamicProperty, "type"_a)
        .def("getDynamicProperty", [](GpuShaderCreatorRcPtr & self, DynamicPropertyType type)
            {
                return self->getDynamicProperty(type);
            }, "type"_a)
        .def("getDynamicProperties", [](GpuShaderCreatorRcPtr & self)
            {
                return DynamicPropertyIterator(self);
            })

        // Methods to specialize parts of a OCIO shader program
        .def("addToDeclareShaderCode", &GpuShaderCreator::addToDeclareShaderCode, "shaderCode"_a)
        .def("addToHelperShaderCode", &GpuShaderCreator::addToHelperShaderCode, "shaderCode"_a)
        .def("addToFunctionHeaderShaderCode", 
             &GpuShaderCreator::addToFunctionHeaderShaderCode, 
             "shaderCode"_a)
        .def("addToFunctionShaderCode", &GpuShaderCreator::addToFunctionShaderCode, "shaderCode"_a)
        .def("addToFunctionFooterShaderCode", 
             &GpuShaderCreator::addToFunctionFooterShaderCode, 
             "shaderCode"_a)
        .def("createShaderText", &GpuShaderCreator::createShaderText, 
             "shaderDeclarations"_a, "shaderHelperMethods"_a, "shaderFunctionHeader"_a, 
             "shaderFunctionBody"_a, "shaderFunctionFooter"_a)
        .def("finalize", &GpuShaderCreator::finalize);

    py::enum_<GpuShaderCreator::TextureType>(cls, "TextureType")
        .value("TEXTURE_RED_CHANNEL", GpuShaderCreator::TEXTURE_RED_CHANNEL)
        .value("TEXTURE_RGB_CHANNEL", GpuShaderCreator::TEXTURE_RGB_CHANNEL)
        .export_values();

    py::class_<DynamicPropertyIterator>(cls, "DynamicPropertyIterator")
    .def("__len__", [](DynamicPropertyIterator & it) { return it.m_obj->getNumDynamicProperties(); })
    .def("__getitem__", [](DynamicPropertyIterator & it, int i)
        {
            return it.m_obj->getDynamicProperty(i);
        })
    .def("__iter__", [](DynamicPropertyIterator & it) -> DynamicPropertyIterator & { return it; })
    .def("__next__", [](DynamicPropertyIterator & it)
        {
            int i = it.nextIndex(it.m_obj->getNumDynamicProperties());
            return it.m_obj->getDynamicProperty(i);
        });
    // Subclasses
    bindPyGpuShaderDesc(m);
}

} // namespace OCIO_NAMESPACE
