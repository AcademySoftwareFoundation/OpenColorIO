/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/


#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "GPUShader.h"

#include <sstream>
#include <vector>
#include <string>

#include <string.h>


namespace
{
    class PrivateImpl
    {
        static void  CreateArray(
            float * buf, unsigned w, unsigned h, unsigned d, 
            OCIO_NAMESPACE::GpuShader::TextureType type, std::vector<float>& res)
        {
            if(buf)
            {
                const unsigned size 
                    = w * h * d * (type==OCIO_NAMESPACE::GpuShader::TEXTURE_RGB_CHANNEL ? 3 : 1);
                res.resize(size);
                memcpy(&res[0], buf, size * sizeof(float));
            }
        }

    public:
        struct Texture
        {
            Texture(const char * n, const char * identifier, 
                unsigned w, unsigned h, unsigned d,
                OCIO_NAMESPACE::GpuShader::TextureType channel, 
                OCIO_NAMESPACE::Interpolation interpolation, 
                float * r, float * g, float * b)
                :   name(n)
                ,   id(identifier)
                ,   width(w)
                ,   height(h)
                ,   depth(d)
                ,   type(channel)
                ,   interp(interpolation)
                ,   red(0x0)
                ,   green(0x0)
                ,   blue(0x0)
            {
                // An unfortunate copy is mandatory to allow the creation of a GPU shader cache. 
                // The cache needs a decoupling of the processor and shader instances forbidding
                // shared naked pointer usage.
                CreateArray(r, width, height, depth, type, red);
                CreateArray(g, width, height, depth, type, green);
                CreateArray(b, width, height, depth, type, blue);
            }

            std::string name;
            std::string id;
            unsigned width;
            unsigned height;
            unsigned depth;
            OCIO_NAMESPACE::GpuShader::TextureType type;
            OCIO_NAMESPACE::Interpolation interp;

            std::vector<float> red;
            std::vector<float> green;
            std::vector<float> blue;
        };

        typedef std::vector<Texture> Textures;

    public:       
        PrivateImpl(const OCIO_NAMESPACE::GpuShaderDesc & desc) : m_desc(desc) {}
        virtual ~PrivateImpl() {}

        void addTexture(
            const char * name, const char * id, unsigned width, unsigned height, 
            OCIO_NAMESPACE::GpuShader::TextureType channel,
            OCIO_NAMESPACE::Interpolation interpolation, 
            float * red, float * green, float * blue)
        {
            Texture t(
                name, id, width, height, 1, channel, interpolation, red, green, blue);
            textures.push_back(t);
        }

