/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/


#ifndef INCLUDED_OCIO_GPU_SHADER_H
#define INCLUDED_OCIO_GPU_SHADER_H


#include <OpenColorIO/OpenColorIO.h>


OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////

    // LegacyGpuShader
    // *************
    // 
    // The class holds all the information to build a shader program but bake
    // the color transform (i.e. using the processor) to have at most one 3D Texture.
    // 

    class LegacyGpuShader : public GpuShader
    {
    public:
        static GpuShaderRcPtr Create(const GpuShaderDesc & desc, unsigned edgelen);

        GpuLanguage getLanguage() const;
        const char * getFunctionName() const;
        const char * getPixelName() const;
        const char * getNamePrefix() const;
        unsigned getEdgelen() const;

        // Accessors to the uniforms
        //
        unsigned getNumUniforms() const;
        void getUniform(unsigned index, const char *& name, UniformType & type, void *& value) const;
        void addUniform(unsigned index, const char * name, UniformType type, void * value);

        // Accessors to the 1D & 2D textures built from 1D Luts
        //
        unsigned getNumTextures() const;
        void addTexture(
            const char * name, const char * id, unsigned width, unsigned height,
            TextureType channel, Interpolation interpolation,
            float * red, float * green, float * blue);
        // Get the texture 1D or 2D information
        void getTexture(
            unsigned index, const char *& name, const char *& id, unsigned & width, unsigned & height,
            TextureType & channel, Interpolation & interpolation);
        // Get the texture 1D or 2D values only
        void getTextureValues(
            unsigned index, const float *& red, const float *& green, const float *& blue) const;

        // Accessors to the 3D textures built from 3D Luts
        //
        unsigned getNum3DTextures() const;
        void add3DTexture(
            const char * name, const char * id, unsigned edgelen, 
            Interpolation interpolation, float * values);
        void get3DTexture(
            unsigned index, const char *& name, const char *& id, unsigned & edgelen) const;
        void get3DTextureValues(unsigned index, const float *& value) const;

        // Get the complete shader text
        const char * getShaderText() const;

        // Get the corresponding cache identifier
        const char * getCacheID() const;

        void finalize();

        GpuShaderRcPtr clone() const;

        // Methods to specialize each part of a complete shader program
        //
        void addToDeclareShaderCode(const char * shaderCode);
        void addToHelperShaderCode(const char * shaderCode);
        void addToMainHeaderShaderCode(const char * shaderCode);
        void addToMainShaderCode(const char * shaderCode);
        void addToMainFooterShaderCode(const char * shaderCode);

        // Method called to build the complete shader program
        void createShaderText(
            const char * shaderDeclarations, const char * shaderHelperMethods,
            const char * shaderMainHeader, const char * shaderMainBody,
            const char * shaderMainFooter);

    private:
        LegacyGpuShader(const GpuShaderDesc & desc, unsigned edgelen);
        virtual ~LegacyGpuShader();
        
        LegacyGpuShader(const LegacyGpuShader &);
        LegacyGpuShader& operator= (const LegacyGpuShader &);
        
        static void Deleter(LegacyGpuShader* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };


    ///////////////////////////////////////////////////////////////////////////

    // GenericGpuShader
    // *************
    // 
    // The class holds all the information to build a shader program without baking
    // the color transform. It mainly means that the processor could contain several 1D or 3D luts
    // even after the optimization step.
    // 

    class GenericGpuShader : public GpuShader
    {
    public:
        static GpuShaderRcPtr Create(const GpuShaderDesc & desc);

        //
        GpuLanguage getLanguage() const;
        const char * getFunctionName() const;
        const char * getPixelName() const;
        const char * getNamePrefix() const;

        //
        unsigned getNumUniforms() const;
        void getUniform(
            unsigned index, const char *& name, UniformType & type, void *& value) const;
        void addUniform(
            unsigned index, const char * name, UniformType type, void * value);

        //
        unsigned getNumTextures() const;
        void addTexture(
            const char * name, const char * id, unsigned width, unsigned height,
            TextureType channel, Interpolation interpolation,
            float * red, float * green, float * blue);
        void getTexture(
            unsigned index, const char *& name, const char *& id, unsigned & width, unsigned & height,
            TextureType & channel, Interpolation & interpolation);
        void getTextureValues(
            unsigned index, const float *& red, const float *& green, const float *& blue) const;

        //
        unsigned getNum3DTextures() const;
        void add3DTexture(
            const char * name, const char * id, unsigned edgelen, 
            Interpolation interpolation, float * values);
        void get3DTexture(
            unsigned index, const char *& name, const char *& id, unsigned & edgelen) const;
        void get3DTextureValues(unsigned index, const float *& value) const;

        const char * getShaderText() const;

        const char * getCacheID() const;

        void finalize();

        GpuShaderRcPtr clone() const;

        //
        void addToDeclareShaderCode(const char * shaderCode);
        void addToHelperShaderCode(const char * shaderCode);
        void addToMainHeaderShaderCode(const char * shaderCode);
        void addToMainShaderCode(const char * shaderCode);
        void addToMainFooterShaderCode(const char * shaderCode);

        void createShaderText(
            const char * shaderDeclarations, const char * shaderHelperMethods,
            const char * shaderMainHeader, const char * shaderMainBody,
            const char * shaderMainFooter);

    private:
        GenericGpuShader(const GpuShaderDesc & desc);
        virtual ~GenericGpuShader();
        
        GenericGpuShader(const GenericGpuShader &);
        GenericGpuShader& operator= (const GenericGpuShader &);
        
        static void Deleter(GenericGpuShader* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

}
OCIO_NAMESPACE_EXIT

#endif
