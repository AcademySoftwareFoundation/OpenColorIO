// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyGpuShaderCreator.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum GpuShaderDescIterator
{
    IT_TEXTURE = 0,
    IT_TEXTURE_3D,
    IT_UNIFORM
};

using TextureIterator         = PyIterator<GpuShaderDescRcPtr, IT_TEXTURE>;
using Texture3DIterator       = PyIterator<GpuShaderDescRcPtr, IT_TEXTURE_3D>;
using UniformIterator         = PyIterator<GpuShaderDescRcPtr, IT_UNIFORM>;

struct Texture
{
    std::string m_textureName;
    std::string m_samplerName;
    unsigned m_width;
    unsigned m_height;
    GpuShaderDesc::TextureType m_channel;
    GpuShaderDesc::TextureDimensions m_dimensions;
    Interpolation m_interpolation;
    GpuShaderDescRcPtr m_shaderDesc;
    int m_index;
};

struct Texture3D
{
    std::string m_textureName;
    std::string m_samplerName;
    unsigned m_edgelen;
    Interpolation m_interpolation;
    GpuShaderDescRcPtr m_shaderDesc;
    int m_index;
};

} // namespace

void bindPyGpuShaderDesc(py::module & m)
{
    GpuShaderDescRcPtr DEFAULT = GpuShaderDesc::CreateShaderDesc();

    auto clsGpuShaderDesc = 
        py::class_<GpuShaderDesc, GpuShaderDescRcPtr, GpuShaderCreator>(
            m.attr("GpuShaderDesc"));

    auto clsUniformData = 
        py::class_<GpuShaderDesc::UniformData>(
            clsGpuShaderDesc, "UniformData");

    auto clsUniformIterator = 
        py::class_<UniformIterator>(
            clsGpuShaderDesc, "UniformIterator");

    auto clsTexture = 
        py::class_<Texture>(
            clsGpuShaderDesc, "Texture");

    auto clsTextureIterator = 
        py::class_<TextureIterator>(
            clsGpuShaderDesc, "TextureIterator");

    auto clsTexture3D = 
        py::class_<Texture3D>(
            clsGpuShaderDesc, "Texture3D");

    auto clsTexture3DIterator = 
        py::class_<Texture3DIterator>(
            clsGpuShaderDesc, "Texture3DIterator");

    clsGpuShaderDesc
        .def_static("CreateShaderDesc", [](GpuLanguage lang,
                                           const std::string & functionName,
                                           const std::string & pixelName,
                                           const std::string & resourcePrefix,
                                           const std::string & uid) 
            {
                GpuShaderDescRcPtr p = GpuShaderDesc::CreateShaderDesc();
                p->setLanguage(lang);
                if (!functionName.empty())   { p->setFunctionName(functionName.c_str()); }
                if (!pixelName.empty())      { p->setPixelName(pixelName.c_str()); }
                if (!resourcePrefix.empty()) { p->setResourcePrefix(resourcePrefix.c_str()); }
                if (!uid.empty())   { p->setUniqueID(uid.c_str()); }
                return p;
            }, 
                    "language"_a = DEFAULT->getLanguage(),
                    "functionName"_a = DEFAULT->getFunctionName(),
                    "pixelName"_a = DEFAULT->getPixelName(),
                    "resourcePrefix"_a = DEFAULT->getResourcePrefix(),
                    "uid"_a = DEFAULT->getUniqueID(),
                    DOC(GpuShaderDesc, CreateShaderDesc))  

        .def("clone", &GpuShaderDesc::clone, 
             DOC(GpuShaderDesc, clone))
        .def("getShaderText", &GpuShaderDesc::getShaderText,
             DOC(GpuShaderDesc, getShaderText))
        .def("getUniforms", [](GpuShaderDescRcPtr & self) 
            {
                return UniformIterator(self);
            })

        // 1D lut related methods
        .def("addTexture", [](GpuShaderDescRcPtr & self,
                              const std::string & textureName, 
                              const std::string & samplerName, 
                              unsigned width, unsigned height,
                              GpuShaderDesc::TextureType channel, 
                              GpuShaderDesc::TextureDimensions dimensions,
                              Interpolation interpolation,
                              const py::buffer & values)
            {
                py::buffer_info info = values.request();
                py::ssize_t numChannels;

                switch (channel)
                {
                    case GpuShaderDesc::TEXTURE_RED_CHANNEL:
                        numChannels = 1;
                        break;
                    case GpuShaderDesc::TEXTURE_RGB_CHANNEL:
                        numChannels = 3;
                        break;
                    default:
                        throw Exception("Error: Unsupported texture type");
                }

                checkBufferType(info, py::dtype("float32"));
                checkBufferSize(info, width*height*numChannels);

                py::gil_scoped_release release;

                self->addTexture(textureName.c_str(),
                                 samplerName.c_str(), 
                                 width, height,
                                 channel, 
                                 dimensions,
                                 interpolation,
                                 static_cast<float *>(info.ptr));
            },
             "textureName"_a, "samplerName"_a, "width"_a, "height"_a, "channel"_a, "dimensions"_a,
             "interpolation"_a, "values"_a, 
             DOC(GpuShaderCreator, addTexture))

        .def("getTextures", [](GpuShaderDescRcPtr & self) 
            {
                return TextureIterator(self);
            })

        // 3D lut related methods
        .def("add3DTexture", [](GpuShaderDescRcPtr & self,
                                const std::string & textureName, 
                                const std::string & samplerName, 
                                unsigned edgelen,
                                Interpolation interpolation,
                                const py::buffer & values)
            {
                py::buffer_info info = values.request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferSize(info, edgelen*edgelen*edgelen*3);

                py::gil_scoped_release release;

                self->add3DTexture(textureName.c_str(), 
                                   samplerName.c_str(), 
                                   edgelen,
                                   interpolation,
                                   static_cast<float *>(info.ptr));
            },
             "textureName"_a, "samplerName"_a, "edgeLen"_a, "interpolation"_a, "values"_a, 
             DOC(GpuShaderCreator, add3DTexture))
        .def("get3DTextures", [](GpuShaderDescRcPtr & self) 
            {
                return Texture3DIterator(self);
            });

    clsUniformData
        .def(py::init<>())
        .def(py::init<const GpuShaderDesc::UniformData &>(), "data"_a)
        
        .def_readwrite("type", &GpuShaderDesc::UniformData::m_type)
        
        .def("getDouble", [](GpuShaderDesc::UniformData & self) -> double
            {
                return self.m_getDouble();
            })
        .def("getBool", [](GpuShaderDesc::UniformData & self) -> bool
            {
                return self.m_getBool();
            })
        .def("getFloat3", [](GpuShaderDesc::UniformData & self) -> Float3
            {
                return self.m_getFloat3();
            })
        .def("getVectorFloat", [](GpuShaderDesc::UniformData & self) -> py::array
            {
                return py::array(py::dtype("float32"),
                                 { self.m_vectorFloat.m_getSize() },
                                 { sizeof(float) }, 
                                 self.m_vectorFloat.m_getVector());
            })
        .def("getVectorInt", [](GpuShaderDesc::UniformData & self) -> py::array
            {
                return py::array(py::dtype("intc"),
                                 { self.m_vectorInt.m_getSize() },
                                 { sizeof(int) }, 
                                 self.m_vectorInt.m_getVector());
            });

    clsUniformIterator
        .def("__len__", [](UniformIterator & it) 
            { 
                return it.m_obj->getNumUniforms(); 
            })
        .def("__getitem__", [](UniformIterator & it, int i)
            { 
                // GpuShaderDesc provides index check with exception
                GpuShaderDesc::UniformData data;
                const char * name = it.m_obj->getUniform(i, data);

                return py::make_tuple(name, data);
            })
        .def("__iter__", [](UniformIterator & it) -> UniformIterator & 
            { 
                return it; 
            })
        .def("__next__", [](UniformIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumUniforms());

                GpuShaderDesc::UniformData data;
                const char * name = it.m_obj->getUniform(i, data);

                return py::make_tuple(name, data);
            });

    clsTexture
        .def_readonly("textureName", &Texture::m_textureName)
        .def_readonly("samplerName", &Texture::m_samplerName)
        .def_readonly("width", &Texture::m_width)
        .def_readonly("height", &Texture::m_height)
        .def_readonly("channel", &Texture::m_channel)
        .def_readonly("dimensions", &Texture::m_dimensions)
        .def_readonly("interpolation", &Texture::m_interpolation)
        .def("getValues", [](Texture & self)
            {
                py::gil_scoped_release release;
            
                const float * values;
                self.m_shaderDesc->getTextureValues(self.m_index, values);
            
                py::ssize_t numChannels;
                switch (self.m_channel)
                {
                case GpuShaderDesc::TEXTURE_RED_CHANNEL:
                    numChannels = 1;
                    break;
                case GpuShaderDesc::TEXTURE_RGB_CHANNEL:
                    numChannels = 3;
                    break;
                default:
                    throw Exception("Error: Unsupported texture type");
                }
            
                py::gil_scoped_acquire acquire;
            
                return py::array(py::dtype("float32"),
                                 { self.m_height * self.m_width * numChannels },
                                 { sizeof(float) }, 
                                 values);
            }, DOC(GpuShaderDesc, getTextureValues));

    clsTextureIterator
        .def("__len__", [](TextureIterator & it) 
            { 
                return it.m_obj->getNumTextures(); 
            })
        .def("__getitem__", [](TextureIterator & it, int i) -> Texture
            { 
                // GpuShaderDesc provides index check with exception
                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned width, height;
                GpuShaderDesc::TextureType channel;
                GpuShaderDesc::TextureDimensions dimensions;
                Interpolation interpolation;
                it.m_obj->getTexture(i, textureName, samplerName, width, height, channel, dimensions,
                                     interpolation);

                return { textureName, samplerName, width, height, channel, dimensions, interpolation,
                         it.m_obj, i};
            })
        .def("__iter__", [](TextureIterator & it) -> TextureIterator & 
            { 
                return it; 
            })
        .def("__next__", [](TextureIterator & it) -> Texture
            {
                int i = it.nextIndex(it.m_obj->getNumTextures());

                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned width, height;
                GpuShaderDesc::TextureType channel;
                GpuShaderDesc::TextureDimensions dimensions;
                Interpolation interpolation;
                it.m_obj->getTexture(i, textureName, samplerName, width, height, channel, dimensions,
                                     interpolation);

                return { textureName, samplerName, width, height, channel, dimensions, interpolation,
                         it.m_obj, i};
            });

    clsTexture3D
        .def_readonly("textureName", &Texture3D::m_textureName)
        .def_readonly("samplerName", &Texture3D::m_samplerName)
        .def_readonly("edgeLen", &Texture3D::m_edgelen)
        .def_readonly("interpolation", &Texture3D::m_interpolation)
        .def("getValues", [](Texture3D & self)
            {
                py::gil_scoped_release release;
        
                const float * values;
                self.m_shaderDesc->get3DTextureValues(self.m_index, values);
        
                py::gil_scoped_acquire acquire;
        
                return py::array(py::dtype("float32"),
                                 { self.m_edgelen * self.m_edgelen * self.m_edgelen * 3 },
                                 { sizeof(float) }, 
                                 values);
            }, DOC(GpuShaderDesc, get3DTextureValues));

    clsTexture3DIterator
        .def("__len__", [](Texture3DIterator & it) 
            { 
                return it.m_obj->getNum3DTextures(); 
            })
        .def("__getitem__", [](Texture3DIterator & it, int i) -> Texture3D
            { 
                // GpuShaderDesc provides index check with exception
                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned edgelen;
                Interpolation interpolation;
                it.m_obj->get3DTexture(i, textureName, samplerName, edgelen, interpolation);

                return { textureName, samplerName, edgelen, interpolation, it.m_obj, i };
            })
        .def("__iter__", [](Texture3DIterator & it) -> Texture3DIterator & 
            { 
                return it; 
            })
        .def("__next__", [](Texture3DIterator & it) -> Texture3D
            {
                int i = it.nextIndex(it.m_obj->getNum3DTextures());

                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned edgelen;
                Interpolation interpolation;
                it.m_obj->get3DTexture(i, textureName, samplerName, edgelen, interpolation);

                return { textureName, samplerName, edgelen, interpolation, it.m_obj, i };
            });
}

} // namespace OCIO_NAMESPACE
