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
    std::string brightnessR{ "brightnessR" };
    std::string brightnessG{ "brightnessG" };
    std::string brightnessB{ "brightnessB" };
    std::string brightnessM{ "brightnessM" };

    std::string contrastR{ "contrastR" };
    std::string contrastG{ "contrastG" };
    std::string contrastB{ "contrastB" };
    std::string contrastM{ "contrastM" };

    std::string gammaR{ "gammaR" };
    std::string gammaG{ "gammaG" };
    std::string gammaB{ "gammaB" };
    std::string gammaM{ "gammaM" };

    std::string exposureR{ "exposureR" };
    std::string exposureG{ "exposureG" };
    std::string exposureB{ "exposureB" };
    std::string exposureM{ "exposureM" };

    std::string offsetR{ "offsetR" };
    std::string offsetG{ "offsetG" };
    std::string offsetB{ "offsetB" };
    std::string offsetM{ "offsetM" };

    std::string liftR{ "liftR" };
    std::string liftG{ "liftG" };
    std::string liftB{ "liftB" };
    std::string liftM{ "liftM" };

    std::string gainR{ "gainR" };
    std::string gainG{ "gainG" };
    std::string gainB{ "gainB" };
    std::string gainM{ "gainM" };

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

DynamicPropertyGradingPrimaryImplRcPtr GetOrAddShaderProp(GpuShaderCreatorRcPtr & shaderCreator,
                                                          DynamicPropertyGradingPrimaryImplRcPtr & prop)
{
    // Dynamic property is decoupled and added to the shader (or only retrieved if it has
    // already been added by another op).
    DynamicPropertyGradingPrimaryImplRcPtr shaderProp;
    if (shaderCreator->hasDynamicProperty(DYNAMIC_PROPERTY_GRADING_PRIMARY))
    {
        // Dynamic property might have been added for a different type of primary that
        // requires different uniforms using the same dynmaic property.
        auto dp = shaderCreator->getDynamicProperty(DYNAMIC_PROPERTY_GRADING_PRIMARY);
        shaderProp = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyGradingPrimaryImpl>(dp);
    }
    else
    {
        shaderProp = prop->createEditableCopy();
        DynamicPropertyRcPtr newProp = shaderProp;
        shaderCreator->addDynamicProperty(newProp);
    }
    return shaderProp;
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
        // properties are shared i.e. only one instance.

        propNames.brightnessR = BuildResourceName(shaderCreator, opPrefix, propNames.brightnessR);
        propNames.brightnessG = BuildResourceName(shaderCreator, opPrefix, propNames.brightnessG);
        propNames.brightnessB = BuildResourceName(shaderCreator, opPrefix, propNames.brightnessB);
        propNames.brightnessM = BuildResourceName(shaderCreator, opPrefix, propNames.brightnessM);

        propNames.contrastR = BuildResourceName(shaderCreator, opPrefix, propNames.contrastR);
        propNames.contrastG = BuildResourceName(shaderCreator, opPrefix, propNames.contrastG);
        propNames.contrastB = BuildResourceName(shaderCreator, opPrefix, propNames.contrastB);
        propNames.contrastM = BuildResourceName(shaderCreator, opPrefix, propNames.contrastM);

        propNames.gammaR = BuildResourceName(shaderCreator, opPrefix, propNames.gammaR);
        propNames.gammaG = BuildResourceName(shaderCreator, opPrefix, propNames.gammaG);
        propNames.gammaB = BuildResourceName(shaderCreator, opPrefix, propNames.gammaB);
        propNames.gammaM = BuildResourceName(shaderCreator, opPrefix, propNames.gammaM);

        propNames.pivot = BuildResourceName(shaderCreator, opPrefix, propNames.pivot);
        propNames.pivotBlack = BuildResourceName(shaderCreator, opPrefix, propNames.pivotBlack);
        propNames.pivotWhite = BuildResourceName(shaderCreator, opPrefix, propNames.pivotWhite);
        propNames.clampBlack = BuildResourceName(shaderCreator, opPrefix, propNames.clampBlack);
        propNames.clampWhite = BuildResourceName(shaderCreator, opPrefix, propNames.clampWhite);
        propNames.saturation = BuildResourceName(shaderCreator, opPrefix, propNames.saturation);

        propNames.localBypass = BuildResourceName(shaderCreator, opPrefix, propNames.localBypass);

        // Dynamic property is decoupled and added to the shader (or only retrieved if it has
        // already been added by another op). Different style do use different members of the
        // value and thus will require different uniforms.
        DynamicPropertyGradingPrimaryImplRcPtr shaderProp = GetOrAddShaderProp(shaderCreator, prop);

        // Use the shader dynamic property to bind the uniforms.
        const auto & value = shaderProp->getValue();

        // Add uniforms if they are not already there.
        auto getBR = std::bind(&GradingRGBM::m_red,    &value.m_brightness);
        auto getBG = std::bind(&GradingRGBM::m_green,  &value.m_brightness);
        auto getBB = std::bind(&GradingRGBM::m_blue,   &value.m_brightness);
        auto getBM = std::bind(&GradingRGBM::m_master, &value.m_brightness);
        AddUniform(shaderCreator, getBR, propNames.brightnessR);
        AddUniform(shaderCreator, getBG, propNames.brightnessG);
        AddUniform(shaderCreator, getBB, propNames.brightnessB);
        AddUniform(shaderCreator, getBM, propNames.brightnessM);

        auto getCR = std::bind(&GradingRGBM::m_red, &value.m_contrast);
        auto getCG = std::bind(&GradingRGBM::m_green, &value.m_contrast);
        auto getCB = std::bind(&GradingRGBM::m_blue, &value.m_contrast);
        auto getCM = std::bind(&GradingRGBM::m_master, &value.m_contrast);
        AddUniform(shaderCreator, getCR, propNames.contrastR);
        AddUniform(shaderCreator, getCG, propNames.contrastG);
        AddUniform(shaderCreator, getCB, propNames.contrastB);
        AddUniform(shaderCreator, getCM, propNames.contrastM);

        auto getGR = std::bind(&GradingRGBM::m_red, &value.m_gamma);
        auto getGG = std::bind(&GradingRGBM::m_green, &value.m_gamma);
        auto getGB = std::bind(&GradingRGBM::m_blue, &value.m_gamma);
        auto getGM = std::bind(&GradingRGBM::m_master, &value.m_gamma);
        AddUniform(shaderCreator, getGR, propNames.gammaR);
        AddUniform(shaderCreator, getGG, propNames.gammaG);
        AddUniform(shaderCreator, getGB, propNames.gammaB);
        AddUniform(shaderCreator, getGM, propNames.gammaM);

        auto getPVal = std::bind(&GradingPrimary::m_pivot, &value);
        AddUniform(shaderCreator, getPVal, propNames.pivot);
        auto getPBVal = std::bind(&GradingPrimary::m_pivotBlack, &value);
        AddUniform(shaderCreator, getPBVal, propNames.pivotBlack);
        auto getPWVal = std::bind(&GradingPrimary::m_pivotWhite, &value);
        AddUniform(shaderCreator, getPWVal, propNames.pivotWhite);
        auto getCBVal = std::bind(&GradingPrimary::m_clampBlack, &value);
        AddUniform(shaderCreator, getCBVal, propNames.clampBlack);
        auto getCWVal = std::bind(&GradingPrimary::m_clampWhite, &value);
        AddUniform(shaderCreator, getCWVal, propNames.clampWhite);
        auto getSVal = std::bind(&GradingPrimary::m_saturation, &value);
        AddUniform(shaderCreator, getSVal, propNames.saturation);
        auto getLBP = std::bind(&DynamicPropertyGradingPrimaryImpl::getLocalBypass, shaderProp.get());
        AddBoolUniform(shaderCreator, getLBP, propNames.localBypass);
    }
    else
    {
        const auto & value = prop->getValue();

        st.declareVar(propNames.brightnessR, static_cast<float>(value.m_brightness.m_red));
        st.declareVar(propNames.brightnessG, static_cast<float>(value.m_brightness.m_green));
        st.declareVar(propNames.brightnessB, static_cast<float>(value.m_brightness.m_blue));
        st.declareVar(propNames.brightnessM, static_cast<float>(value.m_brightness.m_master));

        st.declareVar(propNames.contrastR, static_cast<float>(value.m_contrast.m_red));
        st.declareVar(propNames.contrastG, static_cast<float>(value.m_contrast.m_green));
        st.declareVar(propNames.contrastB, static_cast<float>(value.m_contrast.m_blue));
        st.declareVar(propNames.contrastM, static_cast<float>(value.m_contrast.m_master));

        st.declareVar(propNames.gammaR, static_cast<float>(value.m_gamma.m_red));
        st.declareVar(propNames.gammaG, static_cast<float>(value.m_gamma.m_green));
        st.declareVar(propNames.gammaB, static_cast<float>(value.m_gamma.m_blue));
        st.declareVar(propNames.gammaM, static_cast<float>(value.m_gamma.m_master));

        st.declareVar(propNames.pivot, static_cast<float>(value.m_pivot));
        st.declareVar(propNames.pivotBlack, static_cast<float>(value.m_pivotBlack));
        st.declareVar(propNames.pivotWhite, static_cast<float>(value.m_pivotWhite));
        st.declareVar(propNames.clampBlack, static_cast<float>(value.m_clampBlack));
        st.declareVar(propNames.clampWhite, static_cast<float>(value.m_clampWhite));
        st.declareVar(propNames.saturation, static_cast<float>(value.m_saturation));
    }
}

void AddGPLogForwardShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << st.vec3fDecl("brightness") << " = " << props.brightnessM << " + "
                 << st.vec3fConst(props.brightnessR, props.brightnessG, props.brightnessB) << ";";
    st.newLine() << "outColor.rgb += brightness * 6.25 / 1023.;";

    st.newLine() << st.vec3fDecl("contrast") << " = " << props.contrastM << " * "
                 << st.vec3fConst(props.contrastR, props.contrastG, props.contrastB) << ";";
    st.newLine() << st.vec3fDecl("gamma") << " = 1.0 / ( " << props.gammaM << " * "
                 << st.vec3fConst(props.gammaR, props.gammaG, props.gammaB) << " );";
    st.newLine() << "float actualPivot = 0.5 + " << props.pivot << " * 0.5;";

    st.newLine() << "outColor.rgb = ( outColor.rgb - actualPivot ) * contrast + actualPivot;";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( gamma.r != 1. || gamma.g != 1. || gamma.b != 1. )";
    st.newLine() << "{";
    st.newLine() << "  " << st.vec3fDecl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << "  " << st.vec3fDecl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "  " << "outColor.rgb = pow( normalizedOut, gamma ) * scale + "
                         << props.pivotBlack << ";";
    st.newLine() << "}";

    st.declareVec3f("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + " << props.saturation << " * (outColor.rgb - luma);";

    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
}

void AddGPLogInverseShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
    st.declareVec3f("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + (outColor.rgb - luma) / " << props.saturation << ";";

    st.newLine() << st.vec3fDecl("gamma") << " = 1.0 / ( " << props.gammaM << " * "
                 << st.vec3fConst(props.gammaR, props.gammaG, props.gammaB) << " );";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( gamma.r != 1. || gamma.g != 1. || gamma.b != 1. )";
    st.newLine() << "{";
    st.newLine() << "  " << st.vec3fDecl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << "  " << st.vec3fDecl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "  " << "outColor.rgb = pow( normalizedOut, 1. / gamma ) * scale + "
                         << props.pivotBlack << ";";
    st.newLine() << "}";

    st.newLine() << st.vec3fDecl("contrast") << " = " << props.contrastM << " * "
                 << st.vec3fConst(props.contrastR, props.contrastG, props.contrastB) << ";";
    st.newLine() << "float actualPivot = 0.5 + " << props.pivot << " * 0.5;";
    st.newLine() << "outColor.rgb = ( outColor.rgb - actualPivot ) / contrast + actualPivot;";

    st.newLine() << st.vec3fDecl("brightness") << " = " << props.brightnessM << " + "
                 << st.vec3fConst(props.brightnessR, props.brightnessG, props.brightnessB) << ";";

    st.newLine() << "outColor.rgb -= brightness * 6.25 / 1023.;";
}

