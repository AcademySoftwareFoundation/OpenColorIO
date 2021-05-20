// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingprimary/GradingPrimaryOpGPU.h"


namespace OCIO_NAMESPACE
{
namespace
{

struct GPProperties
{
    std::string brightness{ "brightness" };
    std::string contrast{ "contrast" };
    std::string gamma{ "gamma" };
    std::string exposure{ "exposure" };
    std::string offset{ "offset" };
    std::string slope{ "slope" };

    std::string pivot{ "pivot" };
    std::string pivotBlack{ "pivotBlack" };
    std::string pivotWhite{ "pivotWhite" };
    std::string clampBlack{ "clampBlack" };
    std::string clampWhite{ "clampWhite" };
    std::string saturation{ "saturation" };

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

void AddUniform(GpuShaderCreatorRcPtr & shaderCreator,
                const GpuShaderCreator::Float3Getter & getter,
                const std::string & name)
{
    // Add the uniform if it does not already exist.
    if (shaderCreator->addUniform(name.c_str(), getter))
    {
        // Declare uniform.
        GpuShaderText stDecl(shaderCreator->getLanguage());
        stDecl.declareUniformFloat3(name);
        shaderCreator->addToDeclareShaderCode(stDecl.string().c_str());
    }
}

static constexpr char opPrefix[] = "grading_primary";

void AddGPLogProperties(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st,
                        ConstGradingPrimaryOpDataRcPtr & gpData,
                        GPProperties & propNames)
{
    auto prop = gpData->getDynamicPropertyInternal();
    if (gpData->isDynamic())
    {
        // Build names. No need to add an index to the name to avoid collisions as the dynamic
        // properties are unique.

        propNames.brightness = BuildResourceName(shaderCreator, opPrefix, propNames.brightness);
        propNames.contrast = BuildResourceName(shaderCreator, opPrefix, propNames.contrast);
        propNames.gamma = BuildResourceName(shaderCreator, opPrefix, propNames.gamma);

        propNames.pivot = BuildResourceName(shaderCreator, opPrefix, propNames.pivot);
        propNames.pivotBlack = BuildResourceName(shaderCreator, opPrefix, propNames.pivotBlack);
        propNames.pivotWhite = BuildResourceName(shaderCreator, opPrefix, propNames.pivotWhite);
        propNames.clampBlack = BuildResourceName(shaderCreator, opPrefix, propNames.clampBlack);
        propNames.clampWhite = BuildResourceName(shaderCreator, opPrefix, propNames.clampWhite);
        propNames.saturation = BuildResourceName(shaderCreator, opPrefix, propNames.saturation);

        propNames.localBypass = BuildResourceName(shaderCreator, opPrefix, propNames.localBypass);

        // Property is decoupled and added to shader creator.
        DynamicPropertyGradingPrimaryImplRcPtr shaderProp = prop->createEditableCopy();
        DynamicPropertyRcPtr newProp = shaderProp;
        shaderCreator->addDynamicProperty(newProp);
        DynamicPropertyGradingPrimaryImpl * primaryProp = shaderProp.get();

        // Use the shader dynamic property to bind the uniforms.
        const auto & value = shaderProp->getValue();

        // Add uniforms if they are not already there.
        const auto getB = std::bind(&DynamicPropertyGradingPrimaryImpl::getBrightness, primaryProp);
        AddUniform(shaderCreator, getB, propNames.brightness);

        const auto getC = std::bind(&DynamicPropertyGradingPrimaryImpl::getContrast, primaryProp);
        AddUniform(shaderCreator, getC, propNames.contrast);

        const auto getG = std::bind(&DynamicPropertyGradingPrimaryImpl::getGamma, primaryProp);
        AddUniform(shaderCreator, getG, propNames.gamma);

        const auto getPVal = std::bind(&DynamicPropertyGradingPrimaryImpl::getPivot, primaryProp);
        AddUniform(shaderCreator, getPVal, propNames.pivot);
        const auto getPBVal = std::bind(&GradingPrimary::m_pivotBlack, &value);
        AddUniform(shaderCreator, getPBVal, propNames.pivotBlack);
        const auto getPWVal = std::bind(&GradingPrimary::m_pivotWhite, &value);
        AddUniform(shaderCreator, getPWVal, propNames.pivotWhite);
        const auto getCBVal = std::bind(&GradingPrimary::m_clampBlack, &value);
        AddUniform(shaderCreator, getCBVal, propNames.clampBlack);
        const auto getCWVal = std::bind(&GradingPrimary::m_clampWhite, &value);
        AddUniform(shaderCreator, getCWVal, propNames.clampWhite);
        const auto getSVal = std::bind(&GradingPrimary::m_saturation, &value);
        AddUniform(shaderCreator, getSVal, propNames.saturation);
        const auto getLBP = std::bind(&DynamicPropertyGradingPrimaryImpl::getLocalBypass, shaderProp.get());
        AddBoolUniform(shaderCreator, getLBP, propNames.localBypass);
    }
    else
    {
        const auto & value = prop->getValue();
        const auto & comp = prop->getComputedValue();

        st.declareFloat3(propNames.brightness, comp.getBrightness());
        st.declareFloat3(propNames.contrast, comp.getContrast());
        st.declareFloat3(propNames.gamma, comp.getGamma());

        st.declareVar(propNames.pivot, static_cast<float>(comp.getPivot()));
        st.declareVar(propNames.pivotBlack, static_cast<float>(value.m_pivotBlack));
        st.declareVar(propNames.pivotWhite, static_cast<float>(value.m_pivotWhite));
        st.declareVar(propNames.clampBlack, static_cast<float>(value.m_clampBlack));
        st.declareVar(propNames.clampWhite, static_cast<float>(value.m_clampWhite));
        st.declareVar(propNames.saturation, static_cast<float>(value.m_saturation));
    }
}

void AddGPLogForwardShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb += " << props.brightness << ";";

