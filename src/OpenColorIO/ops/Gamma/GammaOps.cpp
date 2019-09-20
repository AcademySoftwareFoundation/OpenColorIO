// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/Gamma/GammaOpCPU.h"
#include "ops/Gamma/GammaOps.h"
#include "ops/Gamma/GammaOpUtils.h"


OCIO_NAMESPACE_ENTER
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
    
    ComputeParamsFwd(gamma->getRedParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, red);
    ComputeParamsFwd(gamma->getGreenParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, green);
    ComputeParamsFwd(gamma->getBlueParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, blue);
    ComputeParamsFwd(gamma->getAlphaParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, alpha);

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

    ComputeParamsRev(gamma->getRedParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, red);
    ComputeParamsRev(gamma->getGreenParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, green);
    ComputeParamsRev(gamma->getBlueParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, blue);
    ComputeParamsRev(gamma->getAlphaParams(), BIT_DEPTH_F32, BIT_DEPTH_F32, alpha);

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

    GammaOp(BitDepth inBitDepth, 
            BitDepth outBitDepth,
            const FormatMetadataImpl & info,
            GammaOpData::Style style,
            GammaOpData::Params red,
            GammaOpData::Params green,
            GammaOpData::Params blue,
            GammaOpData::Params alpha);

    virtual ~GammaOp();
    
    TransformDirection getDirection() const noexcept override { return TRANSFORM_DIR_FORWARD; }

    std::string getInfo() const override;
    
    OpRcPtr clone() const override;
    
    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;
    
    void finalize(FinalizationFlags fFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;
    
    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

protected:
    ConstGammaOpDataRcPtr gammaData() const { return DynamicPtrCast<const GammaOpData>(data()); }
    GammaOpDataRcPtr gammaData() { return DynamicPtrCast<GammaOpData>(data()); }
};


GammaOp::GammaOp(GammaOpDataRcPtr & gamma)
    :   Op()
{
    data() = gamma;
}

GammaOp::GammaOp(BitDepth inBitDepth, 
                 BitDepth outBitDepth,
                 const FormatMetadataImpl & info,
                 GammaOpData::Style style,
                 GammaOpData::Params red,
                 GammaOpData::Params green,
                 GammaOpData::Params blue,
                 GammaOpData::Params alpha)
    :   Op()
{
    GammaOpDataRcPtr gamma
        = std::make_shared<GammaOpData>(inBitDepth, outBitDepth,
                                        info,
                                        style,
                                        red, green, blue, alpha);

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
        std::ostringstream os;
        os << "GammaOp can only be combined with other ";
        os << "GammaOps.  secondOp:" << secondOp->getInfo();
        throw Exception(os.str().c_str());
    }

    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(secondOp);

    GammaOpDataRcPtr res = gammaData()->compose(*typedRcPtr->gammaData());
    CreateGammaOp(ops, res, TRANSFORM_DIR_FORWARD);
}

void GammaOp::finalize(FinalizationFlags /*fFlags*/)
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

void GammaOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    if(getInputBitDepth()!=BIT_DEPTH_F32 || getOutputBitDepth()!=BIT_DEPTH_F32)
    {
        throw Exception(
            "Only 32F bit depth is supported for the GPU shader");
    }

    GpuShaderText ss(shaderDesc->getLanguage());
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

    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
}

}  // Anon namespace
    
void CreateGammaOp(OpRcPtrVec & ops,
                   const FormatMetadataImpl & info,
                   GammaOpData::Style style,
                   const double * gamma4,
                   const double * offset4)
{
    GammaOpData::Params paramR;
    GammaOpData::Params paramG;
    GammaOpData::Params paramB;
    GammaOpData::Params paramA;

    if(style==GammaOpData::BASIC_FWD || style==GammaOpData::BASIC_REV)
    {
        paramR = { gamma4[0] };
        paramG = { gamma4[1] };
        paramB = { gamma4[2] };
        paramA = { gamma4[3] };
    }
    else
    {
        paramR = { gamma4[0], offset4 ? offset4[0] : 0. };
        paramG = { gamma4[1], offset4 ? offset4[1] : 0. };
        paramB = { gamma4[2], offset4 ? offset4[2] : 0. };
        paramA = { gamma4[3], offset4 ? offset4[3] : 0. };
    }

    GammaOpDataRcPtr gammaData 
        = std::make_shared<GammaOpData>(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                        info, 
                                        style,
                                        paramR, paramG, paramB, paramA);

    CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
}

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

        group->push_back(expTransform);
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

        group->push_back(expTransform);
    }
}