void AddGPLinProperties(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st,
                        ConstGradingPrimaryOpDataRcPtr & gpData,
                        GPProperties & propNames)
{
    auto prop = gpData->getDynamicPropertyInternal();
    if (gpData->isDynamic())
    {
        // Build names. No need to add an index to the name to avoid collisions as the dynamic
        // properties are shared i.e. only one instance.

        propNames.offsetR = BuildResourceName(shaderCreator, opPrefix, propNames.offsetR);
        propNames.offsetG = BuildResourceName(shaderCreator, opPrefix, propNames.offsetG);
        propNames.offsetB = BuildResourceName(shaderCreator, opPrefix, propNames.offsetB);
        propNames.offsetM = BuildResourceName(shaderCreator, opPrefix, propNames.offsetM);

        propNames.exposureR = BuildResourceName(shaderCreator, opPrefix, propNames.exposureR);
        propNames.exposureG = BuildResourceName(shaderCreator, opPrefix, propNames.exposureG);
        propNames.exposureB = BuildResourceName(shaderCreator, opPrefix, propNames.exposureB);
        propNames.exposureM = BuildResourceName(shaderCreator, opPrefix, propNames.exposureM);

        propNames.contrastR = BuildResourceName(shaderCreator, opPrefix, propNames.contrastR);
        propNames.contrastG = BuildResourceName(shaderCreator, opPrefix, propNames.contrastG);
        propNames.contrastB = BuildResourceName(shaderCreator, opPrefix, propNames.contrastB);
        propNames.contrastM = BuildResourceName(shaderCreator, opPrefix, propNames.contrastM);

        propNames.pivot = BuildResourceName(shaderCreator, opPrefix, propNames.pivot);
        propNames.clampBlack = BuildResourceName(shaderCreator, opPrefix, propNames.clampBlack);
        propNames.clampWhite = BuildResourceName(shaderCreator, opPrefix, propNames.clampWhite);
        propNames.saturation = BuildResourceName(shaderCreator, opPrefix, propNames.saturation);

        propNames.localBypass = BuildResourceName(shaderCreator, opPrefix, propNames.localBypass);

        // Dynamic property is decoupled and added to the shader (or only retrieved if it has
        // already been added by another op).
        DynamicPropertyGradingPrimaryImplRcPtr shaderProp = GetOrAddShaderProp(shaderCreator, prop);

        // Use the shader dynamic property to bind the uniforms.
        const auto & value = shaderProp->getValue();

        auto getOR = std::bind(&GradingRGBM::m_red, &value.m_offset);
        auto getOG = std::bind(&GradingRGBM::m_green, &value.m_offset);
        auto getOB = std::bind(&GradingRGBM::m_blue, &value.m_offset);
        auto getOM = std::bind(&GradingRGBM::m_master, &value.m_offset);
        AddUniform(shaderCreator, getOR, propNames.offsetR);
        AddUniform(shaderCreator, getOG, propNames.offsetG);
        AddUniform(shaderCreator, getOB, propNames.offsetB);
        AddUniform(shaderCreator, getOM, propNames.offsetM);

        auto getER = std::bind(&GradingRGBM::m_red, &value.m_exposure);
        auto getEG = std::bind(&GradingRGBM::m_green, &value.m_exposure);
        auto getEB = std::bind(&GradingRGBM::m_blue, &value.m_exposure);
        auto getEM = std::bind(&GradingRGBM::m_master, &value.m_exposure);
        AddUniform(shaderCreator, getER, propNames.exposureR);
        AddUniform(shaderCreator, getEG, propNames.exposureG);
        AddUniform(shaderCreator, getEB, propNames.exposureB);
        AddUniform(shaderCreator, getEM, propNames.exposureM);

        auto getCR = std::bind(&GradingRGBM::m_red, &value.m_contrast);
        auto getCG = std::bind(&GradingRGBM::m_green, &value.m_contrast);
        auto getCB = std::bind(&GradingRGBM::m_blue, &value.m_contrast);
        auto getCM = std::bind(&GradingRGBM::m_master, &value.m_contrast);
        AddUniform(shaderCreator, getCR, propNames.contrastR);
        AddUniform(shaderCreator, getCG, propNames.contrastG);
        AddUniform(shaderCreator, getCB, propNames.contrastB);
        AddUniform(shaderCreator, getCM, propNames.contrastM);

        auto getPVal = std::bind(&GradingPrimary::m_pivot, &value);
        AddUniform(shaderCreator, getPVal, propNames.pivot);
        auto getCBVal = std::bind(&GradingPrimary::m_clampBlack, &value);
        AddUniform(shaderCreator, getCBVal, propNames.clampBlack);
        auto getCWVal = std::bind(&GradingPrimary::m_clampWhite, &value);
        AddUniform(shaderCreator, getCWVal, propNames.clampWhite);
        auto getSVal = std::bind(&GradingPrimary::m_saturation, &value);
        AddUniform(shaderCreator, getSVal, propNames.saturation);
        auto getLBP = std::bind(&DynamicPropertyGradingPrimaryImpl::getLocalBypass, shaderProp.get());
        AddBoolUniform(shaderCreator, getLBP, propNames.localBypass);
    }
    else
    {
        const auto & value = prop->getValue();
        st.declareVar(propNames.offsetR, static_cast<float>(value.m_offset.m_red));
        st.declareVar(propNames.offsetG, static_cast<float>(value.m_offset.m_green));
        st.declareVar(propNames.offsetB, static_cast<float>(value.m_offset.m_blue));
        st.declareVar(propNames.offsetM, static_cast<float>(value.m_offset.m_master));

        st.declareVar(propNames.exposureR, static_cast<float>(value.m_exposure.m_red));
        st.declareVar(propNames.exposureG, static_cast<float>(value.m_exposure.m_green));
        st.declareVar(propNames.exposureB, static_cast<float>(value.m_exposure.m_blue));
        st.declareVar(propNames.exposureM, static_cast<float>(value.m_exposure.m_master));

        st.declareVar(propNames.contrastR, static_cast<float>(value.m_contrast.m_red));
        st.declareVar(propNames.contrastG, static_cast<float>(value.m_contrast.m_green));
        st.declareVar(propNames.contrastB, static_cast<float>(value.m_contrast.m_blue));
        st.declareVar(propNames.contrastM, static_cast<float>(value.m_contrast.m_master));

        st.declareVar(propNames.pivot, static_cast<float>(value.m_pivot));
        st.declareVar(propNames.clampBlack, static_cast<float>(value.m_clampBlack));
        st.declareVar(propNames.clampWhite, static_cast<float>(value.m_clampWhite));
        st.declareVar(propNames.saturation, static_cast<float>(value.m_saturation));
    }
}

void AddGPLinForwardShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb += " << props.offsetM << " + "
                 << st.vec3fConst(props.offsetR, props.offsetG, props.offsetB) << ";";

    st.newLine() << st.vec3fDecl("exposure") << " = " << props.exposureM << " + "
                 << st.vec3fConst(props.exposureR, props.exposureG, props.exposureB) << ";";
    st.newLine() << "outColor.rgb *= pow( " << st.vec3fConst(2.f) << ", exposure );";

    st.newLine() << st.vec3fDecl("contrast") << " = " << props.contrastM << " * "
                 << st.vec3fConst(props.contrastR, props.contrastG, props.contrastB) << ";";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    // Although note that the log-to-lin in Tone Op also prevents out == in.
    st.newLine() << "if ( contrast.r != 1. || contrast.g != 1. || contrast.b != 1. )";
    st.newLine() << "{";
    // TODO: move pow() out of shader.
    st.newLine() << "  float actualPivot = 0.18 * pow( 2., " << props.pivot << " );";

    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << "  outColor.rgb = pow( abs(outColor.rgb / actualPivot), contrast ) * "
                                  << "sign(outColor.rgb) * actualPivot;";
    st.newLine() << "}";

    st.declareVec3f("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + " << props.saturation << " * (outColor.rgb - luma);";

    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
}

void AddGPLinInverseShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";

    st.declareVec3f("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + (outColor.rgb - luma) / " << props.saturation << ";";

    st.newLine() << st.vec3fDecl("contrast") << " = " << props.contrastM << " * "
                 << st.vec3fConst(props.contrastR, props.contrastG, props.contrastB) << ";";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    // Although note that the log-to-lin in Tone Op also prevents out == in.
    st.newLine() << "if ( contrast.r != 1. || contrast.g != 1. || contrast.b != 1. )";
    st.newLine() << "{";
    // TODO: move pow() out of shader.
    st.newLine() << "  float actualPivot = 0.18 * pow( 2., " << props.pivot << " );";

    // NB: The sign(outColor.rgb) is a vec3, preserving the sign of each channel.
    st.newLine() << "  outColor.rgb = pow( abs(outColor.rgb / actualPivot), 1. / contrast ) * "
                                  << "sign(outColor.rgb) * actualPivot;";
    st.newLine() << "}";

    st.newLine() << st.vec3fDecl("exposure") << " = " << props.exposureM << " + "
                 << st.vec3fConst(props.exposureR, props.exposureG, props.exposureB) << ";";
    st.newLine() << "outColor.rgb /= pow( " << st.vec3fConst(2.f) << ", exposure );";

    st.newLine() << "outColor.rgb -= ( " << props.offsetM << " + "
                 << st.vec3fConst(props.offsetR, props.offsetG, props.offsetB) << " );";
}

