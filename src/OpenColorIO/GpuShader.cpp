/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sstream>
#include <vector>
#include <string>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShader.h"
#include "HashUtils.h"


namespace
{

// 129 allows for a MESH dimension of 7 in the 3dl file format.
const unsigned max3DLUTDimension = 129;


static void  CreateArray(const float * buf, 
                         unsigned w, unsigned h, unsigned d, 
                         OCIO_NAMESPACE::GpuShaderDesc::TextureType type, 
                         std::vector<float> & res)
{
    if(buf!=nullptr)
    {
        const unsigned size 
            = w * h * d * (type==OCIO_NAMESPACE::GpuShaderDesc::TEXTURE_RGB_CHANNEL ? 3 : 1);
        res.resize(size);
        memcpy(&res[0], buf, size * sizeof(float));
    }
    else
    {
        throw OCIO_NAMESPACE::Exception("Missing texture values");
    }
}

class PrivateImpl
{
public:
    struct Texture
    {
        Texture(const char * n, const char * identifier, 
                unsigned w, unsigned h, unsigned d,
                OCIO_NAMESPACE::GpuShaderDesc::TextureType channel, 
                OCIO_NAMESPACE::Interpolation interpolation, 
                const float * v)
            :   name(n)
            ,   id(identifier)
            ,   width(w)
            ,   height(h)
            ,   depth(d)
            ,   type(channel)
            ,   interp(interpolation)
        {
            // An unfortunate copy is mandatory to allow the creation of a GPU shader cache. 
            // The cache needs a decoupling of the processor and shader instances forbidding
            // shared naked pointer usage.
            CreateArray(v, width, height, depth, type, values);
        }

        std::string name;
        std::string id;
        unsigned width;
        unsigned height;
        unsigned depth;
        OCIO_NAMESPACE::GpuShaderDesc::TextureType type;
        OCIO_NAMESPACE::Interpolation interp;

        std::vector<float> values;

        Texture() = delete;
    };

    typedef std::vector<Texture> Textures;

public:       
    PrivateImpl()
        :   m_max1DLUTWidth(4 * 1024)
    {
    }

    virtual ~PrivateImpl() {}

    inline unsigned get3dLutMaxDimension() const { return max3DLUTDimension; }

    inline unsigned get1dLutMaxWidth() const { return m_max1DLUTWidth; }
    inline void set1dLutMaxWidth(unsigned maxWidth) { m_max1DLUTWidth = maxWidth; }

    void addTexture(const char * name, const char * id, unsigned width, unsigned height, 
                    OCIO_NAMESPACE::GpuShaderDesc::TextureType channel,
                    OCIO_NAMESPACE::Interpolation interpolation, const float * values)
    {
        if(width > get1dLutMaxWidth())
        {
            std::stringstream ss;
            ss  << "1D LUT size exceeds the maximum: "
                << width << " > " << get1dLutMaxWidth();
            throw OCIO_NAMESPACE::Exception(ss.str().c_str());
        }

        Texture t(name, id, width, height, 1, channel, interpolation, values);
        textures.push_back(t);
    }

    void getTexture(unsigned index, const char *& name, const char *& id, 
                    unsigned & width, unsigned & height, 
                    OCIO_NAMESPACE::GpuShaderDesc::TextureType & channel, 
                    OCIO_NAMESPACE::Interpolation & interpolation) const
    {
        if(index >= textures.size())
        {
            std::ostringstream ss;
            ss << "1D LUT access error: index = " << index
               << " where size = " << textures.size();
            throw OCIO_NAMESPACE::Exception(ss.str().c_str());
        }

        const Texture & t = textures[index];
        name          = t.name.c_str();
        id            = t.id.c_str();
        width         = t.width;
        height        = t.height;
        channel       = t.type;
        interpolation = t.interp;
    }

    void getTextureValues(unsigned index, const float *& values) const
    {
        if(index >= textures.size())
        {
            std::ostringstream ss;
            ss << "1D LUT access error: index = " << index
               << " where size = " << textures.size();
            throw OCIO_NAMESPACE::Exception(ss.str().c_str());
        }

        const Texture & t = textures[index];
        values   = &t.values[0];
    }

