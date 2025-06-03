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

std::size_t alignOffset(std::size_t offset, std::size_t alignment)
{
    if (alignment == 0)
    {
        throw Exception("Alignment cannot be zero.");
    }
    return ((offset + alignment - 1) / alignment) * alignment;
}

}

namespace GPUShaderImpl
{

constexpr size_t GPU_FLOAT_SIZE = 4;
constexpr size_t GPU_FLOAT_ALIGNMENT = 4;
constexpr size_t GPU_INT_SIZE = 4;
constexpr size_t GPU_INT_ALIGNMENT = 4;
constexpr size_t GPU_VEC3_SIZE = 12;
constexpr size_t GPU_VEC3_ALIGNMENT = 16;
constexpr size_t GPU_ARRAY_ALIGNMENT = 16;

class PrivateImpl
{
public:
    struct Texture
    {
        Texture(const char * textureName,
                const char * samplerName,
                unsigned w, unsigned h, unsigned d,
                GpuShaderDesc::TextureType channel,
                unsigned dimensions,
                Interpolation interpolation,
                const float * v)
            :   m_textureName(textureName)
            ,   m_samplerName(samplerName)
            ,   m_width(w)
            ,   m_height(h)
            ,   m_depth(d)
            ,   m_type(channel)
            ,   m_dimensions(dimensions)
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
        unsigned m_width;
        unsigned m_height;
        unsigned m_depth;
        GpuShaderDesc::TextureType m_type;
        unsigned m_dimensions;
        Interpolation m_interp;

        std::vector<float> m_values;

        Texture() = delete;
    };

    typedef std::vector<Texture> Textures;

    struct Uniform
    {
        Uniform(const char * name, const GpuShaderCreator::DoubleGetter & getDouble, std::size_t bufferOffset)
            : Uniform(name)
        {
            m_data.m_type = UNIFORM_DOUBLE;
            m_data.m_getDouble = getDouble;
            m_data.m_bufferOffset = bufferOffset;
        }

        Uniform(const char * name, const GpuShaderCreator::BoolGetter & getBool, std::size_t bufferOffset)
            : Uniform(name)
        {
            m_data.m_type = UNIFORM_BOOL;
            m_data.m_getBool = getBool;
            m_data.m_bufferOffset = bufferOffset;
        }

        Uniform(const char * name, const GpuShaderCreator::Float3Getter & getFloat3, std::size_t bufferOffset)
            : Uniform(name)
        {
            m_data.m_type = UNIFORM_FLOAT3;
            m_data.m_getFloat3 = getFloat3;
            m_data.m_bufferOffset = bufferOffset;
        }

        Uniform(const char * name, const GpuShaderCreator::SizeGetter & getSize,
                const GpuShaderCreator::VectorFloatGetter & getVectorFloat, std::size_t bufferOffset)
            : Uniform(name)
        {
            m_data.m_type = UNIFORM_VECTOR_FLOAT;
            m_data.m_vectorFloat.m_getSize = getSize;
            m_data.m_vectorFloat.m_getVector = getVectorFloat;
            m_data.m_bufferOffset = bufferOffset;
        }

        Uniform(const char * name, const GpuShaderCreator::SizeGetter & getSize,
                const GpuShaderCreator::VectorIntGetter & getVectorInt, std::size_t bufferOffset)
            : Uniform(name)
        {
            m_data.m_type = UNIFORM_VECTOR_INT;
            m_data.m_vectorInt.m_getSize = getSize;
            m_data.m_vectorInt.m_getVector = getVectorInt;
            m_data.m_bufferOffset = bufferOffset;
        }

        const std::string m_name;
        GpuShaderDesc::UniformData m_data;

        Uniform() = delete;
    private:
        Uniform(const char * name)
            : m_name(name)
        {
            if (m_name.empty())
            {
                throw Exception("The dynamic property name is invalid.");
            }
        }
    };

