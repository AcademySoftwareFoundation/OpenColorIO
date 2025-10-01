// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "ops/gradinghuecurve/GradingHueCurveOpGPU.h"
#include "ops/fixedfunction/FixedFunctionOpGPU.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

// The curve evaluation is done using a piecewise quadratic polynomial function. The shader
// may handle a dynamic number of curves and a dynamic number of knots and coefficients per
// curve.
//
// For optimization, the knots of ALL the curves are packed in one single array. This is
// exactly the same for coefficients. For example :
//
// KnotsArray = { Curve1[kn0, kn1], Curve2[kn0, kn1, kn2], Curve3[kn0, kn1] }
//
// In order to access knots of a specific curve in this single array, the position of the
// first knot and the number of knots of each curve is stored in an offset array.
// This array is dynamic according to the number of curves. For example :
//
// KnotOffsetArray = {Curve1StartPos, Curve1NumKnots, Curve2StartPos, Curve2NumKnots}
//
// Here is an example of what the arrays would look like in memory with the following
// curve information:
//
// Curve 1 : Knots = { 0, 1, 2 }    Coefficients = { 10, 11, 12, 13, 14, 15 }
// Curve 2 : Knots = { 0.1, 0.5, 1, 3 } Coefficients = { 20, 21, 22, 23, 24, 25, 26, 27, 28 }
//
// KnotsArray : { 0, 1, 2, 0.1, 0.5, 1, 3 }
// CoefsArray : { 10, 11, 12, 13, 14, 15, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
//
// KnotsOffsetsArray : { 0, 3, 3, 4 }
// CoefsOffsetsArray : { 0, 6, 6, 9 }
//
// To access the knots of the second curve in C++, you would do the following :
//
// {
//   const unsigned curveIdx = 1; // Second curve. This is 0 based.
//   const unsigned startPos = KnotsOffsetsArray[curveIdx*2]; // Data is in pairs.
//   const unsigned numKnots = KnotsOffsetsArray[curveIdx*2+1];
//
//   const float firstKnot = KnotsArray[startPos];
//   const float lastKnot = KnotsArray[startPos+numKnots-1];
// }
//
// In GLSL, offset arrays are loaded as vec2 uniforms. To achieve the previous example
// in GLSL, you would do the following :
//
// {
//   const int curveIdx = 1;
//   const int startPos = KnotsOffsetsArray[curveIdx*2];
//   const int numKnots = KnotsOffsetsArray[curveIdx*2+1];
//
//   const float firstKnot = KnotsArray[startPos].x;
//   const float lastKnot = KnotsArray[startPos+numKnots-1].x;
// }
//
// The coefficients array contains the polynomial coefficients which are stored
// as all the quadratic terms for the first curve, then all the linear terms for
// the first curve, then all the constant terms for the first curve.  The number
// of coefficient sets is the number of knots minus one.

