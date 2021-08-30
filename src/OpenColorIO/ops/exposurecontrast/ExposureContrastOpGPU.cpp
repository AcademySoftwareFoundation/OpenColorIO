// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "ops/exposurecontrast/ExposureContrastOpGPU.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
namespace
{

static constexpr char EC_EXPOSURE[] = "exposureVal";
static constexpr char EC_CONTRAST[] = "contrastVal";
static constexpr char EC_GAMMA[]    = "gammaVal";

void AddUniform(GpuShaderCreatorRcPtr & shaderCreator,
                DynamicPropertyDoubleRcPtr prop,
                const std::string & name)
{
    GpuShaderCreator::DoubleGetter getDouble = std::bind(&DynamicPropertyDouble::getValue,
                                                        prop.get());
    shaderCreator->addUniform(name.c_str(), getDouble);
    // Declare uniform.
    GpuShaderText stDecl(shaderCreator->getLanguage());
    stDecl.declareUniformFloat(name);
    shaderCreator->addToDeclareShaderCode(stDecl.string().c_str());
}

std::string AddProperty(GpuShaderCreatorRcPtr & shaderCreator,
                        GpuShaderText & st,
                        DynamicPropertyDoubleImplRcPtr prop,
                        const std::string & name)
{
    std::string finalName;

    if(prop->isDynamic() && shaderCreator->getLanguage() != LANGUAGE_OSL_1)
    {
        // Build the name for the uniform. The same type of property should give the same name, so
        // that uniform is declared only once, but multiple instances of the shader code can
        // reference that name.
        // Note: No need to add an index to the name to avoid collisions as the dynamic properties
        // are unique.
        finalName = BuildResourceName(shaderCreator, "exposure_contrast", name);

        // Property is decoupled and added to shader creator.
        auto shaderProp = prop->createEditableCopy();
        DynamicPropertyRcPtr newProp = shaderProp;
        shaderCreator->addDynamicProperty(newProp);
        auto newPropDouble = DynamicPropertyValue::AsDouble(newProp);

        // Uniform is added, connected to the shader creator instance of the dynamic property.
        AddUniform(shaderCreator, newPropDouble, finalName);
    }
    else
    {
        // Declare a local variable to be used by the shader code.
        finalName = name;
        st.declareVar(finalName, (float)prop->getValue());

        if (shaderCreator->getLanguage() == LANGUAGE_OSL_1 && prop->isDynamic())
        {
            std::string msg("The dynamic properties are not yet supported by the 'Open Shading language"\
                            " (OSL)' translation: The '");
            msg += name;
            msg += "' dynamic property is replaced by a local variable.";

            LogWarning(msg);
        }
    }

    return finalName;
}

void AddProperties(GpuShaderCreatorRcPtr & shaderCreator,
                   GpuShaderText & st,
                   ConstExposureContrastOpDataRcPtr & ec,
                   std::string & exposureName,
                   std::string & contrastName,
                   std::string & gammaName)
{
    exposureName = AddProperty(shaderCreator, st, ec->getExposureProperty(), EC_EXPOSURE);
    contrastName = AddProperty(shaderCreator, st, ec->getContrastProperty(), EC_CONTRAST);
    gammaName    = AddProperty(shaderCreator, st, ec->getGammaProperty(),    EC_GAMMA);
}

void AddECLinearShader(GpuShaderCreatorRcPtr & shaderCreator,
                       GpuShaderText & st,
                       ConstExposureContrastOpDataRcPtr & ec,
                       const std::string & exposureName,
                       const std::string & contrastName,
                       const std::string & gammaName)
{
    const double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());

    st.newLine() << st.floatDecl("exposure") << " = pow( 2., " << exposureName << " );";
    st.newLine() << st.floatDecl("contrast") << " = max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                 << shaderCreator->getPixelName() << ".rgb * exposure;";

    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
        st.indent();
        // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
        st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                     <<   "pow( "
                     <<     "max( "
                     <<       st.float3Const(0.0f) << ", "
                     <<       shaderCreator->getPixelName() << ".rgb / " << st.float3Const(pivot)
                     <<     " ), "
                     <<     st.float3Const("contrast")
                     <<   " ) * "
                     <<   st.float3Const(pivot) << ";";
        st.dedent();
    }
    st.newLine() << "}";
}

void AddECLinearRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                          GpuShaderText & st,
                          ConstExposureContrastOpDataRcPtr & ec,
                          const std::string & exposureName,
                          const std::string & contrastName,
                          const std::string & gammaName)
{
    const double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());

    st.newLine() << st.floatDecl("exposure") << " = pow( 2., " << exposureName << " );";
    st.newLine() << st.floatDecl("contrast") << " = 1. / max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";

    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
        st.indent();
        // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
        st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                     <<   "pow( "
                     <<      "max( "
                     <<         st.float3Const(0.0f) << ", "
                     <<         shaderCreator->getPixelName() << ".rgb / " << st.float3Const(pivot)
                     <<      " ), "
                     <<      st.float3Const("contrast")
                     <<    " ) * "
                     <<    st.float3Const(pivot) << ";";
        st.dedent();
    }
    st.newLine() << "}";

    st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                 << shaderCreator->getPixelName() << ".rgb / exposure;";
}

