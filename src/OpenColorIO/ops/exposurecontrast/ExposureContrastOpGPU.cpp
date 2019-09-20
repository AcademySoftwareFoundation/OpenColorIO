// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpGPU.h"


OCIO_NAMESPACE_ENTER
{
namespace
{

static constexpr const char * EC_EXPOSURE = "exposureVal";
static constexpr const char * EC_CONTRAST = "contrastVal";
static constexpr const char * EC_GAMMA = "gammaVal";

void AddUniform(GpuShaderDescRcPtr & shaderDesc,
                GpuShaderText & st,
                DynamicPropertyImplRcPtr prop,
                const std::string & name)
{
    // Add the uniform if it does not already exist.
    if (shaderDesc->addUniform(name.c_str(), prop))
    {
        // Declare uniform.
        GpuShaderText stDecl(shaderDesc->getLanguage());
        stDecl.declareUniformFloat(name);
        shaderDesc->addToDeclareShaderCode(stDecl.string().c_str());
    }
}

std::string AddDynamicProperty(GpuShaderDescRcPtr & shaderDesc,
                               GpuShaderText & st,
                               DynamicPropertyImplRcPtr prop,
                               const std::string & name)
{
    std::string finalName;

    if(prop->isDynamic())
    {
        finalName = shaderDesc->getResourcePrefix();
        finalName += name;

        // NB: No need to add an index to the name to avoid collisions
        //     as the dynamic properties are shared i.e. only one instance.

        AddUniform(shaderDesc, st, prop, finalName);
    }
    else
    {
        finalName = name;

        st.declareVar(finalName, (float)prop->getDoubleValue());
    }

    return finalName;
}

void AddProperties(GpuShaderDescRcPtr & shaderDesc,
                   GpuShaderText & st,
                   ConstExposureContrastOpDataRcPtr & ec,
                   std::string & exposureName,
                   std::string & contrastName,
                   std::string & gammaName)
{
    exposureName = AddDynamicProperty(shaderDesc, st, ec->getExposureProperty(), EC_EXPOSURE);
    contrastName = AddDynamicProperty(shaderDesc, st, ec->getContrastProperty(), EC_CONTRAST);
    gammaName    = AddDynamicProperty(shaderDesc, st, ec->getGammaProperty(),    EC_GAMMA);
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
                     <<       st.vec3fConst(0.0f) << ", "
                     <<       "outColor.rgb / " << st.vec3fConst(pivot)
                     <<     " ), "
                     <<     st.vec3fConst("contrast")
                     <<   " ) * "
                     <<   st.vec3fConst(pivot) << ";";
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
                   <<         st.vec3fConst(0.0f) << ", "
                   <<         "outColor.rgb / " << st.vec3fConst(pivot)
                   <<      " ), "
                   <<      st.vec3fConst("contrast")
                   <<    " ) * "
                   <<    st.vec3fConst(pivot) << ";";
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
                     <<       st.vec3fConst(0.0f) << ", "
                     <<       "outColor.rgb / " << st.vec3fConst(pivot)
                     <<     " ), "
                     <<     st.vec3fConst("contrast")
                     <<   " ) * "
                     <<   st.vec3fConst(pivot) << ";";
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
                     <<       st.vec3fConst(0.0f) << ", "
                     <<       "outColor.rgb / " << st.vec3fConst(pivot)
                     <<     " ), "
                     <<     st.vec3fConst("contrast")
                     <<   " ) * "
                     <<   st.vec3fConst(pivot) << ";";
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

void GetExposureContrastGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                                         ConstExposureContrastOpDataRcPtr & ec)
{
    std::string exposureName;
    std::string contrastName;
    std::string gammaName;

    GpuShaderText st(shaderDesc->getLanguage());
    st.indent();

    st.newLine() << "";
    st.newLine() << "// Add ExposureContrast "
                 << ExposureContrastOpData::ConvertStyleToString(ec->getStyle())
                 << " processing";
    st.newLine() << "";
    st.newLine() << "{";
    st.indent();

    AddProperties(shaderDesc, st, ec,
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
    shaderDesc->addToFunctionShaderCode(st.string().c_str());
}

}
OCIO_NAMESPACE_EXIT