    typedef std::vector<Uniform> Uniforms;

public:
    PrivateImpl() : m_max1DLUTWidth(4 * 1024), m_allowTexture1D(true), m_uniformBufferSize(0) {}
    PrivateImpl(const PrivateImpl & rhs) = delete;
    PrivateImpl& operator= (const PrivateImpl & rhs) = delete;

    virtual ~PrivateImpl() {}

    inline unsigned get3dLutMaxLength() const { return Lut3DOpData::maxSupportedLength; }

    inline unsigned get1dLutMaxWidth() const { return m_max1DLUTWidth; }
    inline void set1dLutMaxWidth(unsigned maxWidth) { m_max1DLUTWidth = maxWidth; }

    inline bool getAllowTexture1D() const { return m_allowTexture1D; }
    inline void setAllowTexture1D(bool allowed) { m_allowTexture1D = allowed; }

    unsigned addTexture(const char * textureName,
                        const char * samplerName,
                        unsigned width, unsigned height,
                        GpuShaderDesc::TextureType channel,
                        GpuShaderDesc::TextureDimensions dimensions,
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
        unsigned textureIndex = static_cast<unsigned>(m_textures.size());
        unsigned numDimensions = static_cast<unsigned>(dimensions);
        Texture t(textureName, samplerName, width, height, 1, channel, numDimensions, interpolation, values);
        m_textures.push_back(t);
        return textureIndex;
    }

