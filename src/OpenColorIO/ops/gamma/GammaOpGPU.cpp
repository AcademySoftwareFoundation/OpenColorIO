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
void AddBasicFwdShader(GpuShaderCreatorRcPtr & shaderCreator,
                       ConstGammaOpDataRcPtr gamma,
                       GpuShaderText & ss)
{
    const double redGamma   = gamma->getRedParams()[0];
    const double grnGamma   = gamma->getGreenParams()[0];
    const double bluGamma   = gamma->getBlueParams()[0];
    const double alphaGamma = gamma->getAlphaParams()[0];

    const std::string pxl(shaderCreator->getPixelName());

    ss.declareFloat4( "gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << ss.float4Decl("res") << " = pow( max( "
                 << ss.float4Const(0.0f)
                 << ", " << pxl 
                 << " ), gamma );";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

void AddBasicRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                       ConstGammaOpDataRcPtr gamma,
                       GpuShaderText & ss)
{
    const double redGamma   = 1. / gamma->getRedParams()[0];
    const double grnGamma   = 1. / gamma->getGreenParams()[0];
    const double bluGamma   = 1. / gamma->getBlueParams()[0];
    const double alphaGamma = 1. / gamma->getAlphaParams()[0];

    const std::string pxl(shaderCreator->getPixelName());

    ss.declareFloat4( "gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << ss.float4Decl("res") << " = pow( max( " 
                 << ss.float4Const(0.0f)
                 << ", " << pxl 
                 << " ), gamma );";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

// Create shader for basic mirror gamma style
void AddBasicMirrorFwdShader(GpuShaderCreatorRcPtr & shaderCreator,
                             ConstGammaOpDataRcPtr gamma,
                             GpuShaderText & ss)
{
    const double redGamma = gamma->getRedParams()[0];
    const double grnGamma = gamma->getGreenParams()[0];
    const double bluGamma = gamma->getBlueParams()[0];
    const double alphaGamma = gamma->getAlphaParams()[0];

    const std::string pxl(shaderCreator->getPixelName());

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << ss.float4Decl("signcol") << " = " << ss.sign(pxl) << ";";
    ss.newLine() << ss.float4Decl("res")     << " = signcol * pow( abs( " << pxl << " ), gamma );";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

void AddBasicMirrorRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                             ConstGammaOpDataRcPtr gamma,
                             GpuShaderText & ss)
{
    const double redGamma = 1. / gamma->getRedParams()[0];
    const double grnGamma = 1. / gamma->getGreenParams()[0];
    const double bluGamma = 1. / gamma->getBlueParams()[0];
    const double alphaGamma = 1. / gamma->getAlphaParams()[0];

    const std::string pxl(shaderCreator->getPixelName());

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << ss.float4Decl("signcol") << " = " << ss.sign(pxl) << ";";
    ss.newLine() << ss.float4Decl("res")     << " = signcol * pow( abs( " << pxl << " ), gamma );";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

// Create shader for basic pass thru gamma style
void AddBasicPassThruFwdShader(GpuShaderCreatorRcPtr & shaderCreator,
                               ConstGammaOpDataRcPtr gamma,
                               GpuShaderText & ss)
{
    const double redGamma = gamma->getRedParams()[0];
    const double grnGamma = gamma->getGreenParams()[0];
    const double bluGamma = gamma->getBlueParams()[0];
    const double alphaGamma = gamma->getAlphaParams()[0];

    const std::string pxl(shaderCreator->getPixelName());

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);
    ss.declareFloat4("breakPnt", 0.f, 0.f, 0.f, 0.f);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan(pxl, "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("powSeg") << " = pow(max( "
                                            << ss.float4Const(0.0f)
                                            << ", " << pxl << " ), gamma);";

    ss.newLine() << ss.float4Decl("res") << " = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * " << pxl << ";";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

void AddBasicPassThruRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                               ConstGammaOpDataRcPtr gamma,
                               GpuShaderText & ss)
{
    const double redGamma = 1. / gamma->getRedParams()[0];
    const double grnGamma = 1. / gamma->getGreenParams()[0];
    const double bluGamma = 1. / gamma->getBlueParams()[0];
    const double alphaGamma = 1. / gamma->getAlphaParams()[0];

    const std::string pxl(shaderCreator->getPixelName());

    ss.declareFloat4("gamma", redGamma, grnGamma, bluGamma, alphaGamma);
    ss.declareFloat4("breakPnt", 0.f, 0.f, 0.f, 0.f);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan(pxl, "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("powSeg") << " = pow(max( "
                                            << ss.float4Const(0.0f)
                                            << ", " << pxl << " ), gamma);";

    ss.newLine() << ss.float4Decl("res") << " = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * " << pxl << ";";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

// Create shader for moncurveFwd style
void AddMoncurveFwdShader(GpuShaderCreatorRcPtr & shaderCreator,
                          ConstGammaOpDataRcPtr gamma,
                          GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsFwd(gamma->getRedParams(),   red);
    ComputeParamsFwd(gamma->getGreenParams(), green);
    ComputeParamsFwd(gamma->getBlueParams(),  blue);
    ComputeParamsFwd(gamma->getAlphaParams(), alpha);

    const std::string pxl(shaderCreator->getPixelName());

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.

    ss.declareFloat4( "breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4( "slope" , red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4( "scale" , red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4( "offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4( "gamma" , red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan(pxl, "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = " << pxl << " * slope;";

    ss.newLine() << ss.float4Decl("powSeg") << " = pow( max( "
                 << ss.float4Const(0.0f) << ", scale * " << pxl << " + offset), gamma);";

    ss.newLine() << ss.float4Decl("res") << " = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

// Create shader for moncurveRev style
void AddMoncurveRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                          ConstGammaOpDataRcPtr gamma,
                          GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsRev(gamma->getRedParams(),   red);
    ComputeParamsRev(gamma->getGreenParams(), green);
    ComputeParamsRev(gamma->getBlueParams(),  blue);
    ComputeParamsRev(gamma->getAlphaParams(), alpha);

    const std::string pxl(shaderCreator->getPixelName());

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.

    ss.declareFloat4( "breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4( "slope" , red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4( "scale" , red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4( "offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4( "gamma" , red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan(pxl, "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = " << pxl << " * slope;";
    ss.newLine() << ss.float4Decl("powSeg") << " = pow( max( "
                 << ss.float4Const(0.0f) << ", " << pxl << " ), gamma ) * scale - offset;";

    ss.newLine() << ss.float4Decl("res") << " = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

// Create shader for moncurveMirrorFwd style
void AddMoncurveMirrorFwdShader(GpuShaderCreatorRcPtr & shaderCreator,
                                ConstGammaOpDataRcPtr gamma,
                                GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsFwd(gamma->getRedParams(), red);
    ComputeParamsFwd(gamma->getGreenParams(), green);
    ComputeParamsFwd(gamma->getBlueParams(), blue);
    ComputeParamsFwd(gamma->getAlphaParams(), alpha);

    const std::string pxl(shaderCreator->getPixelName());

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.
    ss.declareFloat4("breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4("slope", red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4("scale", red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4("offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4("gamma", red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("signcol") << " = " << ss.sign(pxl) << ";";
    ss.newLine() << pxl << " = abs( " << pxl << " );";

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan(pxl, "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = " << pxl << " * slope;";

    // Max() not needed since offset cannot be negative.
    ss.newLine() << ss.float4Decl("powSeg") << " = pow( scale * " << pxl << " + offset, gamma);";

    ss.newLine() << ss.float4Decl("res") << " = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";

    ss.newLine() << "res = signcol * res;";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

// Create shader for moncurveMirrorRev style
void AddMoncurveMirrorRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                                ConstGammaOpDataRcPtr gamma,
                                GpuShaderText & ss)
{
    RendererParams red, green, blue, alpha;

    ComputeParamsRev(gamma->getRedParams(), red);
    ComputeParamsRev(gamma->getGreenParams(), green);
    ComputeParamsRev(gamma->getBlueParams(), blue);
    ComputeParamsRev(gamma->getAlphaParams(), alpha);

    const std::string pxl(shaderCreator->getPixelName());

    // Even if all components are the same, on OS X, a vec4 needs to be
    // declared.  This code will work in both cases.
    ss.declareFloat4("breakPnt", red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareFloat4("slope", red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareFloat4("scale", red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareFloat4("offset", red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareFloat4("gamma", red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.float4Decl("signcol") << " = " << ss.sign(pxl) << ";";
    ss.newLine() << pxl << " = abs( " << pxl << " );";

    ss.newLine() << ss.float4Decl("isAboveBreak") << " = "
                 << ss.float4GreaterThan(pxl, "breakPnt") << ";";

    ss.newLine() << ss.float4Decl("linSeg") << " = " << pxl << " * slope;";
    ss.newLine() << ss.float4Decl("powSeg") << " = pow( " << pxl << ", gamma ) * scale - offset;";

    ss.newLine() << ss.float4Decl("res") << " = isAboveBreak * powSeg + ( "
                 << ss.float4Const(1.0f) << " - isAboveBreak ) * linSeg;";

    ss.newLine() << "res = signcol * res;";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";
}

}  // Anon namespace

void GetGammaGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                              ConstGammaOpDataRcPtr & gammaData)
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add Gamma '"
                 << GammaOpData::ConvertStyleToString(gammaData->getStyle())
                 << "' processing";
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    switch (gammaData->getStyle())
    {
    case GammaOpData::MONCURVE_FWD:
    {
        AddMoncurveFwdShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::MONCURVE_REV:
    {
        AddMoncurveRevShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::MONCURVE_MIRROR_FWD:
    {
        AddMoncurveMirrorFwdShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::MONCURVE_MIRROR_REV:
    {
        AddMoncurveMirrorRevShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_FWD:
    {
        AddBasicFwdShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_REV:
    {
        AddBasicRevShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_MIRROR_FWD:
    {
        AddBasicMirrorFwdShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_MIRROR_REV:
    {
        AddBasicMirrorRevShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_PASS_THRU_FWD:
    {
        AddBasicPassThruFwdShader(shaderCreator, gammaData, ss);
        break;
    }
    case GammaOpData::BASIC_PASS_THRU_REV:
    {
        AddBasicPassThruRevShader(shaderCreator, gammaData, ss);
        break;
    }
    }

    ss.dedent();
    ss.newLine() << "}";
    ss.dedent();

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}


} // namespace OCIO_NAMESPACE
