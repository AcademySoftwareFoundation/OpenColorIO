/*
Copyright (c) 2018 Autodesk Inc., et al.
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


#include <OpenColorIO/OpenColorIO.h>

#include "ops/FixedFunction/FixedFunctionOpGPU.h"


OCIO_NAMESPACE_ENTER
{

void Add_hue_weight_shader(GpuShaderText & ss, float width)
{
    float center = 0.f;  // If changed, must uncomment code below.

    // Convert from degrees to radians.
    const float PI = 3.14159265358979f;
    center = center * PI / 180.f;
    const float widthR = width * PI / 180.f;
    // Actually want to multiply by (4/width).
    const float inv_width = 4.f / widthR;

    // TODO: Use formatters in GPUShaderUtils but increase precision.
    // (See the CPU renderer for more info on the algorithm.)

    //! \todo There is a performance todo in the GPUHueVec shader that would also apply here.
    ss.newLine() << "float a = 2.0 * outColor.r - (outColor.g + outColor.b);";
    ss.newLine() << "float b = 1.7320508075688772 * (outColor.g - outColor.b);";
    ss.newLine() << "float hue = " << ss.atan2("b", "a") << ";";

    // Since center is currently zero, commenting these lines out as a performance optimization
    //  << "    hue = hue - float(" << center << ");\n"
    //  << "    hue = mix( hue, hue + 6.28318530717959, step( hue, -3.14159265358979));\n"
    //  << "    hue = mix( hue, hue - 6.28318530717959, step( 3.14159265358979, hue));\n"

    ss.newLine() << "float knot_coord = clamp(2. + hue * float(" << inv_width << "), 0., 4.);";
    ss.newLine() << "int j = int(min(knot_coord, 3.));";
    ss.newLine() << "float t = knot_coord - float(j);";
    ss.newLine() << ss.vec4fDecl("monomials") << " = " << ss.vec4fConst("t*t*t", "t*t", "t", "1.") << ";";
    ss.newLine() << ss.vec4fDecl("m0") << " = " << ss.vec4fConst(0.25,  0.00,  0.00,  0.00) << ";";
    ss.newLine() << ss.vec4fDecl("m1") << " = " << ss.vec4fConst(-0.75,  0.75,  0.75,  0.25) << ";";
    ss.newLine() << ss.vec4fDecl("m2") << " = " << ss.vec4fConst(0.75, -1.50,  0.00,  1.00) << ";";
    ss.newLine() << ss.vec4fDecl("m3") << " = " << ss.vec4fConst(-0.25,  0.75, -0.75,  0.25) << ";";
    ss.newLine() << ss.vec4fDecl("coefs") << " = " << ss.lerp( "m0", "m1", "float(j == 1)") << ";";
    ss.newLine() << "coefs = " << ss.lerp("coefs", "m2", "float(j == 2)") << ";";
    ss.newLine() << "coefs = " << ss.lerp("coefs", "m3", "float(j == 3)") << ";";
    ss.newLine() << "float f_H = dot(coefs, monomials);";
}

void Add_RedMod_03_Fwd_Shader(GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.85f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(ss, 120.f);

    ss.newLine() << ss.vec3fDecl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.vec3fDecl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";
    ss.newLine() << "float oldChroma = max(1e-10, maxval.r - minval.r);";
    ss.newLine() << ss.vec3fDecl("delta") << " = outColor.rgb - minval;";

    ss.newLine() << "float f_S = ( max(1e-10, maxval.r) - max(1e-10, minval.r) ) / max(1e-2, maxval.r);";

    ss.newLine() << "outColor.r = outColor.r + f_H * f_S * (" << _pivot 
                 << " - outColor.r) * " << _1minusScale << ";";

    ss.newLine() << ss.vec3fDecl("maxval2") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << "float newChroma = maxval2.r - minval.r;";
    ss.newLine() << "outColor.rgb = minval.r + delta * newChroma / oldChroma;";
}

void Add_RedMod_03_Inv_Shader(GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.85f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(ss, 120.f);

    ss.newLine() << "if (f_H > 0.)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.vec3fDecl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.vec3fDecl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";
    ss.newLine() << "float oldChroma = max(1e-10, maxval.r - minval.r);";
    ss.newLine() << ss.vec3fDecl("delta") << " = outColor.rgb - minval;";

    // Note: If f_H == 0, the following generally doesn't change the red value,
    //       but it does for R < 0, hence the need for the if-statement above.
    ss.newLine() << "float ka = f_H * " << _1minusScale << " - 1.;";
    ss.newLine() << "float kb = outColor.r - f_H * (" << _pivot << " + minval.r) * " 
                 << _1minusScale << ";";
    ss.newLine() << "float kc = f_H * " << _pivot << " * minval.r * " << _1minusScale << ";";
    ss.newLine() << "outColor.r = ( -kb - sqrt( kb * kb - 4. * ka * kc)) / ( 2. * ka);";

    ss.newLine() << ss.vec3fDecl("maxval2") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << "float newChroma = maxval2.r - minval.r;";
    ss.newLine() << "outColor.rgb = minval.r + delta * newChroma / oldChroma;";

    ss.dedent();
    ss.newLine() << "}";
}

void Add_RedMod_10_Fwd_Shader(GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.82f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(ss, 135.f);

    ss.newLine() << ss.vec3fDecl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.vec3fDecl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";
    ss.newLine() << "float f_S = ( max(1e-10, maxval.r) - max(1e-10, minval.r) ) / max(1e-2, maxval.r);";

    ss.newLine() << "outColor.r = outColor.r + f_H * f_S * (" << _pivot
                 << " - outColor.r) * " << _1minusScale << ";";
}

void Add_RedMod_10_Inv_Shader(GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.82f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(ss, 135.f);

    ss.newLine() << "if (f_H > 0.)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.vec3fDecl("minval") << " = min( outColor.gbr, outColor.brg);";

    // Note: If f_H == 0, the following generally doesn't change the red value
    //       but it does for R < 0, hence the if.
    ss.newLine() << "float ka = f_H * " << _1minusScale << " - 1.;";
    ss.newLine() << "float kb = outColor.r - f_H * (" << _pivot << " + minval.r) * " 
                 << _1minusScale << ";";
    ss.newLine() << "float kc = f_H * " << _pivot << " * minval.r * " << _1minusScale << ";";
    ss.newLine() << "outColor.r = ( -kb - sqrt( kb * kb - 4. * ka * kc)) / ( 2. * ka);";

    ss.dedent();
    ss.newLine() << "}";
}

void Add_Glow_03_Fwd_Shader(GpuShaderText & ss, float glowGain, float glowMid)
{
    ss.newLine() << "float chroma = sqrt( outColor.b * (outColor.b - outColor.g)"
                 << " + outColor.g * (outColor.g - outColor.r)"
                 << " + outColor.r * (outColor.r - outColor.b) );";
    ss.newLine() << "float YC = (outColor.b + outColor.g + outColor.r + 1.75 * chroma) / 3.;";
    ss.newLine() << ss.vec3fDecl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.vec3fDecl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";

    ss.newLine() << "float sat = ( max(1e-10, maxval.r) - max(1e-10, minval.r) ) / max(1e-2, maxval.r);";

    ss.newLine() << "float x = (sat - 0.4) * 5.;";
    ss.newLine() << "float t = max( 0., 1. - 0.5 * abs(x));";
    ss.newLine() << "float s = 0.5 * (1. + sign(x) * (1. - t * t));";

    ss.newLine() << "float GlowGain = " << glowGain << " * s;";
    ss.newLine() << "float GlowMid = " << glowMid << ";";
    ss.newLine() << "float glowGainOut = " << ss.lerp( "GlowGain", "GlowGain * (GlowMid / YC - 0.5)",
                                                       "float( YC > GlowMid * 2. / 3. )" ) << ";";
    ss.newLine() << "glowGainOut = " << ss.lerp( "glowGainOut", "0.", "float( YC > GlowMid * 2. )" ) << ";";

    ss.newLine() << "outColor.rgb = outColor.rgb * glowGainOut + outColor.rgb;";
}

void Add_Glow_03_Inv_Shader(GpuShaderText & ss, float glowGain, float glowMid)
{
    ss.newLine() << "float chroma = sqrt( outColor.b * (outColor.b - outColor.g)"
                 << " + outColor.g * (outColor.g - outColor.r)"
                 << " + outColor.r * (outColor.r - outColor.b) );";
    ss.newLine() << "float YC = (outColor.b + outColor.g + outColor.r + 1.75 * chroma) / 3.;";
    ss.newLine() << ss.vec3fDecl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.vec3fDecl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";

    ss.newLine() << "float sat = ( max(1e-10, maxval.r) - max(1e-10, minval.r) ) / max(1e-2, maxval.r);";

    ss.newLine() << "float x = (sat - 0.4) * 5.;";
    ss.newLine() << "float t = max( 0., 1. - 0.5 * abs(x));";
    ss.newLine() << "float s = 0.5 * (1. + sign(x) * (1. - t * t));";

    ss.newLine() << "float GlowGain = " << glowGain << " * s;";
    ss.newLine() << "float GlowMid = " << glowMid << ";";
    ss.newLine() << "float glowGainOut = " 
                 << ss.lerp( "-GlowGain / (1. + GlowGain)",
                             "GlowGain * (GlowMid / YC - 0.5) / (GlowGain * 0.5 - 1.)",
                             "float( YC > (1. + GlowGain) * GlowMid * 2. / 3. )" ) << ";";
    ss.newLine() << "glowGainOut = " << ss.lerp( "glowGainOut", "0.", "float( YC > GlowMid * 2. )" ) << ";";

    ss.newLine() << "outColor.rgb = outColor.rgb * glowGainOut + outColor.rgb;";
}

void Add_Surround_10_Fwd_Shader(GpuShaderText & ss, float gamma)
{
    // TODO: -- add vector inner product to GPUShaderUtils
    ss.newLine() << "float Y = max( 1e-10, 0.27222871678091454 * outColor.r + "
                 <<                       "0.67408176581114831 * outColor.g + "
                 <<                       "0.053689517407937051 * outColor.b );";

    ss.newLine() << "float Ypow_over_Y = pow( Y, " << gamma - 1.f << ");";

    ss.newLine() << "outColor.rgb = outColor.rgb * Ypow_over_Y;";
}

void Add_Surround_Shader(GpuShaderText & ss, float gamma)
{
    // TODO: -- add vector inner product to GPUShaderUtils
    ss.newLine() << "float Y = max( 1e-4, 0.2627 * outColor.r + "
                 <<                      "0.6780 * outColor.g + "
                 <<                      "0.0593 * outColor.b );";

    ss.newLine() << "float Ypow_over_Y = pow( Y, " << gamma - 1.f << ");";

    ss.newLine() << "outColor.rgb = outColor.rgb * Ypow_over_Y;";
}

void GetFixedFunctionGPUShaderProgram(GpuShaderText & ss, 
                                      ConstFixedFunctionOpDataRcPtr & func)
{
    ss.newLine() << "";
    ss.newLine() << "// Add FixedFunction "
                 << FixedFunctionOpData::ConvertStyleToString(func->getStyle(), true)
                 << " processing";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();

    switch(func->getStyle())
    {
        case FixedFunctionOpData::ACES_RED_MOD_03_FWD:
        {
            Add_RedMod_03_Fwd_Shader(ss);
            break;
         }
        case FixedFunctionOpData::ACES_RED_MOD_03_INV:
        {
            Add_RedMod_03_Inv_Shader(ss);
            break;
        }
        case FixedFunctionOpData::ACES_RED_MOD_10_FWD:
        {
            Add_RedMod_10_Fwd_Shader(ss);
            break;
        }
        case FixedFunctionOpData::ACES_RED_MOD_10_INV:
        {
            Add_RedMod_10_Inv_Shader(ss);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_03_FWD:
        {
            Add_Glow_03_Fwd_Shader(ss, 0.075f, 0.1f);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_03_INV:
        {
            Add_Glow_03_Inv_Shader(ss, 0.075f, 0.1f);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_10_FWD:
        {
            // Use 03 renderer with different params.
            Add_Glow_03_Fwd_Shader(ss, 0.05f, 0.08f);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_10_INV:
        {
            // Use 03 renderer with different params.
            Add_Glow_03_Inv_Shader(ss, 0.05f, 0.08f);
            break;
        }
        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD:
        {
            Add_Surround_10_Fwd_Shader(ss, 0.9811f);
            break;
        }
        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV:
        {
            // Call forward renderer with the inverse gamma.
            Add_Surround_10_Fwd_Shader(ss, 1.0192640913260627f);
            break;
        }
        case FixedFunctionOpData::REC2100_SURROUND:
        {
            Add_Surround_Shader(ss, (float) func->getParams()[0]);
            break;
        }
    }

    ss.dedent();
    ss.newLine() << "}";
}

}
OCIO_NAMESPACE_EXIT
