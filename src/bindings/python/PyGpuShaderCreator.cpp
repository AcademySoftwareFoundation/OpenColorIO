// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyDynamicProperty.h"
#include "PyGpuShaderCreator.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum GpuShaderCreatorIterator
{
    IT_DYNAMIC_PROPERTY = 0,
};

using DynamicPropertyIterator = PyIterator<GpuShaderCreatorRcPtr, IT_DYNAMIC_PROPERTY>;

} // namespace

void bindPyGpuShaderCreator(py::module & m)
{
    auto clsGpuShaderCreator = 
        py::class_<GpuShaderCreator, GpuShaderCreatorRcPtr>(
            m.attr("GpuShaderCreator"));

    auto enumTextureType = 
        py::enum_<GpuShaderCreator::TextureType>(
            clsGpuShaderCreator, "TextureType", 
            DOC(GpuShaderCreator, TextureType));

    auto enumTextureDimensions =
        py::enum_<GpuShaderCreator::TextureDimensions>(
            clsGpuShaderCreator, "TextureDimensions",
            DOC(GpuShaderCreator, TextureDimensions));

    auto clsDynamicPropertyIterator = 
        py::class_<DynamicPropertyIterator>(
            clsGpuShaderCreator, "DynamicPropertyIterator");

    clsGpuShaderCreator
        .def("clone", &GpuShaderCreator::clone, 
             DOC(GpuShaderCreator, clone))
        .def("getUniqueID", &GpuShaderCreator::getUniqueID, 
             DOC(GpuShaderCreator, getUniqueID))
        .def("setUniqueID", &GpuShaderCreator::setUniqueID, "uid"_a, 
             DOC(GpuShaderCreator, setUniqueID))
        .def("getLanguage", &GpuShaderCreator::getLanguage, 
             DOC(GpuShaderCreator, getLanguage))
        .def("setLanguage", &GpuShaderCreator::setLanguage, "language"_a, 
             DOC(GpuShaderCreator, setLanguage))
        .def("getFunctionName", &GpuShaderCreator::getFunctionName, 
             DOC(GpuShaderCreator, getFunctionName))
        .def("setFunctionName", &GpuShaderCreator::setFunctionName, "name"_a, 
             DOC(GpuShaderCreator, setFunctionName))
        .def("getPixelName", &GpuShaderCreator::getPixelName, 
             DOC(GpuShaderCreator, getPixelName))
        .def("setPixelName", &GpuShaderCreator::setPixelName, "name"_a, 
             DOC(GpuShaderCreator, setPixelName))
        .def("getResourcePrefix", &GpuShaderCreator::getResourcePrefix, 
             DOC(GpuShaderCreator, getResourcePrefix))
        .def("setResourcePrefix", &GpuShaderCreator::setResourcePrefix, "prefix"_a, 
             DOC(GpuShaderCreator, setResourcePrefix))
        .def("getCacheID", &GpuShaderCreator::getCacheID, 
             DOC(GpuShaderCreator, getCacheID))
        .def("begin", &GpuShaderCreator::begin, "uid"_a, 
             DOC(GpuShaderCreator, begin))
        .def("end", &GpuShaderCreator::end, 
             DOC(GpuShaderCreator, end))
        .def("getTextureMaxWidth", &GpuShaderCreator::getTextureMaxWidth, 
             DOC(GpuShaderCreator, getTextureMaxWidth))
        .def("setTextureMaxWidth", &GpuShaderCreator::setTextureMaxWidth, "maxWidth"_a, 
             DOC(GpuShaderCreator, setTextureMaxWidth))
        .def("setAllowTexture1D", &GpuShaderCreator::setAllowTexture1D, "allowed"_a,
            DOC(GpuShaderCreator, setAllowTexture1D))
        .def("getAllowTexture1D", &GpuShaderCreator::getAllowTexture1D,
             DOC(GpuShaderCreator, getAllowTexture1D))
        .def("getNextResourceIndex", &GpuShaderCreator::getNextResourceIndex,
            DOC(GpuShaderCreator, getNextResourceIndex))

        // Dynamic properties.
        .def("hasDynamicProperty", &GpuShaderCreator::hasDynamicProperty, "type"_a, 
             DOC(GpuShaderCreator, hasDynamicProperty))
        .def("getDynamicProperty", [](GpuShaderCreatorRcPtr & self, DynamicPropertyType type)
            {
                return PyDynamicProperty(self->getDynamicProperty(type));
            }, 
             "type"_a, 
             DOC(GpuShaderCreator, getDynamicProperty))
        .def("getDynamicProperties", [](GpuShaderCreatorRcPtr & self)
            {
                return DynamicPropertyIterator(self);
            })

        // Methods to specialize parts of a OCIO shader program
        .def("addToDeclareShaderCode", &GpuShaderCreator::addToDeclareShaderCode, "shaderCode"_a, 
             DOC(GpuShaderCreator, addToDeclareShaderCode))
        .def("addToHelperShaderCode", &GpuShaderCreator::addToHelperShaderCode, "shaderCode"_a, 
             DOC(GpuShaderCreator, addToHelperShaderCode))
        .def("addToFunctionHeaderShaderCode", 
             &GpuShaderCreator::addToFunctionHeaderShaderCode, 
             "shaderCode"_a, 
             DOC(GpuShaderCreator, addToFunctionHeaderShaderCode))
        .def("addToFunctionShaderCode", &GpuShaderCreator::addToFunctionShaderCode, "shaderCode"_a, 
             DOC(GpuShaderCreator, addToFunctionShaderCode))
        .def("addToFunctionFooterShaderCode", 
             &GpuShaderCreator::addToFunctionFooterShaderCode, 
             "shaderCode"_a, 
             DOC(GpuShaderCreator, addToFunctionFooterShaderCode))
        .def("createShaderText", &GpuShaderCreator::createShaderText, 
             "shaderDeclarations"_a, "shaderHelperMethods"_a, "shaderFunctionHeader"_a, 
             "shaderFunctionBody"_a, "shaderFunctionFooter"_a, 
             DOC(GpuShaderCreator, createShaderText))
        .def("finalize", &GpuShaderCreator::finalize, 
             DOC(GpuShaderCreator, finalize));

    enumTextureType
        .value("TEXTURE_RED_CHANNEL", GpuShaderCreator::TEXTURE_RED_CHANNEL)
        .value("TEXTURE_RGB_CHANNEL", GpuShaderCreator::TEXTURE_RGB_CHANNEL)
        .export_values();

    enumTextureDimensions
        .value("TEXTURE_1D", GpuShaderCreator::TEXTURE_1D)
        .value("TEXTURE_2D", GpuShaderCreator::TEXTURE_2D)
        .export_values();

    clsDynamicPropertyIterator
        .def("__len__", [](DynamicPropertyIterator & it) 
            { 
                return it.m_obj->getNumDynamicProperties(); 
            })
        .def("__getitem__", [](DynamicPropertyIterator & it, int i)
            {
                return PyDynamicProperty(it.m_obj->getDynamicProperty(i));
            })
        .def("__iter__", [](DynamicPropertyIterator & it) -> DynamicPropertyIterator & 
            { 
                return it; 
            })
        .def("__next__", [](DynamicPropertyIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumDynamicProperties());
                return PyDynamicProperty(it.m_obj->getDynamicProperty(i));
            });

    // Subclasses
    bindPyGpuShaderDesc(m);
}

} // namespace OCIO_NAMESPACE