void AddGPVideoProperties(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st,
                          ConstGradingPrimaryOpDataRcPtr & gpData,
                          GPProperties & propNames)
{
    auto prop = gpData->getDynamicPropertyInternal();
    if (gpData->isDynamic())
    {
        // Build names. No need to add an index to the name to avoid collisions as the dynamic
        // properties are shared i.e. only one instance.
        propNames.liftR = BuildResourceName(shaderCreator, opPrefix, propNames.liftR);
        propNames.liftG = BuildResourceName(shaderCreator, opPrefix, propNames.liftG);
        propNames.liftB = BuildResourceName(shaderCreator, opPrefix, propNames.liftB);
        propNames.liftM = BuildResourceName(shaderCreator, opPrefix, propNames.liftM);

        propNames.gammaR = BuildResourceName(shaderCreator, opPrefix, propNames.gammaR);
        propNames.gammaG = BuildResourceName(shaderCreator, opPrefix, propNames.gammaG);
        propNames.gammaB = BuildResourceName(shaderCreator, opPrefix, propNames.gammaB);
        propNames.gammaM = BuildResourceName(shaderCreator, opPrefix, propNames.gammaM);

        propNames.gainR = BuildResourceName(shaderCreator, opPrefix, propNames.gainR);
        propNames.gainG = BuildResourceName(shaderCreator, opPrefix, propNames.gainG);
        propNames.gainB = BuildResourceName(shaderCreator, opPrefix, propNames.gainB);
        propNames.gainM = BuildResourceName(shaderCreator, opPrefix, propNames.gainM);

        propNames.offsetR = BuildResourceName(shaderCreator, opPrefix, propNames.offsetR);
        propNames.offsetG = BuildResourceName(shaderCreator, opPrefix, propNames.offsetG);
        propNames.offsetB = BuildResourceName(shaderCreator, opPrefix, propNames.offsetB);
        propNames.offsetM = BuildResourceName(shaderCreator, opPrefix, propNames.offsetM);

        propNames.pivotBlack = BuildResourceName(shaderCreator, opPrefix, propNames.pivotBlack);
        propNames.pivotWhite = BuildResourceName(shaderCreator, opPrefix, propNames.pivotWhite);
        propNames.clampBlack = BuildResourceName(shaderCreator, opPrefix, propNames.clampBlack);
        propNames.clampWhite = BuildResourceName(shaderCreator, opPrefix, propNames.clampWhite);
        propNames.saturation = BuildResourceName(shaderCreator, opPrefix, propNames.saturation);

        propNames.localBypass = BuildResourceName(shaderCreator, opPrefix, propNames.localBypass);

        // Dynamic property is decoupled and added to the shader (or only retrieved if it has
        // already been added by another op).
        DynamicPropertyGradingPrimaryImplRcPtr shaderProp = GetOrAddShaderProp(shaderCreator, prop);

        // Use the shader dynamic property to bind the uniforms.
        const auto & value = shaderProp->getValue();

        // NB: No need to add an index to the name to avoid collisions
        //     as the dynamic properties are shared i.e. only one instance.

        auto getLR = std::bind(&GradingRGBM::m_red, &value.m_lift);
        auto getLG = std::bind(&GradingRGBM::m_green, &value.m_lift);
        auto getLB = std::bind(&GradingRGBM::m_blue, &value.m_lift);
        auto getLM = std::bind(&GradingRGBM::m_master, &value.m_lift);
        AddUniform(shaderCreator, getLR, propNames.liftR);
        AddUniform(shaderCreator, getLG, propNames.liftG);
        AddUniform(shaderCreator, getLB, propNames.liftB);
        AddUniform(shaderCreator, getLM, propNames.liftM);

        auto getGR = std::bind(&GradingRGBM::m_red, &value.m_gamma);
        auto getGG = std::bind(&GradingRGBM::m_green, &value.m_gamma);
        auto getGB = std::bind(&GradingRGBM::m_blue, &value.m_gamma);
        auto getGM = std::bind(&GradingRGBM::m_master, &value.m_gamma);
        AddUniform(shaderCreator, getGR, propNames.gammaR);
        AddUniform(shaderCreator, getGG, propNames.gammaG);
        AddUniform(shaderCreator, getGB, propNames.gammaB);
        AddUniform(shaderCreator, getGM, propNames.gammaM);

        auto getGAR = std::bind(&GradingRGBM::m_red, &value.m_gain);
        auto getGAG = std::bind(&GradingRGBM::m_green, &value.m_gain);
        auto getGAB = std::bind(&GradingRGBM::m_blue, &value.m_gain);
        auto getGAM = std::bind(&GradingRGBM::m_master, &value.m_gain);
        AddUniform(shaderCreator, getGAR, propNames.gainR);
        AddUniform(shaderCreator, getGAG, propNames.gainG);
        AddUniform(shaderCreator, getGAB, propNames.gainB);
        AddUniform(shaderCreator, getGAM, propNames.gainM);

        auto getOR = std::bind(&GradingRGBM::m_red, &value.m_offset);
        auto getOG = std::bind(&GradingRGBM::m_green, &value.m_offset);
        auto getOB = std::bind(&GradingRGBM::m_blue, &value.m_offset);
        auto getOM = std::bind(&GradingRGBM::m_master, &value.m_offset);
        AddUniform(shaderCreator, getOR, propNames.offsetR);
        AddUniform(shaderCreator, getOG, propNames.offsetG);
        AddUniform(shaderCreator, getOB, propNames.offsetB);
        AddUniform(shaderCreator, getOM, propNames.offsetM);

        auto getPBVal = std::bind(&GradingPrimary::m_pivotBlack, &value);
        AddUniform(shaderCreator, getPBVal, propNames.pivotBlack);
        auto getPWVal = std::bind(&GradingPrimary::m_pivotWhite, &value);
        AddUniform(shaderCreator, getPWVal, propNames.pivotWhite);
        auto getCBVal = std::bind(&GradingPrimary::m_clampBlack, &value);
        AddUniform(shaderCreator, getCBVal, propNames.clampBlack);
        auto getCWVal = std::bind(&GradingPrimary::m_clampWhite, &value);
        AddUniform(shaderCreator, getCWVal, propNames.clampWhite);
        auto getSVal = std::bind(&GradingPrimary::m_saturation, &value);
        AddUniform(shaderCreator, getSVal, propNames.saturation);
        auto getLBP = std::bind(&DynamicPropertyGradingPrimaryImpl::getLocalBypass, shaderProp.get());
        AddBoolUniform(shaderCreator, getLBP, propNames.localBypass);
    }
    else
    {
        const auto & value = prop->getValue();

        st.declareVar(propNames.liftR, static_cast<float>(value.m_lift.m_red));
        st.declareVar(propNames.liftG, static_cast<float>(value.m_lift.m_green));
        st.declareVar(propNames.liftB, static_cast<float>(value.m_lift.m_blue));
        st.declareVar(propNames.liftM, static_cast<float>(value.m_lift.m_master));

        st.declareVar(propNames.gammaR, static_cast<float>(value.m_gamma.m_red));
        st.declareVar(propNames.gammaG, static_cast<float>(value.m_gamma.m_green));
        st.declareVar(propNames.gammaB, static_cast<float>(value.m_gamma.m_blue));
        st.declareVar(propNames.gammaM, static_cast<float>(value.m_gamma.m_master));

        st.declareVar(propNames.gainR, static_cast<float>(value.m_gain.m_red));
        st.declareVar(propNames.gainG, static_cast<float>(value.m_gain.m_green));
        st.declareVar(propNames.gainB, static_cast<float>(value.m_gain.m_blue));
        st.declareVar(propNames.gainM, static_cast<float>(value.m_gain.m_master));

        st.declareVar(propNames.offsetR, static_cast<float>(value.m_offset.m_red));
        st.declareVar(propNames.offsetG, static_cast<float>(value.m_offset.m_green));
        st.declareVar(propNames.offsetB, static_cast<float>(value.m_offset.m_blue));
        st.declareVar(propNames.offsetM, static_cast<float>(value.m_offset.m_master));

        st.declareVar(propNames.pivotBlack, static_cast<float>(value.m_pivotBlack));
        st.declareVar(propNames.pivotWhite, static_cast<float>(value.m_pivotWhite));
        st.declareVar(propNames.clampBlack, static_cast<float>(value.m_clampBlack));
        st.declareVar(propNames.clampWhite, static_cast<float>(value.m_clampWhite));
        st.declareVar(propNames.saturation, static_cast<float>(value.m_saturation));
    }
}

void AddGPVideoForwardShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << st.vec3fDecl("lift") << " = " << props.liftM << " + "
                 << st.vec3fConst(props.liftR, props.liftG, props.liftB) << ";";
    st.newLine() << st.vec3fDecl("offset") << " = " << props.offsetM << " + "
                 << st.vec3fConst(props.offsetR, props.offsetG, props.offsetB) << ";";
    st.newLine() << st.vec3fDecl("gain") << " = " << props.gainM << " * "
                 << st.vec3fConst(props.gainR, props.gainG, props.gainB) << ";";
    st.newLine() << st.vec3fDecl("gamma") << " = 1.0 / ( " << props.gammaM << " * "
                 << st.vec3fConst(props.gammaR, props.gammaG, props.gammaB) << " );";

    st.newLine() << st.vec3fDecl("slope") << " = (" << props.pivotWhite << " - " << props.pivotBlack << ")"
                 << " / ( " << props.pivotWhite << " / gain + (lift - " << props.pivotBlack << ") );";

    st.newLine() << "outColor.rgb += ( lift + offset );";
    st.newLine() << "outColor.rgb = ( outColor.rgb - " << props.pivotBlack << " ) * slope + " << props.pivotBlack << ";";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( gamma.r != 1. || gamma.g != 1. || gamma.b != 1. )";
    st.newLine() << "{";
    st.newLine() << "  " << st.vec3fDecl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "  " << st.vec3fDecl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "  outColor.rgb = pow( normalizedOut, gamma ) * scale + " << props.pivotBlack << ";";
    st.newLine() << "}";

    st.declareVec3f("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + " << props.saturation << " * (outColor.rgb - luma);";

    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";
}

