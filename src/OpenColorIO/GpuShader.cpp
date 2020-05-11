// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"
#include "GpuShader.h"
#include "ops/lut3d/Lut3DOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace
{

static void  CreateArray(const float * buf,
                         unsigned w, unsigned h, unsigned d,
                         GpuShaderDesc::TextureType type,
                         std::vector<float> & res)
{
    if(buf==nullptr)
    {
        throw Exception("The buffer is invalid");
    }

    const size_t size
        = w * h * d * (type==GpuShaderDesc::TEXTURE_RGB_CHANNEL ? 3 : 1);
    res.resize(size);
    std::memcpy(&res[0], buf, size * sizeof(float));
}
}

namespace GPUShaderImpl
{

class PrivateImpl
{
public:
    struct Texture
    {
        Texture(const char * textureName,
                const char * samplerName,
                const char * uid,
                unsigned w, unsigned h, unsigned d,
                GpuShaderDesc::TextureType channel,
                Interpolation interpolation,
                const float * v)
            :   m_textureName(textureName)
            ,   m_samplerName(samplerName)
            ,   m_uid(uid)
            ,   m_width(w)
            ,   m_height(h)
            ,   m_depth(d)
            ,   m_type(channel)
            ,   m_interp(interpolation)
        {
            if (!textureName || !*textureName)
            {
                throw Exception("The texture name is invalid.");
            }

            if (!samplerName || !*samplerName)
            {
                throw Exception("The texture sampler name is invalid.");
            }

            if (w==0 || h==0 || d==0)
            {
                std::stringstream ss;
                ss << "The texture buffer size is invalid: ["
                   << w << " x " << h << " x " << d << "].";

                throw Exception(ss.str().c_str());
            }

            // An unfortunate copy is mandatory to allow the creation of a GPU shader cache.
            // The cache needs a decoupling of the processor and shader instances forbidding
            // shared naked pointer usage.
            CreateArray(v, m_width, m_height, m_depth, m_type, m_values);
        }

        std::string m_textureName;
        std::string m_samplerName;
        std::string m_uid;
        unsigned m_width;
        unsigned m_height;
        unsigned m_depth;
        GpuShaderDesc::TextureType m_type;
        Interpolation m_interp;

        std::vector<float> m_values;

        Texture() = delete;
    };

    typedef std::vector<Texture> Textures;

    struct Uniform
    {
        Uniform(const char * name, const DynamicPropertyRcPtr & value)
            :   m_name(name)
        {
            if (m_name.empty())
            {
                throw Exception("The dynamic property name is invalid.");
            }

            if (!value->isDynamic())
            {
                // When a dynamic property is not dynamic, the GLSL fragment
                // shader program should use a constant variable
                // (i.e. not a uniform variable).
                throw Exception("The dynamic property is not dynamic.");
            }

            m_value
                = std::make_shared<DynamicPropertyImpl>(
                    *dynamic_cast<DynamicPropertyImpl*>(value.get()) );

        }

        std::string m_name;
        DynamicPropertyRcPtr m_value;

        Uniform() = delete;
    };

    typedef std::vector<Uniform> Uniforms;

public:
    PrivateImpl() : m_max1DLUTWidth(4 * 1024) {}
    PrivateImpl(const PrivateImpl & rhs) = delete;
    PrivateImpl& operator= (const PrivateImpl & rhs) = delete;

    virtual ~PrivateImpl() {}

    inline unsigned get3dLutMaxDimension() const { return Lut3DOpData::maxSupportedLength; }

    inline unsigned get1dLutMaxWidth() const { return m_max1DLUTWidth; }
    inline void set1dLutMaxWidth(unsigned maxWidth) { m_max1DLUTWidth = maxWidth; }

    void addTexture(const char * textureName,
                    const char * samplerName,
                    const char * uid,
                    unsigned width, unsigned height,
                    GpuShaderDesc::TextureType channel,
                    Interpolation interpolation,
                    const float * values)
    {
        if(width > get1dLutMaxWidth())
        {
            std::stringstream ss;
            ss  << "1D LUT size exceeds the maximum: "
                << width << " > " << get1dLutMaxWidth();
            throw Exception(ss.str().c_str());
        }

        Texture t(textureName, samplerName, uid, width, height, 1, channel, interpolation, values);
        m_textures.push_back(t);
    }

    void getTexture(unsigned index,
                    const char *& textureName,
                    const char *& samplerName,
                    const char *& uid,
                    unsigned & width, unsigned & height,
                    GpuShaderDesc::TextureType & channel,
                    Interpolation & interpolation) const
    {
        if(index >= m_textures.size())
        {
            std::ostringstream ss;
            ss << "1D LUT access error: index = " << index
               << " where size = " << m_textures.size();
            throw Exception(ss.str().c_str());
        }

        const Texture & t = m_textures[index];
        textureName   = t.m_textureName.c_str();
        samplerName   = t.m_samplerName.c_str();
        uid           = t.m_uid.c_str();
        width         = t.m_width;
        height        = t.m_height;
        channel       = t.m_type;
        interpolation = t.m_interp;
    }