void AddECVideoShader(GpuShaderCreatorRcPtr & shaderCreator,
                      GpuShaderText & st,
                      ConstExposureContrastOpDataRcPtr & ec,
                      const std::string & exposureName,
                      const std::string & contrastName,
                      const std::string & gammaName)
{
    double pivot = std::pow(std::max(EC::MIN_PIVOT, ec->getPivot()), EC::VIDEO_OETF_POWER);

    st.newLine() << st.floatDecl("exposure") << " = pow( pow( 2., " << exposureName << " ), "
                                             << EC::VIDEO_OETF_POWER << ");";
    st.newLine() << st.floatDecl("contrast") << " = max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                 << shaderCreator->getPixelName() << ".rgb * exposure;";
    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
        st.indent();
        // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
        st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                     <<   "pow( "
                     <<     "max( "
                     <<       st.float3Const(0.0f) << ", "
                     <<       shaderCreator->getPixelName() << ".rgb / " << st.float3Const(pivot)
                     <<     " ), "
                     <<     st.float3Const("contrast")
                     <<   " ) * "
                     <<   st.float3Const(pivot) << ";";
        st.dedent();
    }
    st.newLine() << "}";
}

void AddECVideoRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                         GpuShaderText & st,
                         ConstExposureContrastOpDataRcPtr & ec,
                         const std::string & exposureName,
                         const std::string & contrastName,
                         const std::string & gammaName)
{
    double pivot = std::pow(std::max(EC::MIN_PIVOT, ec->getPivot()), EC::VIDEO_OETF_POWER);

    st.newLine() << st.floatDecl("exposure") << " = pow( pow( 2., " << exposureName << " ), "
                                             << EC::VIDEO_OETF_POWER << ");";
    st.newLine() << st.floatDecl("contrast") << " = 1. / max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";

    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
        st.indent();
        // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
        st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                     <<   "pow( "
                     <<     "max( "
                     <<       st.float3Const(0.0f) << ", "
                     <<       shaderCreator->getPixelName() << ".rgb / " << st.float3Const(pivot)
                     <<     " ), "
                     <<     st.float3Const("contrast")
                     <<   " ) * "
                     <<   st.float3Const(pivot) << ";";
        st.dedent();
    }
    st.newLine() << "}";

    st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                 << shaderCreator->getPixelName() << ".rgb / exposure;";
}

void AddECLogarithmicShader(GpuShaderCreatorRcPtr & shaderCreator,
                            GpuShaderText & st,
                            ConstExposureContrastOpDataRcPtr & ec,
                            const std::string & exposureName,
                            const std::string & contrastName,
                            const std::string & gammaName)
{
    double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());
    float logPivot = (float)std::max(0., std::log2(pivot / 0.18) *
                                         ec->getLogExposureStep() +
                                         ec->getLogMidGray());

    st.newLine() << st.floatDecl("exposure") << " = " << exposureName << " * "
                                             << ec->getLogExposureStep() << ";";
    st.newLine() << st.floatDecl("contrast") << " = max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << st.floatDecl("offset") << " = ( exposure - " << logPivot << " ) * contrast + "
                                           << logPivot << ";";

    st.newLine() << shaderCreator->getPixelName() << ".rgb = "
                 << shaderCreator->getPixelName() << ".rgb * contrast + offset;";
}

void AddECLogarithmicRevShader(GpuShaderCreatorRcPtr & shaderCreator,
                               GpuShaderText & st,
                               ConstExposureContrastOpDataRcPtr & ec,
                               const std::string & exposureName,
                               const std::string & contrastName,
                               const std::string & gammaName)
{
    double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());
    float logPivot = (float)std::max(0., std::log2(pivot / 0.18) *
                                         ec->getLogExposureStep() +
                                         ec->getLogMidGray());

    st.newLine() << st.floatDecl("exposure") << " = " << exposureName << " * "
                                             << ec->getLogExposureStep() << ";";
    st.newLine() << st.floatDecl("contrast") << " = max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << st.floatDecl("offset") << " = " << logPivot << " - " << logPivot 
                                           << " / contrast - exposure;";

    st.newLine() << shaderCreator->getPixelName() << ".rgb = " 
                 << shaderCreator->getPixelName() << ".rgb / contrast + offset;";
}


}

void GetExposureContrastGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                         ConstExposureContrastOpDataRcPtr & ec)
{
    std::string exposureName;
    std::string contrastName;
    std::string gammaName;

    GpuShaderText st(shaderCreator->getLanguage());
    st.indent();

    st.newLine() << "";
    st.newLine() << "// Add ExposureContrast '"
                 << ExposureContrastOpData::ConvertStyleToString(ec->getStyle())
                 << "' processing";
    st.newLine() << "";
    st.newLine() << "{";
    st.indent();

    AddProperties(shaderCreator, st, ec,
                  exposureName,
                  contrastName,
                  gammaName);

    switch (ec->getStyle())
    {
    case ExposureContrastOpData::STYLE_LINEAR:
        AddECLinearShader(shaderCreator, st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_LINEAR_REV:
        AddECLinearRevShader(shaderCreator, st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_VIDEO:
        AddECVideoShader(shaderCreator, st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_VIDEO_REV:
        AddECVideoRevShader(shaderCreator, st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_LOGARITHMIC:
        AddECLogarithmicShader(shaderCreator, st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_LOGARITHMIC_REV:
        AddECLogarithmicRevShader(shaderCreator, st, ec, exposureName, contrastName, gammaName);
        break;
    }

    st.dedent();
    st.newLine() << "}";

    st.dedent();
    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

} // namespace OCIO_NAMESPACE
