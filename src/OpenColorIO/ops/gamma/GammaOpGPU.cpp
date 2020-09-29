// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ops/gamma/GammaOpGPU.h"
#include "ops/gamma/GammaOpUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

// Create shader for basic gamma style
void AddBasicFwdShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    const double redGamma   = gamma->getRedParams()[0];
    const double grnGamma   = gamma->getGreenParams()[0];
    const double bluGamma   = gamma->getBlueParams()[0];
    const double alphaGamma = gamma->getAlphaParams()[0];

    ss.declareFloat4( "gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << "outColor = pow( max( "
                 << ss.float4Const(0.0f)
                 << ", outColor ), gamma );";
}

void AddBasicRevShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    const double redGamma   = 1. / gamma->getRedParams()[0];
    const double grnGamma   = 1. / gamma->getGreenParams()[0];
    const double bluGamma   = 1. / gamma->getBlueParams()[0];
    const double alphaGamma = 1. / gamma->getAlphaParams()[0];

    ss.declareFloat4( "gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << "outColor = pow( max( " 
                 << ss.float4Const(0.0f)
                 << ", outColor ), gamma );";
}

// Create shader for basic mirror gamma style
void AddBasicMirrorFwdShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    const double redGamma = gamma->getRedParams()[0];
    const double grnGamma = gamma->getGreenParams()[0];
    const double bluGamma = gamma->getBlueParams()[0];
    const double alphaGamma = gamma->getAlphaParams()[0];

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << ss.float4Decl("signcol") << " = sign(outColor);";
    ss.newLine() << "outColor = signcol * pow( abs( outColor ), gamma );";
}

void AddBasicMirrorRevShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    const double redGamma = 1. / gamma->getRedParams()[0];
    const double grnGamma = 1. / gamma->getGreenParams()[0];
    const double bluGamma = 1. / gamma->getBlueParams()[0];
    const double alphaGamma = 1. / gamma->getAlphaParams()[0];

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << ss.float4Decl("signcol") << " = sign(outColor);";
    ss.newLine() << "outColor = signcol * pow( abs( outColor ), gamma );";
}

// Create shader for basic pass thru gamma style
void AddBasicPassThruFwdShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    const double redGamma = gamma->getRedParams()[0];
    const double grnGamma = gamma->getGreenParams()[0];
    const double bluGamma = gamma->getBlueParams()[0];
    const double alphaGamma = gamma->getAlphaParams()[0];

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);
    ss.declareFloat4("breakPnt", 0.f, 0.f, 0.f, 0.f);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("powSeg") << " = pow(max( "
                                            << ss.float4Const(0.0f)
                                            << ", outColor ), gamma);";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * outColor;";
}

void AddBasicPassThruRevShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    const double redGamma = 1. / gamma->getRedParams()[0];
    const double grnGamma = 1. / gamma->getGreenParams()[0];
    const double bluGamma = 1. / gamma->getBlueParams()[0];
    const double alphaGamma = 1. / gamma->getAlphaParams()[0];

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);
    ss.declareFloat4("breakPnt", 0.f, 0.f, 0.f, 0.f);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("powSeg") << " = pow(max( "
                                            << ss.float4Const(0.0f)
                                            << ", outColor ), gamma);";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * outColor;";
}

// Create shader for moncurveFwd style
void AddMoncurveFwdShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsFwd(gamma->getRedParams(),   red);
    ComputeParamsFwd(gamma->getGreenParams(), green);
    ComputeParamsFwd(gamma->getBlueParams(),  blue);
    ComputeParamsFwd(gamma->getAlphaParams(), alpha);

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.

    ss.declareFloat4( "breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4( "slope" , red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4( "scale" , red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4( "offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4( "gamma" , red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = outColor * slope;";

    ss.newLine() << ss.float4Decl("powSeg") << " = pow( max( "
                 << ss.float4Const(0.0f) << ", scale * outColor + offset), gamma);";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";
}

// Create shader for moncurveRev style
void AddMoncurveRevShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsRev(gamma->getRedParams(),   red);
    ComputeParamsRev(gamma->getGreenParams(), green);
    ComputeParamsRev(gamma->getBlueParams(),  blue);
    ComputeParamsRev(gamma->getAlphaParams(), alpha);

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.

    ss.declareFloat4( "breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4( "slope" , red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4( "scale" , red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4( "offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4( "gamma" , red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = outColor * slope;";
    ss.newLine() << ss.float4Decl("powSeg") << " = pow( max( "
                 << ss.float4Const(0.0f) << ", outColor ), gamma ) * scale - offset;";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";
}

// Create shader for moncurveMirrorFwd style
void AddMoncurveMirrorFwdShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsFwd(gamma->getRedParams(), red);
    ComputeParamsFwd(gamma->getGreenParams(), green);
    ComputeParamsFwd(gamma->getBlueParams(), blue);
    ComputeParamsFwd(gamma->getAlphaParams(), alpha);

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.
    ss.declareFloat4("breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4("slope", red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4("scale", red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4("offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4("gamma", red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("signcol") << " = sign( outColor );";

    ss.newLine() << "outColor = abs( outColor );";

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = outColor * slope;";

    // Max() not needed since offset cannot be negative.
    ss.newLine() << ss.float4Decl("powSeg") << " = pow( scale * outColor + offset, gamma);";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";

    ss.newLine() << "outColor = signcol * outColor;";
}

// Create shader for moncurveMirrorRev style
void AddMoncurveMirrorRevShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsRev(gamma->getRedParams(), red);
    ComputeParamsRev(gamma->getGreenParams(), green);
    ComputeParamsRev(gamma->getBlueParams(), blue);
    ComputeParamsRev(gamma->getAlphaParams(), alpha);

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.
    ss.declareFloat4("breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4("slope", red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4("scale", red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4("offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4("gamma", red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("signcol") << " = sign( outColor );";

    ss.newLine() << "outColor = abs( outColor );";

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = outColor * slope;";
    ss.newLine() << ss.float4Decl("powSeg") << " = pow( outColor, gamma ) * scale - offset;";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";

    ss.newLine() << "outColor = signcol * outColor;";
}

}  // Anon namespace

void GetGammaGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                              ConstGammaOpDataRcPtr & gammaData)
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add Gamma "
        << GammaOpData::ConvertStyleToString(gammaData->getStyle())
        << " processing";
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    switch (gammaData->getStyle())
    {
    case GammaOpData::MONCURVE_FWD:
    {
        AddMoncurveFwdShader(gammaData, ss);
        break;
    }
    case GammaOpData::MONCURVE_REV:
    {
        AddMoncurveRevShader(gammaData, ss);
        break;
    }
    case GammaOpData::MONCURVE_MIRROR_FWD:
    {
        AddMoncurveMirrorFwdShader(gammaData, ss);
        break;
    }
    case GammaOpData::MONCURVE_MIRROR_REV:
    {
        AddMoncurveMirrorRevShader(gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_FWD:
    {
        AddBasicFwdShader(gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_REV:
    {
        AddBasicRevShader(gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_MIRROR_FWD:
    {
        AddBasicMirrorFwdShader(gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_MIRROR_REV:
    {
        AddBasicMirrorRevShader(gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_PASS_THRU_FWD:
    {
        AddBasicPassThruFwdShader(gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_PASS_THRU_REV:
    {
        AddBasicPassThruRevShader(gammaData, ss);
        break;
    }
    }

    ss.dedent();
    ss.newLine() << "}";
    ss.dedent();

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}


} // namespace OCIO_NAMESPACE
