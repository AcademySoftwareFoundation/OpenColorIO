// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/lut1d/Lut1DOpGPU.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{
void PadLutChannels(unsigned long width,
                    unsigned long height,
                    const std::vector<float> & channel,
                    std::vector<float> & chn)
{
    const unsigned long currWidth = (unsigned long)(channel.size() / 3);

    if (height>1)
    {
        // Fill the texture values.
        //
        // Make the last texel of a given row the same as the first texel
        // of its next row.  This will preserve the continuity along row breaks
        // as long as the lookup position used by the sampler is based on (width-1)
        // to account for the 1 texel padding at the end of each row.
        unsigned long leftover = currWidth;

        const unsigned long step = width - 1;
        for (unsigned long i = 0; i < (currWidth - step); i += step)
        {
            std::transform(&channel[3 * i],
                           &channel[3 * (i + step)],
                           std::back_inserter(chn),
                           [](float val) {return SanitizeFloat(val); });

            chn.push_back(SanitizeFloat(channel[3 * (i + step) + 0]));
            chn.push_back(SanitizeFloat(channel[3 * (i + step) + 1]));
            chn.push_back(SanitizeFloat(channel[3 * (i + step) + 2]));
            leftover -= step;
        }

        // If there are still texels to fill, add them to the texture data.
        if (leftover > 0)
        {
            std::transform(&channel[3 * (currWidth - leftover)],
                           &channel[3 * (currWidth - 1)],
                           std::back_inserter(chn),
                           [](float val) {return SanitizeFloat(val); });

            chn.push_back(SanitizeFloat(channel[3 * (currWidth - 1) + 0]));
            chn.push_back(SanitizeFloat(channel[3 * (currWidth - 1) + 1]));
            chn.push_back(SanitizeFloat(channel[3 * (currWidth - 1) + 2]));
        }
    }
    else
    {
        for (auto & val : channel)
        {
            chn.push_back(SanitizeFloat(val));
        }
    }

    // Pad the remaining of the texture with the last LUT entry.
    // Note: GPU Textures are expected a size of width*height.

    unsigned long missingEntries = width*height - ((unsigned long)chn.size() / 3);
    for (unsigned long idx = 0; idx < missingEntries; ++idx)
    {
        chn.push_back(SanitizeFloat(channel[3 * (currWidth - 1) + 0]));
        chn.push_back(SanitizeFloat(channel[3 * (currWidth - 1) + 1]));
        chn.push_back(SanitizeFloat(channel[3 * (currWidth - 1) + 2]));
    }
}
}

