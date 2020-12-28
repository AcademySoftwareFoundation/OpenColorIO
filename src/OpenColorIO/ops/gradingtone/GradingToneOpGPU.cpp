// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingtone/GradingToneOpGPU.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
namespace
{

struct GTProperties
{
    std::string blacksR{ "blacksR" };
    std::string blacksG{ "blacksG" };
    std::string blacksB{ "blacksB" };
    std::string blacksM{ "blacksM" };
    std::string blacksS{ "blacksStart" };
    std::string blacksW{ "blacksWidth" };

    std::string shadowsR{ "shadowsR" };
    std::string shadowsG{ "shadowsG" };
    std::string shadowsB{ "shadowsB" };
    std::string shadowsM{ "shadowsM" };
    std::string shadowsS{ "shadowsStart" };
    std::string shadowsW{ "shadowsWidth" };

    std::string midtonesR{ "midtonesR" };
    std::string midtonesG{ "midtonesG" };
    std::string midtonesB{ "midtonesB" };
    std::string midtonesM{ "midtonesM" };
    std::string midtonesS{ "midtonesStart" };
    std::string midtonesW{ "midtonesWidth" };

    std::string highlightsR{ "highlightsR" };
    std::string highlightsG{ "highlightsG" };
    std::string highlightsB{ "highlightsB" };
    std::string highlightsM{ "highlightsM" };
    std::string highlightsS{ "highlightsStart" };
    std::string highlightsW{ "highlightsWidth" };

    std::string whitesR{ "whitesR" };
    std::string whitesG{ "whitesG" };
    std::string whitesB{ "whitesB" };
    std::string whitesM{ "whitesM" };
    std::string whitesS{ "whitesStart" };
    std::string whitesW{ "whitesWidth" };

    std::string sContrast{ "sContrast" };

