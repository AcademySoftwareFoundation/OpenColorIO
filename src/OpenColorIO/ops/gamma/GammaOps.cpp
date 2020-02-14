// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/gamma/GammaOpCPU.h"
#include "ops/gamma/GammaOp.h"
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


class GammaOp;
typedef OCIO_SHARED_PTR<GammaOp> GammaOpRcPtr;
typedef OCIO_SHARED_PTR<const GammaOp> ConstGammaOpRcPtr;

class GammaOp : public Op
{
public:
    GammaOp() = delete;
    explicit GammaOp(GammaOpDataRcPtr & gamma);
    GammaOp(const GammaOp &) = delete;

    GammaOp(GammaOpData::Style style,
            GammaOpData::Params red,
            GammaOpData::Params green,
            GammaOpData::Params blue,
            GammaOpData::Params alpha);

    virtual ~GammaOp();

    std::string getInfo() const override;

    OpRcPtr clone() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize(OptimizationFlags oFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderCreator) const override;

protected:
    ConstGammaOpDataRcPtr gammaData() const { return DynamicPtrCast<const GammaOpData>(data()); }
    GammaOpDataRcPtr gammaData() { return DynamicPtrCast<GammaOpData>(data()); }
};


GammaOp::GammaOp(GammaOpDataRcPtr & gamma)
    :   Op()
{
    data() = gamma;
}

GammaOp::GammaOp(GammaOpData::Style style,
                 GammaOpData::Params red,
                 GammaOpData::Params green,
                 GammaOpData::Params blue,
                 GammaOpData::Params alpha)
    :   Op()
{
    GammaOpDataRcPtr gamma = std::make_shared<GammaOpData>(style, red, green, blue, alpha);

    data() = gamma;
}

GammaOp::~GammaOp()
{
}

std::string GammaOp::getInfo() const
{
    return "<GammaOp>";
}

OpRcPtr GammaOp::clone() const
{
    GammaOpDataRcPtr f = gammaData()->clone();
    return std::make_shared<GammaOp>(f);
}

bool GammaOp::isSameType(ConstOpRcPtr & op) const
{
    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(op);
    return (bool)typedRcPtr;
}

bool GammaOp::isInverse(ConstOpRcPtr & op) const
{
    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(op);
    if(!typedRcPtr) return false;

    return gammaData()->isInverse(*typedRcPtr->gammaData());
}

bool GammaOp::canCombineWith(ConstOpRcPtr & op) const
{
    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(op);
    return typedRcPtr ? gammaData()->mayCompose(*typedRcPtr->gammaData()) : false;
}

void GammaOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("GammaOp: canCombineWith must be checked before calling combineWith.");
    }

    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(secondOp);

    GammaOpDataRcPtr res = gammaData()->compose(*typedRcPtr->gammaData());
    CreateGammaOp(ops, res, TRANSFORM_DIR_FORWARD);
}

void GammaOp::finalize(OptimizationFlags oFlags)
{
    gammaData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<GammaOp ";
    cacheIDStream << gammaData()->getCacheID() << " ";
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr GammaOp::getCPUOp() const
{
    ConstGammaOpDataRcPtr data = gammaData();
    return GetGammaRenderer(data);
}

void GammaOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderCreator) const
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add Gamma "
                 << GammaOpData::ConvertStyleToString(gammaData()->getStyle())
                 << " processing";
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    switch(gammaData()->getStyle())
    {
        case GammaOpData::MONCURVE_FWD:
        {
            AddMoncurveFwdShader(gammaData(), ss);
            break;
        }
        case GammaOpData::MONCURVE_REV:
        {
            AddMoncurveRevShader(gammaData(), ss);
            break;
        }
        case GammaOpData::BASIC_FWD:
        {
            AddBasicFwdShader(gammaData(), ss);
            break;
        }
        case GammaOpData::BASIC_REV:
        {
            AddBasicRevShader(gammaData(), ss);
            break;
        }
    }

    ss.dedent();
    ss.newLine() << "}";
    ss.dedent();

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

}  // Anon namespace