namespace
{
struct GCProperties
{
    std::string m_knotsOffsets{ "knotsOffsets" };
    std::string m_knots{ "knots" };
    std::string m_coefsOffsets{ "coefsOffsets" };
    std::string m_coefs{ "coefs" };
    std::string m_localBypass{ "localBypass" };
    std::string m_eval{ "evalBSplineCurve" };
    std::string m_evalRev{ "evalBSplineCurveRev" };
    std::string m_evalRevHue{ "evalBSplineCurveRevHue" };
};

void AddUniform(GpuShaderCreatorRcPtr & shaderCreator,
                const GpuShaderCreator::SizeGetter & getSize,
                const GpuShaderCreator::VectorFloatGetter & getVector,
                unsigned int maxSize,
                const std::string & name)
{
    // Add the uniform if it does not already exist.
    if (shaderCreator->addUniform(name.c_str(), getSize, getVector, maxSize))
    {
        // Declare uniform.
        GpuShaderText stDecl(shaderCreator->getLanguage());
        stDecl.declareUniformArrayFloat(name, maxSize);
        shaderCreator->addToParameterDeclareShaderCode(stDecl.string().c_str());
    }
}

void AddUniform(GpuShaderCreatorRcPtr & shaderCreator,
                const GpuShaderCreator::SizeGetter & getSize,
                const GpuShaderCreator::VectorIntGetter & getVector,
                const std::string & name)
{
    // 8 curves x 2 values (count and offset).
    static constexpr unsigned arrayLen = HueCurveType::HUE_NUM_CURVES * 2;

    // Add the uniform if it does not already exist.
    if (shaderCreator->addUniform(name.c_str(), getSize, getVector, arrayLen))
    {
        // Declare uniform.
        GpuShaderText stDecl(shaderCreator->getLanguage());
        // Need 2 ints for each curve.
        stDecl.declareUniformArrayInt(name, arrayLen);
        shaderCreator->addToParameterDeclareShaderCode(stDecl.string().c_str());
    }
}

void AddUniform(GpuShaderCreatorRcPtr & shaderCreator,
                const GpuShaderCreator::BoolGetter & getBool,
                const std::string & name)
{
    // Add the uniform if it does not already exist.
    if (shaderCreator->addUniform(name.c_str(), getBool))
    {
        // Declare uniform.
        GpuShaderText stDecl(shaderCreator->getLanguage());
        stDecl.declareUniformBool(name);
        shaderCreator->addToParameterDeclareShaderCode(stDecl.string().c_str());
    }
}

std::string BuildResourceNameIndexed(GpuShaderCreatorRcPtr & shaderCreator, const std::string & prefix,
                                     const std::string & base, unsigned int index)
{
    std::string name{ BuildResourceName(shaderCreator, prefix, base) };
    name += "_";
    name += std::to_string(index);
    // Note: Remove potentially problematic double underscores from GLSL resource names.
    StringUtils::ReplaceInPlace(name, "__", "_");
    return name;
}

static const std::string opPrefix{ "grading_huecurve" };

void SetGCProperties(GpuShaderCreatorRcPtr & shaderCreator, bool dynamic, GCProperties & propNames)
{
    if (dynamic)
    {
        // If there are several dynamic ops, they will use the same names for uniforms.
        propNames.m_knotsOffsets = BuildResourceName(shaderCreator, opPrefix,
                                                     propNames.m_knotsOffsets);
        propNames.m_knots = BuildResourceName(shaderCreator, opPrefix, propNames.m_knots);
        propNames.m_coefsOffsets = BuildResourceName(shaderCreator, opPrefix,
                                                     propNames.m_coefsOffsets);
        propNames.m_coefs = BuildResourceName(shaderCreator, opPrefix, propNames.m_coefs);
        propNames.m_localBypass = BuildResourceName(shaderCreator, opPrefix,
                                                    propNames.m_localBypass);
        propNames.m_eval = BuildResourceName(shaderCreator, opPrefix, propNames.m_eval);
        propNames.m_evalRev = BuildResourceName(shaderCreator, opPrefix, propNames.m_evalRev);
        propNames.m_evalRevHue = BuildResourceName(shaderCreator, opPrefix, propNames.m_evalRevHue);
    }
    else
    {
        // Non-dynamic ops need a helper function for each op.
        const auto resIndex = shaderCreator->getNextResourceIndex();

        propNames.m_knotsOffsets = BuildResourceNameIndexed(shaderCreator, opPrefix,
                                                            propNames.m_knotsOffsets, resIndex);
        propNames.m_knots = BuildResourceNameIndexed(shaderCreator, opPrefix,
                                                     propNames.m_knots, resIndex);
        propNames.m_coefsOffsets = BuildResourceNameIndexed(shaderCreator, opPrefix,
                                                            propNames.m_coefsOffsets, resIndex);
        propNames.m_coefs = BuildResourceNameIndexed(shaderCreator, opPrefix,
                                                     propNames.m_coefs, resIndex);
        propNames.m_eval = BuildResourceNameIndexed(shaderCreator, opPrefix,
                                                    propNames.m_eval, resIndex);
        propNames.m_evalRev = BuildResourceNameIndexed(shaderCreator, opPrefix,
                                                       propNames.m_evalRev, resIndex);
        propNames.m_evalRevHue = BuildResourceNameIndexed(shaderCreator, opPrefix,
                                                          propNames.m_evalRevHue, resIndex);
    }
}

// Only called once for dynamic ops.
void AddGCPropertiesUniforms(GpuShaderCreatorRcPtr & shaderCreator,
                             DynamicPropertyGradingHueCurveImplRcPtr & shaderProp,
                             const GCProperties & propNames)
{
    // Use the shader dynamic property to bind the uniforms.
    auto curveProp = shaderProp.get();

    // Note: No need to add an index to the name to avoid collisions as the dynamic properties
    // are unique.

    auto getNK = std::bind(&DynamicPropertyGradingHueCurveImpl::getNumKnots, curveProp);
    auto getKO = std::bind(&DynamicPropertyGradingHueCurveImpl::getKnotsOffsetsArray,
                           curveProp);
    auto getK = std::bind(&DynamicPropertyGradingHueCurveImpl::getKnotsArray, curveProp);
    auto getNC = std::bind(&DynamicPropertyGradingHueCurveImpl::getNumCoefs, curveProp);
    auto getCO = std::bind(&DynamicPropertyGradingHueCurveImpl::getCoefsOffsetsArray,
                           curveProp);
    auto getC = std::bind(&DynamicPropertyGradingHueCurveImpl::getCoefsArray, curveProp);
    auto getLB = std::bind(&DynamicPropertyGradingHueCurveImpl::getLocalBypass, curveProp);
    // Uniforms are added if they are not already there (added by another op).
    AddUniform(shaderCreator, DynamicPropertyGradingHueCurveImpl::GetNumOffsetValues,
               getKO, propNames.m_knotsOffsets);
    AddUniform(shaderCreator, getNK, getK,
               DynamicPropertyGradingHueCurveImpl::GetMaxKnots(),
               propNames.m_knots);
    AddUniform(shaderCreator, DynamicPropertyGradingHueCurveImpl::GetNumOffsetValues,
               getCO, propNames.m_coefsOffsets);
    AddUniform(shaderCreator, getNC, getC,
               DynamicPropertyGradingHueCurveImpl::GetMaxCoefs(),
               propNames.m_coefs);
    AddUniform(shaderCreator, getLB, propNames.m_localBypass);
}

void AddCurveFunctionName(GpuShaderCreatorRcPtr & shaderCreator,
                          GpuShaderText & st,
                          const std::string & funcName,
                          bool isFwd)
{
    st.newLine() << "";
    if (shaderCreator->getLanguage() == LANGUAGE_OSL_1 || shaderCreator->getLanguage() == GPU_LANGUAGE_MSL_2_0)
    {
        if (isFwd)
        {
            st.newLine() << st.floatKeyword() << " " << funcName << "(int curveIdx, float x, float identity_x)";
        }
        else
        {
            st.newLine() << st.floatKeyword() << " " << funcName << "(int curveIdx, float x)";
        }
    }
    else
    {
        if (isFwd)
        {
            st.newLine() << st.floatKeyword() << " " << funcName << "(in int curveIdx, in float x, in float identity_x)";
        }
        else
        {
            st.newLine() << st.floatKeyword() << " " << funcName << "(in int curveIdx, in float x)";
        }
    }
}

void AddCurveEvalMethodTextToShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                           ConstGradingHueCurveOpDataRcPtr & gcData,
                                           const GCProperties & props,
                                           bool dyn)
{
    GpuShaderText st(shaderCreator->getLanguage());

    // Dynamic version uses uniforms declared globally. Non-dynamic version declares local
    // variables in the op specific helper function.
    if (!dyn)
    {
        auto propGC = gcData->getDynamicPropertyInternal();
        const int numOffsets = propGC->GetNumOffsetValues();

        // 2 ints for each curve.
        st.newLine() << "";
        st.declareIntArrayConst(props.m_knotsOffsets, numOffsets, propGC->getKnotsOffsetsArray());
        st.declareFloatArrayConst(props.m_knots, propGC->getNumKnots(), propGC->getKnotsArray());
        st.declareIntArrayConst(props.m_coefsOffsets, numOffsets, propGC->getCoefsOffsetsArray());
        st.declareFloatArrayConst(props.m_coefs, propGC->getNumCoefs(), propGC->getCoefsArray());
    }

    // Both the forward and inverse hue curve eval need the forward spline eval, so always add that.

    AddCurveFunctionName(shaderCreator, st, props.m_eval, true);

    st.newLine() << "{";
    st.indent();
    GradingBSplineCurveImpl::AddShaderEvalFwd(st, props.m_knotsOffsets, props.m_coefsOffsets,
                                              props.m_knots, props.m_coefs);
    st.dedent();
    st.newLine() << "}";

    if (gcData->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        // Add inverse curve eval.
        AddCurveFunctionName(shaderCreator, st, props.m_evalRev, false);

        st.newLine() << "{";
        st.indent();
        GradingBSplineCurveImpl::AddShaderEvalRev(st, props.m_knotsOffsets, props.m_coefsOffsets,
                                                  props.m_knots, props.m_coefs);
        st.dedent();
        st.newLine() << "}";

        // Add inverse hue curve eval.
        AddCurveFunctionName(shaderCreator, st, props.m_evalRevHue, false);

        st.newLine() << "{";
        st.indent();
        GradingBSplineCurveImpl::AddShaderEvalRevHue(st, props.m_knotsOffsets, props.m_coefsOffsets,
                                                     props.m_knots, props.m_coefs);
        st.dedent();
        st.newLine() << "}";
    }

    shaderCreator->addToHelperShaderCode(st.string().c_str());
}