void BuildExponentWithLinearOps(OpRcPtrVec & ops,
                                const Config & /*config*/,
                                const ExponentWithLinearTransform & transform,
                                TransformDirection dir)
{
    TransformDirection combinedDir 
        = CombineTransformDirections(dir, transform.getDirection());
    
    double gamma4[4] = { 1., 1., 1., 1. };
    transform.getGamma(gamma4);

    double offset4[4] = { 0., 0., 0., 0. };
    transform.getOffset(offset4);

    const bool noOffset = offset4[0] == 0. && offset4[1] == 0. &&
                          offset4[2] == 0. && offset4[3] == 0.;

    const auto style = noOffset? GammaOpData::BASIC_FWD : GammaOpData::MONCURVE_FWD;

    GammaOpData::Params paramR;
    GammaOpData::Params paramG;
    GammaOpData::Params paramB;
    GammaOpData::Params paramA;

    if (style == GammaOpData::BASIC_FWD)
    {
        paramR = { gamma4[0] };
        paramG = { gamma4[1] };
        paramB = { gamma4[2] };
        paramA = { gamma4[3] };
    }
    else
    {
        paramR = { gamma4[0], offset4[0] };
        paramG = { gamma4[1], offset4[1] };
        paramB = { gamma4[2], offset4[2] };
        paramA = { gamma4[3], offset4[3] };
    }

    auto gammaData = std::make_shared<GammaOpData>(
        BIT_DEPTH_F32, BIT_DEPTH_F32,
        FormatMetadataImpl(transform.getFormatMetadata()),
        style,
        paramR, paramG, paramB, paramA);

    CreateGammaOp(ops, gammaData, combinedDir);
}

void BuildExponentOps(OpRcPtrVec & ops,
                      const Config & config,
                      const ExponentTransform & transform,
                      TransformDirection dir)
{
    TransformDirection combinedDir = CombineTransformDirections(dir,
        transform.getDirection());
    
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
        CreateGammaOp(ops, FormatMetadataImpl(transform.getFormatMetadata()),
                      combinedDir==TRANSFORM_DIR_FORWARD ? GammaOpData::BASIC_FWD
                                                         : GammaOpData::BASIC_REV,
                      vec4, nullptr);
    }
}

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "MathUtils.h"
#include "ParseUtils.h"
#include "UnitTest.h"

namespace
{
const OCIO::FormatMetadataImpl desc(OCIO::METADATA_ROOT);

void ApplyGamma(const OCIO::OpRcPtr & op, 
                float * image, const float * result,
                long numPixels, 
                float errorThreshold)
{
    OCIO_CHECK_NO_THROW(op->apply(image, numPixels));

    for(long idx=0; idx<(numPixels*4); ++idx)
    {
        // Using rel error with a large minExpected value of 1 will transition
        // from absolute error for expected values < 1 and
        // relative error for values > 1.
        const bool equalRel
            = OCIO::EqualWithSafeRelError(image[idx], result[idx],
                                          errorThreshold, 1.0f);
        if (!equalRel)
        {
            // As most of the error thresholds are 1e-7f, the output 
            // value precision should then be bigger than 7 digits 
            // to highlight small differences. 
            std::ostringstream message;
            message.precision(9);
            message << "Index: " << idx
                    << " - Values: " << image[idx]
                    << " and: " << result[idx]
                    << " - Threshold: " << errorThreshold;
            OCIO_CHECK_ASSERT_MESSAGE(0, message.str());
        }
    }
}

};

OCIO_ADD_TEST(GammaOps, apply_basic_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

    const std::vector<double> gamma4 = { 1.2, 2.12, 1.123, 1.05 };

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00001478f, 0.48297336f,
        0.00010933f, 0.00001323f, 0.03458935f, 0.73928129f,
        0.18946611f, 0.23005184f, 0.72391760f, 1.00001204f,
        0.76507961f, 0.89695119f, 1.00001264f, 1.53070319f,
        1.00601125f, 1.10895324f, 1.57668686f, 0.0f  };
#else
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00001478f, 0.48296818f,
        0.00010933f, 0.00001323f, 0.03458942f, 0.73928916f,
        powf(input_32f[12], (float)gamma4[0]),
        powf(input_32f[13], (float)gamma4[1]),
        powf(input_32f[14], (float)gamma4[2]),
        powf(input_32f[15], (float)gamma4[3]),
        powf(input_32f[16], (float)gamma4[0]),
        powf(input_32f[17], (float)gamma4[1]),
        powf(input_32f[18], (float)gamma4[2]),
        powf(input_32f[19], (float)gamma4[3]),
        1.00600302f, 1.10897374f, 1.57670521f, 0.0f  };
