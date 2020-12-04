// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

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

    if(prop->isDynamic())
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

void AddECLinearShader(GpuShaderText & st,
                       ConstExposureContrastOpDataRcPtr & ec,
                       const std::string & exposureName,
                       const std::string & contrastName,
                       const std::string & gammaName)
{
    const double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());

    st.newLine() << "float exposure = pow( 2., " << exposureName << " );";
    st.newLine() << "float contrast = max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << "outColor.rgb = outColor.rgb * exposure;";

    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
        st.indent();
        // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
        st.newLine() << "outColor.rgb = "
                     <<   "pow( "
                     <<     "max( "
                     <<       st.float3Const(0.0f) << ", "
                     <<       "outColor.rgb / " << st.float3Const(pivot)
                     <<     " ), "
                     <<     st.float3Const("contrast")
                     <<   " ) * "
                     <<   st.float3Const(pivot) << ";";
        st.dedent();
    }
    st.newLine() << "}";
}

void AddECLinearRevShader(GpuShaderText & st,
                          ConstExposureContrastOpDataRcPtr & ec,
                          const std::string & exposureName,
                          const std::string & contrastName,
                          const std::string & gammaName)
{
    const double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());

    st.newLine() << "float exposure = pow( 2., " << exposureName << " );";
    st.newLine() << "float contrast = 1. / max( " << EC::MIN_CONTRAST << ", "
                                                  << "( " << contrastName << " * " << gammaName << " ) );";

    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
      st.indent();
      // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
      st.newLine() << "outColor.rgb = "
                   <<   "pow( "
                   <<      "max( "
                   <<         st.float3Const(0.0f) << ", "
                   <<         "outColor.rgb / " << st.float3Const(pivot)
                   <<      " ), "
                   <<      st.float3Const("contrast")
                   <<    " ) * "
                   <<    st.float3Const(pivot) << ";";
      st.dedent();
    }
    st.newLine() << "}";

    st.newLine() << "outColor.rgb = outColor.rgb / exposure;";

}

void AddECVideoShader(GpuShaderText & st,
                      ConstExposureContrastOpDataRcPtr & ec,
                      const std::string & exposureName,
                      const std::string & contrastName,
                      const std::string & gammaName)
{
    double pivot = std::pow(std::max(EC::MIN_PIVOT, ec->getPivot()), EC::VIDEO_OETF_POWER);

    st.newLine() << "float exposure = pow( pow( 2., " << exposureName << " ), " << EC::VIDEO_OETF_POWER << ");";
    st.newLine() << "float contrast = max( " << EC::MIN_CONTRAST << ", "
                                        << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << "outColor.rgb = outColor.rgb * exposure;";
    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
        st.indent();
        // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
        st.newLine() << "outColor.rgb = "
                     <<   "pow( "
                     <<     "max( "
                     <<       st.float3Const(0.0f) << ", "
                     <<       "outColor.rgb / " << st.float3Const(pivot)
                     <<     " ), "
                     <<     st.float3Const("contrast")
                     <<   " ) * "
                     <<   st.float3Const(pivot) << ";";
        st.dedent();
    }
    st.newLine() << "}";
}

void AddECVideoRevShader(GpuShaderText & st,
                         ConstExposureContrastOpDataRcPtr & ec,
                         const std::string & exposureName,
                         const std::string & contrastName,
                         const std::string & gammaName)
{
    double pivot = std::pow(std::max(EC::MIN_PIVOT, ec->getPivot()), EC::VIDEO_OETF_POWER);

    st.newLine() << "float exposure = pow( pow( 2., " << exposureName << " ), "
                                        << EC::VIDEO_OETF_POWER << ");";
    st.newLine() << "float contrast = 1. / max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";

    st.newLine() << "if (contrast != 1.0)";
    st.newLine() << "{";
    {
        st.indent();
        // outColor = pow(max(0, outColor/pivot), contrast) * pivot;
        st.newLine() << "outColor.rgb = "
                     <<   "pow( "
                     <<     "max( "
                     <<       st.float3Const(0.0f) << ", "
                     <<       "outColor.rgb / " << st.float3Const(pivot)
                     <<     " ), "
                     <<     st.float3Const("contrast")
                     <<   " ) * "
                     <<   st.float3Const(pivot) << ";";
        st.dedent();
    }
    st.newLine() << "}";

    st.newLine() << "outColor.rgb = outColor.rgb / exposure;";
}

void AddECLogarithmicShader(GpuShaderText & st,
                            ConstExposureContrastOpDataRcPtr & ec,
                            const std::string & exposureName,
                            const std::string & contrastName,
                            const std::string & gammaName)
{
    double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());
    float logPivot = (float)std::max(0., std::log2(pivot / 0.18) *
                                         ec->getLogExposureStep() +
                                         ec->getLogMidGray());

    st.newLine() << "float exposure = " << exposureName << " * "
                                        << ec->getLogExposureStep() << ";";
    st.newLine() << "float contrast = max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << "float offset = ( exposure - " << logPivot << " ) * contrast + " << logPivot << ";";
    st.newLine() << "outColor.rgb = outColor.rgb * contrast + offset;";
}

void AddECLogarithmicRevShader(GpuShaderText & st,
                               ConstExposureContrastOpDataRcPtr & ec,
                               const std::string & exposureName,
                               const std::string & contrastName,
                               const std::string & gammaName)
{
    double pivot = std::max(EC::MIN_PIVOT, ec->getPivot());
    float logPivot = (float)std::max(0., std::log2(pivot / 0.18) *
                                         ec->getLogExposureStep() +
                                         ec->getLogMidGray());

    st.newLine() << "float exposure = " << exposureName << " * "
                                        << ec->getLogExposureStep() << ";";
    st.newLine() << "float contrast = max( " << EC::MIN_CONTRAST << ", "
                                             << "( " << contrastName << " * " << gammaName << " ) );";
    st.newLine() << "float offset = " << logPivot << " - " << logPivot << " / contrast - exposure;";
    st.newLine() << "outColor.rgb = outColor.rgb / contrast + offset;";
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
        AddECLinearShader(st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_LINEAR_REV:
        AddECLinearRevShader(st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_VIDEO:
        AddECVideoShader(st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_VIDEO_REV:
        AddECVideoRevShader(st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_LOGARITHMIC:
        AddECLogarithmicShader(st, ec, exposureName, contrastName, gammaName);
        break;
    case ExposureContrastOpData::STYLE_LOGARITHMIC_REV:
        AddECLogarithmicRevShader(st, ec, exposureName, contrastName, gammaName);
        break;
    }

    st.dedent();
    st.newLine() << "}";

    st.dedent();
    shaderCreator->addToFunctionShaderCode(st.string().c_str());
}

} // namespace OCIO_NAMESPACE