    void add3DTexture(const char * name, const char * id, unsigned dimension, 
                      OCIO_NAMESPACE::Interpolation interpolation, const float * values)
    {
        if(dimension > get3dLutMaxDimension())
        {
            std::stringstream ss;
            ss  << "3D LUT dimension exceeds the maximum: "
                << dimension << " > " << get3dLutMaxDimension();
            throw OCIO_NAMESPACE::Exception(ss.str().c_str());
        }

        Texture t(
            name, id, dimension, dimension, dimension, 
            OCIO_NAMESPACE::GpuShaderDesc::TEXTURE_RGB_CHANNEL, 
            interpolation, values);
        textures3D.push_back(t);
    }

    void get3DTexture(unsigned index, const char *& name, const char *& id, 
                      unsigned & edgelen, 
                      OCIO_NAMESPACE::Interpolation & interpolation) const
    {
        if(index >= textures3D.size())
        {
            std::ostringstream ss;
            ss << "3D LUT access error: index = " << index
               << " where size = " << textures3D.size();
            throw OCIO_NAMESPACE::Exception(ss.str().c_str());
        }

        const Texture & t = textures3D[index];
        name          = t.name.c_str();
        id            = t.id.c_str();
        edgelen       = t.width;
        interpolation = t.interp;
    }

    void get3DTextureValues(unsigned index, const float *& values) const
    {
        if(index >= textures3D.size())
        {
            std::ostringstream ss;
            ss << "3D LUT access error: index = " << index
               << " where size = " << textures3D.size();
            throw OCIO_NAMESPACE::Exception(ss.str().c_str());
        }

        const Texture & t = textures3D[index];
        values = &t.values[0];
    }

    void createShaderText(const char * shaderDeclarations,
                          const char * shaderHelperMethods,
                          const char * shaderFunctionHeader,
                          const char * shaderFunctionBody,
                          const char * shaderFunctionFooter)
    {
        shaderCode.resize(0);
        shaderCode += (shaderDeclarations && *shaderDeclarations) ? shaderDeclarations : "";
        shaderCode += (shaderHelperMethods && *shaderHelperMethods) ? shaderHelperMethods : "";
        shaderCode += (shaderFunctionHeader && *shaderFunctionHeader) ? shaderFunctionHeader : "";
        shaderCode += (shaderFunctionBody && shaderFunctionBody) ? shaderFunctionBody : "";
        shaderCode += (shaderFunctionFooter && shaderFunctionFooter) ? shaderFunctionFooter : "";

        shaderCodeID.resize(0);
    }

    void finalize(const std::string & cacheID)
    {
        // Finalize the shader program
        createShaderText(declarations.c_str(),
                         helperMethods.c_str(), 
                         functionHeader.c_str(), 
                         functionBody.c_str(), 
                         functionFooter.c_str());

        // Compute the identifier
        std::ostringstream ss;
        ss << shaderCode;
        for(auto & t : textures3D)
        {
            ss << t.id;
        }
        for(auto & t : textures)
        {
            ss << t.id;
        }

        const std::string id = ss.str();
        shaderCodeID = cacheID 
            + OCIO_NAMESPACE::CacheIDHash(id.c_str(), unsigned(id.length()));
    }

    std::string declarations;
    std::string helperMethods;
    std::string functionHeader;
    std::string functionBody;
    std::string functionFooter;

    std::string shaderCode;
    std::string shaderCodeID;

    Textures textures;
    Textures textures3D;

protected:
    unsigned m_max1DLUTWidth;

private:
    PrivateImpl(const PrivateImpl & rhs) = delete;
    PrivateImpl& operator= (const PrivateImpl & rhs) = delete;
};

};


