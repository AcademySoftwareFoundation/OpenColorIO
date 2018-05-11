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

#ifndef INCLUDED_OCIO_GPU_SHADER_H
#define INCLUDED_OCIO_GPU_SHADER_H


#include <OpenColorIO/OpenColorIO.h>


OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////

    // LegacyGpuShaderDesc
    // *************
    // 
    // The class holds all the information to build a 'baked' shader program
    // (i.e. which contains at most one 3D Texture).
    // 

    class LegacyGpuShaderDesc : public GpuShaderDesc
    {
    public:
        static GpuShaderDescRcPtr Create(unsigned edgelen);

        unsigned getEdgelen() const;

        // Accessors to the 3D textures built from 3D Luts
        //
        unsigned getNum3DTextures() const;
        void add3DTexture(const char * name, const char * id, unsigned edgelen, 
                          Interpolation interpolation, float * values);
        void get3DTexture(unsigned index, const char *& name, 
                          const char *& id, unsigned & edgelen, 
                          Interpolation & interpolation) const;
        void get3DTextureValues(unsigned index, const float *& value) const;

        // Get the complete shader text
        const char * getShaderText() const;

        // Get the corresponding cache identifier
        virtual const char * getCacheID() const;

        virtual void finalize();

        // Methods to specialize each part of a complete shader program
        //
        void addToDeclareShaderCode(const char * shaderCode);
        void addToHelperShaderCode(const char * shaderCode);
        void addToFunctionHeaderShaderCode(const char * shaderCode);
        void addToFunctionShaderCode(const char * shaderCode);
        void addToFunctionFooterShaderCode(const char * shaderCode);

        // Method called to build the complete shader program
        void createShaderText(const char * shaderDeclarations,
                              const char * shaderHelperMethods,
                              const char * shaderFunctionHeader,
                              const char * shaderFunctionBody,
                              const char * shaderFunctionFooter);

    protected:

        unsigned getTextureMaxWidth() const;
        void setTextureMaxWidth(unsigned maxWidth);

        // Uniforms are not used by the legacy shader builder
        //
        unsigned getNumUniforms() const;
        void getUniform(unsigned index, const char *& name, UniformType & type, void *& value) const;
        void addUniform(unsigned index, const char * name, UniformType type, void * value);

        // 1D & 2D textures are not used by the legacy shader builder
        //
        unsigned getNumTextures() const;
        void addTexture(const char * name, const char * id, unsigned width, unsigned height,
                        TextureType channel, Interpolation interpolation, float * values);
        // Get the texture 1D or 2D information
        void getTexture(unsigned index, const char *& name, const char *& id, 
                        unsigned & width, unsigned & height,
                        TextureType & channel, Interpolation & interpolation) const;
        // Get the texture 1D or 2D values only
        void getTextureValues(unsigned index, const float *& values) const;

    private:
        LegacyGpuShaderDesc();
        LegacyGpuShaderDesc(unsigned edgelen);
        virtual ~LegacyGpuShaderDesc();
        
        LegacyGpuShaderDesc(const LegacyGpuShaderDesc &);
        LegacyGpuShaderDesc& operator= (const LegacyGpuShaderDesc &);
        
        static void Deleter(LegacyGpuShaderDesc* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };


    ///////////////////////////////////////////////////////////////////////////

    // GenericGpuShaderDesc
    // *************
    // 
    // The class holds all the information to build a shader program without baking
    // the color transform. It mainly means that the processor could contain 
    // several 1D or 3D luts.
    // 

    class GenericGpuShaderDesc : public GpuShaderDesc
    {
    public:
        static GpuShaderDescRcPtr Create();

        unsigned getTextureMaxWidth() const;
        void setTextureMaxWidth(unsigned maxWidth);

        // Accessors to the uniforms
        //
        unsigned getNumUniforms() const;
        void getUniform(
            unsigned index, const char *& name, UniformType & type, void *& value) const;
        void addUniform(
            unsigned index, const char * name, UniformType type, void * value);

        // Accessors to the 1D & 2D textures built from 1D Luts
        //
        unsigned getNumTextures() const;
        void addTexture(const char * name, const char * id, unsigned width, unsigned height,
                        TextureType channel, Interpolation interpolation, float * values);
        void getTexture(unsigned index, const char *& name, const char *& id, 
                        unsigned & width, unsigned & height,
                        TextureType & channel, Interpolation & interpolation) const;
        void getTextureValues(unsigned index, const float *& values) const;

        // Accessors to the 3D textures built from 3D Luts
        //
        unsigned getNum3DTextures() const;
        void add3DTexture(const char * name, const char * id, unsigned edgelen, 
                          Interpolation interpolation, float * values);
        void get3DTexture(unsigned index, const char *& name, 
                          const char *& id, unsigned & edgelen, 
                          Interpolation & interpolation) const;
        void get3DTextureValues(unsigned index, const float *& value) const;

        // Get the complete shader text
        const char * getShaderText() const;

        // Get the corresponding cache identifier
        virtual const char * getCacheID() const;

        virtual void finalize();

        // Methods to specialize each part of a complete shader program
        //
        void addToDeclareShaderCode(const char * shaderCode);
        void addToHelperShaderCode(const char * shaderCode);
        void addToFunctionHeaderShaderCode(const char * shaderCode);
        void addToFunctionShaderCode(const char * shaderCode);
        void addToFunctionFooterShaderCode(const char * shaderCode);

        // Method called to build the complete shader program
        void createShaderText(const char * shaderDeclarations, 
                              const char * shaderHelperMethods,
                              const char * shaderMainHeader,
                              const char * shaderMainBody,
                              const char * shaderMainFooter);

    private:

        GenericGpuShaderDesc();
        virtual ~GenericGpuShaderDesc();
        
        GenericGpuShaderDesc(const GenericGpuShaderDesc &);
        GenericGpuShaderDesc& operator= (const GenericGpuShaderDesc &);
        
        static void Deleter(GenericGpuShaderDesc* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

}
OCIO_NAMESPACE_EXIT

#endif