    void getTextureValues(unsigned index, const float *& values) const
    {
        if(index >= m_textures.size())
        {
            std::ostringstream ss;
            ss << "1D LUT access error: index = " << index
               << " where size = " << m_textures.size();
            throw Exception(ss.str().c_str());
        }

        const Texture & t = m_textures[index];
        values   = &t.m_values[0];
    }

    void add3DTexture(const char * textureName,
                      const char * samplerName,
                      const char * uid,
                      unsigned dimension,
                      Interpolation interpolation,
                      const float * values)
    {
        if(dimension > get3dLutMaxDimension())
        {
            std::stringstream ss;
            ss  << "3D LUT dimension exceeds the maximum: "
                << dimension << " > " << get3dLutMaxDimension();
            throw Exception(ss.str().c_str());
        }

        Texture t(textureName, samplerName, uid, dimension, dimension, dimension,
                  GpuShaderDesc::TEXTURE_RGB_CHANNEL,
                  interpolation, values);
        m_textures3D.push_back(t);
    }

    void get3DTexture(unsigned index,
                      const char *& textureName,
                      const char *& samplerName,
                      const char *& uid,
                      unsigned & edgelen,
                      Interpolation & interpolation) const
    {
        if(index >= m_textures3D.size())
        {
            std::ostringstream ss;
            ss << "3D LUT access error: index = " << index
               << " where size = " << m_textures3D.size();
            throw Exception(ss.str().c_str());
        }

        const Texture & t = m_textures3D[index];
        textureName   = t.m_textureName.c_str();
        samplerName   = t.m_samplerName.c_str();
        uid           = t.m_uid.c_str();
        edgelen       = t.m_width;
        interpolation = t.m_interp;
    }

    void get3DTextureValues(unsigned index, const float *& values) const
    {
        if(index >= m_textures3D.size())
        {
            std::ostringstream ss;
            ss << "3D LUT access error: index = " << index
               << " where size = " << m_textures3D.size();
            throw Exception(ss.str().c_str());
        }

        const Texture & t = m_textures3D[index];
        values = &t.m_values[0];
    }

    unsigned getNumUniforms() const
    {
        return (unsigned)m_uniforms.size();
    }

    void getUniform(unsigned index, const char *& name, DynamicPropertyRcPtr & value) const
    {
        if (index >= (unsigned)m_uniforms.size())
        {
            std::ostringstream ss;
            ss << "Uniforms access error: index = " << index
               << " where size = " << m_uniforms.size();
            throw Exception(ss.str().c_str());
        }
        const Uniform & u = m_uniforms[index];
        name = u.m_name.c_str();
        value = u.m_value;
    }

    bool addUniform(const char * name, const DynamicPropertyRcPtr & value)
    {
        for (auto u : m_uniforms)
        {
            if (*u.m_value == *value)
            {
                if(std::string(name)!=u.m_name)
                {
                    std::string err("Same dynamic properties must have the same name: ");
                    err += u.m_name + " vs. " + name;
                    throw Exception(err.c_str());
                }

                // Uniform is already there.
                return false;
            }
        }
        m_uniforms.emplace_back(name, value);
        return true;
    }

    Textures m_textures;
    Textures m_textures3D;
    Uniforms m_uniforms;

private:
    unsigned m_max1DLUTWidth;
};

} // namespace GPUShaderImpl

class LegacyGpuShaderDesc::Impl : public GPUShaderImpl::PrivateImpl
{
public:
    Impl() = delete;
    Impl(const Impl &) = delete;
    Impl & operator=(const Impl &) = delete;