#endif

    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_FWD,
                            &gamma4[0], nullptr));

    OCIO_CHECK_NO_THROW(OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

OCIO_ADD_TEST(GammaOps, apply_basic_style_rev)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

    const std::vector<double> gamma4 = { 1.2, 2.12, 1.123, 1.05 };

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00014792f, 0.51678240f,
        0.00177476f, 0.08215060f, 0.06941742f, 0.76033723f, 
        0.31498342f, 0.72111737f, 0.77400052f, 1.00001109f, 
        0.83031141f, 0.97609287f, 1.00001061f, 1.47130167f, 
        1.00417137f, 1.02327621f, 1.43483067f, 0.0f };
#else
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00014792f, 0.51677888f,
        0.00177476f, 0.08215017f, 0.06941755f, 0.76034504f,
        powf(input_32f[12], (float)(1./gamma4[0])),
        powf(input_32f[13], (float)(1./gamma4[1])),
        powf(input_32f[14], (float)(1./gamma4[2])),
        powf(input_32f[15], (float)(1./gamma4[3])),
        powf(input_32f[16], (float)(1./gamma4[0])),
        powf(input_32f[17], (float)(1./gamma4[1])),
        powf(input_32f[18], (float)(1./gamma4[2])),
        powf(input_32f[19], (float)(1./gamma4[3])),
        1.00416493f, 1.02328109f, 1.43484282f, 0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_REV,
                            &gamma4[0], nullptr));

    OCIO_CHECK_NO_THROW(OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

OCIO_ADD_TEST(GammaOps, apply_moncurve_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        -0.07738016f, -0.33144456f, -0.20408163f,  0.0f,
        -0.00019345f,  0.0f,         0.00004081f,  0.49101364f, 
         0.00003869f,  0.00220963f,  0.04081632f,  0.73652046f,
         0.05087645f,  0.30550804f,  0.67475068f,  1.00001871f,
         0.60383129f,  0.91060406f,  1.00002050f,  1.63147723f,
         1.01142657f,  1.09394502f,  1.84183871f, -0.24550682f };
#else
    const float expected_32f[numPixels*4] = {
        -0.07738015f, -0.33144456f, -0.20408163f,  0.0f,
        -0.00019345f,  0.0f,         0.00004081f,  0.49101364f, 
         0.00003869f,  0.00220963f,  0.04081632f,  0.73652046f,
         0.05087607f,  0.30550399f,  0.67474484f,  1.0f,
         0.60382729f,  0.91061854f,  1.0f,         1.63146877f,
         1.01141202f,  1.09396457f,  1.84183657f, -0.24550682f };
#endif

    OCIO::OpRcPtrVec ops;

    const std::vector<double> gamma4 = { 2.4,   2.2, 2.0, 1.8 };
    const std::vector<double> offset4= { 0.055, 0.2, 0.4, 0.6 };

    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::MONCURVE_FWD,
                            &gamma4[0], &offset4[0]));

    OCIO_CHECK_NO_THROW(OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

OCIO_ADD_TEST(GammaOps, apply_moncurve_style_rev)
{
    const float errorThreshold = 1e-6f;
    const long numPixels = 6;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f }; 

#ifdef USE_SSE
    const float expected_32f[numPixels*4] = {
        -6.18606853f, -1.69711625f, -0.30625000f,  0.0f,
        -0.01546517f,  0.0f,         0.00006125f,  0.50915080f,
         0.00309303f,  0.01131410f,  0.06125000f,  0.76366448f,
         0.51735591f,  0.67569005f,  0.81243133f,  1.00001215f,
         0.90233862f,  0.97234255f,  1.00000989f,  1.40423023f,
         1.00229334f,  1.02690458f,  1.31464004f, -0.25457540f };
#else
    const float expected_32f[numPixels*4] = {
        -6.18606853f, -1.69711625f, -0.30625000f,  0.0f,
        -0.01546517f,  0.0f,         0.00006125f,  0.50915080f,
         0.00309303f,  0.01131410f,  0.06125000f,  0.76367092f,
         0.51735413f,  0.67568808f,  0.81243550f,  1.0f,

#ifdef _WIN32
         0.90233647f,  0.97234553f,  1.0f,         1.40423405f,
#else
         0.90233647f,  0.97234553f,  1.0f,         1.40423429f,
#endif

         1.00228834f,  1.02691006f,  1.31464290f, -0.25457540f };
#endif

    OCIO::OpRcPtrVec ops;

    const std::vector<double> gamma4 = { 2.4, 2.2, 2.0, 1.8 };
    const std::vector<double> offset4= { 0.1, 0.2, 0.4, 0.6 };

    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::MONCURVE_REV,
                            &gamma4[0], &offset4[0]));

    OCIO_CHECK_NO_THROW(OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, errorThreshold);
}

