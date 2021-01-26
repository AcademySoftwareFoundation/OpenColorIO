// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GPU_SHADER_H
#define INCLUDED_OCIO_GPU_SHADER_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
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

    // Accessors to the 3D textures built from 3D LUT
    //
    unsigned getNum3DTextures() const noexcept override;
    void add3DTexture(const char * textureName,
                      const char * samplerName,
                      unsigned edgelen,
                      Interpolation interpolation,
                      const float * values) override;
    void get3DTexture(unsigned index,
                      const char *& textureName,
                      const char *& samplerName,
                      unsigned & edgelen,
                      Interpolation & interpolation) const override;
    void get3DTextureValues(unsigned index, const float *& value) const override;

protected:

    unsigned getTextureMaxWidth() const noexcept override;
    void setTextureMaxWidth(unsigned maxWidth) override;

    // Uniforms are not used by the legacy shader builder
    //
    unsigned getNumUniforms() const noexcept override;
    const char * getUniform(unsigned index, GpuShaderDesc::UniformData & data) const override;
    bool addUniform(const char * name, const DoubleGetter & getter) override;
    bool addUniform(const char * name, const BoolGetter & getter) override;
    bool addUniform(const char * name, const Float3Getter & getter) override;
    bool addUniform(const char * name,
                    const SizeGetter & getSize,
                    const VectorFloatGetter & getVector) override;
    bool addUniform(const char * name,
                    const SizeGetter & getSize,
                    const VectorIntGetter & getVector) override;

    // 1D & 2D textures are not used by the legacy shader builder
    //
    unsigned getNumTextures() const noexcept override;
    void addTexture(const char * textureName,
                    const char * samplerName,
                    unsigned width, unsigned height,
                    TextureType channel, Interpolation interpolation,
                    const float * values) override;
    // Get the texture 1D or 2D information
    void getTexture(unsigned index,
                    const char *& textureName,
                    const char *& samplerName,
                    unsigned & width, unsigned & height,
                    TextureType & channel,
                    Interpolation & interpolation) const override;
    // Get the texture 1D or 2D values only
    void getTextureValues(unsigned index, const float *& values) const override;

private:
    LegacyGpuShaderDesc();
    explicit LegacyGpuShaderDesc(unsigned edgelen);
    virtual ~LegacyGpuShaderDesc();

    LegacyGpuShaderDesc(const LegacyGpuShaderDesc &) = delete;
    LegacyGpuShaderDesc& operator= (const LegacyGpuShaderDesc &) = delete;

    static void Deleter(LegacyGpuShaderDesc* c);

    class Impl;
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
// several 1D or 3D LUTs.
//

class GenericGpuShaderDesc : public GpuShaderDesc
{
public:
    static GpuShaderDescRcPtr Create();

    unsigned getTextureMaxWidth() const noexcept override;
    void setTextureMaxWidth(unsigned maxWidth) override;

    // Accessors to the uniforms
    //
    unsigned getNumUniforms() const noexcept override;
    const char * getUniform(unsigned index, GpuShaderDesc::UniformData & data) const override;
    bool addUniform(const char * name, const DoubleGetter & getDouble) override;
    bool addUniform(const char * name, const BoolGetter & getBool) override;
    bool addUniform(const char * name, const Float3Getter & getter) override;
    bool addUniform(const char * name,
                    const SizeGetter & getSize,
                    const VectorFloatGetter & getVectorFloat) override;
    bool addUniform(const char * name,
                    const SizeGetter & getSize,
                    const VectorIntGetter & getVectorInt) override;

    // Accessors to the 1D & 2D textures built from 1D LUT
    //
    unsigned getNumTextures() const noexcept override;
    void addTexture(const char * textureName,
                    const char * samplerNName,
                    unsigned width, unsigned height,
                    TextureType channel,
                    Interpolation interpolation,
                    const float * values) override;
    void getTexture(unsigned index,
                    const char *& textureName,
                    const char *& samplerName,
                    unsigned & width, unsigned & height,
                    TextureType & channel,
                    Interpolation & interpolation) const override;
    void getTextureValues(unsigned index, const float *& values) const override;

    // Accessors to the 3D textures built from 3D LUT
    //
    unsigned getNum3DTextures() const noexcept override;
    void add3DTexture(const char * textureName,
                      const char * samplerName,
                      unsigned edgelen,
                      Interpolation interpolation,
                      const float * values) override;
    void get3DTexture(unsigned index,
                      const char *& textureName,
                      const char *& samplerName,
                      unsigned & edgelen,
                      Interpolation & interpolation) const override;
    void get3DTextureValues(unsigned index, const float *& value) const override;

private:

    GenericGpuShaderDesc();
    virtual ~GenericGpuShaderDesc();

    GenericGpuShaderDesc(const GenericGpuShaderDesc &) = delete;
    GenericGpuShaderDesc& operator= (const GenericGpuShaderDesc &) = delete;

    static void Deleter(GenericGpuShaderDesc* c);

    class Impl;
    Impl * m_impl;

    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

} // namespace OCIO_NAMESPACE

#endif