    std::string localBypass{ "localBypass" };
};

void AddUniform(GpuShaderCreatorRcPtr & shaderCreator,
                const GpuShaderCreator::DoubleGetter & getter,
                const std::string & name)
{
    // Add the uniform if it does not already exist.
    if (shaderCreator->addUniform(name.c_str(), getter))
    {
        // Declare uniform.
        GpuShaderText stDecl(shaderCreator->getLanguage());
        stDecl.declareUniformFloat(name);
        shaderCreator->addToDeclareShaderCode(stDecl.string().c_str());
    }
}

void AddBoolUniform(GpuShaderCreatorRcPtr & shaderCreator,
                    const GpuShaderCreator::BoolGetter & getBool,
                    const std::string & name)
{
    // Add the uniform if it does not already exist.
    if (shaderCreator->addUniform(name.c_str(), getBool))
    {
        // Declare uniform.
        GpuShaderText stDecl(shaderCreator->getLanguage());
        stDecl.declareUniformBool(name);
        shaderCreator->addToDeclareShaderCode(stDecl.string().c_str());
    }
}

void AddGTProperties(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st,
                     ConstGradingToneOpDataRcPtr & gtData,
                     GTProperties & propNames)
{
    auto prop = gtData->getDynamicPropertyInternal();
    if (gtData->isDynamic())
    {
        // Build names. No need to add an index to the name to avoid collisions as the dynamic
        // properties are unique.
        static const std::string opPrefix{ "grading_tone" };

        propNames.blacksR = BuildResourceName(shaderCreator, opPrefix, propNames.blacksR);
        propNames.blacksG = BuildResourceName(shaderCreator, opPrefix, propNames.blacksG);
        propNames.blacksB = BuildResourceName(shaderCreator, opPrefix, propNames.blacksB);
        propNames.blacksM = BuildResourceName(shaderCreator, opPrefix, propNames.blacksM);
        propNames.blacksS = BuildResourceName(shaderCreator, opPrefix, propNames.blacksS);
        propNames.blacksW = BuildResourceName(shaderCreator, opPrefix, propNames.blacksW);

        propNames.shadowsR = BuildResourceName(shaderCreator, opPrefix, propNames.shadowsR);
        propNames.shadowsG = BuildResourceName(shaderCreator, opPrefix, propNames.shadowsG);
        propNames.shadowsB = BuildResourceName(shaderCreator, opPrefix, propNames.shadowsB);
        propNames.shadowsM = BuildResourceName(shaderCreator, opPrefix, propNames.shadowsM);
        propNames.shadowsS = BuildResourceName(shaderCreator, opPrefix, propNames.shadowsS);
        propNames.shadowsW = BuildResourceName(shaderCreator, opPrefix, propNames.shadowsW);

        propNames.midtonesR = BuildResourceName(shaderCreator, opPrefix, propNames.midtonesR);
        propNames.midtonesG = BuildResourceName(shaderCreator, opPrefix, propNames.midtonesG);
        propNames.midtonesB = BuildResourceName(shaderCreator, opPrefix, propNames.midtonesB);
        propNames.midtonesM = BuildResourceName(shaderCreator, opPrefix, propNames.midtonesM);
        propNames.midtonesS = BuildResourceName(shaderCreator, opPrefix, propNames.midtonesS);
        propNames.midtonesW = BuildResourceName(shaderCreator, opPrefix, propNames.midtonesW);

        propNames.highlightsR = BuildResourceName(shaderCreator, opPrefix, propNames.highlightsR);
        propNames.highlightsG = BuildResourceName(shaderCreator, opPrefix, propNames.highlightsG);
        propNames.highlightsB = BuildResourceName(shaderCreator, opPrefix, propNames.highlightsB);
        propNames.highlightsM = BuildResourceName(shaderCreator, opPrefix, propNames.highlightsM);
        propNames.highlightsS = BuildResourceName(shaderCreator, opPrefix, propNames.highlightsS);
        propNames.highlightsW = BuildResourceName(shaderCreator, opPrefix, propNames.highlightsW);

        propNames.whitesR = BuildResourceName(shaderCreator, opPrefix, propNames.whitesR);
        propNames.whitesG = BuildResourceName(shaderCreator, opPrefix, propNames.whitesG);
        propNames.whitesB = BuildResourceName(shaderCreator, opPrefix, propNames.whitesB);
        propNames.whitesM = BuildResourceName(shaderCreator, opPrefix, propNames.whitesM);
        propNames.whitesS = BuildResourceName(shaderCreator, opPrefix, propNames.whitesS);
        propNames.whitesW = BuildResourceName(shaderCreator, opPrefix, propNames.whitesW);

        propNames.sContrast = BuildResourceName(shaderCreator, opPrefix, propNames.sContrast);

        propNames.localBypass = BuildResourceName(shaderCreator, opPrefix, propNames.localBypass);

        // Property is decoupled and added to shader creator.
        DynamicPropertyGradingToneImplRcPtr shaderProp = prop->createEditableCopy();
        DynamicPropertyRcPtr newProp = shaderProp;
        shaderCreator->addDynamicProperty(newProp);

        // Use the shader dynamic property to bind the uniforms.
        const auto & value = shaderProp->getValue();
        using CompVal = GradingTonePreRender;
        const auto & compVal = shaderProp->getComputedValue();

        // Add uniforms if they are not already there.
        auto getBDR = std::bind(&GradingRGBMSW::m_red,    &value.m_blacks);
        auto getBDG = std::bind(&GradingRGBMSW::m_green,  &value.m_blacks);
        auto getBDB = std::bind(&GradingRGBMSW::m_blue,   &value.m_blacks);
        auto getBDM = std::bind(&GradingRGBMSW::m_master, &value.m_blacks);
        auto getBDS = std::bind(&CompVal::m_blacksStart, &compVal);
        auto getBDW = std::bind(&CompVal::m_blacksWidth, &compVal);
        AddUniform(shaderCreator, getBDR, propNames.blacksR);
        AddUniform(shaderCreator, getBDG, propNames.blacksG);
        AddUniform(shaderCreator, getBDB, propNames.blacksB);
        AddUniform(shaderCreator, getBDM, propNames.blacksM);
        AddUniform(shaderCreator, getBDS, propNames.blacksS);
        AddUniform(shaderCreator, getBDW, propNames.blacksW);

        auto getSR = std::bind(&GradingRGBMSW::m_red,    &value.m_shadows);
        auto getSG = std::bind(&GradingRGBMSW::m_green,  &value.m_shadows);
        auto getSB = std::bind(&GradingRGBMSW::m_blue,   &value.m_shadows);
        auto getSM = std::bind(&GradingRGBMSW::m_master, &value.m_shadows);
        auto getSS = std::bind(&CompVal::m_shadowsStart, &compVal);
        auto getSW = std::bind(&CompVal::m_shadowsWidth, &compVal);
        AddUniform(shaderCreator, getSR, propNames.shadowsR);
        AddUniform(shaderCreator, getSG, propNames.shadowsG);
        AddUniform(shaderCreator, getSB, propNames.shadowsB);
        AddUniform(shaderCreator, getSM, propNames.shadowsM);
        AddUniform(shaderCreator, getSS, propNames.shadowsS);
        AddUniform(shaderCreator, getSW, propNames.shadowsW);

        auto getMR = std::bind(&GradingRGBMSW::m_red,    &value.m_midtones);
        auto getMG = std::bind(&GradingRGBMSW::m_green,  &value.m_midtones);
        auto getMB = std::bind(&GradingRGBMSW::m_blue,   &value.m_midtones);
        auto getMM = std::bind(&GradingRGBMSW::m_master, &value.m_midtones);
        auto getMS = std::bind(&GradingRGBMSW::m_start,  &value.m_midtones);
        auto getMW = std::bind(&GradingRGBMSW::m_width,  &value.m_midtones);
        AddUniform(shaderCreator, getMR, propNames.midtonesR);
        AddUniform(shaderCreator, getMG, propNames.midtonesG);
        AddUniform(shaderCreator, getMB, propNames.midtonesB);
        AddUniform(shaderCreator, getMM, propNames.midtonesM);
        AddUniform(shaderCreator, getMS, propNames.midtonesS);
        AddUniform(shaderCreator, getMW, propNames.midtonesW);

        auto getHR = std::bind(&GradingRGBMSW::m_red,    &value.m_highlights);
        auto getHG = std::bind(&GradingRGBMSW::m_green,  &value.m_highlights);
        auto getHB = std::bind(&GradingRGBMSW::m_blue,   &value.m_highlights);
        auto getHM = std::bind(&GradingRGBMSW::m_master, &value.m_highlights);
        auto getHS = std::bind(&CompVal::m_highlightsStart, &compVal);
        auto getHW = std::bind(&CompVal::m_highlightsWidth, &compVal);
        AddUniform(shaderCreator, getHR, propNames.highlightsR);
        AddUniform(shaderCreator, getHG, propNames.highlightsG);
        AddUniform(shaderCreator, getHB, propNames.highlightsB);
        AddUniform(shaderCreator, getHM, propNames.highlightsM);
        AddUniform(shaderCreator, getHS, propNames.highlightsS);
        AddUniform(shaderCreator, getHW, propNames.highlightsW);

        auto getWDR = std::bind(&GradingRGBMSW::m_red,    &value.m_whites);
        auto getWDG = std::bind(&GradingRGBMSW::m_green,  &value.m_whites);
        auto getWDB = std::bind(&GradingRGBMSW::m_blue,   &value.m_whites);
        auto getWDM = std::bind(&GradingRGBMSW::m_master, &value.m_whites);
        auto getWDS = std::bind(&CompVal::m_whitesStart, &compVal);
        auto getWDW = std::bind(&CompVal::m_whitesWidth, &compVal);
        AddUniform(shaderCreator, getWDR, propNames.whitesR);
        AddUniform(shaderCreator, getWDG, propNames.whitesG);
        AddUniform(shaderCreator, getWDB, propNames.whitesB);
        AddUniform(shaderCreator, getWDM, propNames.whitesM);
        AddUniform(shaderCreator, getWDS, propNames.whitesS);
        AddUniform(shaderCreator, getWDW, propNames.whitesW);

        auto getSC = std::bind(&GradingTone::m_scontrast, &value);
        AddUniform(shaderCreator, getSC, propNames.sContrast);

        auto getLB = std::bind(&DynamicPropertyGradingToneImpl::getLocalBypass, shaderProp.get());
        AddBoolUniform(shaderCreator, getLB, propNames.localBypass);
    }
    else
    {
        const auto & value = prop->getValue();
        const auto & compVal = prop->getComputedValue();

        st.declareVar(propNames.blacksR, static_cast<float>(value.m_blacks.m_red));
        st.declareVar(propNames.blacksG, static_cast<float>(value.m_blacks.m_green));
        st.declareVar(propNames.blacksB, static_cast<float>(value.m_blacks.m_blue));
        st.declareVar(propNames.blacksM, static_cast<float>(value.m_blacks.m_master));
        st.declareVar(propNames.blacksS, static_cast<float>(compVal.m_blacksStart));
        st.declareVar(propNames.blacksW, static_cast<float>(compVal.m_blacksWidth));

        st.declareVar(propNames.shadowsR, static_cast<float>(value.m_shadows.m_red));
        st.declareVar(propNames.shadowsG, static_cast<float>(value.m_shadows.m_green));
        st.declareVar(propNames.shadowsB, static_cast<float>(value.m_shadows.m_blue));
        st.declareVar(propNames.shadowsM, static_cast<float>(value.m_shadows.m_master));
        st.declareVar(propNames.shadowsS, static_cast<float>(compVal.m_shadowsStart));
        st.declareVar(propNames.shadowsW, static_cast<float>(compVal.m_shadowsWidth));

        st.declareVar(propNames.midtonesR, static_cast<float>(value.m_midtones.m_red));
        st.declareVar(propNames.midtonesG, static_cast<float>(value.m_midtones.m_green));
        st.declareVar(propNames.midtonesB, static_cast<float>(value.m_midtones.m_blue));
        st.declareVar(propNames.midtonesM, static_cast<float>(value.m_midtones.m_master));
        st.declareVar(propNames.midtonesS, static_cast<float>(value.m_midtones.m_start));
        st.declareVar(propNames.midtonesW, static_cast<float>(value.m_midtones.m_width));

        st.declareVar(propNames.highlightsR, static_cast<float>(value.m_highlights.m_red));
        st.declareVar(propNames.highlightsG, static_cast<float>(value.m_highlights.m_green));
        st.declareVar(propNames.highlightsB, static_cast<float>(value.m_highlights.m_blue));
        st.declareVar(propNames.highlightsM, static_cast<float>(value.m_highlights.m_master));
        st.declareVar(propNames.highlightsS, static_cast<float>(compVal.m_highlightsStart));
        st.declareVar(propNames.highlightsW, static_cast<float>(compVal.m_highlightsWidth));

        st.declareVar(propNames.whitesR, static_cast<float>(value.m_whites.m_red));
        st.declareVar(propNames.whitesG, static_cast<float>(value.m_whites.m_green));
        st.declareVar(propNames.whitesB, static_cast<float>(value.m_whites.m_blue));
        st.declareVar(propNames.whitesM, static_cast<float>(value.m_whites.m_master));
        st.declareVar(propNames.whitesS, static_cast<float>(compVal.m_whitesStart));
        st.declareVar(propNames.whitesW, static_cast<float>(compVal.m_whitesWidth));

        st.declareVar(propNames.sContrast, static_cast<float>(value.m_scontrast));
    }
}

static constexpr unsigned R = 0;
static constexpr unsigned G = 1;
static constexpr unsigned B = 2;
static constexpr unsigned M = 3;

void Add_MidsPre_Shader(unsigned channel, std::string & channelSuffix, GpuShaderText & st,
                        const GTProperties & props, GradingStyle style)
{
    // TODO: Everything in here should move to C++ (doesn't vary per pixel).

    std::string channelValue;
    if (channel == R)
    {
        channelSuffix = "r";
        channelValue = props.midtonesR;
    }
    else if (channel == G)
    {
        channelSuffix = "g";
        channelValue = props.midtonesG;
    }
    else if (channel == B)
    {
        channelSuffix = "b";
        channelValue = props.midtonesB;
    }
    else
    {
        channelSuffix = "rgb";
        channelValue = props.midtonesM;
    }

    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();

    float top{ 0.f }, topSC{ 0.f }, bottom{ 0.f }, pivot{ 0.f };
    GradingTonePreRender::FromStyle(style, top, topSC, bottom, pivot);

    std::string topPoint{ std::to_string(top) }, bottomPoint{ std::to_string(bottom) };

    st.newLine() << "const float halo = 0.4;";
    st.newLine() << "float mid_adj = clamp(" << channelValue << ", 0.01, 1.99);";

    st.newLine() << "if (mid_adj != 1.)";
    st.newLine() << "{";
    st.indent();

    st.newLine() << "const float x0 = " << bottomPoint << ";";
    st.newLine() << "const float x5 = " << topPoint << ";";

    st.newLine() << "const float max_width = (x5 - x0) * 0.95;";
    st.newLine() << "float width = clamp(" << props.midtonesW << ", 0.01, max_width);";
    st.newLine() << "float min_cent = x0 + width * 0.51;";
    st.newLine() << "float max_cent = x5 - width * 0.51;";
    st.newLine() << "float center = clamp(" << props.midtonesS << ", min_cent, max_cent);";

    st.newLine() << "float x1 = center - width * 0.5;";
    st.newLine() << "float x4 = x1 + width;";

    st.newLine() << "float x2 = x1 + (x4 - x1) * 0.25;";
    st.newLine() << "float x3 = x1 + (x4 - x1) * 0.75;";
    st.newLine() << "float y0 = x0;";
    st.newLine() << "const float m0 = 1.;";
    st.newLine() << "const float m5 = 1.;";

    st.newLine() << "const float min_slope = 0.1;";

    st.newLine() << "mid_adj = mid_adj - 1.;";
    st.newLine() << "mid_adj = mid_adj * (1. - min_slope);";

    st.newLine() << "float m2 = 1. + mid_adj;";
    st.newLine() << "float m3 = 1. - mid_adj;";
    st.newLine() << "float m1 = 1. + mid_adj * halo;";
    st.newLine() << "float m4 = 1. - mid_adj * halo;";

    st.newLine() << "if (center <= (x5 + x0) * 0.5)";
    st.newLine() << "{";
    st.newLine() << "  float area = (x1 - x0) * (m1 - m0) * 0.5 + ";
    st.newLine() << "    (x2 - x1) * ((m1 - m0) + (m2 - m1)*0.5) + (center - x2) * (m2 - m0) * 0.5;";
    st.newLine() << "  m4 = ( -0.5*(x5 - x4)*m5 + (x4 - x3) * (0.5*m3 - m5) + ";
    st.newLine() << "    (x3 - center) * (m3 - m5) * 0.5 + area ) / ( -0.5*(x5 - x3) );";
    st.newLine() << "}";
    st.newLine() << "else";
    st.newLine() << "{";
    st.newLine() << "  float area = (x5 - x4) * (m4 - m5) * 0.5 + ";
    st.newLine() << "    (x4 - x3) * ((m4 - m5) + (m3 - m4) * 0.5) + (x3 - center) * (m3 - m5) * 0.5;";
    st.newLine() << "  m1 = ( -0.5*(x1 - x0)*m0 + (x2 - x1) * (0.5*m2 - m0) + ";
    st.newLine() << "    (center - x2) * (m2 - m0) * 0.5 + area ) / ( -0.5*(x2 - x0) );";
    st.newLine() << "}";

    st.newLine() << "float y1 = y0 + (m0 + m1) * (x1 - x0) * 0.5;";
    st.newLine() << "float y2 = y1 + (m1 + m2) * (x2 - x1) * 0.5;";
    st.newLine() << "float y3 = y2 + (m2 + m3) * (x3 - x2) * 0.5;";
    st.newLine() << "float y4 = y3 + (m3 + m4) * (x4 - x3) * 0.5;";
    st.newLine() << "float y5 = y4 + (m4 + m5) * (x5 - x4) * 0.5;";
}

void Add_MidsFwd_Shader(unsigned channel, GpuShaderText & st,
                        const GTProperties & props, GradingStyle style)
{
    std::string channelSuffix;
    Add_MidsPre_Shader(channel, channelSuffix, st, props, style);

    if (channel != M)
    {
        st.newLine() << "float t = outColor." << channelSuffix << ";";
        st.newLine() << "float tL = (t - x0) / (x1 - x0);";
        st.newLine() << "float tM = (t - x1) / (x2 - x1);";
        st.newLine() << "float tR = (t - x2) / (x3 - x2);";
        st.newLine() << "float tR2 = (t - x3) / (x4 - x3);";
        st.newLine() << "float tR3 = (t - x4) / (x5 - x4);";

        st.newLine() << "float fL = tL * (x1 - x0) * ( tL * 0.5 * (m1 - m0) + m0 ) + y0;";
        st.newLine() << "float fM = tM * (x2 - x1) * ( tM * 0.5 * (m2 - m1) + m1 ) + y1;";
        st.newLine() << "float fR = tR * (x3 - x2) * ( tR * 0.5 * (m3 - m2) + m2 ) + y2;";
        st.newLine() << "float fR2 = tR2 * (x4 - x3) * ( tR2 * 0.5 * (m4 - m3) + m3 ) + y3;";
        st.newLine() << "float fR3 = tR3 * (x5 - x4) * ( tR3 * 0.5 * (m5 - m4) + m4 ) + y4;";

        st.newLine() << "float res = (t < x1) ? fL : fM;";
        st.newLine() << "if (t > x2) res = fR;";
        st.newLine() << "if (t > x3) res = fR2;";
        st.newLine() << "if (t > x4) res = fR3;";
        st.newLine() << "if (t < x0) res = y0 + (t - x0) * m0;";
        st.newLine() << "if (t > x5) res = y5 + (t - x5) * m5;";
        st.newLine() << "outColor." << channelSuffix << " = res;";
    }
    else
    {
        st.newLine() << st.float3Decl("t") << " = outColor.rgb;";
        st.newLine() << st.float3Decl("res") << ";";
        st.newLine() << st.float3Decl("tL") << " = (t - x0) / (x1 - x0);";
        st.newLine() << st.float3Decl("tM") << " = (t - x1) / (x2 - x1);";
        st.newLine() << st.float3Decl("tR") << " = (t - x2) / (x3 - x2);";
        st.newLine() << st.float3Decl("tR2") << " = (t - x3) / (x4 - x3);";
        st.newLine() << st.float3Decl("tR3") << " = (t - x4) / (x5 - x4);";

        st.newLine() << st.float3Decl("fL") << " = tL * (x1 - x0) * ( tL * 0.5 * (m1 - m0) + m0 ) + y0;";
        st.newLine() << st.float3Decl("fM") << " = tM * (x2 - x1) * ( tM * 0.5 * (m2 - m1) + m1 ) + y1;";
        st.newLine() << st.float3Decl("fR") << " = tR * (x3 - x2) * ( tR * 0.5 * (m3 - m2) + m2 ) + y2;";
        st.newLine() << st.float3Decl("fR2") << " = tR2 * (x4 - x3) * ( tR2 * 0.5 * (m4 - m3) + m3 ) + y3;";
        st.newLine() << st.float3Decl("fR3") << " = tR3 * (x5 - x4) * ( tR3 * 0.5 * (m5 - m4) + m4 ) + y4;";

        st.newLine() << "res.r = (t.r < x1) ? fL.r : fM.r;";
        st.newLine() << "res.g = (t.g < x1) ? fL.g : fM.g;";
        st.newLine() << "res.b = (t.b < x1) ? fL.b : fM.b;";
        st.newLine() << "res.r = (t.r > x2) ? fR.r : res.r;";
        st.newLine() << "res.g = (t.g > x2) ? fR.g : res.g;";
        st.newLine() << "res.b = (t.b > x2) ? fR.b : res.b;";
        st.newLine() << "res.r = (t.r > x3) ? fR2.r : res.r;";
        st.newLine() << "res.g = (t.g > x3) ? fR2.g : res.g;";
        st.newLine() << "res.b = (t.b > x3) ? fR2.b : res.b;";
        st.newLine() << "res.r = (t.r > x4) ? fR3.r : res.r;";
        st.newLine() << "res.g = (t.g > x4) ? fR3.g : res.g;";
        st.newLine() << "res.b = (t.b > x4) ? fR3.b : res.b;";
        st.newLine() << "res.r = (t.r < x0) ? y0 + (t.r - x0) * m0 : res.r;";
        st.newLine() << "res.g = (t.g < x0) ? y0 + (t.g - x0) * m0 : res.g;";
        st.newLine() << "res.b = (t.b < x0) ? y0 + (t.b - x0) * m0 : res.b;";
        st.newLine() << "res.r = (t.r > x5) ? y5 + (t.r - x5) * m5 : res.r;";
        st.newLine() << "res.g = (t.g > x5) ? y5 + (t.g - x5) * m5 : res.g;";
        st.newLine() << "res.b = (t.b > x5) ? y5 + (t.b - x5) * m5 : res.b;";
        st.newLine() << "outColor.rgb = res;";
    }

    st.dedent();
    st.newLine() << "}";  // if (mid_adj != 1.)

    st.dedent();
    st.newLine() << "}";  // local scope
}

void Add_MidsRev_Shader(unsigned channel, GpuShaderText & st,
                        const GTProperties & props, GradingStyle style)
{
    std::string channelSuffix;
    Add_MidsPre_Shader(channel, channelSuffix, st, props, style);

    if (channel != M)
    {
        st.newLine() << "float t = outColor." << channelSuffix << ";";
        st.newLine() << "float res;";

        st.newLine() << "if (t >= y5)";
        st.newLine() << "{";
        st.indent();
        st.newLine() << "res = x5 + (t - y5) / m5;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "else if (t >= y4)";
        st.newLine() << "{";
        st.indent();
        st.newLine() << "float c = y4 - t;";
        st.newLine() << "float b = m4 * (x5 - x4);";
        st.newLine() << "float a = 0.5 * (m5 - m4) * (x5 - x4);";
        st.newLine() << "float discrim = sqrt(b * b - 4. * a * c);";
        st.newLine() << "float tmp = (-2. * c) / (discrim + b);";
        st.newLine() << "res =  tmp * (x5 - x4) + x4;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "else if (t >= y3)";
        st.newLine() << "{";
        st.indent();
        st.newLine() << "float c = y3 - t;";
        st.newLine() << "float b = m3 * (x4 - x3);";
        st.newLine() << "float a = 0.5 * (m4 - m3) * (x4 - x3);";
        st.newLine() << "float discrim = sqrt(b * b - 4. * a * c);";
        st.newLine() << "float tmp = (-2. * c) / (discrim + b);";
        st.newLine() << "res =  tmp * (x4 - x3) + x3;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "else if (t >= y2)";
        st.newLine() << "{";
        st.indent();
        st.newLine() << "float c = y2 - t;";
        st.newLine() << "float b = m2 * (x3 - x2);";
        st.newLine() << "float a = 0.5 * (m3 - m2) * (x3 - x2);";
        st.newLine() << "float discrim = sqrt(b * b - 4. * a * c);";
        st.newLine() << "float tmp = (-2. * c) / (discrim + b);";
        st.newLine() << "res =  tmp * (x3 - x2) + x2;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "else if (t >= y1)";
        st.newLine() << "{";
        st.indent();
        st.newLine() << "float c = y1 - t;";
        st.newLine() << "float b = m1 * (x2 - x1);";
        st.newLine() << "float a = 0.5 * (m2 - m1) * (x2 - x1);";
        st.newLine() << "float discrim = sqrt(b * b - 4. * a * c);";
        st.newLine() << "float tmp = (-2. * c) / (discrim + b);";
        st.newLine() << "res =  tmp * (x2 - x1) + x1;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "else if (t >= y0)";
        st.newLine() << "{";
        st.indent();
        st.newLine() << "float c = y0 - t;";
        st.newLine() << "float b = m0 * (x1 - x0);";
        st.newLine() << "float a = 0.5 * (m1 - m0) * (x1 - x0);";
        st.newLine() << "float discrim = sqrt(b * b - 4. * a * c);";
        st.newLine() << "float tmp = (-2. * c) / (discrim + b);";
        st.newLine() << "res =  tmp * (x1 - x0) + x0;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "else";
        st.newLine() << "{";
        st.indent();
        st.newLine() << "res = x0 + (t - y0) / m0;";
        st.dedent();
        st.newLine() << "}";

        st.newLine() << "outColor." << channelSuffix << " = res;";
    }
    else
    {
        st.newLine() << st.float3Decl("t") << " = outColor.rgb;";
        st.newLine() << st.float3Decl("outL") << ";";
        st.newLine() << st.float3Decl("outM") << ";";
        st.newLine() << st.float3Decl("outR") << ";";
        st.newLine() << st.float3Decl("outR2") << ";";
        st.newLine() << st.float3Decl("outR3") << ";";

        // TODO: Would probably be better to call the preceding if-block 3 times
        // rather than trying to do a float3 computation here.  Extra computation is
        // done and it still doesn't avoid the if/else.
        st.newLine() << "{";
        st.indent();
        st.newLine() << st.float3Decl("c") << " = y4 - t;";
        st.newLine() << "float b = m4 * (x5 - x4);";
        st.newLine() << "float a = 0.5 * (m5 - m4) * (x5 - x4);";
        st.newLine() << st.float3Decl("discrim") << " = sqrt(b * b - 4. * a * c);";
        st.newLine() << st.float3Decl("tmp") << " = (-2. * c) / (discrim + b);";
        st.newLine() << "outR3 =  tmp * (x5 - x4) + x4;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "{";
        st.indent();
        st.newLine() << st.float3Decl("c") << " = y3 - t;";
        st.newLine() << "float b = m3 * (x4 - x3);";
        st.newLine() << "float a = 0.5 * (m4 - m3) * (x4 - x3);";
        st.newLine() << st.float3Decl("discrim") << " = sqrt(b * b - 4. * a * c);";
        st.newLine() << st.float3Decl("tmp") << " = (-2. * c) / (discrim + b);";
        st.newLine() << "outR2 =  tmp * (x4 - x3) + x3;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "{";
        st.indent();
        st.newLine() << st.float3Decl("c") << " = y2 - t;";
        st.newLine() << "float b = m2 * (x3 - x2);";
        st.newLine() << "float a = 0.5 * (m3 - m2) * (x3 - x2);";
        st.newLine() << st.float3Decl("discrim") << " = sqrt(b * b - 4. * a * c);";
        st.newLine() << st.float3Decl("tmp") << " = (-2. * c) / (discrim + b);";
        st.newLine() << "outR =  tmp * (x3 - x2) + x2;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "{";
        st.indent();
        st.newLine() << st.float3Decl("c") << " = y1 - t;";
        st.newLine() << "float b = m1 * (x2 - x1);";
        st.newLine() << "float a = 0.5 * (m2 - m1) * (x2 - x1);";
        st.newLine() << st.float3Decl("discrim") << " = sqrt(b * b - 4. * a * c);";
        st.newLine() << st.float3Decl("tmp") << " = (-2. * c) / (discrim + b);";
        st.newLine() << "outM =  tmp * (x2 - x1) + x1;";
        st.dedent();
        st.newLine() << "}";
        st.newLine() << "{";
        st.indent();
        st.newLine() << st.float3Decl("c") << " = y0 - t;";
        st.newLine() << "float b = m0 * (x1 - x0);";
        st.newLine() << "float a = 0.5 * (m1 - m0) * (x1 - x0);";
        st.newLine() << st.float3Decl("discrim") << " = sqrt(b * b - 4. * a * c);";
        st.newLine() << st.float3Decl("tmp") << " = (-2. * c) / (discrim + b);";
        st.newLine() << "outL =  tmp * (x1 - x0) + x0;";
        st.dedent();
        st.newLine() << "}";

        st.newLine() << st.float3Decl("res") << ";";
        st.newLine() << "res.r = (t.r < y1) ? outL.r : outM.r;";
        st.newLine() << "res.g = (t.g < y1) ? outL.g : outM.g;";
        st.newLine() << "res.b = (t.b < y1) ? outL.b : outM.b;";
        st.newLine() << "res.r = (t.r > y2) ? outR.r : res.r;";
        st.newLine() << "res.g = (t.g > y2) ? outR.g : res.g;";
        st.newLine() << "res.b = (t.b > y2) ? outR.b : res.b;";
        st.newLine() << "res.r = (t.r > y3) ? outR2.r : res.r;";
        st.newLine() << "res.g = (t.g > y3) ? outR2.g : res.g;";
        st.newLine() << "res.b = (t.b > y3) ? outR2.b : res.b;";
        st.newLine() << "res.r = (t.r > y4) ? outR3.r : res.r;";
        st.newLine() << "res.g = (t.g > y4) ? outR3.g : res.g;";
        st.newLine() << "res.b = (t.b > y4) ? outR3.b : res.b;";
        st.newLine() << "res.r = (t.r < y0) ? x0 + (t.r - y0) * m0 : res.r;";
        st.newLine() << "res.g = (t.g < y0) ? x0 + (t.g - y0) * m0 : res.g;";
        st.newLine() << "res.b = (t.b < y0) ? x0 + (t.b - y0) * m0 : res.b;";
        st.newLine() << "res.r = (t.r > y5) ? x5 + (t.r - y5) * m5 : res.r;";
        st.newLine() << "res.g = (t.g > y5) ? x5 + (t.g - y5) * m5 : res.g;";
        st.newLine() << "res.b = (t.b > y5) ? x5 + (t.b - y5) * m5 : res.b;";
        st.newLine() << "outColor.rgb = res;";
    }

    st.dedent();
    st.newLine() << "}";  // if (mid_adj != 1.)

    st.dedent();
    st.newLine() << "}";  // local scope
}

void Add_HighlightShadowPre_Shader(unsigned channel, std::string & channelSuffix, GpuShaderText & st,
                                   const GTProperties & props, bool isShadow)
{
    // TODO: Everything in here should move to C++ (doesn't vary per pixel).

    std::string channelValue;
    std::string start = isShadow ? props.shadowsS : props.highlightsS;
    std::string pivot = isShadow ? props.shadowsW : props.highlightsW;

    if (channel == R)
    {
        channelSuffix = "r";
        channelValue = isShadow ? props.shadowsR : props.highlightsR;
    }
    else if (channel == G)
    {
        channelSuffix = "g";
        channelValue = isShadow ? props.shadowsG : props.highlightsG;
    }
    else if (channel == B)
    {
        channelSuffix = "b";
        channelValue = isShadow ? props.shadowsB : props.highlightsB;
    }
    else
    {
        channelSuffix = "rgb";
        channelValue = isShadow ? props.shadowsM : props.highlightsM;
    }

    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();
    if (isShadow)
    {
        st.newLine() << "float x0 = " << pivot << ";";
        st.newLine() << "float x2 = " << start << ";";
        st.newLine() << "float m2 = 1.;";
    }
    else
    {
        st.newLine() << "float x0 = " << start << ";";
        st.newLine() << "float x2 = " << pivot << ";";
        st.newLine() << "float m0 = 1.;";
    }
    st.newLine() << "float y0 = x0;";
    st.newLine() << "float y2 = x2;";
    st.newLine() << "float x1 = x0 + (x2 - x0) * 0.5;";

    st.newLine() << "float val = " << channelValue << ";";
    if (!isShadow)
    {
        st.newLine() << "val = 2. - val;";
    }
}

void Add_FauxCubicFwdEval_Shader(unsigned channel, std::string & channelSuffix, GpuShaderText & st)
{
    st.newLine() << "float y1 = ( 0.5 / (x2 - x0) ) * "
                    "( (2.*y0 + m0 * (x1 - x0)) * (x2 - x1) + (2.*y2 - m2 * (x2 - x1)) * (x1 - x0) );";

    if (channel != M)
    {
        st.newLine() << "float t = outColor." << channelSuffix << ";";
        st.newLine() << "float res, tL, tR, fL, fR;";
    }
    else
    {
        st.newLine() << st.float3Decl("t") << " = outColor." << channelSuffix << ";";
        st.newLine() << st.float3Decl("res") << ";";
        st.newLine() << st.float3Decl("tL") << ";";
        st.newLine() << st.float3Decl("tR") << ";";
        st.newLine() << st.float3Decl("fL") << ";";
        st.newLine() << st.float3Decl("fR") << ";";
    }

    st.newLine() << "  tL = (t - x0) / (x1 - x0);";
    st.newLine() << "  tR = (t - x1) / (x2 - x1);";
    st.newLine() << "  fL = y0 * (1. - tL*tL) + y1 * tL*tL + m0 * (1. - tL) * tL * (x1 - x0);";
    st.newLine() << "  fR = y1 * (1. - tR)*(1. - tR) + "
                           "y2 * (2. - tR)*tR + m2 * (tR - 1.)*tR * (x2 - x1);";
    if (channel != M)
    {
        st.newLine() << "  res = (t < x1) ? fL : fR;";
        st.newLine() << "  res = (t < x0) ? y0 + (t - x0) * m0 : res;";
        st.newLine() << "  res = (t > x2) ? y2 + (t - x2) * m2 : res;";
    }
    else
    {
        st.newLine() << "  res.r = (t.r < x1) ? fL.r : fR.r;";
        st.newLine() << "  res.g = (t.g < x1) ? fL.g : fR.g;";
        st.newLine() << "  res.b = (t.b < x1) ? fL.b : fR.b;";
        st.newLine() << "  res.r = (t.r < x0) ? y0 + (t.r - x0) * m0 : res.r;";
        st.newLine() << "  res.g = (t.g < x0) ? y0 + (t.g - x0) * m0 : res.g;";
        st.newLine() << "  res.b = (t.b < x0) ? y0 + (t.b - x0) * m0 : res.b;";
        st.newLine() << "  res.r = (t.r > x2) ? y2 + (t.r - x2) * m2 : res.r;";
        st.newLine() << "  res.g = (t.g > x2) ? y2 + (t.g - x2) * m2 : res.g;";
        st.newLine() << "  res.b = (t.b > x2) ? y2 + (t.b - x2) * m2 : res.b;";
    }
    st.newLine() << "  outColor." << channelSuffix << " = res;";
}

void Add_FauxCubicRevEval_Shader(unsigned channel, std::string & channelSuffix, GpuShaderText & st)
{
    st.newLine() << "float y1 = ( 0.5 / (x2 - x0) ) * "
                    "( (2.*y0 + m0 * (x1 - x0)) * (x2 - x1) + (2.*y2 - m2 * (x2 - x1)) * (x1 - x0) );";

    if (channel != M)
    {
        st.newLine() << "float t = outColor." << channelSuffix << ";";
        st.newLine() << "float res, cL, cR, discrimL, discrimR, outL, outR;";
    }
    else
    {
        st.newLine() << st.float3Decl("t") << " = outColor." << channelSuffix << ";";
        st.newLine() << st.float3Decl("res") << ";";
        st.newLine() << st.float3Decl("cL") << ";";
        st.newLine() << st.float3Decl("cR") << ";";
        st.newLine() << st.float3Decl("discrimL") << ";";
        st.newLine() << st.float3Decl("discrimR") << ";";
        st.newLine() << st.float3Decl("outL") << ";";
        st.newLine() << st.float3Decl("outR") << ";";
    }

    st.newLine() << "  cL = y0 - t;";
    st.newLine() << "  float bL = m0 * (x1 - x0);";
    st.newLine() << "  float aL = y1 - y0 - m0 * (x1 - x0);";
    st.newLine() << "  discrimL = sqrt( bL * bL - 4. * aL * cL );";
    st.newLine() << "  outL = (-2. * cL) / ( discrimL + bL ) * (x1 - x0) + x0;";
    st.newLine() << "  cR = y1 - t;";
    st.newLine() << "  float bR = 2.*y2 - 2.*y1 - m2 * (x2 - x1);";
    st.newLine() << "  float aR = y1 - y2 + m2 * (x2 - x1);";
    st.newLine() << "  discrimR = sqrt( bR * bR - 4. * aR * cR );";
    st.newLine() << "  outR = (-2. * cR) / ( discrimR + bR ) * (x2 - x1) + x1;";
    if (channel != M)
    {
        st.newLine() << "  res = (t < y1) ? outL : outR;";
        st.newLine() << "  res = (t < y0) ? x0 + (t - y0) / m0 : res;";
        st.newLine() << "  res = (t > y2) ? x2 + (t - y2) / m2 : res;";
    }
    else
    {
        st.newLine() << "  res.r = (t.r < y1) ? outL.r : outR.r;";
        st.newLine() << "  res.g = (t.g < y1) ? outL.g : outR.g;";
        st.newLine() << "  res.b = (t.b < y1) ? outL.b : outR.b;";
        st.newLine() << "  res.r = (t.r < y0) ? x0 + (t.r - y0) / m0 : res.r;";
        st.newLine() << "  res.g = (t.g < y0) ? x0 + (t.g - y0) / m0 : res.g;";
        st.newLine() << "  res.b = (t.b < y0) ? x0 + (t.b - y0) / m0 : res.b;";
        st.newLine() << "  res.r = (t.r > y2) ? x2 + (t.r - y2) / m2 : res.r;";
        st.newLine() << "  res.g = (t.g > y2) ? x2 + (t.g - y2) / m2 : res.g;";
        st.newLine() << "  res.b = (t.b > y2) ? x2 + (t.b - y2) / m2 : res.b;";
    }
    st.newLine() << "  outColor." << channelSuffix << " = res;";
}

void Add_HighlightShadowFwd_Shader(unsigned channel, bool isShadow, GpuShaderText & st,
                                   const GTProperties & props)
{
    std::string channelSuffix;
    Add_HighlightShadowPre_Shader(channel, channelSuffix, st, props, isShadow);

    st.newLine() << "if (val < 1.)";
    st.newLine() << "{";
    if (isShadow)
    {
        st.newLine() << "  float m0 = max( 0.01, val );";
    }
    else
    {
        st.newLine() << "  float m2 = max( 0.01, val );";
    }
    Add_FauxCubicFwdEval_Shader(channel, channelSuffix, st);    // Fwd
    st.newLine() << "}";

    st.newLine() << "else if (val > 1.)";
    st.newLine() << "{";
    if (isShadow)
    {
        st.newLine() << "  float m0 = max( 0.01, 2. - val );";
    }
    else
    {
        st.newLine() << "  float m2 = max( 0.01, 2. - val );";
    }
    Add_FauxCubicRevEval_Shader(channel, channelSuffix, st);    // Rev
    st.newLine() << "}";

    st.dedent();
    st.newLine() << "}";  // establish scope
}

void Add_HighlightShadowRev_Shader(unsigned channel, bool isShadow, GpuShaderText & st,
                                   const GTProperties & props)
{
    std::string channelSuffix;
    Add_HighlightShadowPre_Shader(channel, channelSuffix, st, props, isShadow);

    st.newLine() << "if (val < 1.)";
    st.newLine() << "{";
    if (isShadow)
    {
        st.newLine() << "  float m0 = max( 0.01, val );";
    }
    else
    {
        st.newLine() << "  float m2 = max( 0.01, val );";
    }
    Add_FauxCubicRevEval_Shader(channel, channelSuffix, st);    // Rev
    st.newLine() << "}";

    st.newLine() << "else if (val > 1.)";
    st.newLine() << "{";
    if (isShadow)
    {
        st.newLine() << "  float m0 = max( 0.01, 2. - val );";
    }
    else
    {
        st.newLine() << "  float m2 = max( 0.01, 2. - val );";
    }
    Add_FauxCubicFwdEval_Shader(channel, channelSuffix, st);    // Fwd
    st.newLine() << "}";

    st.dedent();
    st.newLine() << "}";  // establish scope
}

void Add_WhiteBlackPre_Shader(unsigned channel, std::string & channelSuffix, bool isBlack,
                              GpuShaderText & st, const GTProperties & props)
{
    std::string channelValue;
    std::string start = isBlack ? props.blacksS : props.whitesS;
    std::string width = isBlack ? props.blacksW : props.whitesW;
    if (channel == R)
    {
        channelSuffix = "r";
        channelValue = isBlack ? props.blacksR : props.whitesR;
    }
    else if (channel == G)
    {
        channelSuffix = "g";
        channelValue = isBlack ? props.blacksG : props.whitesG;
    }
    else if(channel == B)
    {
        channelSuffix = "b";
        channelValue = isBlack ? props.blacksB : props.whitesB;
    }
    else
    {
        channelSuffix = "rgb";
        channelValue = isBlack ? props.blacksM : props.whitesM;
    }

    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();
    if (!isBlack)
    {
        st.newLine() << "float x0 = " << start << ";";
        st.newLine() << "float x1 = x0 + " << width << ";";
        st.newLine() << "const float m0 = 1.;";
        st.newLine() << "float y0 = x0;";
        st.newLine() << "float m1 = " << channelValue << ";";
        st.newLine() << "float mtest = m1;";
    }
    else
    {
        st.newLine() << "float x1 = " << start << ";";
        st.newLine() << "float x0 = x1 - " << width << ";";
        st.newLine() << "const float m1 = 1.;";
        st.newLine() << "float y1 = x1;";
        st.newLine() << "float m0 = " << channelValue << ";";
        st.newLine() << "m0 = 2. - m0;";  // increasing blacks control should lighten
        st.newLine() << "float mtest = m0;";
    }

    if (channel != M)
    {
        st.newLine() << "float t = outColor." << channelSuffix << ";";
    }
    else
    {
        st.newLine() << st.float3Decl("t") << " = outColor.rgb;";
    }
}

void Add_WBFwd_Shader(unsigned channel, std::string & channelSuffix, bool linearExtrap, GpuShaderText & st)
{
    if (channel != M)
    {
        st.newLine() << "  float tlocal = (t - x0) / (x1 - x0);";
        st.newLine() << "  float res = tlocal * (x1 - x0) * ( tlocal * 0.5 * (m1 - m0) + m0 ) + y0;";
        st.newLine() << "  res = (t < x0) ? y0 + (t - x0) * m0 : res;";
    }
    else
    {
        st.newLine() << "  " << st.float3Decl("tlocal") << " = (t - x0) / (x1 - x0);";
        st.newLine() << "  " << st.float3Decl("res") << " = tlocal * (x1 - x0) * ( tlocal * 0.5 * (m1 - m0) + m0 ) + y0;";
        st.newLine() << "  res.r = (t.r < x0) ? y0 + (t.r - x0) * m0 : res.r;";
        st.newLine() << "  res.g = (t.g < x0) ? y0 + (t.g - x0) * m0 : res.g;";
        st.newLine() << "  res.b = (t.b < x0) ? y0 + (t.b - x0) * m0 : res.b;";
    }
    if ( linearExtrap )
    {
        if (channel != M)
        {
            st.newLine() << "  res = (t > x1) ? y1 + (t - x1) * m1 : res;";
        }
        else
        {
            st.newLine() << "  res.r = (t.r > x1) ? y1 + (t.r - x1) * m1 : res.r;";
            st.newLine() << "  res.g = (t.g > x1) ? y1 + (t.g - x1) * m1 : res.g;";
            st.newLine() << "  res.b = (t.b > x1) ? y1 + (t.b - x1) * m1 : res.b;";
        }
    }
}

void Add_WBRev_Shader(unsigned channel, std::string & channelSuffix, bool linearExtrap, GpuShaderText & st)
{
    st.newLine() << "  float a = 0.5 * (m1 - m0) * (x1 - x0);";
    st.newLine() << "  float b = m0 * (x1 - x0);";
    if (channel != M)
    {
        st.newLine() << "  float c = y0 - t;";
        st.newLine() << "  float discrim = sqrt( b * b - 4. * a * c );";
        st.newLine() << "  float tmp = ( -2. * c ) / ( discrim + b );";
        st.newLine() << "  float res = tmp * (x1 - x0) + x0;";
        st.newLine() << "  res = (t < y0) ? x0 + (t - y0) / m0 : res;";
    }
    else
    {
        st.newLine() << "  " << st.float3Decl("c") << " = y0 - t;";
        st.newLine() << "  " << st.float3Decl("discrim") << " = sqrt( b * b - 4. * a * c );";
        st.newLine() << "  " << st.float3Decl("tmp") << " = ( -2. * c ) / ( discrim + b );";
        st.newLine() << "  " << st.float3Decl("res") << " = tmp * (x1 - x0) + x0;";
        st.newLine() << "  res.r = (t.r < y0) ? x0 + (t.r - y0) / m0 : res.r;";
        st.newLine() << "  res.g = (t.g < y0) ? x0 + (t.g - y0) / m0 : res.g;";
        st.newLine() << "  res.b = (t.b < y0) ? x0 + (t.b - y0) / m0 : res.b;";
    }
    if ( linearExtrap )
    {
        if (channel != M)
        {
            st.newLine() << "  res = (t > y1) ? x1 + (t - y1) / m1 : res;";
        }
        else
        {
            // TODO: When m1 = 1., y1=x1, this becomes t.
            st.newLine() << "  res.r = (t.r > y1) ? x1 + (t.r - y1) / m1 : res.r;";
            st.newLine() << "  res.g = (t.g > y1) ? x1 + (t.g - y1) / m1 : res.g;";
            st.newLine() << "  res.b = (t.b > y1) ? x1 + (t.b - y1) / m1 : res.b;";
        }
    }
}

void Add_WBExtrapPre_Shader(GpuShaderText & st)
{
    st.newLine() << "  res = (res - x0) / gain + x0;";
    // Quadratic extrapolation for better HDR control.
    st.newLine() << "  float new_y1 = (x1 - x0) / gain + x0;";
    st.newLine() << "  float xd = x0 + (x1 - x0) * 0.99;";
    st.newLine() << "  float md = m0 + (xd - x0) * (m1 - m0) / (x1 - x0);";
    st.newLine() << "  md = 1. / md;";
    st.newLine() << "  float aa = 0.5 * (1. / m1 - md) / (x1 - xd);";
    st.newLine() << "  float bb = 1. / m1 - 2. * aa * x1;";
    st.newLine() << "  float cc = new_y1 - bb * x1 - aa * x1 * x1;";
    st.newLine() << "  t = (t - x0) / gain + x0;";
}

void Add_WhiteBlackFwd_Shader(unsigned channel, bool isBlack, GpuShaderText & st,
                              const GTProperties & props)
{
    std::string channelSuffix;
    Add_WhiteBlackPre_Shader(channel, channelSuffix, isBlack, st, props);

    // Slope is decreasing case.

    st.newLine() << "if (mtest < 1.)";
    st.newLine() << "{";
    if (!isBlack)
    {
        st.newLine() << "  m1 = max( 0.01, m1 );";
        st.newLine() << "  float y1 = y0 + (m0 + m1) * (x1 - x0) * 0.5;";
    }
    else
    {
        st.newLine() << "  m0 = max( 0.01, m0 );";
        st.newLine() << "  float y0 = y1 - (m0 + m1) * (x1 - x0) * 0.5;";
    }

    Add_WBFwd_Shader(channel, channelSuffix, true, st);

    if (channel != M)
    {
        st.newLine() << "  outColor." << channelSuffix << " = res;";
    }
    else
    {
        st.newLine() << "  outColor.rgb = res;";
    }
    st.newLine() << "}";

    // Slope is increasing case.

    st.newLine() << "else if (mtest > 1.)";
    st.newLine() << "{";
    if (!isBlack)
    {
        st.newLine() << "  m1 = 2. - m1;";
        st.newLine() << "  m1 = max( 0.01, m1 );";
        st.newLine() << "  float gain = (m0 + m1) * 0.5;";
        st.newLine() << "  t = (t - x0) * gain + x0;";
    }
    else
    {
        st.newLine() << "  m0 = 2. - m0;";
        st.newLine() << "  m0 = max( 0.01, m0 );";
        st.newLine() << "  float y0 = y1 - (m0 + m1) * (x1 - x0) * 0.5;";
        st.newLine() << "  float gain = (m0 + m1) * 0.5;";
        st.newLine() << "  t = (t - x1) * gain + x1;";
    }

    Add_WBRev_Shader(channel, channelSuffix, isBlack, st);

    if (!isBlack)
    {
        Add_WBExtrapPre_Shader(st);

        if (channel != M)
        {
            st.newLine() << "  if (t > x1) res = (aa * t  + bb) * t + cc;";
        }
        else
        {
            st.newLine() << "  if (t.r > x1) res.r = (aa * t.r + bb) * t.r + cc;";
            st.newLine() << "  if (t.g > x1) res.g = (aa * t.g + bb) * t.g + cc;";
            st.newLine() << "  if (t.b > x1) res.b = (aa * t.b + bb) * t.b + cc;";
        }
    }
    else
    {
        st.newLine() << "  res = (res - x1) / gain + x1;";
    }

    if (channel != M)
    {
        st.newLine() << "  outColor." << channelSuffix << " = res;";
    }
    else
    {
        st.newLine() << "  outColor.rgb = res;";
    }
    st.newLine() << "}";   // else if (mtest > 1.)

    st.dedent();
    st.newLine() << "}";   // establish scope so local variable names won't conflict
}

void Add_WhiteBlackRev_Shader(unsigned channel, bool isBlack, GpuShaderText & st,
                              const GTProperties & props)
{
    std::string channelSuffix;
    Add_WhiteBlackPre_Shader(channel, channelSuffix, isBlack, st, props);

    // Slope is decreasing case.

    st.newLine() << "if (mtest < 1.)";
    st.newLine() << "{";
    if (!isBlack)
    {
        st.newLine() << "  m1 = max( 0.01, m1 );";
        st.newLine() << "  float y1 = y0 + (m0 + m1) * (x1 - x0) * 0.5;";
    }
    else
    {
        st.newLine() << "  m0 = max( 0.01, m0 );";
        st.newLine() << "  float y0 = y1 - (m0 + m1) * (x1 - x0) * 0.5;";
    }

    Add_WBRev_Shader(channel, channelSuffix, true, st);

    if (channel != M)
    {
        st.newLine() << "  outColor." << channelSuffix << " = res;";
    }
    else
    {
        st.newLine() << "  outColor.rgb = res;";
    }
    st.newLine() << "}";

    // Slope is increasing case.

    st.newLine() << "else if (mtest > 1.)";
    st.newLine() << "{";
    if (!isBlack)
    {
        st.newLine() << "  m1 = 2. - m1;";
        st.newLine() << "  m1 = max( 0.01, m1 );";
        st.newLine() << "  float gain = (m0 + m1) * 0.5;";
        st.newLine() << "  t = (t - x0) * gain + x0;";
    }
    else
    {
        st.newLine() << "  m0 = 2. - m0;";
        st.newLine() << "  m0 = max( 0.01, m0 );";
        st.newLine() << "  float y0 = y1 - (m0 + m1) * (x1 - x0) * 0.5;";
        st.newLine() << "  float gain = (m0 + m1) * 0.5;";
        st.newLine() << "  t = (t - x1) * gain + x1;";
    }

    Add_WBFwd_Shader(channel, channelSuffix, isBlack, st);

    if (!isBlack)
    {
        Add_WBExtrapPre_Shader(st);

        if (channel != M)
        {
            st.newLine() << "  float c = cc - t;";
            st.newLine() << "  float discrim = sqrt( bb * bb - 4. * aa * c );";
            st.newLine() << "  float res1 = ( -2. * c ) / ( discrim + bb );";
            st.newLine() << "  float brk = (aa * x1 + bb) * x1 + cc;";
            st.newLine() << "  res = (t < brk) ? res : res1;";
        }
        else
        {
            st.newLine() << "  " << st.float3Decl("c") << " = cc - t;";
            st.newLine() << "  " << st.float3Decl("discrim") << " = sqrt( bb * bb - 4. * aa * c );";
            st.newLine() << "  " << st.float3Decl("res1") << " = ( -2. * c ) / ( discrim + bb );";
            st.newLine() << "  float brk = (aa * x1 + bb) * x1 + cc;";
            st.newLine() << "  res.r = (t.r < brk) ? res.r : res1.r;";
            st.newLine() << "  res.g = (t.g < brk) ? res.g : res1.g;";
            st.newLine() << "  res.b = (t.b < brk) ? res.b : res1.b;";
        }
    }
    else
    {
        st.newLine() << "  res = (res - x1) / gain + x1;";
    }

    if (channel != M)
    {
        st.newLine() << "  outColor." << channelSuffix << " = res;";
    }
    else
    {
        st.newLine() << "  outColor.rgb = res;";
    }
    st.newLine() << "}";   // else if (mtest > 1.)

    st.dedent();
    st.newLine() << "}";   // establish scope so local variable names won't conflict
}

void Add_SContrastTopPre_Shader(GpuShaderText & st, const GTProperties & props, GradingStyle style)
{
    float top{ 0.f }, topSC{ 0.f }, bottom{ 0.f }, pivot{ 0.f };
    GradingTonePreRender::FromStyle(style, top, topSC, bottom, pivot);
    const std::string topPoint{ std::to_string(topSC) };

    st.newLine() << "float contrast = " << props.sContrast << ";";
    st.newLine() << "if (contrast != 1.)";
    st.newLine() << "{";
    st.indent();

    // Limit the range of values to prevent reversals.
    st.newLine() << "contrast = (contrast > 1.) ? "
                    "1. / (1.8125 - 0.8125 * min( contrast, 1.99 )) : "
                    "0.28125 + 0.71875 * max( contrast, 0.01 );";
    st.newLine() << "const float pivot = " << std::to_string(pivot) << ";";


    st.newLine() << st.float3Decl("t") << " = outColor.rgb;";

    // Top end
    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();
    st.newLine() << "const float x3 = " << topPoint << ";";
    st.newLine() << "const float y3 = " << topPoint << ";";
    st.newLine() << "const float y0 = pivot + (y3 - pivot) * 0.25;";
    st.newLine() << "float m0 = contrast;";
    st.newLine() << "float x0 = pivot + (y0 - pivot) / m0;";
    st.newLine() << "float min_width = (x3 - x0) * 0.3;";
    st.newLine() << "float m3 = 1. / m0;";
    // NB: Due to the if (contrast != 1.) clause above, m0 != m3.
    st.newLine() << "float center = (y3 - y0 - m3*x3 + m0*x0) / (m0 - m3);";
    st.newLine() << "float x1 = x0;";
    st.newLine() << "float x2 = 2. * center - x1;";
    st.newLine() << "if (x2 > x3)";
    st.newLine() << "{";
    st.newLine() << "  x2 = x3;";
    st.newLine() << "  x1 = 2. * center - x2;";
    st.newLine() << "}";
    st.newLine() << "else if ((x2 - x1) < min_width)";
    st.newLine() << "{";
    st.newLine() << "  x2 = x1 + min_width;";
    st.newLine() << "  float new_center = (x2 + x1) * 0.5;";
    st.newLine() << "  m3 = (y3 - y0 + m0*x0 - new_center * m0) / (x3 - new_center);";
    st.newLine() << "}";
    st.newLine() << "float y1 = y0;";
    st.newLine() << "float y2 = y1 + (m0 + m3) * (x2 - x1) * 0.5;";

    // TODO: the above should not be in the GLSL (is not per-pixel)
}

void Add_SContrastBottomPre_Shader(GpuShaderText & st, GradingStyle style)
{
    float top{ 0.f }, topSC{ 0.f }, bottom{ 0.f }, pivot{ 0.f };
    GradingTonePreRender::FromStyle(style, top, topSC, bottom, pivot);
    const std::string bottomPoint{ std::to_string(bottom) };

    // Bottom end
    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.setIndent(4);
    st.newLine() << "const float x0 = " << bottomPoint << ";";
    st.newLine() << "const float y0 = " << bottomPoint << ";";
    st.newLine() << "const float y3 = pivot - (pivot - y0) * 0.25;";
    st.newLine() << "float m3 = contrast;";
    st.newLine() << "float x3 = pivot - (pivot - y3) / m3;";
    st.newLine() << "float min_width = (x3 - x0) * 0.3;";
    st.newLine() << "float m0 = 1. / m3;";
    st.newLine() << "float center = (y3 - y0 - m3*x3 + m0*x0) / (m0 - m3);";
    st.newLine() << "float x2 = x3;";
    st.newLine() << "float x1 = 2. * center - x2;";
    st.newLine() << "if (x1 < x0)";
    st.newLine() << "{";
    st.newLine() << "  x1 = x0;";
    st.newLine() << "  x2 = 2. * center - x1;";
    st.newLine() << "}";
    st.newLine() << "else if ((x2 - x1) < min_width)";
    st.newLine() << "{";
    st.newLine() << "  x1 = x2 - min_width;";
    st.newLine() << "  float new_center = (x2 + x1) * 0.5;";
    st.newLine() << "  m0 = (y3 - y0 - m3*x3 + new_center * m3) / (new_center - x0);";
    st.newLine() << "}";
    st.newLine() << "float y2 = y3;";
    st.newLine() << "float y1 = y2 - (m0 + m3) * (x2 - x1) * 0.5;";

    // TODO: the above should not be in the GLSL (is not per-pixel)
}

void Add_SContrastFwd_Shader(GpuShaderText & st, const GTProperties & props, GradingStyle style)
{
    Add_SContrastTopPre_Shader(st, props, style);

    st.newLine() << "outColor.rgb = (t - pivot) * contrast + pivot;";

    st.newLine() << st.float3Decl("tR") << " = (t - x1) / (x2 - x1);";
    st.newLine() << st.float3Decl("res") << " = tR * (x2 - x1) * ( tR * 0.5 * (m3 - m0) + m0 ) + y1;";

    st.newLine() << "outColor.r = (t.r > x1) ? res.r : outColor.r;";
    st.newLine() << "outColor.g = (t.g > x1) ? res.g : outColor.g;";
    st.newLine() << "outColor.b = (t.b > x1) ? res.b : outColor.b;";
    st.newLine() << "outColor.r = (t.r > x2) ? y2 + (t.r - x2) * m3 : outColor.r;";
    st.newLine() << "outColor.g = (t.g > x2) ? y2 + (t.g - x2) * m3 : outColor.g;";
    st.newLine() << "outColor.b = (t.b > x2) ? y2 + (t.b - x2) * m3 : outColor.b;";
    st.dedent();
    st.newLine() << "}";  // end local scope

    Add_SContrastBottomPre_Shader(st, style);

    st.newLine() << st.float3Decl("tR") << " = (t - x1) / (x2 - x1);";
    st.newLine() << st.float3Decl("res") << " = tR * (x2 - x1) * ( tR * 0.5 * (m3 - m0) + m0 ) + y1;";

    st.newLine() << "outColor.r = (t.r < x2) ? res.r : outColor.r;";
    st.newLine() << "outColor.g = (t.g < x2) ? res.g : outColor.g;";
    st.newLine() << "outColor.b = (t.b < x2) ? res.b : outColor.b;";
    st.newLine() << "outColor.r = (t.r < x1) ? y1 + (t.r - x1) * m0 : outColor.r;";
    st.newLine() << "outColor.g = (t.g < x1) ? y1 + (t.g - x1) * m0 : outColor.g;";
    st.newLine() << "outColor.b = (t.b < x1) ? y1 + (t.b - x1) * m0 : outColor.b;";
    st.dedent();
    st.newLine() << "}";  // end local scope

    st.dedent();
    st.newLine() << "}";  // end if contrast != 1.
}

void Add_SContrastRev_Shader(GpuShaderText & st, const GTProperties & props, GradingStyle style)
{
    Add_SContrastTopPre_Shader(st, props, style);

    st.newLine() << "outColor.rgb = (t - pivot) / contrast + pivot;";

    st.newLine() << st.float3Decl("c") << " = y1 - t;";
    st.newLine() << "float b = m0 * (x2 - x1);";
    st.newLine() << "float a = (m3 - m0) * 0.5 * (x2 - x1);";
    st.newLine() << st.float3Decl("discrim") << " = sqrt( b * b - 4. * a * c );";
    st.newLine() << st.float3Decl("res") << " = (x2 - x1) * (-2. * c) / ( discrim + b ) + x1;";

    st.newLine() << "outColor.r = (t.r > y1) ? res.r : outColor.r;";
    st.newLine() << "outColor.g = (t.g > y1) ? res.g : outColor.g;";
    st.newLine() << "outColor.b = (t.b > y1) ? res.b : outColor.b;";
    st.newLine() << "outColor.r = (t.r > y2) ? x2 + (t.r - y2) / m3 : outColor.r;";
    st.newLine() << "outColor.g = (t.g > y2) ? x2 + (t.g - y2) / m3 : outColor.g;";
    st.newLine() << "outColor.b = (t.b > y2) ? x2 + (t.b - y2) / m3 : outColor.b;";
    st.dedent();
    st.newLine() << "}";  // end local scope

    Add_SContrastBottomPre_Shader(st, style);

    st.newLine() << st.float3Decl("c") << " = y1 - t;";
    st.newLine() << "float b = m0 * (x2 - x1);";
    st.newLine() << "float a = (m3 - m0) * 0.5 * (x2 - x1);";
    st.newLine() << st.float3Decl("discrim") << " = sqrt( b * b - 4. * a * c );";
    st.newLine() << st.float3Decl("res") << " = (x2 - x1) * (-2. * c) / ( discrim + b ) + x1;";

    st.newLine() << "outColor.r = (t.r > y2) ? outColor.r : res.r;";
    st.newLine() << "outColor.g = (t.g > y2) ? outColor.g : res.g;";
    st.newLine() << "outColor.b = (t.b > y2) ? outColor.b : res.b;";
    st.newLine() << "outColor.r = (t.r > y1) ? outColor.r : x1 + (t.r - y1) / m0;";
    st.newLine() << "outColor.g = (t.g > y1) ? outColor.g : x1 + (t.g - y1) / m0;";
    st.newLine() << "outColor.b = (t.b > y1) ? outColor.b : x1 + (t.b - y1) / m0;";
    st.dedent();
    st.newLine() << "}";  // end local scope

    st.dedent();
    st.newLine() << "}";  // end if contrast != 1.
}

void AddGTForwardShader(GpuShaderText & st, const GTProperties & props, GradingStyle style)
{
    if (style == GRADING_LIN)
    {
        // NB:  Although the linToLog and logToLin are correct inverses, the limits of
        // floating-point arithmetic cause errors in the lowest bit of the round trip.
        AddLinToLogShader(st);
    }

    Add_MidsFwd_Shader(R, st, props, style);
    Add_MidsFwd_Shader(G, st, props, style);
    Add_MidsFwd_Shader(B, st, props, style);
    Add_MidsFwd_Shader(M, st, props, style);

    Add_HighlightShadowFwd_Shader(R, false, st, props);
    Add_HighlightShadowFwd_Shader(G, false, st, props);
    Add_HighlightShadowFwd_Shader(B, false, st, props);
    Add_HighlightShadowFwd_Shader(M, false, st, props);

    Add_WhiteBlackFwd_Shader(R, false, st, props);
    Add_WhiteBlackFwd_Shader(G, false, st, props);
    Add_WhiteBlackFwd_Shader(B, false, st, props);
    Add_WhiteBlackFwd_Shader(M, false, st, props);

    Add_HighlightShadowFwd_Shader(R, true, st, props);
    Add_HighlightShadowFwd_Shader(G, true, st, props);
    Add_HighlightShadowFwd_Shader(B, true, st, props);
    Add_HighlightShadowFwd_Shader(M, true, st, props);

    Add_WhiteBlackFwd_Shader(R, true, st, props);
    Add_WhiteBlackFwd_Shader(G, true, st, props);
    Add_WhiteBlackFwd_Shader(B, true, st, props);
    Add_WhiteBlackFwd_Shader(M, true, st, props);

    Add_SContrastFwd_Shader(st, props, style);


    if (style == GRADING_LIN)
    {
        AddLogToLinShader(st);
    }

    // TODO: The grading controls at high values are able to push values above the max half-float
    // at which point they overflow to infinity.  Currently the ACES view transforms make black for
    // Inf but even if it is probably not desirable to output Inf under any circumstances.
    st.newLine() << "outColor = min( outColor, 65504. );";
}

void AddGTInverseShader(GpuShaderText & st, const GTProperties & props, GradingStyle style)
{

    if (style == GRADING_LIN)
    {
        // NB:  Although the linToLog and logToLin are correct inverses, the limits of
        // floating-point arithmetic cause errors in the lowest bit of the round trip.
        AddLinToLogShader(st);
    }

    Add_SContrastRev_Shader(st, props, style);

    Add_WhiteBlackRev_Shader(M, true, st, props);
    Add_WhiteBlackRev_Shader(R, true, st, props);
    Add_WhiteBlackRev_Shader(G, true, st, props);
    Add_WhiteBlackRev_Shader(B, true, st, props);

    Add_HighlightShadowRev_Shader(M, true, st, props);
    Add_HighlightShadowRev_Shader(R, true, st, props);
    Add_HighlightShadowRev_Shader(G, true, st, props);
    Add_HighlightShadowRev_Shader(B, true, st, props);

    Add_WhiteBlackRev_Shader(M, false, st, props);
    Add_WhiteBlackRev_Shader(R, false, st, props);
    Add_WhiteBlackRev_Shader(G, false, st, props);
    Add_WhiteBlackRev_Shader(B, false, st, props);

    Add_HighlightShadowRev_Shader(M, false, st, props);
    Add_HighlightShadowRev_Shader(R, false, st, props);
    Add_HighlightShadowRev_Shader(G, false, st, props);
    Add_HighlightShadowRev_Shader(B, false, st, props);

    Add_MidsRev_Shader(M, st, props, style);
    Add_MidsRev_Shader(R, st, props, style);
    Add_MidsRev_Shader(G, st, props, style);
    Add_MidsRev_Shader(B, st, props, style);

    if (style == GRADING_LIN)
    {
        AddLogToLinShader(st);
    }

    // TODO: The grading controls at high values are able to push values above the max half-float
    // at which point they overflow to infinity.  Currently the ACES view transforms make black for
    // Inf but even if it is probably not desirable to output Inf under any circumstances.
    st.newLine() << "outColor = min( outColor, 65504. );";
}

}

void GetGradingToneGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                    ConstGradingToneOpDataRcPtr & gtData)
{
    const bool dyn = gtData->isDynamic();
    if (!dyn)
    {
        auto propGT = gtData->getDynamicPropertyInternal();
        if (propGT->getLocalBypass())
        {
            return;
        }
    }

    const GradingStyle style = gtData->getStyle();
    const TransformDirection dir = gtData->getDirection();

    GpuShaderText st(shaderCreator->getLanguage());
    st.indent();

    st.newLine() << "";
    st.newLine() << "// Add GradingTone '"
                 << GradingStyleToString(style) << "' "
                 << TransformDirectionToString(dir) << " processing";
    st.newLine() << "";
    st.newLine() << "{";
    st.indent();

    // Properties holds shader variables names and is initialized with undecorated names suitable
    // for local variables.
    GTProperties properties;
    AddGTProperties(shaderCreator, st, gtData, properties);

    if (dyn)
    {
        st.newLine() << "if (!" << properties.localBypass << ")";
        st.newLine() << "{";
        st.indent();
    }

    switch (dir)
    {
    case TRANSFORM_DIR_FORWARD:
        AddGTForwardShader(st, properties, style);
        break;
    case TRANSFORM_DIR_INVERSE:
        AddGTInverseShader(st, properties, style);
        break;
    }

    if (dyn)
    {
        st.dedent();
        st.newLine() << "}";
    }

    st.dedent();
    st.newLine() << "}";

    st.dedent();
    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

} // OCIO_NAMESPACE
