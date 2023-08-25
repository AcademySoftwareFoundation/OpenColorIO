// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GPU_SHADER_H
#define INCLUDED_OCIO_GPU_SHADER_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

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

    bool getAllowTexture1D() const noexcept override;
    void setAllowTexture1D(bool allowed) override;

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
                    const char * samplerName,
                    unsigned width, unsigned height,
                    TextureType channel,
                    TextureDimensions dimensions,
                    Interpolation interpolation,
                    const float * values) override;
    void getTexture(unsigned index,
                    const char *& textureName,
                    const char *& samplerName,
                    unsigned & width, unsigned & height,
                    TextureType & channel,
                    TextureDimensions & dimensions,
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

    class ImplGeneric;
    ImplGeneric * m_implGeneric;

    ImplGeneric * getImplGeneric() { return m_implGeneric; }
    const ImplGeneric * getImplGeneric() const { return m_implGeneric; }
};

} // namespace OCIO_NAMESPACE

#endif