    st.newLine() << "outColor.rgb = ( outColor.rgb - " << props.pivot << " ) * " << props.contrast
                 << " + " << props.pivot << ";";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( " << props.gamma << " != " << st.float3Const(1.f) << " )";
    st.newLine() << "{";
    st.indent();
    st.newLine() << st.float3Decl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << st.float3Decl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "outColor.rgb = pow( normalizedOut, " << props.gamma << " ) * scale + "
                         << props.pivotBlack << ";";
    st.dedent();
    st.newLine() << "}";

    st.declareFloat3("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + " << props.saturation << " * (outColor.rgb - luma);";

    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
}

void AddGPLogInverseShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
    st.newLine() << "if (" << props.saturation << " != 0. && " << props.saturation << " != 1.)";
    st.newLine() << "{";
    st.indent();
    st.declareFloat3("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + (outColor.rgb - luma) / " << props.saturation << ";";
    st.dedent();
    st.newLine() << "}";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( " << props.gamma << " != " << st.float3Const(1.f) << " )";
    st.newLine() << "{";
    st.indent();
    st.newLine() << st.float3Decl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << st.float3Decl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "outColor.rgb = pow( normalizedOut, " << props.gamma << " ) * scale + "
                         << props.pivotBlack << ";";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "outColor.rgb = ( outColor.rgb - " << props.pivot << " ) * " << props.contrast
                 << " + " << props.pivot << ";";

    st.newLine() << "outColor.rgb += " << props.brightness << ";";
}

void AddGPLinProperties(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st,
                        ConstGradingPrimaryOpDataRcPtr & gpData,
                        GPProperties & propNames)
{
    auto prop = gpData->getDynamicPropertyInternal();
    if (gpData->isDynamic())
    {
        // Build names. No need to add an index to the name to avoid collisions as the dynamic
        // properties are unique.

        propNames.offset = BuildResourceName(shaderCreator, opPrefix, propNames.offset);
        propNames.exposure = BuildResourceName(shaderCreator, opPrefix, propNames.exposure);
        propNames.contrast = BuildResourceName(shaderCreator, opPrefix, propNames.contrast);

        propNames.pivot = BuildResourceName(shaderCreator, opPrefix, propNames.pivot);
        propNames.clampBlack = BuildResourceName(shaderCreator, opPrefix, propNames.clampBlack);
        propNames.clampWhite = BuildResourceName(shaderCreator, opPrefix, propNames.clampWhite);
        propNames.saturation = BuildResourceName(shaderCreator, opPrefix, propNames.saturation);

        propNames.localBypass = BuildResourceName(shaderCreator, opPrefix, propNames.localBypass);

        // Property is decoupled and added to shader creator.
        DynamicPropertyGradingPrimaryImplRcPtr shaderProp = prop->createEditableCopy();
        DynamicPropertyRcPtr newProp = shaderProp;
        shaderCreator->addDynamicProperty(newProp);
        DynamicPropertyGradingPrimaryImpl * primaryProp = shaderProp.get();

        // Use the shader dynamic property to bind the uniforms.
        const auto & value = shaderProp->getValue();

        const auto getO = std::bind(&DynamicPropertyGradingPrimaryImpl::getOffset, primaryProp);
        AddUniform(shaderCreator, getO, propNames.offset);

        const auto getE = std::bind(&DynamicPropertyGradingPrimaryImpl::getExposure, primaryProp);
        AddUniform(shaderCreator, getE, propNames.exposure);

        const auto getC = std::bind(&DynamicPropertyGradingPrimaryImpl::getContrast, primaryProp);
        AddUniform(shaderCreator, getC, propNames.contrast);

        const auto getPVal = std::bind(&DynamicPropertyGradingPrimaryImpl::getPivot, primaryProp);
        AddUniform(shaderCreator, getPVal, propNames.pivot);
        const auto getCBVal = std::bind(&GradingPrimary::m_clampBlack, &value);
        AddUniform(shaderCreator, getCBVal, propNames.clampBlack);
        const auto getCWVal = std::bind(&GradingPrimary::m_clampWhite, &value);
        AddUniform(shaderCreator, getCWVal, propNames.clampWhite);
        const auto getSVal = std::bind(&GradingPrimary::m_saturation, &value);
        AddUniform(shaderCreator, getSVal, propNames.saturation);
        const auto getLBP = std::bind(&DynamicPropertyGradingPrimaryImpl::getLocalBypass, shaderProp.get());
        AddBoolUniform(shaderCreator, getLBP, propNames.localBypass);
    }
    else
    {
        const auto & value = prop->getValue();
        const auto & comp = prop->getComputedValue();

        st.declareFloat3(propNames.offset, comp.getOffset());
        st.declareFloat3(propNames.exposure, comp.getExposure());
        st.declareFloat3(propNames.contrast, comp.getContrast());

        st.declareVar(propNames.pivot, static_cast<float>(comp.getPivot()));
        st.declareVar(propNames.clampBlack, static_cast<float>(value.m_clampBlack));
        st.declareVar(propNames.clampWhite, static_cast<float>(value.m_clampWhite));
        st.declareVar(propNames.saturation, static_cast<float>(value.m_saturation));
    }
}

void AddGPLinForwardShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb += " << props.offset << ";";