void AddGCForwardShader(GpuShaderCreatorRcPtr & shaderCreator,
                        GpuShaderText & st,
                        const GCProperties & props,
                        bool dyn,
                        bool doLinToLog,
                        bool doRGBToHSY,
                        bool drawCurveOnly,
                        GradingStyle style)
{
    const std::string pix(shaderCreator->getPixelName());
    if (drawCurveOnly)
    {
        // Note that this is not within the localBypass if-statement since outColor
        // needs to get processed even if isDefault is true for all curves since
        // the default for the horizontal curves is all 1 rather than an identity.
        st.newLine() << pix << ".r = " << props.m_eval << "(1, " << pix << ".r, 1.);"; // HUE-SAT
        st.newLine() << pix << ".g = " << props.m_eval << "(1, " << pix << ".g, 1.);"; // HUE-SAT
        st.newLine() << pix << ".b = " << props.m_eval << "(1, " << pix << ".b, 1.);"; // HUE-SAT
        return;
    }

    if (dyn)
    {
        st.newLine() << "if (!" << st.castToBool(props.m_localBypass) << ")";
        st.newLine() << "{";
        st.indent();
    }    

    // Add the conversion from RGB to HSY.
    if (doRGBToHSY)
    {
        FixedFunctionOpData::Style hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
        switch (style)
        {
        case GRADING_LIN:
            hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
            break;
        case GRADING_LOG:
            hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LOG;
            break;
        case GRADING_VIDEO:
            hsyStyle = FixedFunctionOpData::RGB_TO_HSY_VID;
            break;
        }

        st.newLine() << "{";   // establish scope so local variable names won't conflict
        st.indent();
        ConstFixedFunctionOpDataRcPtr funcOpData = std::make_shared<FixedFunctionOpData>(hsyStyle);
        GetFixedFunctionGPUProcessingText(shaderCreator, st, funcOpData);
        st.dedent();
        st.newLine() << "}";
    }

    // Lin to Log (on Luma only).
    if (doLinToLog)
    {
        // NB:  Although the linToLog and logToLin are correct inverses, the limits of
        // floating-point arithmetic cause errors in the lowest bit of the round trip.

        st.newLine() << "// Convert from lin to log.";
        AddLinToLogShaderChannelBlue(shaderCreator, st);
        st.newLine() << "";
    }

    // Apply the hue curves.

    st.newLine() << "";
    // HUE-SAT
    st.newLine() << "float hueSatGain = max(0., " << props.m_eval << "(1, " << pix << ".r, 1.));";
    // HUE-LUM
    st.newLine() << "float hueLumGain = max(0., " << props.m_eval << "(2, " << pix << ".r, 1.));";
    // HUE-HUE
    st.newLine() << pix << ".r = " << props.m_eval << "(0, " << pix << ".r, " << pix << ".r);";
    // SAT-SAT
    st.newLine() << "" << pix << ".g = max(0., " << props.m_eval << "(4, " << pix << ".g, " << pix << ".g));";
    // LUM-SAT
    st.newLine() << "float lumSatGain = max(0., " << props.m_eval << "(3, " << pix << ".b, 1.));";
    // SAT-LUM
    st.newLine() << "float satGain = lumSatGain * hueSatGain;";
    st.newLine() << "" << pix << ".g = satGain * " << pix << ".g;";
    st.newLine() << "float satLumGain = max(0., " << props.m_eval << "(6, " << pix << ".g, 1.));";
    // LUM-LUM
    st.newLine() << pix << ".b = " << props.m_eval << "(5, " << pix << ".b, " << pix << ".b);";
    st.newLine() << "";

    // Log to Lin.
    if (doLinToLog)
    {
        st.newLine() << "";
        st.newLine() << "// Convert from log to lin.";
        AddLogToLinShaderChannelBlue(shaderCreator, st);
    }

    st.newLine() << "";
    st.newLine() << "hueLumGain = 1. - (1. - hueLumGain) * min( 1., " << pix << ".g );";
    if (style == GRADING_LOG)
    {
        // Use shift rather than scale for log mode.
        st.newLine() << pix << ".b = " << pix << ".b + (hueLumGain + satLumGain - 2.) * 0.1;";
    }
    else
    {
        // Note this is applied in linear space, for linear style.
        st.newLine() << pix << ".b = " << pix << ".b * hueLumGain * satLumGain;";
    }
    st.newLine() << "";

    // HUE-FX
    st.newLine() << pix << ".r = " << pix << ".r - floor( " << pix << ".r );";
    st.newLine() << pix << ".r = " << pix << ".r + " << props.m_eval << "(7, " << pix << ".r, 0.);";

    // Add the conversion from HSY to RGB.
    if (doRGBToHSY)
    {
        FixedFunctionOpData::Style hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
        switch (style)
        {
        case GRADING_LIN:
            hsyStyle = FixedFunctionOpData::HSY_LIN_TO_RGB;
            break;
        case GRADING_LOG:
            hsyStyle = FixedFunctionOpData::HSY_LOG_TO_RGB;
            break;
        case GRADING_VIDEO:
            hsyStyle = FixedFunctionOpData::HSY_VID_TO_RGB;
            break;
        }
        st.newLine() << "{";
        st.indent();
        ConstFixedFunctionOpDataRcPtr funcOpData = std::make_shared<FixedFunctionOpData>(hsyStyle);
        GetFixedFunctionGPUProcessingText(shaderCreator, st, funcOpData);
        st.dedent();
        st.newLine() << "}";
    }

    if (dyn)
    {
        st.dedent();
        st.newLine() << "}";
    }
}