        void getTexture(
            unsigned index, const char *& name, const char *& id, 
            unsigned & width, unsigned & height, 
            OCIO_NAMESPACE::GpuShader::TextureType & channel, 
            OCIO_NAMESPACE::Interpolation & interpolation) const
        {
            if(index >= textures.size())
            {
                std::ostringstream ss;
                ss << "1D Lut access error: index = " << index
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

        void getTextureValues(
            unsigned index, const float *& red, const float *& green, const float *& blue) const
        {
            if(index >= textures.size())
            {
                std::ostringstream ss;
                ss << "1D Lut access error: index = " << index
                   << " where size = " << textures.size();
                throw OCIO_NAMESPACE::Exception(ss.str().c_str());
            }

            const Texture & t = textures[index];
            red   = &t.red[0];
            green = &t.green[0];
            blue  = &t.blue[0];
        }

        void add3DTexture(
            const char * name, const char * id, unsigned dimension, 
            OCIO_NAMESPACE::Interpolation interpolation, float * values)
        {
            Texture t(
                name, id, dimension, dimension, dimension, 
                OCIO_NAMESPACE::GpuShader::TEXTURE_RGB_CHANNEL, 
                interpolation, values, 0x0, 0x0);
            textures3D.push_back(t);
        }

        void get3DTexture(
            unsigned index, const char *& name, const char *& id, unsigned & dimension) const
        {
            if(index >= textures3D.size())
            {
                std::ostringstream ss;
                ss << "3D Lut access error: index = " << index
                   << " where size = " << textures3D.size();
                throw OCIO_NAMESPACE::Exception(ss.str().c_str());
            }

            const Texture & t = textures3D[index];
            name      = t.name.c_str();
            id        = t.id.c_str();
            dimension = t.width;
        }

        void get3DTextureValues(unsigned index, const float *& value) const
        {
            if(index >= textures3D.size())
            {
                std::ostringstream ss;
                ss << "3D Lut access error: index = " << index
                   << " where size = " << textures3D.size();
                throw OCIO_NAMESPACE::Exception(ss.str().c_str());
            }

            const Texture & t = textures3D[index];
            value = &t.red[0];
        }

        void createShaderText(
            const char * shaderDeclarations, const char * shaderHelperMethods,
            const char * shaderMainHeader, const char * shaderMainBody,
            const char * shaderMainFooter)
        {
            shaderCode.resize(0);
            shaderCode += (shaderDeclarations && *shaderDeclarations) ? shaderDeclarations : "";
            shaderCode += (shaderHelperMethods && *shaderHelperMethods) ? shaderHelperMethods : "";
            shaderCode += (shaderMainHeader && *shaderMainHeader) ? shaderMainHeader : "";
            shaderCode += (shaderMainBody && shaderMainBody) ? shaderMainBody : "";
            shaderCode += (shaderMainFooter && shaderMainFooter) ? shaderMainFooter : "";

            shaderCodeID.resize(0);
        }

        void finalize()
        {
            // Finalize the shader program
            createShaderText(
                declarations.c_str(), helpers.c_str(), 
                mainHeader.c_str(), mainBody.c_str(), mainFooter.c_str());

            // Compute the identifier
            std::ostringstream ss;
            ss << m_desc.getCacheID();
            ss << shaderCode;
            for(unsigned idx=0; idx<textures3D.size(); ++idx)
            {
                ss << textures3D[idx].id;
            }
            for(unsigned idx=0; idx<textures.size(); ++idx)
            {
                ss << textures[idx].id;
            }

            const std::string id(ss.str());
            shaderCodeID = OCIO_NAMESPACE::CacheIDHash(id.c_str(), unsigned(id.length()));
        }

        inline const OCIO_NAMESPACE::GpuShaderDesc & getDesc() const { return m_desc; }

        std::string declarations;
        std::string helpers;
        std::string mainHeader;
        std::string mainBody;
        std::string mainFooter;

        std::string shaderCode;
        std::string shaderCodeID;

        Textures textures;
        Textures textures3D;

    private:
        PrivateImpl(const PrivateImpl & rhs);
        PrivateImpl& operator= (const PrivateImpl & rhs);

        const OCIO_NAMESPACE::GpuShaderDesc m_desc;
    };
};


OCIO_NAMESPACE_ENTER
{
    GpuShaderRcPtr GpuShader::CreateLegacyShader(const GpuShaderDesc & desc, unsigned edgelen)
    {
        return LegacyGpuShader::Create(desc, edgelen);
    }

    GpuShaderRcPtr GpuShader::CreateShader(const GpuShaderDesc & desc)
    {
        return GenericGpuShader::Create(desc);
    }

    class LegacyGpuShader::Impl : public PrivateImpl
    {
    public:       
        Impl(const GpuShaderDesc & desc, unsigned edgelen)
            :   PrivateImpl(desc)
            ,   _edgelen(edgelen)
        {
        }
        
        virtual ~Impl() {}

        inline unsigned getEdgelen() const { return _edgelen; }
    
    private:
        Impl(const Impl & rhs);
        Impl& operator= (const Impl & rhs);

        unsigned _edgelen;
    };

    GpuShaderRcPtr LegacyGpuShader::Create(const GpuShaderDesc & desc, unsigned edgelen)
    {
        return GpuShaderRcPtr(new LegacyGpuShader(desc, edgelen), &LegacyGpuShader::Deleter);
    }

    LegacyGpuShader::LegacyGpuShader(const GpuShaderDesc & desc, unsigned edgelen)
        :   GpuShader()
        ,   m_impl(new Impl(desc, edgelen))
    {
    }

    LegacyGpuShader::~LegacyGpuShader()
    {
        delete m_impl;
        m_impl = 0x0;
    }

    GpuLanguage LegacyGpuShader::getLanguage() const
    {
        return getImpl()->getDesc().getLanguage();
    }

    const char * LegacyGpuShader::getFunctionName() const
    {
        return getImpl()->getDesc().getFunctionName();
    }

    const char * LegacyGpuShader::getPixelName() const
    {
        return getImpl()->getDesc().getPixelName();
    }

    const char * LegacyGpuShader::getNamePrefix() const
    {
        return getImpl()->getDesc().getNamePrefix();
    }

    unsigned LegacyGpuShader::getEdgelen() const
    {
        return getImpl()->getEdgelen();
    }

    unsigned LegacyGpuShader::getNumUniforms() const
    {
        return 0;
    }

    void LegacyGpuShader::getUniform(unsigned, const char *&, UniformType &, void *&) const
    {
        throw Exception("Uniforms are not supported");
    }

    void LegacyGpuShader::addUniform(unsigned, const char *, UniformType, void *)
    {
        throw Exception("Uniforms are not supported");
    }

    unsigned LegacyGpuShader::getNumTextures() const
    {
        return 0;
    }

    void LegacyGpuShader::addTexture(
        const char *, const char *, unsigned, unsigned,
        TextureType, Interpolation, float *, float *, float *)
    {
        throw Exception("1D luts are not supported");
    }

    void LegacyGpuShader::getTexture(
        unsigned, const char *&, const char *&, 
        unsigned &, unsigned &, TextureType &, Interpolation &) const
    {
        throw Exception("1D luts are not supported");
    }

    void LegacyGpuShader::getTextureValues(
        unsigned, const float *&, const float *&, const float *&) const
    {
        throw Exception("1D luts are not supported");
    }

    unsigned LegacyGpuShader::getNum3DTextures() const
    {
        return unsigned(getImpl()->textures3D.size());
    }

    void LegacyGpuShader::add3DTexture(
        const char * name, const char * id, unsigned dimension, 
        Interpolation interpolation, float * values)
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

    void LegacyGpuShader::get3DTexture(
        unsigned index, const char *& name, const char *& id, unsigned & dimension) const
    {
        getImpl()->get3DTexture(index, name, id, dimension);
    }

    void LegacyGpuShader::get3DTextureValues(unsigned index, const float *& value) const
    {
        getImpl()->get3DTextureValues(index, value);
    }

    const char * LegacyGpuShader::getShaderText() const
    {
        return getImpl()->shaderCode.c_str();
    }

    const char * LegacyGpuShader::getCacheID() const
    {
        return getImpl()->shaderCodeID.c_str();
    }

    GpuShaderRcPtr LegacyGpuShader::clone() const
    {
        return Create(getImpl()->getDesc(), getImpl()->getEdgelen());
    }

    void LegacyGpuShader::addToDeclareShaderCode(const char * shaderCode)
    {
        if(getImpl()->declarations.empty())
        {
            getImpl()->declarations += "\n// Declaration of all variables\n\n";
        }
        getImpl()->declarations += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void LegacyGpuShader::addToHelperShaderCode(const char * shaderCode)
    {
        getImpl()->helpers += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void LegacyGpuShader::addToMainShaderCode(const char * shaderCode)
    {
        getImpl()->mainBody += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void LegacyGpuShader::addToMainHeaderShaderCode(const char * shaderCode)
    {
        getImpl()->mainHeader += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void LegacyGpuShader::addToMainFooterShaderCode(const char * shaderCode)
    {
        getImpl()->mainFooter += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void LegacyGpuShader::createShaderText(
        const char * shaderDeclarations, const char * shaderHelperMethods,
        const char * shaderMainHeader, const char * shaderMainBody,
        const char * shaderMainFooter)
    {
        getImpl()->createShaderText(
            shaderDeclarations, shaderHelperMethods, shaderMainHeader, 
            shaderMainBody, shaderMainFooter);
    }

    void LegacyGpuShader::finalize()
    {
        getImpl()->finalize();
    }

    void LegacyGpuShader::Deleter(LegacyGpuShader* c)
    {
        delete c;
    }


    class GenericGpuShader::Impl : public PrivateImpl
    {
    public:       
        Impl(const GpuShaderDesc & desc) : PrivateImpl(desc) {}
        ~Impl() {}

    private:
        Impl(const Impl & rhs);
        Impl& operator= (const Impl & rhs);
    };

    GpuShaderRcPtr GenericGpuShader::Create(const GpuShaderDesc & desc)
    {
        return GpuShaderRcPtr(new GenericGpuShader(desc), &GenericGpuShader::Deleter);
    }

    GenericGpuShader::GenericGpuShader(const GpuShaderDesc & desc)
        :   GpuShader()
        ,   m_impl(new Impl(desc))
    {
    }

    GenericGpuShader::~GenericGpuShader()
    {
        delete m_impl;
        m_impl = 0x0;
    }

    GpuLanguage GenericGpuShader::getLanguage() const
    {
        return getImpl()->getDesc().getLanguage();
    }

    const char * GenericGpuShader::getFunctionName() const
    {
        return getImpl()->getDesc().getFunctionName();
    }

    const char * GenericGpuShader::getPixelName() const
    {
        return getImpl()->getDesc().getPixelName();
    }

    const char * GenericGpuShader::getNamePrefix() const
    {
        return getImpl()->getDesc().getNamePrefix();
    }

    unsigned GenericGpuShader::getNumUniforms() const
    {
        // Outside the scope of this code review. Will add in future step.
        return 0;
    }

    void GenericGpuShader::getUniform(
        unsigned, const char *&, UniformType &, void *&) const
    {
        // Outside the scope of this code review. Will add in future step.
        throw Exception("Not yet implemented");
    }

    void GenericGpuShader::addUniform(
        unsigned, const char *, UniformType, void *)
    {
        // Outside the scope of this code review. Will add in future step.
        throw Exception("Not yet implemented");
    }

    unsigned GenericGpuShader::getNumTextures() const
    {
        return unsigned(getImpl()->textures.size());
    }

    void GenericGpuShader::addTexture(
        const char * name, const char * id, unsigned width, unsigned height,
        TextureType channel, Interpolation interpolation,
        float * red, float * green, float * blue)
    {
        getImpl()->addTexture(name, id, width, height, channel, interpolation, red, green, blue);
    }

    void GenericGpuShader::getTexture(
        unsigned index, const char *& name, const char *& id, unsigned & width, unsigned & height,
        TextureType & channel, Interpolation & interpolation) const
    {
        getImpl()->getTexture(index, name, id, width, height, channel, interpolation);
    }

    void GenericGpuShader::getTextureValues(
        unsigned index, const float *& red, const float *& green, const float *& blue) const
    {
        getImpl()->getTextureValues(index, red, green, blue);
    }

    unsigned GenericGpuShader::getNum3DTextures() const
    {
        return unsigned(getImpl()->textures3D.size());
    }

    void GenericGpuShader::add3DTexture(
        const char * name, const char * id, unsigned edgelen, 
        Interpolation interpolation, float * values)
    {
        getImpl()->add3DTexture(name, id, edgelen, interpolation, values);
    }

    void GenericGpuShader::get3DTexture(
        unsigned index, const char *& name, const char *& id, unsigned & edgelen) const
    {
        getImpl()->get3DTexture(index, name, id, edgelen);
    }

    void GenericGpuShader::get3DTextureValues(unsigned index, const float *& value) const
    {
        getImpl()->get3DTextureValues(index, value);
    }

    const char * GenericGpuShader::getShaderText() const
    {
        return getImpl()->shaderCode.c_str();
    }

    const char * GenericGpuShader::getCacheID() const
    {
        return getImpl()->shaderCodeID.c_str();
    }

    GpuShaderRcPtr GenericGpuShader::clone() const
    {
        return Create(getImpl()->getDesc());
    }

    void GenericGpuShader::addToDeclareShaderCode(const char * shaderCode)
    {
        if(getImpl()->declarations.empty())
        {
            getImpl()->declarations += "\n// Declaration of all variables\n\n";
        }
        getImpl()->declarations += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void GenericGpuShader::addToHelperShaderCode(const char * shaderCode)
    {
        getImpl()->helpers += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void GenericGpuShader::addToMainShaderCode(const char * shaderCode)
    {
        getImpl()->mainBody += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void GenericGpuShader::addToMainHeaderShaderCode(const char * shaderCode)
    {
        getImpl()->mainHeader += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void GenericGpuShader::addToMainFooterShaderCode(const char * shaderCode)
    {
        getImpl()->mainFooter += (shaderCode && *shaderCode) ? shaderCode : "";
    }

    void GenericGpuShader::createShaderText(
        const char * shaderDeclarations, const char * shaderHelperMethods,
        const char * shaderMainHeader, const char * shaderMainBody,
        const char * shaderMainFooter)
    {
        getImpl()->createShaderText(
            shaderDeclarations, shaderHelperMethods, shaderMainHeader, 
            shaderMainBody, shaderMainFooter);
    }

    void GenericGpuShader::finalize()
    {
        getImpl()->finalize();
    }

    void GenericGpuShader::Deleter(GenericGpuShader* c)
    {
        delete c;
    }
}
OCIO_NAMESPACE_EXIT