    st.newLine() << "outColor.rgb *= " << props.exposure << ";";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    // Although note that the log-to-lin in Tone Op also prevents out == in.
    st.newLine() << "if ( " << props.contrast << " != " << st.float3Const(1.f) << " )";
    st.newLine() << "{";
    st.indent();

    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << "outColor.rgb = pow( abs(outColor.rgb / " << props.pivot << "), " << props.contrast << " ) * "
                                << "sign(outColor.rgb) * " << props.pivot << ";";
    st.dedent();
    st.newLine() << "}";

    st.declareFloat3("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + " << props.saturation << " * (outColor.rgb - luma);";

    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
}

void AddGPLinInverseShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";

    st.newLine() << "if (" << props.saturation << " != 0. && " << props.saturation << " != 1.)";
    st.newLine() << "{";
    st.indent();
    st.declareFloat3("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + (outColor.rgb - luma) / " << props.saturation << ";";
    st.dedent();
    st.newLine() << "}";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    // Although note that the log-to-lin in Tone Op also prevents out == in.
    st.newLine() << "if ( " << props.contrast << " != " << st.float3Const(1.f) << " )";
    st.newLine() << "{";
    st.indent();
    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << "outColor.rgb = pow( abs(outColor.rgb / " << props.pivot << "), "
                                      << props.contrast << " ) * "
                                << "sign(outColor.rgb) * " << props.pivot << ";";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "outColor.rgb *= " << props.exposure << ";";

    st.newLine() << "outColor.rgb += " << props.offset << ";";
}

void AddGPVideoProperties(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st,
                          ConstGradingPrimaryOpDataRcPtr & gpData,
                          GPProperties & propNames)
{
    auto prop = gpData->getDynamicPropertyInternal();
    if (gpData->isDynamic())
    {
        // Build names. No need to add an index to the name to avoid collisions as the dynamic
        // properties are unique.

        propNames.gamma = BuildResourceName(shaderCreator, opPrefix, propNames.gamma);
        propNames.offset = BuildResourceName(shaderCreator, opPrefix, propNames.offset);
        propNames.slope = BuildResourceName(shaderCreator, opPrefix, propNames.slope);

        propNames.pivotBlack = BuildResourceName(shaderCreator, opPrefix, propNames.pivotBlack);
        propNames.pivotWhite = BuildResourceName(shaderCreator, opPrefix, propNames.pivotWhite);
        propNames.clampBlack = BuildResourceName(shaderCreator, opPrefix, propNames.clampBlack);
        propNames.clampWhite = BuildResourceName(shaderCreator, opPrefix, propNames.clampWhite);
        propNames.saturation = BuildResourceName(shaderCreator, opPrefix, propNames.saturation);

        propNames.localBypass = BuildResourceName(shaderCreator, opPrefix, propNames.localBypass);

        // Property is decoupled and added to shader creator.
        DynamicPropertyGradingPrimaryImplRcPtr shaderProp = prop->createEditableCopy();
        DynamicPropertyRcPtr newProp = shaderProp;
        shaderCreator->addDynamicProperty(newProp);
        DynamicPropertyGradingPrimaryImpl * primaryProp = shaderProp.get();

        // Use the shader dynamic property to bind the uniforms.
        const auto & value = shaderProp->getValue();

        // NB: No need to add an index to the name to avoid collisions as the dynamic properties
        // are unique.

        const auto getG = std::bind(&DynamicPropertyGradingPrimaryImpl::getGamma, primaryProp);
        AddUniform(shaderCreator, getG, propNames.gamma);

        const auto getO = std::bind(&DynamicPropertyGradingPrimaryImpl::getOffset, primaryProp);
        AddUniform(shaderCreator, getO, propNames.offset);

        const auto getS = std::bind(&DynamicPropertyGradingPrimaryImpl::getSlope, primaryProp);
        AddUniform(shaderCreator, getS, propNames.slope);

        const auto getPBVal = std::bind(&GradingPrimary::m_pivotBlack, &value);
        AddUniform(shaderCreator, getPBVal, propNames.pivotBlack);
        const auto getPWVal = std::bind(&GradingPrimary::m_pivotWhite, &value);
        AddUniform(shaderCreator, getPWVal, propNames.pivotWhite);
        const auto getCBVal = std::bind(&GradingPrimary::m_clampBlack, &value);
        AddUniform(shaderCreator, getCBVal, propNames.clampBlack);
        const auto getCWVal = std::bind(&GradingPrimary::m_clampWhite, &value);
        AddUniform(shaderCreator, getCWVal, propNames.clampWhite);
        const auto getSVal = std::bind(&GradingPrimary::m_saturation, &value);
        AddUniform(shaderCreator, getSVal, propNames.saturation);
        const auto getLBP = std::bind(&DynamicPropertyGradingPrimaryImpl::getLocalBypass, shaderProp.get());
        AddBoolUniform(shaderCreator, getLBP, propNames.localBypass);
    }
    else
    {
        const auto & value = prop->getValue();
        const auto & comp = prop->getComputedValue();

        st.declareFloat3(propNames.gamma, comp.getGamma());
        st.declareFloat3(propNames.offset, comp.getOffset());
        st.declareFloat3(propNames.slope, comp.getSlope());

        st.declareVar(propNames.pivotBlack, static_cast<float>(value.m_pivotBlack));
        st.declareVar(propNames.pivotWhite, static_cast<float>(value.m_pivotWhite));
        st.declareVar(propNames.clampBlack, static_cast<float>(value.m_clampBlack));
        st.declareVar(propNames.clampWhite, static_cast<float>(value.m_clampWhite));
        st.declareVar(propNames.saturation, static_cast<float>(value.m_saturation));
    }
}

void AddGPVideoForwardShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb += " << props.offset << ";";
    st.newLine() << "outColor.rgb = ( outColor.rgb - " << props.pivotBlack << " ) * " << props.slope
                               << " + " << props.pivotBlack << ";";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( " << props.gamma << " != " << st.float3Const(1.f) << " )";
    st.newLine() << "{";
    st.indent();
    st.newLine() << st.float3Decl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << st.float3Decl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "  outColor.rgb = pow( normalizedOut, " << props.gamma << " ) * scale + " << props.pivotBlack << ";";
    st.dedent();
    st.newLine() << "}";