    void getTexture(unsigned index,
                    const char *& textureName,
                    const char *& samplerName,
                    unsigned & width, unsigned & height,
                    GpuShaderDesc::TextureType & channel,
                    GpuShaderDesc::TextureDimensions& dimensions,
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
        width         = t.m_width;
        height        = t.m_height;
        channel       = t.m_type;
        if (t.m_dimensions > 2)
        {
            std::stringstream ss;
            ss << "1D LUT cannot have more than two dimensions: "
                << t.m_dimensions << " > 2";
            throw Exception(ss.str().c_str());
        }
        dimensions = static_cast<GpuShaderDesc::TextureDimensions>(t.m_dimensions);
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

    unsigned add3DTexture(const char * textureName,
                          const char * samplerName,
                          unsigned edgelen,
                          Interpolation interpolation,
                          const float * values)
    {
        if(edgelen > get3dLutMaxLength())
        {
            std::stringstream ss;
            ss  << "3D LUT edge length exceeds the maximum: "
                << edgelen << " > " << get3dLutMaxLength();
            throw Exception(ss.str().c_str());
        }

        unsigned textureIndex = static_cast<unsigned>(m_textures3D.size());
        Texture t(textureName, samplerName, edgelen, edgelen, edgelen,
                  GpuShaderDesc::TEXTURE_RGB_CHANNEL, 3,
                  interpolation, values);
        m_textures3D.push_back(t);
        return textureIndex;
    }

    void get3DTexture(unsigned index,
                      const char *& textureName,
                      const char *& samplerName,
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

    const char * getUniform(unsigned index, GpuShaderDesc::UniformData & data) const
    {
        if (index >= (unsigned)m_uniforms.size())
        {
            std::ostringstream ss;
            ss << "Uniforms access error: index = " << index
               << " where size = " << m_uniforms.size();
            throw Exception(ss.str().c_str());
        }
        data        = m_uniforms[index].m_data;
        return m_uniforms[index].m_name.c_str();
    }

    bool addUniform(const char * name, const GpuShaderCreator::DoubleGetter & getter)
    {
        if (uniformNameUsed(name))
        {
            // Uniform is already there.
            return false;
        }
        m_uniformBufferSize = alignOffset(m_uniformBufferSize, GPU_FLOAT_ALIGNMENT);
        m_uniforms.emplace_back(name, getter, m_uniformBufferSize);
        m_uniformBufferSize += GPU_FLOAT_SIZE;
        return true;
    }

    bool addUniform(const char * name, const GpuShaderCreator::BoolGetter & getter)
    {
        if (uniformNameUsed(name))
        {
            // Uniform is already there.
            return false;
        }
        m_uniformBufferSize = alignOffset(m_uniformBufferSize, GPU_INT_ALIGNMENT);
        m_uniforms.emplace_back(name, getter, m_uniformBufferSize);
        m_uniformBufferSize += GPU_INT_SIZE; //bool not supported for buffered uniforms, using int instead
        return true;
    }

    bool addUniform(const char * name, const GpuShaderCreator::Float3Getter & getter)
    {
        if (uniformNameUsed(name))
        {
            // Uniform is already there.
            return false;
        }
        m_uniformBufferSize = alignOffset(m_uniformBufferSize, GPU_VEC3_ALIGNMENT);
        m_uniforms.emplace_back(name, getter, m_uniformBufferSize);
        m_uniformBufferSize += GPU_VEC3_SIZE;
        return true;
    }

    bool addUniform(const char * name,
                    const GpuShaderCreator::SizeGetter & getSize,
                    const GpuShaderCreator::VectorFloatGetter & getVector)
    {
        if (uniformNameUsed(name))
        {
            // Uniform is already there.
            return false;
        }
        m_uniformBufferSize = alignOffset(m_uniformBufferSize, GPU_ARRAY_ALIGNMENT);
        m_uniforms.emplace_back(name, getSize, getVector, m_uniformBufferSize);
        m_uniformBufferSize += GPU_FLOAT_SIZE * getSize();
        return true;
    }

    bool addUniform(const char * name,
                    const GpuShaderCreator::SizeGetter & getSize,
                    const GpuShaderCreator::VectorIntGetter & getVectorInt)
    {
        if (uniformNameUsed(name)) 
        {
            // Uniform is already there.
            return false;
        }
        m_uniformBufferSize = alignOffset(m_uniformBufferSize, GPU_ARRAY_ALIGNMENT);
        m_uniforms.emplace_back(name, getSize, getVectorInt, m_uniformBufferSize);
        m_uniformBufferSize += GPU_INT_SIZE * getSize();
        return true;
    }

    std::size_t getUniformBufferSize() const
    {
        return m_uniformBufferSize;
    }
    Textures m_textures;
    Textures m_textures3D;
    Uniforms m_uniforms;

private:
    bool uniformNameUsed(const char * name) const
    {
        for (auto u : m_uniforms)
        {
            if (std::string(name) == u.m_name)
            {
                return true;
            }
        }
        return false;
    }
    unsigned m_max1DLUTWidth;
    bool m_allowTexture1D;
    std::size_t m_uniformBufferSize;
};

} // namespace GPUShaderImpl

class GenericGpuShaderDesc::ImplGeneric : public GPUShaderImpl::PrivateImpl
{
public:
    ImplGeneric() : GPUShaderImpl::PrivateImpl() {}
    ~ImplGeneric() {}
};

GpuShaderDescRcPtr GenericGpuShaderDesc::Create()
{
    return GpuShaderDescRcPtr(new GenericGpuShaderDesc(), &GenericGpuShaderDesc::Deleter);
}

GenericGpuShaderDesc::GenericGpuShaderDesc()
    :   GpuShaderDesc()
    ,   m_implGeneric(new ImplGeneric())
{
}

GenericGpuShaderDesc::~GenericGpuShaderDesc()
{
    delete m_implGeneric;
    m_implGeneric = nullptr;
}

unsigned GenericGpuShaderDesc::getNumUniforms() const noexcept
{
    return getImplGeneric()->getNumUniforms();
}

const char * GenericGpuShaderDesc::getUniform(unsigned index, GpuShaderDesc::UniformData & data) const
{
    return getImplGeneric()->getUniform(index, data);
}

bool GenericGpuShaderDesc::addUniform(const char * name, const DoubleGetter & getter)
{
    return getImplGeneric()->addUniform(name, getter);
}

bool GenericGpuShaderDesc::addUniform(const char * name, const BoolGetter & getter)
{
    return getImplGeneric()->addUniform(name, getter);
}

bool GenericGpuShaderDesc::addUniform(const char * name, const Float3Getter & getter)
{
    return getImplGeneric()->addUniform(name, getter);
}

bool GenericGpuShaderDesc::addUniform(const char * name,
                                      const SizeGetter & getSize,
                                      const VectorFloatGetter & getFloatArray)
{
    return getImplGeneric()->addUniform(name, getSize, getFloatArray);
}

bool GenericGpuShaderDesc::addUniform(const char * name,
                                      const SizeGetter & getSize,
                                      const VectorIntGetter & getVectorInt)
{
    return getImplGeneric()->addUniform(name, getSize, getVectorInt);
}

std::size_t GenericGpuShaderDesc::getUniformBufferSize() const noexcept
{
    return getImplGeneric()->getUniformBufferSize();
}

unsigned GenericGpuShaderDesc::getTextureMaxWidth() const noexcept
{
    return getImplGeneric()->get1dLutMaxWidth();
}

void GenericGpuShaderDesc::setTextureMaxWidth(unsigned maxWidth)
{
    getImplGeneric()->set1dLutMaxWidth(maxWidth);
}

bool GenericGpuShaderDesc::getAllowTexture1D() const noexcept
{
    return getImplGeneric()->getAllowTexture1D();
}

void GenericGpuShaderDesc::setAllowTexture1D(bool allowed)
{
    getImplGeneric()->setAllowTexture1D(allowed);
}

unsigned GenericGpuShaderDesc::getNumTextures() const noexcept
{
    return unsigned(getImplGeneric()->m_textures.size());
}

unsigned GenericGpuShaderDesc::addTexture(const char * textureName,
                                          const char * samplerName,
                                          unsigned width, unsigned height,
                                          TextureType channel,
                                          TextureDimensions dimensions,
                                          Interpolation interpolation,
                                          const float * values)
{
    return getImplGeneric()->addTexture(textureName, samplerName, width, height, channel, dimensions, interpolation, values);
}

void GenericGpuShaderDesc::getTexture(unsigned index,
                                      const char *& textureName,
                                      const char *& samplerName,
                                      unsigned & width, unsigned & height,
                                      TextureType & channel,
                                      TextureDimensions & dimensions,
                                      Interpolation & interpolation) const
{
    getImplGeneric()->getTexture(index, textureName, samplerName, width, height, channel, dimensions, interpolation);
}

void GenericGpuShaderDesc::getTextureValues(unsigned index, const float *& values) const
{
    getImplGeneric()->getTextureValues(index, values);
}

unsigned GenericGpuShaderDesc::getNum3DTextures() const noexcept
{
    return unsigned(getImplGeneric()->m_textures3D.size());
}

unsigned GenericGpuShaderDesc::add3DTexture(const char * textureName,
                                            const char * samplerName,
                                            unsigned edgelen,
                                            Interpolation interpolation,
                                            const float * values)
{
    return getImplGeneric()->add3DTexture(textureName, samplerName, edgelen, interpolation, values);
}

void GenericGpuShaderDesc::get3DTexture(unsigned index,
                                        const char *& textureName,
                                        const char *& samplerName,
                                        unsigned & edgelen,
                                        Interpolation & interpolation) const
{
    getImplGeneric()->get3DTexture(index, textureName, samplerName, edgelen, interpolation);
}

void GenericGpuShaderDesc::get3DTextureValues(unsigned index, const float *& values) const
{
    getImplGeneric()->get3DTextureValues(index, values);
}

void GenericGpuShaderDesc::Deleter(GenericGpuShaderDesc* c)
{
    delete c;
}

} // namespace OCIO_NAMESPACE

