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

void AddProperty(GpuShaderDescRcPtr shaderDesc,
                 GpuShaderText & st,
                 DynamicPropertyImplRcPtr prop,
                 const std::string & name)
{
    if (prop->isDynamic())
    {
        // Add uniform to shader.
        if (shaderDesc->addUniform(name.c_str(), prop))
        {
            // Declare uniform.
            GpuShaderText stDecl(shaderDesc->getLanguage());
            stDecl.declareUniformFloat(name);
            shaderDesc->addToDeclareShaderCode(stDecl.string().c_str());
        }
    }
    else
    {
        // If the property is not dynamic, declare a local variable rather than
        // a uniform.
        st.declareVar(name, (float)prop->getDoubleValue());
    }

}

void AddProperties(GpuShaderDescRcPtr shaderDesc,
                   GpuShaderText & st,
                   ConstExposureContrastOpDataRcPtr & ec,
                   std::string & exposureName,
                   std::string & contrastName,
                   std::string & gammaName)
{
    exposureName = shaderDesc->getResourcePrefix();
    exposureName += EC_EXPOSURE;
    contrastName = shaderDesc->getResourcePrefix();
    contrastName += EC_CONTRAST;
    gammaName = shaderDesc->getResourcePrefix();
    gammaName += EC_GAMMA;
    AddProperty(shaderDesc, st, ec->getExposureProperty(), exposureName);
    AddProperty(shaderDesc, st, ec->getContrastProperty(), contrastName);
    AddProperty(shaderDesc, st, ec->getGammaProperty(), gammaName);
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

void GetExposureContrastGPUShaderProgram(GpuShaderDescRcPtr shaderDesc,
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