OCIO_NAMESPACE_ENTER
{

class LegacyGpuShaderDesc::Impl : public PrivateImpl
{
public:       
    explicit Impl(unsigned edgelen)
        :   PrivateImpl()
        ,   m_edgelen(edgelen)
    {
    }
    
    virtual ~Impl() {}

    inline unsigned getEdgelen() const { return m_edgelen; }

private:
    unsigned m_edgelen;
};

GpuShaderDescRcPtr LegacyGpuShaderDesc::Create(unsigned edgelen)
{
    return GpuShaderDescRcPtr(new LegacyGpuShaderDesc(edgelen), &LegacyGpuShaderDesc::Deleter);
}

LegacyGpuShaderDesc::LegacyGpuShaderDesc(unsigned edgelen)
    :   GpuShaderDesc()
    ,   m_impl(new Impl(edgelen))
{
}

LegacyGpuShaderDesc::~LegacyGpuShaderDesc()
{
    delete m_impl;
    m_impl = 0x0;
}

unsigned LegacyGpuShaderDesc::getEdgelen() const
{
    return getImpl()->getEdgelen();
}

unsigned LegacyGpuShaderDesc::getNumUniforms() const
{
    return 0;
}

void LegacyGpuShaderDesc::getUniform(unsigned, const char *&, UniformType &, void *&) const
{
    throw Exception("Uniforms are not supported");
}

void LegacyGpuShaderDesc::addUniform(unsigned, const char *, UniformType, void *)
{
    throw Exception("Uniforms are not supported");
}

unsigned LegacyGpuShaderDesc::getTextureMaxWidth() const 
{
    throw Exception("1D LUTs are not supported");
    return 0;
}

void LegacyGpuShaderDesc::setTextureMaxWidth(unsigned)
{
    throw Exception("1D LUTs are not supported");
}

unsigned LegacyGpuShaderDesc::getNumTextures() const
{
    return 0;
}

void LegacyGpuShaderDesc::addTexture(
    const char *, const char *, unsigned, unsigned,
    TextureType, Interpolation, const float *)
{
    throw Exception("1D LUTs are not supported");
}

void LegacyGpuShaderDesc::getTexture(
    unsigned, const char *&, const char *&, 
    unsigned &, unsigned &, TextureType &, Interpolation &) const
{
    throw Exception("1D LUTs are not supported");
}

void LegacyGpuShaderDesc::getTextureValues(unsigned, const float *&) const
{
    throw Exception("1D LUTs are not supported");
}

unsigned LegacyGpuShaderDesc::getNum3DTextures() const
{
    return unsigned(getImpl()->textures3D.size());
}

void LegacyGpuShaderDesc::add3DTexture(const char * name, const char * id, unsigned dimension, 
                                       Interpolation interpolation, const float * values)
{
    if(dimension!=getImpl()->getEdgelen())
    {
        std::ostringstream ss;
        ss << "3D Texture size unexpected: " << dimension
           << " instead of " << getImpl()->getEdgelen();
        throw Exception(ss.str().c_str());
    }

    if(getImpl()->textures3D.size()!=0)
    {
        const std::string ss("3D Texture error: only one 3D texture allowed");
        throw Exception(ss.c_str());
    }

    getImpl()->add3DTexture(name, id, dimension, interpolation, values);
}

void LegacyGpuShaderDesc::get3DTexture(unsigned index, const char *& name, 
                                       const char *& id, unsigned & edgelen, 
                                       Interpolation & interpolation) const
{
    getImpl()->get3DTexture(index, name, id, edgelen, interpolation);
}

void LegacyGpuShaderDesc::get3DTextureValues(unsigned index, const float *& values) const
{
    getImpl()->get3DTextureValues(index, values);
}

const char * LegacyGpuShaderDesc::getShaderText() const
{
    return getImpl()->shaderCode.c_str();
}

const char * LegacyGpuShaderDesc::getCacheID() const
{
    return getImpl()->shaderCodeID.c_str();
}

void LegacyGpuShaderDesc::addToDeclareShaderCode(const char * shaderCode)
{
    if(getImpl()->declarations.empty())
    {
        getImpl()->declarations += "\n// Declaration of all variables\n\n";
    }
    getImpl()->declarations += (shaderCode && *shaderCode) ? shaderCode : "";
}

void LegacyGpuShaderDesc::addToHelperShaderCode(const char * shaderCode)
{
    getImpl()->helperMethods += (shaderCode && *shaderCode) ? shaderCode : "";
}

void LegacyGpuShaderDesc::addToFunctionShaderCode(const char * shaderCode)
{
    getImpl()->functionBody += (shaderCode && *shaderCode) ? shaderCode : "";
}

void LegacyGpuShaderDesc::addToFunctionHeaderShaderCode(const char * shaderCode)
{
    getImpl()->functionHeader += (shaderCode && *shaderCode) ? shaderCode : "";
}

void LegacyGpuShaderDesc::addToFunctionFooterShaderCode(const char * shaderCode)
{
    getImpl()->functionFooter += (shaderCode && *shaderCode) ? shaderCode : "";
}

void LegacyGpuShaderDesc::createShaderText(const char * shaderDeclarations, 
                                           const char * shaderHelperMethods,
                                           const char * shaderFunctionHeader,
                                           const char * shaderFunctionBody,
                                           const char * shaderFunctionFooter)
{
    getImpl()->createShaderText(shaderDeclarations,
                                shaderHelperMethods,
                                shaderFunctionHeader, 
                                shaderFunctionBody,
                                shaderFunctionFooter);
}

void LegacyGpuShaderDesc::finalize()
{
    getImpl()->finalize(GpuShaderDesc::getCacheID());
}

void LegacyGpuShaderDesc::Deleter(LegacyGpuShaderDesc* c)
{
    delete c;
}


class GenericGpuShaderDesc::Impl : public PrivateImpl
{
public:       
    Impl() : PrivateImpl() {}
    ~Impl() {}
};

GpuShaderDescRcPtr GenericGpuShaderDesc::Create()
{
    return GpuShaderDescRcPtr(new GenericGpuShaderDesc(), &GenericGpuShaderDesc::Deleter);
}

GenericGpuShaderDesc::GenericGpuShaderDesc()
    :   GpuShaderDesc()
    ,   m_impl(new Impl())
{
}

GenericGpuShaderDesc::~GenericGpuShaderDesc()
{
    delete m_impl;
    m_impl = 0x0;
}

unsigned GenericGpuShaderDesc::getNumUniforms() const
{
    // Outside the scope of this code review. Will add in future step.
    return 0;
}

void GenericGpuShaderDesc::getUniform(
    unsigned, const char *&, UniformType &, void *&) const
{
    // Outside the scope of this code review. Will add in future step.
    throw Exception("Not yet implemented");
}

void GenericGpuShaderDesc::addUniform(
    unsigned, const char *, UniformType, void *)
{
    // Outside the scope of this code review. Will add in future step.
    throw Exception("Not yet implemented");
}

unsigned GenericGpuShaderDesc::getTextureMaxWidth() const 
{
    return getImpl()->get1dLutMaxWidth();
}

void GenericGpuShaderDesc::setTextureMaxWidth(unsigned maxWidth)
{
    getImpl()->set1dLutMaxWidth(maxWidth);
}

unsigned GenericGpuShaderDesc::getNumTextures() const
{
    return unsigned(getImpl()->textures.size());
}

void GenericGpuShaderDesc::addTexture(const char * name, const char * id, 
                                      unsigned width, unsigned height,
                                      TextureType channel, 
                                      Interpolation interpolation,
                                      const float * values)
{
    getImpl()->addTexture(name, id, width, height, channel, interpolation, values);
}

void GenericGpuShaderDesc::getTexture(unsigned index, const char *& name, 
                                      const char *& id, unsigned & width, unsigned & height,
                                      TextureType & channel, 
                                      Interpolation & interpolation) const
{
    getImpl()->getTexture(index, name, id, width, height, channel, interpolation);
}

void GenericGpuShaderDesc::getTextureValues(unsigned index, const float *& values) const
{
    getImpl()->getTextureValues(index, values);
}

unsigned GenericGpuShaderDesc::getNum3DTextures() const
{
    return unsigned(getImpl()->textures3D.size());
}

void GenericGpuShaderDesc::add3DTexture(
    const char * name, const char * id, unsigned edgelen, 
    Interpolation interpolation, const float * values)
{
    getImpl()->add3DTexture(name, id, edgelen, interpolation, values);
}

void GenericGpuShaderDesc::get3DTexture(unsigned index, const char *& name, 
                                        const char *& id, unsigned & edgelen, 
                                        Interpolation & interpolation) const
{
    getImpl()->get3DTexture(index, name, id, edgelen, interpolation);
}

void GenericGpuShaderDesc::get3DTextureValues(unsigned index, const float *& values) const
{
    getImpl()->get3DTextureValues(index, values);
}

const char * GenericGpuShaderDesc::getShaderText() const
{
    return getImpl()->shaderCode.c_str();
}

const char * GenericGpuShaderDesc::getCacheID() const
{
    return getImpl()->shaderCodeID.c_str();
}

void GenericGpuShaderDesc::addToDeclareShaderCode(const char * shaderCode)
{
    if(getImpl()->declarations.empty())
    {
        getImpl()->declarations += "\n// Declaration of all variables\n\n";
    }
    getImpl()->declarations += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GenericGpuShaderDesc::addToHelperShaderCode(const char * shaderCode)
{
    if(getImpl()->helperMethods.empty())
    {
        getImpl()->helperMethods += "\n// Declaration of all helper methods\n\n";
    }
    getImpl()->helperMethods += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GenericGpuShaderDesc::addToFunctionShaderCode(const char * shaderCode)
{
    getImpl()->functionBody += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GenericGpuShaderDesc::addToFunctionHeaderShaderCode(const char * shaderCode)
{
    getImpl()->functionHeader += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GenericGpuShaderDesc::addToFunctionFooterShaderCode(const char * shaderCode)
{
    getImpl()->functionFooter += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GenericGpuShaderDesc::createShaderText(
    const char * shaderDeclarations, const char * shaderHelperMethods,
    const char * shaderFunctionHeader, const char * shaderFunctionBody,
    const char * shaderFunctionFooter)
{
    getImpl()->createShaderText(shaderDeclarations,
                                shaderHelperMethods,
                                shaderFunctionHeader, 
                                shaderFunctionBody,
                                shaderFunctionFooter);
}

void GenericGpuShaderDesc::finalize()
{
    getImpl()->finalize(GpuShaderDesc::getCacheID());
}

void GenericGpuShaderDesc::Deleter(GenericGpuShaderDesc* c)
{
    delete c;
}

}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


OIIO_ADD_TEST(GpuShader, generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GenericGpuShaderDesc::Create();

    {
        OIIO_CHECK_NE(shaderDesc->getLanguage(), OCIO::GPU_LANGUAGE_GLSL_1_3);
        OIIO_CHECK_NO_THROW(shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3));
        OIIO_CHECK_EQUAL(shaderDesc->getLanguage(), OCIO::GPU_LANGUAGE_GLSL_1_3);

        OIIO_CHECK_NE(std::string(shaderDesc->getFunctionName()), "1sd234_");
        OIIO_CHECK_NO_THROW(shaderDesc->setFunctionName("1sd234_"));
        OIIO_CHECK_EQUAL(std::string(shaderDesc->getFunctionName()), "1sd234_");

        OIIO_CHECK_NE(std::string(shaderDesc->getPixelName()), "pxl_1sd234_");
        OIIO_CHECK_NO_THROW(shaderDesc->setPixelName("pxl_1sd234_"));
        OIIO_CHECK_EQUAL(std::string(shaderDesc->getPixelName()), "pxl_1sd234_");

        OIIO_CHECK_NE(std::string(shaderDesc->getResourcePrefix()), "res_1sd234_");
        OIIO_CHECK_NO_THROW(shaderDesc->setResourcePrefix("res_1sd234_"));
        OIIO_CHECK_EQUAL(std::string(shaderDesc->getResourcePrefix()), "res_1sd234_");

        OIIO_CHECK_NO_THROW(shaderDesc->finalize());
        const std::string id(shaderDesc->getCacheID());
        OIIO_CHECK_NO_THROW(shaderDesc->setResourcePrefix("res_1"));
        OIIO_CHECK_NO_THROW(shaderDesc->finalize());
        OIIO_CHECK_NE(std::string(shaderDesc->getCacheID()), id);
    }

    {
        const unsigned width  = 3;
        const unsigned height = 2;
        const unsigned size = width*height*3;

        float values[size]
            = { 0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,
                0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f };

        OIIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 0U);
        OIIO_CHECK_NO_THROW(shaderDesc->addTexture("lut1", "1234", width, height, 
                                                   OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL, 
                                                   OCIO::INTERP_TETRAHEDRAL,
                                                   &values[0]));

        OIIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 1U);

        const char * name = 0x0;
        const char * id = 0x0;
        unsigned w = 0;
        unsigned h = 0;
        OCIO::GpuShaderDesc::TextureType t = OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OIIO_CHECK_NO_THROW(shaderDesc->getTexture(0, name, id, w, h, t, i));

        OIIO_CHECK_EQUAL(std::string(name), "lut1");
        OIIO_CHECK_EQUAL(std::string(id), "1234");
        OIIO_CHECK_EQUAL(width, w);
        OIIO_CHECK_EQUAL(height, h);
        OIIO_CHECK_EQUAL(OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL, t);
        OIIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OIIO_CHECK_THROW_WHAT(shaderDesc->getTexture(1, name, id, w, h, t, i),
                              OCIO::Exception,
                              "1D LUT access error");

        const float * vals = 0x0;
        OIIO_CHECK_NO_THROW(shaderDesc->getTextureValues(0, vals));
        OIIO_CHECK_NE(vals, 0x0);
        for(unsigned idx=0; idx<size; ++idx)
        {
            OIIO_CHECK_EQUAL(values[idx], vals[idx]);
        }

        OIIO_CHECK_THROW_WHAT(shaderDesc->getTextureValues(1, vals),
                              OCIO::Exception,
                              "1D LUT access error");

        // Support several 1D LUTs

        OIIO_CHECK_NO_THROW(shaderDesc->addTexture("lut2", "1234", width, height, 
                                                   OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL, 
                                                   OCIO::INTERP_TETRAHEDRAL,
                                                   &values[0]));

        OIIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 2U);

        OIIO_CHECK_NO_THROW(shaderDesc->getTextureValues(0, vals));
        OIIO_CHECK_NO_THROW(shaderDesc->getTextureValues(1, vals));
        OIIO_CHECK_THROW_WHAT(shaderDesc->getTextureValues(2, vals),
                              OCIO::Exception,
                              "1D LUT access error");
    }

    {
        const unsigned edgelen = 2;
        const unsigned size = edgelen*edgelen*edgelen*3;
        float values[size]
            = { 0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f,
                0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f, };

        OIIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 0U);
        OIIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "1234", edgelen, 
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OIIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 1U);

        const char * name = 0x0;
        const char * id = 0x0;
        unsigned e = 0;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OIIO_CHECK_NO_THROW(shaderDesc->get3DTexture(0, name, id, e, i));

        OIIO_CHECK_EQUAL(std::string(name), "lut1");
        OIIO_CHECK_EQUAL(std::string(id), "1234");
        OIIO_CHECK_EQUAL(edgelen, e);
        OIIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OIIO_CHECK_THROW_WHAT(shaderDesc->get3DTexture(1, name, id, e, i),
                              OCIO::Exception,
                              "3D LUT access error");

        const float * vals = 0x0;
        OIIO_CHECK_NO_THROW(shaderDesc->get3DTextureValues(0, vals));
        OIIO_CHECK_NE(vals, 0x0);
        for(unsigned idx=0; idx<size; ++idx)
        {
            OIIO_CHECK_EQUAL(values[idx], vals[idx]);
        }

        OIIO_CHECK_THROW_WHAT(shaderDesc->get3DTextureValues(1, vals),
                              OCIO::Exception,
                              "3D LUT access error");

        // Supports several 3D LUTs

        OIIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "1234", edgelen, 
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OIIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 2U);

        // Check the 3D LUT limit

        OIIO_CHECK_THROW(shaderDesc->add3DTexture("lut1", "1234", 130,
                                                  OCIO::INTERP_TETRAHEDRAL,
                                                  &values[0]),
                         OCIO::Exception);
    }

    {
        OIIO_CHECK_NO_THROW(shaderDesc->addToDeclareShaderCode("vec2 coords;\n"));
        OIIO_CHECK_NO_THROW(shaderDesc->addToHelperShaderCode("vec2 helpers() {}\n\n"));
        OIIO_CHECK_NO_THROW(shaderDesc->addToFunctionHeaderShaderCode("void func() {\n"));
        OIIO_CHECK_NO_THROW(shaderDesc->addToFunctionShaderCode("  int i;\n"));
        OIIO_CHECK_NO_THROW(shaderDesc->addToFunctionFooterShaderCode("}\n"));

        OIIO_CHECK_NO_THROW(shaderDesc->finalize());

        std::string fragText;
        fragText += "\n";
        fragText += "// Declaration of all variables\n";
        fragText += "\n";
        fragText += "vec2 coords;\n";
        fragText += "\n";
        fragText += "// Declaration of all helper methods\n";
        fragText += "\n";
        fragText += "vec2 helpers() {}\n\n";
        fragText += "void func() {\n";
        fragText += "  int i;\n";
        fragText += "}\n";

        OIIO_CHECK_EQUAL(fragText, shaderDesc->getShaderText());
    }
}