void AddGPVideoInverseShader(GpuShaderText & st, const GPProperties & props)
{
    st.newLine() << st.vec3fDecl("lift") << " = " << props.liftM << " + "
                 << st.vec3fConst(props.liftR, props.liftG, props.liftB) << ";";
    st.newLine() << st.vec3fDecl("offset") << " = " << props.offsetM << " + "
                 << st.vec3fConst(props.offsetR, props.offsetG, props.offsetB) << ";";
    st.newLine() << st.vec3fDecl("gain") << " = " << props.gainM << " * "
                 << st.vec3fConst(props.gainR, props.gainG, props.gainB) << ";";
    st.newLine() << st.vec3fDecl("gamma") << " = " << props.gammaM << " * "
                 << st.vec3fConst(props.gammaR, props.gammaG, props.gammaB) << ";";

    st.newLine() << st.vec3fDecl("slope") << " = ( " << props.pivotWhite << " / gain + (lift - " << props.pivotBlack << ") )"
                 << " / (" << props.pivotWhite << " - " << props.pivotBlack << ");";

    st.newLine() << "outColor.rgb = clamp( outColor.rgb, " << props.clampBlack << ", "
                                        << props.clampWhite << " );";

    st.declareVec3f("lumaWgts", 0.2126f, 0.7152f, 0.0722f);
    st.newLine() << "float luma = dot( outColor.rgb, lumaWgts );";
    st.newLine() << "outColor.rgb = luma + (outColor.rgb - luma) / " << props.saturation << ";";

    // Not sure if the if helps performance, but it does allow out == in at the default values.
    st.newLine() << "if ( gamma.r != 1. || gamma.g != 1. || gamma.b != 1. )";
    st.newLine() << "{";
    st.newLine() << "  " << st.vec3fDecl("normalizedOut")
                         << " = abs(outColor.rgb - " << props.pivotBlack << ") / "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "  " << st.vec3fDecl("scale")
                         << " = sign(outColor.rgb - " << props.pivotBlack << ") * "
                         << "(" << props.pivotWhite << " - " << props.pivotBlack << ");";
    st.newLine() << "  outColor.rgb = pow( normalizedOut, gamma ) * scale + " << props.pivotBlack << ";";
    st.newLine() << "}";

    st.newLine() << "outColor.rgb = ( outColor.rgb - " << props.pivotBlack << " ) * slope + " << props.pivotBlack << ";";
    st.newLine() << "outColor.rgb -= ( lift + offset );";
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
        AddGPLogProperties(shaderCreator, st, gpData, properties);
        if (dyn)
        {
            st.newLine() << "if (!" << properties.localBypass << ")";
            st.newLine() << "{";
            st.indent();
        }

        if (dir == TRANSFORM_DIR_FORWARD)
        {
            AddGPLogForwardShader(st, properties);
        }
        else
        {
            AddGPLogInverseShader(st, properties);
        }

        if (dyn)
        {
            st.dedent();
            st.newLine() << "}";
        }
        break;
    case GRADING_LIN:
        AddGPLinProperties(shaderCreator, st, gpData, properties);
        if (dyn)
        {
            st.newLine() << "if (!" << properties.localBypass << ")";
            st.newLine() << "{";
            st.indent();
        }

        if (dir == TRANSFORM_DIR_FORWARD)
        {
            AddGPLinForwardShader(st, properties);
        }
        else
        {
            AddGPLinInverseShader(st, properties);
        }

        if (dyn)
        {
            st.dedent();
            st.newLine() << "}";
        }
        break;
    case GRADING_VIDEO:
        AddGPVideoProperties(shaderCreator, st, gpData, properties);
        if (dyn)
        {
            st.newLine() << "if (!" << properties.localBypass << ")";
            st.newLine() << "{";
            st.indent();
        }

        if (dir == TRANSFORM_DIR_FORWARD)
        {
            AddGPVideoForwardShader(st, properties);
        }
        else
        {
            AddGPVideoInverseShader(st, properties);
        }

        if (dyn)
        {
            st.dedent();
            st.newLine() << "}";
        }
        break;
    }
    st.dedent();
    st.newLine() << "}";

    st.dedent();
    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

} // OCIO_NAMESPACE
