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
    IT_TEXTURE_3D
};

using TextureIterator   = PyIterator<GpuShaderDescRcPtr, IT_TEXTURE>;
using Texture3DIterator = PyIterator<GpuShaderDescRcPtr, IT_TEXTURE_3D>;

struct Texture
{
    std::string textureName;
    std::string samplerName;
    unsigned width;
    unsigned height;
    GpuShaderDesc::TextureType channel;
    Interpolation interpolation;
};

struct Texture3D
{
    std::string textureName;
    std::string samplerName;
    unsigned edgelen;
    Interpolation interpolation;
};

} // namespace

void bindPyGpuShaderDesc(py::module & m)
{
    GpuShaderDescRcPtr DEFAULT = GpuShaderDesc::CreateShaderDesc();

    auto clsGpuShaderDesc = 
        py::class_<GpuShaderDesc, GpuShaderDescRcPtr /* holder */, GpuShaderCreator /* base */>(
            m, "GpuShaderDesc", 
            DOC(GpuShaderDesc));

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
        .def_static("CreateLegacyShaderDesc", [](unsigned edgelen,
                                                 GpuLanguage lang,
                                                 const std::string & functionName,
                                                 const std::string & pixelName,
                                                 const std::string & resourcePrefix,
                                                 const std::string & uid) 
            {
                GpuShaderDescRcPtr p = GpuShaderDesc::CreateLegacyShaderDesc(edgelen);
                p->setLanguage(lang);
                if (!functionName.empty())   { p->setFunctionName(functionName.c_str()); }
                if (!pixelName.empty())      { p->setPixelName(pixelName.c_str()); }
                if (!resourcePrefix.empty()) { p->setResourcePrefix(resourcePrefix.c_str()); }
                if (!uid.empty())   { p->setUniqueID(uid.c_str()); }
                return p;
            }, 
                    "edgeLen"_a,
                    "language"_a = DEFAULT->getLanguage(),
                    "functionName"_a = DEFAULT->getFunctionName(),
                    "pixelName"_a = DEFAULT->getPixelName(),
                    "resourcePrefix"_a = DEFAULT->getResourcePrefix(),
                    "uid"_a = DEFAULT->getUniqueID(),
                    DOC(GpuShaderDesc, CreateLegacyShaderDesc)) 
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

        // 1D lut related methods
        .def("getNumTextures", &GpuShaderDesc::getNumTextures, 
             DOC(GpuShaderDesc, getNumTextures))
        .def("addTexture", [](GpuShaderDescRcPtr & self,
                              const std::string & textureName, 
                              const std::string & samplerName, 
                              unsigned width, unsigned height,
                              GpuShaderDesc::TextureType channel, 
                              Interpolation interpolation,
                              const py::buffer & values)
            {
                py::buffer_info info = values.request();
                ssize_t numChannels;

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
                                 interpolation,
                                 static_cast<float *>(info.ptr));
            },
             "textureName"_a, "samplerName"_a, "width"_a, "height"_a, "channel"_a, 
             "interpolation"_a, "values"_a, 
             DOC(GpuShaderCreator, addTexture))
        .def("getTextures", [](GpuShaderDescRcPtr & self) 
            {
                return TextureIterator(self);
            })
        .def("getTextureValues", [](GpuShaderDescRcPtr & self, unsigned index) 
            {
                py::gil_scoped_release release;

                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned width, height;
                GpuShaderDesc::TextureType channel;
                Interpolation interpolation;
                self->getTexture(index, textureName, samplerName, width, height, channel, 
                                 interpolation);

                ssize_t numChannels;

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

                const float * values;
                self->getTextureValues(index, values);

                py::gil_scoped_acquire acquire;

                return py::array(py::dtype("float32"), 
                                 { height * width * numChannels },
                                 { sizeof(float) },
                                 values);
            },
             "index"_a, 
             DOC(GpuShaderDesc, getTextureValues))

        // 3D lut related methods
        .def("getNum3DTextures", &GpuShaderDesc::getNum3DTextures, 
             DOC(GpuShaderDesc, getNum3DTextures))
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
            })
        .def("get3DTextureValues", [](GpuShaderDescRcPtr & self, unsigned index) 
            {
                py::gil_scoped_release release;

                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned edgelen;
                Interpolation interpolation;
                self->get3DTexture(index, textureName, samplerName, edgelen, interpolation);

                const float * values;
                self->get3DTextureValues(index, values);

                py::gil_scoped_acquire acquire;
                
                return py::array(py::dtype("float32"), 
                                 { edgelen*edgelen*edgelen * 3 },
                                 { sizeof(float) },
                                 values);
            },
             "index"_a, 
             DOC(GpuShaderDesc, get3DTextureValues));

    clsTexture
        .def_readonly("textureName", &Texture::textureName)
        .def_readonly("samplerName", &Texture::samplerName)
        .def_readonly("width", &Texture::width)
        .def_readonly("height", &Texture::height)
        .def_readonly("channel", &Texture::channel)
        .def_readonly("interpolation", &Texture::interpolation);

    clsTextureIterator
        .def("__len__", [](TextureIterator & it) { return it.m_obj->getNumTextures(); })
        .def("__getitem__", [](TextureIterator & it, int i) -> Texture
            { 
                // GpuShaderDesc provides index check with exception
                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned width, height;
                GpuShaderDesc::TextureType channel;
                Interpolation interpolation;
                it.m_obj->getTexture(i, textureName, samplerName, width, height, channel, 
                                     interpolation);

                return { textureName, samplerName, width, height, channel, interpolation };
            })
        .def("__iter__", [](TextureIterator & it) -> TextureIterator & { return it; })
        .def("__next__", [](TextureIterator & it) -> Texture
            {
                int i = it.nextIndex(it.m_obj->getNumTextures());

                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned width, height;
                GpuShaderDesc::TextureType channel;
                Interpolation interpolation;
                it.m_obj->getTexture(i, textureName, samplerName, width, height, channel, 
                                     interpolation);

                return { textureName, samplerName, width, height, channel, interpolation };
            });

    clsTexture3D
        .def_readonly("textureName", &Texture3D::textureName)
        .def_readonly("samplerName", &Texture3D::samplerName)
        .def_readonly("edgeLen", &Texture3D::edgelen)
        .def_readonly("interpolation", &Texture3D::interpolation);

    clsTexture3DIterator
        .def("__len__", [](Texture3DIterator & it) { return it.m_obj->getNum3DTextures(); })
        .def("__getitem__", [](Texture3DIterator & it, int i) -> Texture3D
            { 
                // GpuShaderDesc provides index check with exception
                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned edgelen;
                Interpolation interpolation;
                it.m_obj->get3DTexture(i, textureName, samplerName, edgelen, interpolation);

                return { textureName, samplerName, edgelen, interpolation };
            })
        .def("__iter__", [](Texture3DIterator & it) -> Texture3DIterator & { return it; })
        .def("__next__", [](Texture3DIterator & it) -> Texture3D
            {
                int i = it.nextIndex(it.m_obj->getNum3DTextures());

                const char * textureName = nullptr;
                const char * samplerName = nullptr;
                unsigned edgelen;
                Interpolation interpolation;
                it.m_obj->get3DTexture(i, textureName, samplerName, edgelen, interpolation);

                return { textureName, samplerName, edgelen, interpolation };
            });
}

} // namespace OCIO_NAMESPACE