void AddGCInverseShader(GpuShaderCreatorRcPtr & shaderCreator, 
                       GpuShaderText & st,
                       const GCProperties & props,
                       bool dyn,
                       bool doLinToLog,
                       bool doRGBToHSY,
                       bool drawCurveOnly,
                       GradingStyle style)
{
    const std::string pix(shaderCreator->getPixelName());

    if (drawCurveOnly)
    {
        // Note that this is not within the localBypass if-statement since outColor
        // needs to get processed even if isDefault is true for all curves since
        // the default for the horizontal curves is all 1 rather than an identity.
        st.newLine() << pix << ".r = " << props.m_eval << "(1, " << pix << ".r, 1.);"; // HUE-SAT
        st.newLine() << pix << ".g = " << props.m_eval << "(1, " << pix << ".g, 1.);"; // HUE-SAT
        st.newLine() << pix << ".b = " << props.m_eval << "(1, " << pix << ".b, 1.);"; // HUE-SAT
        return;
    }

    if (dyn)
    {
        st.newLine() << "if (!" << st.castToBool(props.m_localBypass) << ")";
        st.newLine() << "{";
        st.indent();
    }

    // Add the conversion from RGB to HSY.
    if (doRGBToHSY)
    {
        FixedFunctionOpData::Style hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
        switch (style)
        {
        case GRADING_LIN:
            hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
            break;
        case GRADING_LOG:
            hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LOG;
            break;
        case GRADING_VIDEO:
            hsyStyle = FixedFunctionOpData::RGB_TO_HSY_VID;
            break;
        }

        st.newLine() << "{";   // establish scope so local variable names won't conflict
        st.indent();
        ConstFixedFunctionOpDataRcPtr funcOpData = std::make_shared<FixedFunctionOpData>(hsyStyle);
        GetFixedFunctionGPUProcessingText(shaderCreator, st, funcOpData);
        st.dedent();
        st.newLine() << "}";
    }

    // Apply the hue curves inverse.

    // Invert HUE-FX.
    st.newLine() << pix << ".r = " << props.m_evalRevHue << "(7, " << pix << ".r);";

    // Invert HUE-HUE.
    st.newLine() << pix << ".r = " << props.m_evalRevHue << "(0, " << pix << ".r);";
    st.newLine() << "";

    // Use the inverted hue to calculate the HUE-SAT & HUE-LUM gains.
    st.newLine() << pix << ".r = " << pix << ".r - floor( " << pix << ".r );";
    st.newLine() << "float hueSatGain = max(0., " << props.m_eval << "(1, " << pix << ".r, 1.));";
    st.newLine() << "float hueLumGain = max(0., " << props.m_eval << "(2, " << pix << ".r, 1.));";

    // Use the output sat to calculate the SAT-LUM gain.
    st.newLine() << "" << pix << ".g = max(0., " << pix << ".g);";
    st.newLine() << "float satLumGain = max(0., " << props.m_eval << "(6, " << pix << ".g, 1.));";

    st.newLine() << "";
    st.newLine() << "hueLumGain = 1. - (1. - hueLumGain) * min( 1., " << pix << ".g );";

    // Invert the lum gain.
    if (style == GRADING_LOG)
    {
        // Use shift rather than scale for log mode.
        st.newLine() << pix << ".b = " << pix << ".b - (hueLumGain + satLumGain - 2.) * 0.1;";
    }
    else
    {
        // Note this is applied in linear space, for linear style.
        st.newLine() << pix << ".b = " << pix << ".b / max(0.01, hueLumGain * satLumGain);";
    }
    st.newLine() << "";

    if (doLinToLog)
    {
        st.newLine() << "// Convert from lin to log.";
        AddLinToLogShaderChannelBlue(shaderCreator, st);
        st.newLine() << "";
    }

    // Invert LUM-LUM.
    st.newLine() << pix << ".b = " << props.m_evalRev << "(5, " << pix << ".b);";
    st.newLine() << "";

    // Use it to calc the LUM-SAT gain.
    st.newLine() << "float lumSatGain = max(0., " << props.m_eval << "(3, " << pix << ".b, 1.));";

    if (doLinToLog)
    {
        st.newLine() << "";
        st.newLine() << "// Convert from log to lin.";
        AddLogToLinShaderChannelBlue(shaderCreator, st);
    }

    // Invert the sat gain.
    st.newLine() << "float satGain = max(0.01, lumSatGain * hueSatGain);";
    st.newLine() << "" << pix << ".g = " << pix << ".g / satGain;";

    // Invert SAT-SAT.
    st.newLine() << "" << pix << ".g = max(0., " << props.m_evalRev << "(4, " << pix << ".g));";

    // Add the conversion from HSY to RGB.
    if (doRGBToHSY)
    {
        FixedFunctionOpData::Style hsyStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
        switch (style)
        {
        case GRADING_LIN:
            hsyStyle = FixedFunctionOpData::HSY_LIN_TO_RGB;
            break;
        case GRADING_LOG:
            hsyStyle = FixedFunctionOpData::HSY_LOG_TO_RGB;
            break;
        case GRADING_VIDEO:
            hsyStyle = FixedFunctionOpData::HSY_VID_TO_RGB;
            break;
        }
        st.newLine() << "{";
        st.indent();
        ConstFixedFunctionOpDataRcPtr funcOpData = std::make_shared<FixedFunctionOpData>(hsyStyle);
        GetFixedFunctionGPUProcessingText(shaderCreator, st, funcOpData);
        st.dedent();
        st.newLine() << "}";
    }

   if (dyn)
   {
       st.dedent();
       st.newLine() << "}";
   }
}

} // anon

void GetGradingHueCurveGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                        ConstGradingHueCurveOpDataRcPtr & gcData)
{
    const bool dyn = gcData->isDynamic() &&  shaderCreator->getLanguage() != LANGUAGE_OSL_1;
    if (!dyn)
    {
        auto propGC = gcData->getDynamicPropertyInternal();
        if (propGC->getLocalBypass())
        {
            return;
        }
    }

    if (gcData->isDynamic() && shaderCreator->getLanguage() == LANGUAGE_OSL_1)
    {
        std::string msg("The dynamic properties are not yet supported by the 'Open Shading language"\
                        " (OSL)' translation: The '");
        msg += opPrefix;
        msg += "' dynamic property is replaced by a local variable.";

        LogWarning(msg);
    }

    const GradingStyle style = gcData->getStyle();
    const TransformDirection dir = gcData->getDirection();

    GpuShaderText st(shaderCreator->getLanguage());
    st.indent();

    st.newLine() << "";
    st.newLine() << "// Add GradingHueCurve "
                 << TransformDirectionToString(dir) << " processing";
    st.newLine() << "";
    st.newLine() << "{";
    st.indent();

    GCProperties properties;
    SetGCProperties(shaderCreator, dyn, properties);

    auto dynProp = gcData->getDynamicPropertyInternal();

    if (dyn)
    {
        // Add the dynamic property to the shader creator.

        // Property is decoupled.
        auto shaderProp = dynProp->createEditableCopy();
        DynamicPropertyRcPtr newProp = shaderProp;
        shaderCreator->addDynamicProperty(newProp);

        // Add uniforms only if needed.
        AddGCPropertiesUniforms(shaderCreator, shaderProp, properties);

        // Add helper function plus global variables if they are not dynamic.
        AddCurveEvalMethodTextToShaderProgram(shaderCreator, gcData, properties, dyn);
    }
    else
    {
        // Declare the op specific helper function.
        AddCurveEvalMethodTextToShaderProgram(shaderCreator, gcData, properties, dyn);
    }

    const bool doLinToLog = style == GRADING_LIN;
    const bool doRGBToHSY = gcData->getRGBToHSY() == HSY_TRANSFORM_1;
    switch (dir)
    {
    case TRANSFORM_DIR_FORWARD:
        AddGCForwardShader(shaderCreator, st, properties, dyn, doLinToLog, doRGBToHSY,
                           dynProp->getValue()->getDrawCurveOnly(), style);
        break;
    case TRANSFORM_DIR_INVERSE:
        AddGCInverseShader(shaderCreator, st, properties, dyn, doLinToLog, doRGBToHSY,
                           dynProp->getValue()->getDrawCurveOnly(), style);
        break;
    }

    st.dedent();
    st.newLine() << "}";

    st.dedent();
    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

} // OCIO_NAMESPACE