void CreateGammaOp(OpRcPtrVec & ops,
                   GammaOpDataRcPtr & gammaData,
                   TransformDirection direction)
{
    auto gamma = gammaData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        gamma = gamma->inverse();
    }

    ops.push_back(std::make_shared<GammaOp>(gamma));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void CreateGammaTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto gamma = DynamicPtrCast<const GammaOp>(op);
    if (!gamma)
    {
        throw Exception("CreateGammaTransform: op has to be a GammaOp");
    }
    auto gammaData = DynamicPtrCast<const GammaOpData>(op->data());

    const auto style = gammaData->getStyle();

    if (style == GammaOpData::MONCURVE_FWD || style == GammaOpData::MONCURVE_REV)
    {
        auto expTransform = ExponentWithLinearTransform::Create();
        if (style == GammaOpData::MONCURVE_REV)
        {
            expTransform->setDirection(TRANSFORM_DIR_INVERSE);
        }

        auto & formatMetadata = expTransform->getFormatMetadata();
        auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
        metadata = gammaData->getFormatMetadata();

        const double gammaVal[] = { gammaData->getRedParams()[0],
                                    gammaData->getGreenParams()[0],
                                    gammaData->getBlueParams()[0],
                                    gammaData->getAlphaParams()[0] };

        const double offset[] = { gammaData->getRedParams()[1],
                                  gammaData->getGreenParams()[1],
                                  gammaData->getBlueParams()[1],
                                  gammaData->getAlphaParams()[1] };

        expTransform->setGamma(gammaVal);
        expTransform->setOffset(offset);

        group->appendTransform(expTransform);
    }
    else
    {
        auto expTransform = ExponentTransform::Create();
        if (style == GammaOpData::BASIC_REV)
        {
            expTransform->setDirection(TRANSFORM_DIR_INVERSE);
        }

        auto & formatMetadata = expTransform->getFormatMetadata();
        auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
        metadata = gammaData->getFormatMetadata();

        const double expVal[] = { gammaData->getRedParams()[0],
                                  gammaData->getGreenParams()[0],
                                  gammaData->getBlueParams()[0],
                                  gammaData->getAlphaParams()[0] };


        expTransform->setValue(expVal);

        group->appendTransform(expTransform);
    }
}

void BuildExponentWithLinearOp(OpRcPtrVec & ops,
                               const Config & /*config*/,
                               const ExponentWithLinearTransform & transform,
                               TransformDirection dir)
{
    TransformDirection combinedDir
        = CombineTransformDirections(dir, transform.getDirection());

    double gammaVals[4] = { 1., 1., 1., 1. };
    transform.getGamma(gammaVals);

    double offsetVals[4] = { 0., 0., 0., 0. };
    transform.getOffset(offsetVals);

    const bool noOffset = offsetVals[0] == 0. && offsetVals[1] == 0. &&
                          offsetVals[2] == 0. && offsetVals[3] == 0.;

    const auto style = noOffset? GammaOpData::BASIC_FWD : GammaOpData::MONCURVE_FWD;

    GammaOpData::Params paramR;
    GammaOpData::Params paramG;
    GammaOpData::Params paramB;
    GammaOpData::Params paramA;

    if (style == GammaOpData::BASIC_FWD)
    {
        paramR = { gammaVals[0] };
        paramG = { gammaVals[1] };
        paramB = { gammaVals[2] };
        paramA = { gammaVals[3] };
    }
    else
    {
        paramR = { gammaVals[0], offsetVals[0] };
        paramG = { gammaVals[1], offsetVals[1] };
        paramB = { gammaVals[2], offsetVals[2] };
        paramA = { gammaVals[3], offsetVals[3] };
    }

    auto gammaData = std::make_shared<GammaOpData>(style, paramR, paramG, paramB, paramA);

    gammaData->getFormatMetadata() = transform.getFormatMetadata();
    CreateGammaOp(ops, gammaData, combinedDir);
}

void BuildExponentOp(OpRcPtrVec & ops,
                     const Config & config,
                     const ExponentTransform & transform,
                     TransformDirection dir)
{
    TransformDirection combinedDir = CombineTransformDirections(dir, transform.getDirection());

    double vec4[4] = { 1., 1., 1., 1. };
    transform.getValue(vec4);

    if(config.getMajorVersion()==1)
    {
        CreateExponentOp(ops,
                         vec4,
                         combinedDir);
    }
    else
    {
        GammaOpData::Params redParams   = { vec4[0] };
        GammaOpData::Params greenParams = { vec4[1] };
        GammaOpData::Params blueParams  = { vec4[2] };
        GammaOpData::Params alphaParams = { vec4[3] };

        auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_FWD,
                                                       redParams,
                                                       greenParams,
                                                       blueParams,
                                                       alphaParams);

        gammaData->getFormatMetadata() = transform.getFormatMetadata();
        CreateGammaOp(ops, gammaData, combinedDir);
    }
}

} // namespace OCIO_NAMESPACE

