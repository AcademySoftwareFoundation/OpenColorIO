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

    ss.declareVec4f( "gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << "outColor = pow( max( "
                 << ss.vec4fConst(0.0f)
                 << ", outColor ), gamma );";
}

void AddBasicRevShader(ConstGammaOpDataRcPtr gamma, GpuShaderText & ss)
{
    const double redGamma   = 1. / gamma->getRedParams()[0];
    const double grnGamma   = 1. / gamma->getGreenParams()[0];
    const double bluGamma   = 1. / gamma->getBlueParams()[0];
    const double alphaGamma = 1. / gamma->getAlphaParams()[0];

    ss.declareVec4f( "gamma", redGamma, grnGamma, bluGamma, alphaGamma);

    ss.newLine() << "outColor = pow( max( "
                 << ss.vec4fConst(0.0f)
                 << ", outColor ), gamma );";
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
    ss.declareVec4f( "breakPnt",
                     red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareVec4f( "slope" ,
                     red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareVec4f( "scale" ,
                     red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareVec4f( "offset",
                     red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareVec4f( "gamma" ,
                     red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.vec4fDecl("isAboveBreak") << " = "
                 << ss.vec4fGreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.vec4fDecl("linSeg") << " = outColor * slope;";

    ss.newLine() << ss.vec4fDecl("powSeg") << " = pow( max( "
                 << ss.vec4fConst(0.0f) << ", scale * outColor + offset), gamma);";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.vec4fConst(1.0f) << " - isAboveBreak ) * linSeg;";
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
    ss.declareVec4f( "breakPnt",
                     red.breakPnt, green.breakPnt, blue.breakPnt, alpha.breakPnt);
    ss.declareVec4f( "slope" ,
                     red.slope, green.slope, blue.slope, alpha.slope);
    ss.declareVec4f( "scale" ,
                     red.scale, green.scale, blue.scale, alpha.scale);
    ss.declareVec4f( "offset",
                     red.offset, green.offset, blue.offset, alpha.offset);
    ss.declareVec4f( "gamma" ,
                     red.gamma, green.gamma, blue.gamma, alpha.gamma);

    ss.newLine() << ss.vec4fDecl("isAboveBreak") << " = "
                 << ss.vec4fGreaterThan("outColor", "breakPnt") << ";";

    ss.newLine() << ss.vec4fDecl("linSeg") << " = outColor * slope;";
    ss.newLine() << ss.vec4fDecl("powSeg") << " = pow( max( "
                 << ss.vec4fConst(0.0f) << ", outColor ), gamma ) * scale - offset;";

    ss.newLine() << "outColor = isAboveBreak * powSeg + ( "
                 << ss.vec4fConst(1.0f) << " - isAboveBreak ) * linSeg;";
}

}

void GetGammaGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstGammaOpDataRcPtr & gamma)
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add Gamma '"
                 << GammaOpData::ConvertStyleToString(gamma->getStyle())
                 << "' processing";
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    switch(gamma->getStyle())
    {
        case GammaOpData::MONCURVE_FWD:
        {
            AddMoncurveFwdShader(gamma, ss);
            break;
        }
        case GammaOpData::MONCURVE_REV:
        {
            AddMoncurveRevShader(gamma, ss);
            break;
        }
        case GammaOpData::BASIC_FWD:
        {
            AddBasicFwdShader(gamma, ss);
            break;
        }
        case GammaOpData::BASIC_REV:
        {
            AddBasicRevShader(gamma, ss);
            break;
        }
    }

    ss.dedent();
    ss.newLine() << "}";
    ss.dedent();

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

} // namespace OCIO_NAMESPACE