OCIO_ADD_TEST(GammaOps, combining)
{
    OCIO::OpRcPtrVec ops;

    const std::vector<double> gamma4_1 = { 1.201, 1.201, 1.201, 1. };
    const std::vector<double> gamma4_2 = { 2.345, 2.345, 2.345, 1. };

    const OCIO::GammaOpData::Params param1R{ 1.201 };
    const OCIO::GammaOpData::Params param1G{ 1.201 };
    const OCIO::GammaOpData::Params param1B{ 1.201 };
    const OCIO::GammaOpData::Params param1A{ 1. };

    OCIO::FormatMetadataImpl info1{ OCIO::METADATA_ROOT };
    info1.addAttribute(OCIO::METADATA_NAME, "gamma1");
    info1.addAttribute(OCIO::METADATA_ID, "ID1");
    info1.addAttribute("Attrib", "1");
    info1.addAttribute("Attrib1", "10");
    auto & child1 = info1.addChildElement("Gamma1Child", "Some content");
    OCIO::GammaOpDataRcPtr gammaData1
        = std::make_shared<OCIO::GammaOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
            info1,
            OCIO::GammaOpData::BASIC_FWD,
            param1R, param1G, param1B, param1A);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData1, OCIO::TRANSFORM_DIR_FORWARD));

    const OCIO::GammaOpData::Params param2R{ 2.345 };
    const OCIO::GammaOpData::Params param2G{ 2.345 };
    const OCIO::GammaOpData::Params param2B{ 2.345 };
    const OCIO::GammaOpData::Params param2A{ 1. };

    OCIO::FormatMetadataImpl info2{ OCIO::METADATA_ROOT };
    info2.addAttribute(OCIO::METADATA_NAME, "gamma2");
    info2.addAttribute(OCIO::METADATA_ID, "ID2");
    info2.addAttribute("Attrib", "2");
    info2.addAttribute("Attrib2", "20");
    auto & child2 = info2.addChildElement("Gamma2Child", "Other content");
    OCIO::GammaOpDataRcPtr gammaData2
        = std::make_shared<OCIO::GammaOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
            info2,
            OCIO::GammaOpData::BASIC_FWD,
            param2R, param2G, param2B, param2A);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(ops[0]->canCombineWith(op1));
    OCIO_CHECK_NO_THROW(ops[0]->combineWith(ops, op1));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op2 = ops[2];
    
    auto combinedData = op2->data();

    // Check metadata of combined op.
    OCIO_CHECK_EQUAL(combinedData->getName(), "gamma1 + gamma2");
    OCIO_CHECK_EQUAL(combinedData->getID(), "ID1 + ID2");
    // 5 attributes: name, id, Attrib, Attrib1 and  Attrib2.
    OCIO_CHECK_EQUAL(combinedData->getFormatMetadata().getNumAttributes(), 5);
    auto & attribs = combinedData->getFormatMetadata().getAttributes();
    OCIO_CHECK_EQUAL(attribs[2].first, "Attrib");
    OCIO_CHECK_EQUAL(attribs[2].second, "1 + 2");
    OCIO_CHECK_EQUAL(attribs[3].first, "Attrib1");
    OCIO_CHECK_EQUAL(attribs[3].second, "10");
    OCIO_CHECK_EQUAL(attribs[4].first, "Attrib2");
    OCIO_CHECK_EQUAL(attribs[4].second, "20");
    auto & children = combinedData->getFormatMetadata().getChildrenElements();
    OCIO_REQUIRE_EQUAL(children.size(), 2);
    OCIO_CHECK_ASSERT(children[0] == child1);
    OCIO_CHECK_ASSERT(children[1] == child2);

    OCIO_REQUIRE_EQUAL(op2->data()->getType(), OCIO::OpData::GammaType);

    OCIO::ConstGammaOpDataRcPtr g = OCIO::DynamicPtrCast<const OCIO::GammaOpData>(op2->data());

    OCIO_CHECK_EQUAL(g->getRedParams()[0], param1R[0] * param2R[0]);
    OCIO_CHECK_EQUAL(g->getGreenParams()[0], param1G[0] * param2G[0]);
    OCIO_CHECK_EQUAL(g->getBlueParams()[0], param1B[0]* param2B[0]);
    OCIO_CHECK_EQUAL(g->getAlphaParams()[0], param1A[0]* param2A[0]);
}

