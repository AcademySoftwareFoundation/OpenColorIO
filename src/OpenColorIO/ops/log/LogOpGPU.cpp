// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/log/LogOpGPU.h"
#include "ops/log/LogUtils.h"

namespace OCIO_NAMESPACE
{
namespace
{

void AddLogShader(GpuShaderCreatorRcPtr & shaderCreator, ConstLogOpDataRcPtr & logData, float base)
{
    const float minValue = std::numeric_limits<float>::min();

    GpuShaderText st(shaderCreator->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Log processing";
    st.newLine() << "";

    const char * pix = shaderCreator->getPixelName();

    st.newLine() << pix << ".rgb = max( " << st.float3Const(minValue) << ", " << pix << ".rgb);";

    if (base == 2.0f)
    {
        st.newLine() << pix << ".rgb = log2(" << pix << ".rgb);";
    }
    else // base 10
    {
        const float oneOverLog10 = 1.0f / logf(base);
        st.newLine() << pix << ".rgb = log(" << pix << ".rgb) * " << st.float3Const(oneOverLog10) << ";";
    }

    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

void AddAntiLogShader(GpuShaderCreatorRcPtr & shaderCreator, ConstLogOpDataRcPtr & logData, float base)
{
    GpuShaderText st(shaderCreator->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Anti-Log processing";
    st.newLine() << "";

    const char * pix = shaderCreator->getPixelName();

    st.newLine() << pix << ".rgb = pow( " << st.float3Const(base) << ", " << pix <<".rgb );";

    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

void AddLogToLinShader(GpuShaderCreatorRcPtr & shaderCreator, ConstLogOpDataRcPtr & logData)
{
    const auto & paramsR = logData->getRedParams();
    const auto & paramsG = logData->getGreenParams();
    const auto & paramsB = logData->getBlueParams();
    const double base = logData->getBase();
    GpuShaderText st(shaderCreator->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Log to Lin processing";
    st.newLine() << "{";
    st.indent();

    const char * pix = shaderCreator->getPixelName();

    const float logSlopeInv[3] = { 1.0f / (float)paramsR[LOG_SIDE_SLOPE],
                                   1.0f / (float)paramsG[LOG_SIDE_SLOPE],
                                   1.0f / (float)paramsB[LOG_SIDE_SLOPE] };

    const float linSlopeInv[3] = { 1.0f / (float)paramsR[LIN_SIDE_SLOPE],
                                   1.0f / (float)paramsG[LIN_SIDE_SLOPE],
                                   1.0f / (float)paramsB[LIN_SIDE_SLOPE] };

    st.declareFloat3("log_slopeinv", logSlopeInv[0], logSlopeInv[1], logSlopeInv[2]);
    st.declareFloat3("lin_slopeinv", linSlopeInv[0], linSlopeInv[1], linSlopeInv[2]);
    st.declareFloat3("lin_offset", paramsR[LIN_SIDE_OFFSET], paramsG[LIN_SIDE_OFFSET], paramsB[LIN_SIDE_OFFSET]);
    st.declareFloat3("log_base", base, base, base);
    st.declareFloat3("log_offset", paramsR[LOG_SIDE_OFFSET], paramsG[LOG_SIDE_OFFSET], paramsB[LOG_SIDE_OFFSET]);
    // Decompose into 3 steps:
    // 1) (x - logOffset) * logSlopeInv
    // 2) pow(base, x)
    // 3) linSlopeInv * (x - linOffset)
    st.newLine() << pix << ".rgb = (" << pix << ".rgb - log_offset) * log_slopeinv;";
    st.newLine() << pix << ".rgb = pow(log_base, " << pix << ".rgb);";
    st.newLine() << pix << ".rgb = lin_slopeinv * (" << pix << ".rgb - lin_offset);";

    st.dedent();
    st.newLine() << "}";

    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

void AddLinToLogShader(GpuShaderCreatorRcPtr & shaderCreator, ConstLogOpDataRcPtr & logData)
{
    // logSlope * log(linSlope * x + linOffset, base) + logOffset

    const auto & paramsR = logData->getRedParams();
    const auto & paramsG = logData->getGreenParams();
    const auto & paramsB = logData->getBlueParams();
    const double base = logData->getBase();

    const float minValue = std::numeric_limits<float>::min();

    GpuShaderText st(shaderCreator->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Lin to Log processing";
    st.newLine() << "{";
    st.indent();

    const char * pix = shaderCreator->getPixelName();

    st.declareFloat3("minValue", minValue, minValue, minValue);
    st.declareFloat3("lin_slope", paramsR[LIN_SIDE_SLOPE], paramsG[LIN_SIDE_SLOPE], paramsB[LIN_SIDE_SLOPE]);
    st.declareFloat3("lin_offset", paramsR[LIN_SIDE_OFFSET], paramsG[LIN_SIDE_OFFSET], paramsB[LIN_SIDE_OFFSET]);
    // We account for the change of base by rolling the multiplier in with log slope.
    const float logSlopeNew[3] = { (float)(paramsR[LOG_SIDE_SLOPE] / log(base)),
                                   (float)(paramsG[LOG_SIDE_SLOPE] / log(base)),
                                   (float)(paramsB[LOG_SIDE_SLOPE] / log(base)) };
    st.declareFloat3("log_slope", logSlopeNew[0], logSlopeNew[1], logSlopeNew[2]);
    st.declareFloat3("log_offset", paramsR[LOG_SIDE_OFFSET], paramsG[LOG_SIDE_OFFSET], paramsB[LOG_SIDE_OFFSET]);
    // Decompose into 2 steps:
    // 1) clamp(fltmin, linSlope * x + linOffset)
    // 2) logSlopeNew * log(x) + logOffset
    st.newLine() << pix << ".rgb = max( minValue, (" << pix << ".rgb * lin_slope + lin_offset) );";
    st.newLine() << pix << ".rgb = log_slope * log(" << pix << ".rgb ) + log_offset;";

    st.dedent();
    st.newLine() << "}";

    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

void AddCameraLogToLinShader(GpuShaderCreatorRcPtr & shaderCreator,
                             ConstLogOpDataRcPtr & logData)
{
    // if in <= logBreak
    //  out = ( in - linearOffset ) / linearSlope
    // else
    //  out = ( pow( base, (in - logOffset) / logSlope ) - linOffset ) / linSlope;
    //

    const auto & paramsR = logData->getRedParams();
    const auto & paramsG = logData->getGreenParams();
    const auto & paramsB = logData->getBlueParams();
    const double base = logData->getBase();

    float linearSlopeR = LogUtil::GetLinearSlope(paramsR, base);
    float linearSlopeG = LogUtil::GetLinearSlope(paramsG, base);
    float linearSlopeB = LogUtil::GetLinearSlope(paramsB, base);
    float logSideBreakR = LogUtil::GetLogSideBreak(paramsR, base);
    float logSideBreakG = LogUtil::GetLogSideBreak(paramsG, base);
    float logSideBreakB = LogUtil::GetLogSideBreak(paramsB, base);
    float linearOffsetR = LogUtil::GetLinearOffset(paramsR, linearSlopeR, logSideBreakR);
    float linearOffsetG = LogUtil::GetLinearOffset(paramsG, linearSlopeG, logSideBreakG);
    float linearOffsetB = LogUtil::GetLinearOffset(paramsB, linearSlopeB, logSideBreakB);

    const float logSlopeInv[3] = { 1.0f / (float)paramsR[LOG_SIDE_SLOPE],
                                   1.0f / (float)paramsG[LOG_SIDE_SLOPE],
                                   1.0f / (float)paramsB[LOG_SIDE_SLOPE] };

    const float linSlopeInv[3] = { 1.0f / (float)paramsR[LIN_SIDE_SLOPE],
                                   1.0f / (float)paramsG[LIN_SIDE_SLOPE],
                                   1.0f / (float)paramsB[LIN_SIDE_SLOPE] };


    GpuShaderText st(shaderCreator->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Camera Log to Lin processing";
    st.newLine() << "{";
    st.indent();

    const char * pix = shaderCreator->getPixelName();
    const std::string pixrgb = pix + std::string(".rgb");

    st.declareFloat3("log_break", logSideBreakR, logSideBreakG, logSideBreakB);
    st.declareFloat3("linear_segment_offset", linearOffsetR, linearOffsetG, linearOffsetB);
    st.declareFloat3("linear_segment_slopeinv", 1.0f / linearSlopeR,
                                               1.0f / linearSlopeG,
                                               1.0f / linearSlopeB);
    st.declareFloat3("lin_slopeinv", linSlopeInv[0], linSlopeInv[1], linSlopeInv[2]);
    st.declareFloat3("lin_offset", paramsR[LIN_SIDE_OFFSET], paramsG[LIN_SIDE_OFFSET], paramsB[LIN_SIDE_OFFSET]);
    st.declareFloat3("log_slopeinv", logSlopeInv[0], logSlopeInv[1], logSlopeInv[2]);
    st.declareFloat3("log_base", base, base, base);
    st.declareFloat3("log_offset", paramsR[LOG_SIDE_OFFSET], paramsG[LOG_SIDE_OFFSET], paramsB[LOG_SIDE_OFFSET]);

    st.newLine() << st.float3Decl("isAboveBreak") << " = " << st.float3GreaterThan(pixrgb, "log_break") << ";";

    // Compute linear segment.
    st.newLine() << st.float3Decl("linSeg") << " = ( " << pixrgb << " - linear_segment_offset ) * linear_segment_slopeinv;";

    // Decompose log segment into 3 steps:
    // 1) (x - logOffset) * logSlopeInv
    // 2) pow(base, x)
    // 3) linSlopeInv * (x - linOffset)
    st.newLine() << st.float3Decl("logSeg") << " = (" << pixrgb << " - log_offset) * log_slopeinv;";
    st.newLine() << "logSeg = pow(log_base, logSeg);";
    st.newLine() << "logSeg = lin_slopeinv * (logSeg - lin_offset);";

    // Combine linear and log segments.
    st.newLine() << pixrgb << " = isAboveBreak * logSeg + ( " << st.float3Const(1.0f) << " - isAboveBreak ) * linSeg;";

    st.dedent();
    st.newLine() << "}";

    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

void AddCameraLinToLogShader(GpuShaderCreatorRcPtr & shaderCreator,
                             ConstLogOpDataRcPtr & logData)
{
    // if in <= linBreak
    //  out = linearSlope × in + linearOffset 
    // else
    //  out = ( logSlope * log( base, max( minValue, (in*linSlope + linOffset) ) ) + logOffset )

    const auto & paramsR = logData->getRedParams();
    const auto & paramsG = logData->getGreenParams();
    const auto & paramsB = logData->getBlueParams();
    const double base = logData->getBase();

    float linearSlopeR = LogUtil::GetLinearSlope(paramsR, base);
    float linearSlopeG = LogUtil::GetLinearSlope(paramsG, base);
    float linearSlopeB = LogUtil::GetLinearSlope(paramsB, base);
    float logSideBreakR = LogUtil::GetLogSideBreak(paramsR, base);
    float logSideBreakG = LogUtil::GetLogSideBreak(paramsG, base);
    float logSideBreakB = LogUtil::GetLogSideBreak(paramsB, base);
    float linearOffsetR = LogUtil::GetLinearOffset(paramsR, linearSlopeR, logSideBreakR);
    float linearOffsetG = LogUtil::GetLinearOffset(paramsG, linearSlopeG, logSideBreakG);
    float linearOffsetB = LogUtil::GetLinearOffset(paramsB, linearSlopeB, logSideBreakB);

    // We account for the change of base by rolling the multiplier in with log slope.
    const float logSlopeNew[3] = { (float)(paramsR[LOG_SIDE_SLOPE] / log(base)),
                                   (float)(paramsG[LOG_SIDE_SLOPE] / log(base)),
                                   (float)(paramsB[LOG_SIDE_SLOPE] / log(base)) };

    const float minValue = std::numeric_limits<float>::min();

    GpuShaderText st(shaderCreator->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Camera Lin to Log processing";
    st.newLine() << "{";
    st.indent();

    const char * pix = shaderCreator->getPixelName();
    const std::string pixrgb = pix + std::string(".rgb");

    st.declareFloat3("minValue", minValue, minValue, minValue);
    st.declareFloat3("linear_break", paramsR[LIN_SIDE_BREAK], paramsG[LIN_SIDE_BREAK], paramsB[LIN_SIDE_BREAK]);
    st.declareFloat3("linear_segment_slope", linearSlopeR, linearSlopeG, linearSlopeB);
    st.declareFloat3("linear_segment_offset", linearOffsetR, linearOffsetG, linearOffsetB);
    st.declareFloat3("lin_slope", paramsR[LIN_SIDE_SLOPE], paramsG[LIN_SIDE_SLOPE], paramsB[LIN_SIDE_SLOPE]);
    st.declareFloat3("lin_offset", paramsR[LIN_SIDE_OFFSET], paramsG[LIN_SIDE_OFFSET], paramsB[LIN_SIDE_OFFSET]);
    st.declareFloat3("log_slope", logSlopeNew[0], logSlopeNew[1], logSlopeNew[2]);
    st.declareFloat3("log_offset", paramsR[LOG_SIDE_OFFSET], paramsG[LOG_SIDE_OFFSET], paramsB[LOG_SIDE_OFFSET]);

    st.newLine() << st.float3Decl("isAboveBreak") << " = " << st.float3GreaterThan(pixrgb, "linear_break") << ";";

    // Compute linear segment.
    st.newLine() << st.float3Decl("linSeg") << " = " << pixrgb << " * linear_segment_slope + linear_segment_offset;";

    // Decompose log into 2 steps:
    // 1) clamp(fltmin, linSlope * x + linOffset)
    // 2) logSlopeNew * log(x) + logOffset
    st.newLine() << st.float3Decl("logSeg") << " = max( minValue, (" << pixrgb << " * lin_slope + lin_offset) );";
    st.newLine() << "logSeg = log_slope * log( logSeg ) + log_offset;";

    // Combine linear and log segments.
    st.newLine() << pixrgb << " = isAboveBreak * logSeg + ( " << st.float3Const(1.0f) << " - isAboveBreak ) * linSeg;";

    st.dedent();
    st.newLine() << "}";

    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

}

void GetLogGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstLogOpDataRcPtr & logData)
{
    const TransformDirection dir = logData->getDirection();
    if (logData->isLog2())
    {
        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
            AddLogShader(shaderCreator, logData, 2.0f);
            break;
        case TRANSFORM_DIR_INVERSE:
            AddAntiLogShader(shaderCreator, logData, 2.0f);
            break;
        }
    }
    else if (logData->isLog10())
    {
        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
            AddLogShader(shaderCreator, logData, 10.0f);
            break;
        case TRANSFORM_DIR_INVERSE:
            AddAntiLogShader(shaderCreator, logData, 10.0f);
            break;
        }
    }
    else
    {
        if (logData->isCamera())
        {
            switch (dir)
            {
            case TRANSFORM_DIR_FORWARD:
                AddCameraLinToLogShader(shaderCreator, logData);
                break;
            case TRANSFORM_DIR_INVERSE:
                AddCameraLogToLinShader(shaderCreator, logData);
                break;
            }
        }
        else
        {
            switch (dir)
            {
            case TRANSFORM_DIR_FORWARD:
                AddLinToLogShader(shaderCreator, logData);
                break;
            case TRANSFORM_DIR_INVERSE:
                AddLogToLinShader(shaderCreator, logData);
                break;
            }
        }
    }

}

} // namespace OCIO_NAMESPACE