void GetLut1DGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                              ConstLut1DOpDataRcPtr & lutData)
{
    const unsigned long defaultMaxWidth = shaderCreator->getTextureMaxWidth();

    const unsigned long length = lutData->getArray().getLength();
    const unsigned long width = std::min(length, defaultMaxWidth);
    const unsigned long height = (length / defaultMaxWidth) + 1;

    // Adjust LUT texture to allow for correct 2d linear interpolation, if needed.

    std::vector<float> values;
    values.reserve(width*height*3);

    PadLutChannels(width, height, lutData->getArray().getValues(), values);

    // Register the RGB LUT.

    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("lut1d_")
            << shaderCreator->getNextResourceIndex();

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    // (Using CacheID here to potentially allow reuse of existing textures.)
    shaderCreator->addTexture(name.c_str(),
                              GpuShaderText::getSamplerName(name).c_str(),
                              lutData->getCacheID().c_str(),
                              width, height,
                              GpuShaderCreator::TEXTURE_RGB_CHANNEL,
                              lutData->getConcreteInterpolation(),
                              &values[0]);

    // Add the LUT code to the OCIO shader program.

    if (height > 1 || lutData->isInputHalfDomain())
    {
        // In case the 1D LUT length exceeds the 1D texture maximum length
        // a 2D texture is used.

        {
            GpuShaderText ss(shaderCreator->getLanguage());
            ss.declareTex2D(name);
            shaderCreator->addToDeclareShaderCode(ss.string().c_str());
        }

        {
            GpuShaderText ss(shaderCreator->getLanguage());

            ss.newLine() << ss.vec2fKeyword() << " " << name << "_computePos(float f)";
            ss.newLine() << "{";
            ss.indent();

            if (lutData->isInputHalfDomain())
            {
                static const float NEG_MIN_EXP = 15.0f;
                static const float EXP_SCALE = 1024.0f;
                static const float HALF_DENRM_MAX = 6.09755515e-05f;  // e.g. 2^-14 - 2^-24

                ss.newLine() << "float dep;";
                ss.newLine() << "float abs_f = abs(f);";
                ss.newLine() << "if (abs_f > " << (float)HALF_NRM_MIN << ")";
                ss.newLine() << "{";
                ss.indent();
                ss.declareVec3f("fComp", NEG_MIN_EXP, NEG_MIN_EXP, NEG_MIN_EXP);
                ss.newLine() << "float absarr = min( abs_f, " << (float)HALF_MAX << ");";
                // Compute the exponent, scaled [-14,15].
                ss.newLine() << "fComp.x = floor( log2( absarr ) );";
                // Lower is the greatest power of 2 <= f.
                ss.newLine() << "float lower = pow( 2.0, fComp.x );";
                // Compute the mantissa (scaled [0-1]).
                ss.newLine() << "fComp.y = ( absarr - lower ) / lower;";
                // The dot product recombines the parts into a raw half without the sign component:
                //   dep = [ exponent + mantissa + NEG_MIN_EXP] * scale
                ss.declareVec3f("scale", EXP_SCALE, EXP_SCALE, EXP_SCALE);
                ss.newLine() << "dep = dot( fComp, scale );";
                ss.dedent();
                ss.newLine() << "}";
                ss.newLine() << "else";
                ss.newLine() << "{";
                ss.indent();
                // Extract bits from denormalized values
                ss.newLine() << "dep = abs_f * 1023.0 / " << HALF_DENRM_MAX << ";";
                ss.dedent();
                ss.newLine() << "}";

                // Adjust position for negative values
                ss.newLine() << "dep += step(f, 0.0) * 32768.0;";

                // At this point 'dep' contains the raw half
                // Note: Raw halfs for NaN floats cannot be computed using
                //       floating-point operations.
                ss.newLine() << ss.vec2fDecl("retVal") << ";";
                ss.newLine() << "retVal.y = floor(dep / " << float(width - 1) << ");";       // floor( dep / (width-1) ))
                ss.newLine() << "retVal.x = dep - retVal.y * " << float(width - 1) << ";";   // dep - retVal.y * (width-1)

                ss.newLine() << "retVal.x = (retVal.x + 0.5) / " << float(width) << ";";   // (retVal.x + 0.5) / width;
                ss.newLine() << "retVal.y = (retVal.y + 0.5) / " << float(height) << ";";  // (retVal.x + 0.5) / height;
            }
            else
            {
                // Need min() to protect against f > 1 causing a bogus x value.
                // min( f, 1.) * (dim - 1)
                ss.newLine() << "float dep = min(f, 1.0) * " << float(length - 1) << ";";

                ss.newLine() << ss.vec2fDecl("retVal") << ";";
                // float(int( dep / (width-1) ))
                ss.newLine() << "retVal.y = float(int(dep / " << float(width - 1) << "));";
                // dep - retVal.y * (width-1)
                ss.newLine() << "retVal.x = dep - retVal.y * " << float(width - 1) << ";";

                // (retVal.x + 0.5) / width;
                ss.newLine() << "retVal.x = (retVal.x + 0.5) / " << float(width) << ";";
                // (retVal.x + 0.5) / height;
                ss.newLine() << "retVal.y = (retVal.y + 0.5) / " << float(height) << ";";
            }

            ss.newLine() << "return retVal;";
            ss.dedent();
            ss.newLine() << "}";

            shaderCreator->addToHelperShaderCode(ss.string().c_str());
        }
    }
    else
    {
        GpuShaderText ss(shaderCreator->getLanguage());
        ss.declareTex1D(name);
        shaderCreator->addToDeclareShaderCode(ss.string().c_str());
    }

    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add a LUT 1D processing for " << name;
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    if (lutData->getHueAdjust() == HUE_DW3)
    {
        ss.newLine() << "// Add the pre hue adjustment";
        ss.newLine() << ss.vec3fDecl("maxval")
                        << " = max(" << shaderCreator->getPixelName() << ".rgb, max("
                        << shaderCreator->getPixelName() << ".gbr, " << shaderCreator->getPixelName() << ".brg));";
        ss.newLine() << ss.vec3fDecl("minval")
                        << " = min(" << shaderCreator->getPixelName() << ".rgb, min("
                        << shaderCreator->getPixelName() << ".gbr, " << shaderCreator->getPixelName() << ".brg));";
        ss.newLine() << "float oldChroma = max(1e-8, maxval.r - minval.r);";
        ss.newLine() << ss.vec3fDecl("delta") << " = " << shaderCreator->getPixelName() << ".rgb - minval;";
        ss.newLine() << "";
    }

    if (height > 1 || lutData->isInputHalfDomain())
    {
        const std::string str = name + "_computePos(" + shaderCreator->getPixelName();

        ss.newLine() << shaderCreator->getPixelName() << ".r = " << ss.sampleTex2D(name, str + ".r)") << ".r;";
        ss.newLine() << shaderCreator->getPixelName() << ".g = " << ss.sampleTex2D(name, str + ".g)") << ".g;";
        ss.newLine() << shaderCreator->getPixelName() << ".b = " << ss.sampleTex2D(name, str + ".b)") << ".b;";
    }
    else
    {
        const float dim = (float)lutData->getArray().getLength();

        ss.newLine() << ss.vec3fDecl(name + "_coords")
                        << " = (" << shaderCreator->getPixelName() << ".rgb * "
                        << ss.vec3fConst(dim - 1)
                        << " + " << ss.vec3fConst(0.5f) << " ) / "
                        << ss.vec3fConst(dim) << ";";

        ss.newLine() << shaderCreator->getPixelName() << ".r = "
                        << ss.sampleTex1D(name, name + "_coords.r") << ".r;";
        ss.newLine() << shaderCreator->getPixelName() << ".g = "
                        << ss.sampleTex1D(name, name + "_coords.g") << ".g;";
        ss.newLine() << shaderCreator->getPixelName() << ".b = "
                        << ss.sampleTex1D(name, name + "_coords.b") << ".b;";
    }

    if (lutData->getHueAdjust() == HUE_DW3)
    {
        ss.newLine() << "";
        ss.newLine() << "// Add the post hue adjustment";
        ss.newLine() << ss.vec3fDecl("maxval2") << " = max(" << shaderCreator->getPixelName()
                        << ".rgb, max(" << shaderCreator->getPixelName() << ".gbr, "
                        << shaderCreator->getPixelName() << ".brg));";
        ss.newLine() << ss.vec3fDecl("minval2") << " = min(" << shaderCreator->getPixelName()
                        << ".rgb, min(" << shaderCreator->getPixelName() << ".gbr, "
                        << shaderCreator->getPixelName() << ".brg));";
        ss.newLine() << "float newChroma = maxval2.r - minval2.r;";
        ss.newLine() << shaderCreator->getPixelName()
                        << ".rgb = minval2.r + delta * newChroma / oldChroma;";
    }

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());

}

} // namespace OCIO_NAMESPACE

