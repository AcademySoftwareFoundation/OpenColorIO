/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "GpuShader.h"

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
            OCIO_NAMESPACE::GpuShaderDesc::TextureType type, std::vector<float>& res)
        {
            if(buf)
            {
                const unsigned size 
                    = w * h * d * (type==OCIO_NAMESPACE::GpuShaderDesc::TEXTURE_RGB_CHANNEL ? 3 : 1);
                res.resize(size);
                memcpy(&res[0], buf, size * sizeof(float));
            }
        }

    public:
        struct Texture
        {
            Texture(const char * n, const char * identifier, 
                unsigned w, unsigned h, unsigned d,
                OCIO_NAMESPACE::GpuShaderDesc::TextureType channel, 
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
            OCIO_NAMESPACE::GpuShaderDesc::TextureType type;
            OCIO_NAMESPACE::Interpolation interp;

            std::vector<float> red;
            std::vector<float> green;
            std::vector<float> blue;
        };

        typedef std::vector<Texture> Textures;

    public:       
        PrivateImpl() {}
        virtual ~PrivateImpl() {}

        void addTexture(
            const char * name, const char * id, unsigned width, unsigned height, 
            OCIO_NAMESPACE::GpuShaderDesc::TextureType channel,
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
            OCIO_NAMESPACE::GpuShaderDesc::TextureType & channel, 
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
                OCIO_NAMESPACE::GpuShaderDesc::TEXTURE_RGB_CHANNEL, 
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
            const char * shaderFunctionHeader, const char * shaderFunctionBody,
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
            createShaderText(
                declarations.c_str(), helperMethods.c_str(), 
                functionHeader.c_str(), functionBody.c_str(), functionFooter.c_str());

            // Compute the identifier
            std::ostringstream ss;
            ss << shaderCode;
            for(unsigned idx=0; idx<textures3D.size(); ++idx)
            {
                ss << textures3D[idx].id;
            }
            for(unsigned idx=0; idx<textures.size(); ++idx)
            {
                ss << textures[idx].id;
            }

            const std::string id = ss.str();
            shaderCodeID = cacheID + OCIO_NAMESPACE::CacheIDHash(id.c_str(), unsigned(id.length()));
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

    private:
        PrivateImpl(const PrivateImpl & rhs);
        PrivateImpl& operator= (const PrivateImpl & rhs);
    };
};


OCIO_NAMESPACE_ENTER
{
    class LegacyGpuShaderDesc::Impl : public PrivateImpl
    {
    public:       
        Impl(unsigned edgelen)
            :   PrivateImpl()
            ,   _edgelen(edgelen)
        {
        }
        
        virtual ~Impl() {}

        inline unsigned getEdgelen() const { return _edgelen; }
    
    private:
        unsigned _edgelen;
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

    unsigned LegacyGpuShaderDesc::getNumTextures() const
    {
        return 0;
    }

    void LegacyGpuShaderDesc::addTexture(
        const char *, const char *, unsigned, unsigned,
        TextureType, Interpolation, float *, float *, float *)
    {
        throw Exception("1D luts are not supported");
    }

    void LegacyGpuShaderDesc::getTexture(
        unsigned, const char *&, const char *&, 
        unsigned &, unsigned &, TextureType &, Interpolation &) const
    {
        throw Exception("1D luts are not supported");
    }

    void LegacyGpuShaderDesc::getTextureValues(
        unsigned, const float *&, const float *&, const float *&) const
    {
        throw Exception("1D luts are not supported");
    }

    unsigned LegacyGpuShaderDesc::getNum3DTextures() const
    {
        return unsigned(getImpl()->textures3D.size());
    }

    void LegacyGpuShaderDesc::add3DTexture(
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

    void LegacyGpuShaderDesc::get3DTexture(
        unsigned index, const char *& name, const char *& id, unsigned & dimension) const
    {
        getImpl()->get3DTexture(index, name, id, dimension);
    }

    void LegacyGpuShaderDesc::get3DTextureValues(unsigned index, const float *& value) const
    {
        getImpl()->get3DTextureValues(index, value);
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

    void LegacyGpuShaderDesc::createShaderText(
        const char * shaderDeclarations, const char * shaderHelperMethods,
        const char * shaderFunctionHeader, const char * shaderFunctionBody,
        const char * shaderFunctionFooter)
    {
        getImpl()->createShaderText(
            shaderDeclarations, shaderHelperMethods, shaderFunctionHeader, 
            shaderFunctionBody, shaderFunctionFooter);
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

    unsigned GenericGpuShaderDesc::getNumTextures() const
    {
        return unsigned(getImpl()->textures.size());
    }

    void GenericGpuShaderDesc::addTexture(
        const char * name, const char * id, unsigned width, unsigned height,
        TextureType channel, Interpolation interpolation,
        float * red, float * green, float * blue)
    {
        getImpl()->addTexture(name, id, width, height, channel, interpolation, red, green, blue);
    }

    void GenericGpuShaderDesc::getTexture(
        unsigned index, const char *& name, const char *& id, unsigned & width, unsigned & height,
        TextureType & channel, Interpolation & interpolation) const
    {
        getImpl()->getTexture(index, name, id, width, height, channel, interpolation);
    }

    void GenericGpuShaderDesc::getTextureValues(
        unsigned index, const float *& red, const float *& green, const float *& blue) const
    {
        getImpl()->getTextureValues(index, red, green, blue);
    }

    unsigned GenericGpuShaderDesc::getNum3DTextures() const
    {
        return unsigned(getImpl()->textures3D.size());
    }

    void GenericGpuShaderDesc::add3DTexture(
        const char * name, const char * id, unsigned edgelen, 
        Interpolation interpolation, float * values)
    {
        getImpl()->add3DTexture(name, id, edgelen, interpolation, values);
    }

    void GenericGpuShaderDesc::get3DTexture(
        unsigned index, const char *& name, const char *& id, unsigned & edgelen) const
    {
        getImpl()->get3DTexture(index, name, id, edgelen);
    }

    void GenericGpuShaderDesc::get3DTextureValues(unsigned index, const float *& value) const
    {
        getImpl()->get3DTextureValues(index, value);
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
        getImpl()->createShaderText(
            shaderDeclarations, shaderHelperMethods, shaderFunctionHeader, 
            shaderFunctionBody, shaderFunctionFooter);
    }

    void GenericGpuShaderDesc::finalize()
    {
        getImpl()->finalize(GpuShaderDesc::getCacheID() );
    }

    void GenericGpuShaderDesc::Deleter(GenericGpuShaderDesc* c)
    {
        delete c;
    }
}
OCIO_NAMESPACE_EXIT