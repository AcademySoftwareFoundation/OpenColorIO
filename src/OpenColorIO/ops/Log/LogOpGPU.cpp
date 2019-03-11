/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/Log/LogOpGPU.h"
#include "ops/Log/LogUtils.h"

OCIO_NAMESPACE_ENTER
{
namespace
{

void AddLogShader(GpuShaderDescRcPtr & shaderDesc,
                  ConstLogOpDataRcPtr & logData, float base)
{
    const float minValue = std::numeric_limits<float>::min();

    GpuShaderText st(shaderDesc->getLanguage());
    
    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Log processing";
    st.newLine() << "";

    const char * pix = shaderDesc->getPixelName();

    st.newLine() << pix << ".rgb = max( " << st.vec3fConst(minValue) << ", " << pix << ".rgb);";

    if (base == 2.0f)
    {
        st.newLine() << pix << ".rgb = log2(" << pix << ".rgb);";
    }
    else // base 10
    {
        const float oneOverLog10 = 1.0f / logf(base);
        st.newLine() << pix << ".rgb = log(" << pix << ".rgb) * " << st.vec3fConst(oneOverLog10) << ";";
    }

    shaderDesc->addToFunctionShaderCode(st.string().c_str());
}

void AddAntiLogShader(GpuShaderDescRcPtr & shaderDesc,
                      ConstLogOpDataRcPtr & logData, float base)
{
    GpuShaderText st(shaderDesc->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Anti-Log processing";
    st.newLine() << "";

    const char * pix = shaderDesc->getPixelName();

    st.newLine() << pix << ".rgb = pow( " << st.vec3fConst(base) << ", " << pix <<".rgb );";

    shaderDesc->addToFunctionShaderCode(st.string().c_str());
}
    
void AddLogToLinShader(GpuShaderDescRcPtr & shaderDesc,
                       ConstLogOpDataRcPtr & logData)
{
    const auto & paramsR = logData->getRedParams();
    const auto & paramsG = logData->getGreenParams();
    const auto & paramsB = logData->getBlueParams();
    const double base = logData->getBase();
    GpuShaderText st(shaderDesc->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Log to Lin processing";
    st.newLine() << "{";
    st.indent();

    const char * pix = shaderDesc->getPixelName();

    const float logSlopeInv[3] = { 1.0f / (float)paramsR[LOG_SIDE_SLOPE],
                                   1.0f / (float)paramsG[LOG_SIDE_SLOPE],
                                   1.0f / (float)paramsB[LOG_SIDE_SLOPE] };

    const float linSlopeInv[3] = { 1.0f / (float)paramsR[LIN_SIDE_SLOPE],
                                   1.0f / (float)paramsG[LIN_SIDE_SLOPE],
                                   1.0f / (float)paramsB[LIN_SIDE_SLOPE] };

    st.declareVec3f("log_slopeinv", logSlopeInv[0], logSlopeInv[1], logSlopeInv[2]);
    st.declareVec3f("lin_slopeinv", linSlopeInv[0], linSlopeInv[1], linSlopeInv[2]);
    st.declareVec3f("lin_offset", paramsR[LIN_SIDE_OFFSET], paramsG[LIN_SIDE_OFFSET], paramsB[LIN_SIDE_OFFSET]);
    st.declareVec3f("log_base", base, base, base);
    st.declareVec3f("log_offset", paramsR[LOG_SIDE_OFFSET], paramsG[LOG_SIDE_OFFSET], paramsB[LOG_SIDE_OFFSET]);
    // Decompose into 3 steps:
    // 1) (x - logOffset) * logSlopeInv
    // 2) pow(base, x)
    // 3) linSlopeInv * (x - linOffset)
    st.newLine() << pix << ".rgb = (" << pix << ".rgb - log_offset) * log_slopeinv;";
    st.newLine() << pix << ".rgb = pow(log_base, " << pix << ".rgb);";
    st.newLine() << pix << ".rgb = lin_slopeinv * (" << pix << ".rgb - lin_offset);";
    
    st.dedent();
    st.newLine() << "}";

    shaderDesc->addToFunctionShaderCode(st.string().c_str());
}

void AddLinToLogShader(GpuShaderDescRcPtr & shaderDesc,
                       ConstLogOpDataRcPtr & logData)
{
    // logSlope * log(linSlope * x + linOffset, base) + logOffset
    
    const auto & paramsR = logData->getRedParams();
    const auto & paramsG = logData->getGreenParams();
    const auto & paramsB = logData->getBlueParams();
    const double base = logData->getBase();

    const float minValue = std::numeric_limits<float>::min();

    GpuShaderText st(shaderDesc->getLanguage());

    st.indent();
    st.newLine() << "";
    st.newLine() << "// Add Lin to Log processing";
    st.newLine() << "{";
    st.indent();

    const char * pix = shaderDesc->getPixelName();

    st.declareVec3f("minValue", minValue, minValue, minValue);
    st.declareVec3f("lin_slope", paramsR[LIN_SIDE_SLOPE], paramsG[LIN_SIDE_SLOPE], paramsB[LIN_SIDE_SLOPE]);
    st.declareVec3f("lin_offset", paramsR[LIN_SIDE_OFFSET], paramsG[LIN_SIDE_OFFSET], paramsB[LIN_SIDE_OFFSET]);
    // We account for the change of base by rolling the multiplier in with log slope.
    const float logSlopeNew[3] = { (float)(paramsR[LOG_SIDE_SLOPE] / log(base)),
                                   (float)(paramsG[LOG_SIDE_SLOPE] / log(base)),
                                   (float)(paramsB[LOG_SIDE_SLOPE] / log(base)) };
    st.declareVec3f("log_slope", logSlopeNew[0], logSlopeNew[1], logSlopeNew[2]);
    st.declareVec3f("log_offset", paramsR[LOG_SIDE_OFFSET], paramsG[LOG_SIDE_OFFSET], paramsB[LOG_SIDE_OFFSET]);
    // Decompose into 2 steps:
    // 1) clamp(fltmin, linSlope * x + linOffset)
    // 2) logSlopeNew * log(x) + logOffset
    st.newLine() << pix << ".rgb = max( minValue, (" << pix << ".rgb * lin_slope + lin_offset) );";
    st.newLine() << pix << ".rgb = log_slope * log(" << pix << ".rgb ) + log_offset;";

    st.dedent();
    st.newLine() << "}";

    shaderDesc->addToFunctionShaderCode(st.string().c_str());
}

}

void GetLogGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                            ConstLogOpDataRcPtr & logData)
{
    const TransformDirection dir = logData->getDirection();
    if (logData->isLog2())
    {
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            AddLogShader(shaderDesc, logData, 2.0f);
        }
        else
        {
            AddAntiLogShader(shaderDesc, logData, 2.0f);
        }
    }
    else if (logData->isLog10())
    {
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            AddLogShader(shaderDesc, logData, 10.0f);
        }
        else
        {
            AddAntiLogShader(shaderDesc, logData, 10.0f);
        }
    }
    else
    {
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            AddLinToLogShader(shaderDesc, logData);
        }
        else
        {
            AddLogToLinShader(shaderDesc, logData);
        }
    }

}

}
OCIO_NAMESPACE_EXIT