    st.declareFloat3("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + " << props.saturation << " * (outColor.rgb - luma);";

    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
}

void AddGPVideoInverseShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";

    st.newLine() << "if (" << props.saturation << " != 0. && " << props.saturation << " != 1.)";
    st.newLine() << "{";
    st.indent();
    st.declareFloat3("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + (outColor.rgb - luma) / " << props.saturation << ";";
    st.dedent();
    st.newLine() << "}";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( " << props.gamma << " != " << st.float3Const(1.f) << " )";
    st.newLine() << "{";
    st.indent();
    st.newLine() << st.float3Decl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << st.float3Decl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "outColor.rgb = pow( normalizedOut, " << props.gamma << " ) * scale + " << props.pivotBlack << ";";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "outColor.rgb = ( outColor.rgb - " << props.pivotBlack << " ) * "<< props.slope
                                <<" + " << props.pivotBlack << ";";
    st.newLine() << "outColor.rgb += " << props.offset << " );";
}
}

void GetGradingPrimaryGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                       ConstGradingPrimaryOpDataRcPtr & gpData)
{
    const bool dyn = gpData->isDynamic();
    if (!dyn)
    {
        auto propGP = gpData->getDynamicPropertyInternal();
        if (propGP->getLocalBypass())
        {
            return;
        }
    }

    const GradingStyle style = gpData->getStyle();
    const TransformDirection dir = gpData->getDirection();

    GpuShaderText st(shaderCreator->getLanguage());
    st.indent();

    st.newLine() << "";
    st.newLine() << "// Add GradingPrimary '"
                 << GradingStyleToString(style) << "' "
                 << TransformDirectionToString(dir) << " processing";
    st.newLine() << "";
    st.newLine() << "{";
    st.indent();

    // Properties holds shader variables names and is initialized with undecorated names suitable
    // for local variables.
    GPProperties properties;
    switch (style)
    {
    case GRADING_LOG:
    {
        AddGPLogProperties(shaderCreator, st, gpData, properties);
        if (dyn)
        {
            st.newLine() << "if (!" << properties.localBypass << ")";
            st.newLine() << "{";
            st.indent();
        }

        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
            AddGPLogForwardShader(st, properties);
            break;
        case TRANSFORM_DIR_INVERSE:
            AddGPLogInverseShader(st, properties);
            break;
        }

        if (dyn)
        {
            st.dedent();
            st.newLine() << "}";
        }
        break;
    }
    case GRADING_LIN:
    {
        AddGPLinProperties(shaderCreator, st, gpData, properties);
        if (dyn)
        {
            st.newLine() << "if (!" << properties.localBypass << ")";
            st.newLine() << "{";
            st.indent();
        }

        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
            AddGPLinForwardShader(st, properties);
            break;
        case TRANSFORM_DIR_INVERSE:
            AddGPLinInverseShader(st, properties);
            break;
        }

        if (dyn)
        {
            st.dedent();
            st.newLine() << "}";
        }
        break;
    }
    case GRADING_VIDEO:
    {
        AddGPVideoProperties(shaderCreator, st, gpData, properties);
        if (dyn)
        {
            st.newLine() << "if (!" << properties.localBypass << ")";
            st.newLine() << "{";
            st.indent();
        }

        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
            AddGPVideoForwardShader(st, properties);
            break;
        case TRANSFORM_DIR_INVERSE:
            AddGPVideoInverseShader(st, properties);
            break;
        }

        if (dyn)
        {
            st.dedent();
            st.newLine() << "}";
        }
        break;
    }
    }
    st.dedent();
    st.newLine() << "}";

    st.dedent();
    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

} // OCIO_NAMESPACE