OIIO_ADD_TEST(GpuShader, legacy_shader)
{
    const unsigned edgelen = 2;

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::LegacyGpuShaderDesc::Create(edgelen);

    {
        const unsigned width  = 3;
        const unsigned height = 2;
        const unsigned size = width*height*3;
        float values[size];

        OIIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 0U);
        OIIO_CHECK_THROW_WHAT(shaderDesc->addTexture("lut1", "1234", width, height, 
                                                     OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL, 
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]), 
                              OCIO::Exception,
                              "1D LUTs are not supported");

        const char * name = 0x0;
        const char * id = 0x0;
        unsigned w = 0;
        unsigned h = 0;
        OCIO::GpuShaderDesc::TextureType t = OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OIIO_CHECK_THROW_WHAT(shaderDesc->getTexture(0, name, id, w, h, t, i),
                              OCIO::Exception,
                              "1D LUTs are not supported");
    }

    {
        const unsigned size = edgelen*edgelen*edgelen*3;
        float values[size]
            = { 0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f,
                0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f, };

        OIIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 0U);
        OIIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "1234", edgelen, 
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OIIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 1U);

        const char * name = 0x0;
        const char * id = 0x0;
        unsigned e = 0;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OIIO_CHECK_NO_THROW(shaderDesc->get3DTexture(0, name, id, e, i));

        OIIO_CHECK_EQUAL(std::string(name), "lut1");
        OIIO_CHECK_EQUAL(std::string(id), "1234");
        OIIO_CHECK_EQUAL(edgelen, e);
        OIIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OIIO_CHECK_THROW_WHAT(shaderDesc->get3DTexture(1, name, id, e, i),
                              OCIO::Exception,
                              "3D LUT access error");

        const float * vals = 0x0;
        OIIO_CHECK_NO_THROW(shaderDesc->get3DTextureValues(0, vals));
        OIIO_CHECK_NE(vals, 0x0);
        for(unsigned idx=0; idx<size; ++idx)
        {
            OIIO_CHECK_EQUAL(values[idx], vals[idx]);
        }

        OIIO_CHECK_THROW_WHAT(shaderDesc->get3DTextureValues(1, vals),
                              OCIO::Exception,
                              "3D LUT access error");

        // Only one 3D LUT

        OIIO_CHECK_THROW_WHAT(shaderDesc->add3DTexture("lut1", "1234", edgelen, 
                                                       OCIO::INTERP_TETRAHEDRAL,
                                                       &values[0]),
                              OCIO::Exception,
                              "only one 3D texture allowed");

    }
}


#endif