OCIO_ADD_TEST(GammaOps, is_inverse)
{
    OCIO::OpRcPtrVec ops;

    const std::vector<double> gamma4 = { 1.001, 1., 1., 1. };

    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_FWD,
                            &gamma4[0], nullptr));

    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_REV,
                            &gamma4[0], nullptr));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));
}

OCIO_ADD_TEST(GammaOps, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    std::vector<double> gamma4 = { 1.001, 1., 1., 1. };

    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_FWD,
                            &gamma4[0], nullptr));
    OCIO_CHECK_EQUAL(ops.size(), 1);

    gamma4[1] = 1.001;
    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_FWD,
                            &gamma4[0], nullptr));
    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[1]->getCacheID());


    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_FWD,
                            &gamma4[0], nullptr));
    OCIO_CHECK_EQUAL(ops.size(), 3);

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[1]->getCacheID() == ops[2]->getCacheID());


    OCIO_CHECK_NO_THROW(
        OCIO::CreateGammaOp(ops, desc, OCIO::GammaOpData::BASIC_REV,
                            &gamma4[0], nullptr));
    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[3]->getCacheID());
    OCIO_CHECK_ASSERT(ops[1]->getCacheID() != ops[3]->getCacheID());
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[3]->getCacheID());
}

OCIO_ADD_TEST(GammaOps, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    OCIO::FormatMetadataImpl metadataSource(OCIO::METADATA_ROOT);
    metadataSource.addAttribute("name", "test");

    const OCIO::GammaOpData::Params red{2., 0.2};
    const OCIO::GammaOpData::Params green{3., 0.3};
    const OCIO::GammaOpData::Params blue{4., 0.4};
    const OCIO::GammaOpData::Params alpha{2.5, 0.25};

    OCIO::GammaOpDataRcPtr gamma
        = std::make_shared<OCIO::GammaOpData>(OCIO::BIT_DEPTH_F32,
                                              OCIO::BIT_DEPTH_F32,
                                              metadataSource,
                                              OCIO::GammaOpData::MONCURVE_FWD,
                                              red, green, blue, alpha);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateGammaTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto gTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentWithLinearTransform>(transform);
    OCIO_REQUIRE_ASSERT(gTransform);

    const auto & metadata = gTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(gTransform->getDirection(), direction);
    double gval[4];
    gTransform->getGamma(gval);
    OCIO_CHECK_EQUAL(gval[0], red[0]);
    OCIO_CHECK_EQUAL(gval[1], green[0]);
    OCIO_CHECK_EQUAL(gval[2], blue[0]);
    OCIO_CHECK_EQUAL(gval[3], alpha[0]);

    double oval[4];
    gTransform->getOffset(oval);
    OCIO_CHECK_EQUAL(oval[0], red[1]);
    OCIO_CHECK_EQUAL(oval[1], green[1]);
    OCIO_CHECK_EQUAL(oval[2], blue[1]);
    OCIO_CHECK_EQUAL(oval[3], alpha[1]);

    const OCIO::GammaOpData::Params red0{ 2. };
    const OCIO::GammaOpData::Params green0{ 3. };
    const OCIO::GammaOpData::Params blue0{ 4. };
    const OCIO::GammaOpData::Params alpha0{ 2.5 };

    OCIO::GammaOpDataRcPtr gamma0
        = std::make_shared<OCIO::GammaOpData>(OCIO::BIT_DEPTH_F32,
                                              OCIO::BIT_DEPTH_F32,
                                              metadataSource,
                                              OCIO::GammaOpData::BASIC_REV,
                                              red0, green0, blue0, alpha0);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma0, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);

    OCIO::ConstOpRcPtr op0(ops[1]);

    OCIO::CreateGammaTransform(group, op0);
    OCIO_REQUIRE_EQUAL(group->size(), 2);
    auto transform0 = group->getTransform(1);
    OCIO_REQUIRE_ASSERT(transform0);
    auto eTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentTransform>(transform0);
    OCIO_REQUIRE_ASSERT(eTransform);

    const auto & metadata0 = eTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata0.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata0.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata0.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(eTransform->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    double expVal[4]{ 0.0 };
    eTransform->getValue(expVal);
    OCIO_CHECK_EQUAL(expVal[0], red0[0]);
    OCIO_CHECK_EQUAL(expVal[1], green0[0]);
    OCIO_CHECK_EQUAL(expVal[2], blue0[0]);
    OCIO_CHECK_EQUAL(expVal[3], alpha0[0]);
}

// Still need bit-depth coverage from these tests:
//      gammaOp_basicfwd
//      gammaOp_basicrev
//      gammaOp_moncurvefwd
//      gammaOp_moncurverev



#endif // OCIO_UNIT_TEST