    explicit Impl(unsigned edgelen)
        :   GPUShaderImpl::PrivateImpl()
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

unsigned LegacyGpuShaderDesc::getNumUniforms() const noexcept
{
    return 0;
}

void LegacyGpuShaderDesc::getUniform(unsigned, const char *&, DynamicPropertyRcPtr &) const
{
    throw Exception("Uniforms are not supported");
}

bool LegacyGpuShaderDesc::addUniform(const char *, const DynamicPropertyRcPtr &)
{
    throw Exception("Uniforms are not supported");
}

unsigned LegacyGpuShaderDesc::getTextureMaxWidth() const noexcept
{
    return 0;
}

void LegacyGpuShaderDesc::setTextureMaxWidth(unsigned)
{
    throw Exception("1D LUTs are not supported");
}

unsigned LegacyGpuShaderDesc::getNumTextures() const noexcept
{
    return 0;
}

void LegacyGpuShaderDesc::addTexture(
    const char *, const char *, const char *, unsigned, unsigned,
    TextureType, Interpolation, const float *)
{
    throw Exception("1D LUTs are not supported");
}

void LegacyGpuShaderDesc::getTexture(
    unsigned, const char *&, const char *&, const char *&,
    unsigned &, unsigned &, TextureType &, Interpolation &) const
{
    throw Exception("1D LUTs are not supported");
}

void LegacyGpuShaderDesc::getTextureValues(unsigned, const float *&) const
{
    throw Exception("1D LUTs are not supported");
}

unsigned LegacyGpuShaderDesc::getNum3DTextures() const noexcept
{
    return unsigned(getImpl()->m_textures3D.size());
}

void LegacyGpuShaderDesc::add3DTexture(const char * textureName,
                                       const char * samplerName,
                                       const char * uid, unsigned dimension,
                                       Interpolation interpolation,
                                       const float * values)
{
    if(dimension!=getImpl()->getEdgelen())
    {
        std::ostringstream ss;
        ss << "3D Texture size unexpected: " << dimension
           << " instead of " << getImpl()->getEdgelen();
        throw Exception(ss.str().c_str());
    }

    if(getImpl()->m_textures3D.size()!=0)
    {
        const std::string ss("3D Texture error: only one 3D texture allowed");
        throw Exception(ss.c_str());
    }

    getImpl()->add3DTexture(textureName, samplerName, uid, dimension, interpolation, values);
}

void LegacyGpuShaderDesc::get3DTexture(unsigned index,
                                       const char *& textureName,
                                       const char *& samplerName,
                                       const char *& uid,
                                       unsigned & edgelen,
                                       Interpolation & interpolation) const
{
    getImpl()->get3DTexture(index, textureName, samplerName, uid, edgelen, interpolation);
}

void LegacyGpuShaderDesc::get3DTextureValues(unsigned index, const float *& values) const
{
    getImpl()->get3DTextureValues(index, values);
}

void LegacyGpuShaderDesc::Deleter(LegacyGpuShaderDesc* c)
{
    delete c;
}


class GenericGpuShaderDesc::Impl : public GPUShaderImpl::PrivateImpl
{
public:
    Impl() : GPUShaderImpl::PrivateImpl() {}
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

unsigned GenericGpuShaderDesc::getNumUniforms() const noexcept
{
    return getImpl()->getNumUniforms();
}

void GenericGpuShaderDesc::getUniform(unsigned index, const char *& name,
                                      DynamicPropertyRcPtr & value) const
{
    return getImpl()->getUniform(index, name, value);
}

bool GenericGpuShaderDesc::addUniform(const char * name, const DynamicPropertyRcPtr & value)
{
    return getImpl()->addUniform(name, value);
}

unsigned GenericGpuShaderDesc::getTextureMaxWidth() const noexcept
{
    return getImpl()->get1dLutMaxWidth();
}

void GenericGpuShaderDesc::setTextureMaxWidth(unsigned maxWidth)
{
    getImpl()->set1dLutMaxWidth(maxWidth);
}

unsigned GenericGpuShaderDesc::getNumTextures() const noexcept
{
    return unsigned(getImpl()->m_textures.size());
}

void GenericGpuShaderDesc::addTexture(const char * textureName,
                                      const char * samplerName,
                                      const char * uid,
                                      unsigned width, unsigned height,
                                      TextureType channel,
                                      Interpolation interpolation,
                                      const float * values)
{
    getImpl()->addTexture(textureName, samplerName, uid, width, height, channel, interpolation, values);
}

void GenericGpuShaderDesc::getTexture(unsigned index,
                                      const char *& textureName,
                                      const char *& samplerName,
                                      const char *& uid,
                                      unsigned & width, unsigned & height,
                                      TextureType & channel,
                                      Interpolation & interpolation) const
{
    getImpl()->getTexture(index, textureName, samplerName, uid, width, height, channel, interpolation);
}

void GenericGpuShaderDesc::getTextureValues(unsigned index, const float *& values) const
{
    getImpl()->getTextureValues(index, values);
}

unsigned GenericGpuShaderDesc::getNum3DTextures() const noexcept
{
    return unsigned(getImpl()->m_textures3D.size());
}

void GenericGpuShaderDesc::add3DTexture(const char * textureName,
                                        const char * samplerName,
                                        const char * uid,
                                        unsigned edgelen,
                                        Interpolation interpolation,
                                        const float * values)
{
    getImpl()->add3DTexture(textureName, samplerName, uid, edgelen, interpolation, values);
}

void GenericGpuShaderDesc::get3DTexture(unsigned index,
                                        const char *& textureName,
                                        const char *& samplerName,
                                        const char *& uid,
                                        unsigned & edgelen,
                                        Interpolation & interpolation) const
{
    getImpl()->get3DTexture(index, textureName, samplerName, uid, edgelen, interpolation);
}

void GenericGpuShaderDesc::get3DTextureValues(unsigned index, const float *& values) const
{
    getImpl()->get3DTextureValues(index, values);
}

void GenericGpuShaderDesc::Deleter(GenericGpuShaderDesc* c)
{
    delete c;
}

} // namespace OCIO_NAMESPACE

