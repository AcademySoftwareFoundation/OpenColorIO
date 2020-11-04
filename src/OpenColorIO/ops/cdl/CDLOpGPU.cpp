// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ops/cdl/CDLOpCPU.h"
#include "ops/cdl/CDLOpGPU.h"


namespace OCIO_NAMESPACE
{

void GetCDLGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstCDLOpDataRcPtr & cdl)
{
    RenderParams params;
    params.update(cdl);

    const float * slope    = params.getSlope();
    const float * offset   = params.getOffset();
    const float * power    = params.getPower();
    const float saturation = params.getSaturation();

    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add CDL '" << CDLOpData::GetStyleName(cdl->getStyle()) << "' processing";
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    // Since alpha is not affected, only need to use the RGB components
    ss.declareFloat3("lumaWeights", 0.2126f,   0.7152f,   0.0722f  );
    ss.declareFloat3("slope",       slope [0], slope [1], slope [2]);
    ss.declareFloat3("offset",      offset[0], offset[1], offset[2]);
    ss.declareFloat3("power",       power [0], power [1], power [2]);

    ss.declareVar("saturation" , saturation);

    ss.newLine() << ss.float3Decl("pix") << " = "
                 << shaderCreator->getPixelName() << ".xyz;";

    if ( !params.isReverse() )
    {
        // Forward style

        // Slope
        ss.newLine() << "pix = pix * slope;";

        // Offset
        ss.newLine() << "pix = pix + offset;";

        // Power
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
            ss.newLine() << "pix = pow(pix, power);";
        }
        else
        {
            ss.newLine() << ss.float3Decl("posPix") << " = step(0.0, pix);";
            ss.newLine() << ss.float3Decl("pixPower") << " = pow(abs(pix), power);";
            ss.newLine() << "pix = " << ss.lerp("pix", "pixPower", "posPix") << ";";
        }

        // Saturation
        ss.newLine() << "float luma = dot(pix, lumaWeights);";
        ss.newLine() << "pix = luma + saturation * (pix - luma);";

        // Post-saturation clamp
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
        }
    }
    else
    {
        // Reverse style

        // Pre-saturation clamp
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
        }

        // Saturation
        ss.newLine() << "float luma = dot(pix, lumaWeights);";
        ss.newLine() << "pix = luma + saturation * (pix - luma);";

        // Power
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
            ss.newLine() << "pix = pow(pix, power);";
        }
        else
        {
            ss.newLine() << ss.float3Decl("posPix") << " = step(0.0, pix);";
            ss.newLine() << ss.float3Decl("pixPower") << " = pow(abs(pix), power);";
            ss.newLine() << "pix = " << ss.lerp("pix", "pixPower", "posPix") << ";";
        }

        // Offset
        ss.newLine() << "pix = pix + offset;";

        // Slope
        ss.newLine() << "pix = pix * slope;";

        // Post-slope clamp
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
        }
    }

    ss.newLine() << shaderCreator->getPixelName() << ".xyz = pix;";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

} // namespace OCIO_NAMESPACE
