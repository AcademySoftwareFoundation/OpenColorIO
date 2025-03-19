// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "utils/StringUtils.h"
#include "ops/fixedfunction/FixedFunctionOpGPU.h"
#include "ACES2/Transform.h"


namespace OCIO_NAMESPACE
{

void Add_hue_weight_shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss, float width)
{
    float center = 0.f;  // If changed, must uncomment code below.

    // Convert from degrees to radians.
    const float PI = 3.14159265358979f;
    center = center * PI / 180.f;
    const float widthR = width * PI / 180.f;
    // Actually want to multiply by (4/width).
    const float inv_width = 4.f / widthR;

    const std::string pxl(shaderCreator->getPixelName());

    // TODO: Use formatters in GPUShaderUtils but increase precision.
    // (See the CPU renderer for more info on the algorithm.)

    //! \todo There is a performance todo in the GPUHueVec shader that would also apply here.
    ss.newLine() << ss.floatDecl("a") << " = 2.0 * " << pxl << ".rgb.r - (" << pxl << ".rgb.g + " << pxl << ".rgb.b);";
    ss.newLine() << ss.floatDecl("b") << " = 1.7320508075688772 * (" << pxl << ".rgb.g - " << pxl << ".rgb.b);";
    ss.newLine() << ss.floatDecl("hue") << " = " << ss.atan2("b", "a") << ";";

    // Since center is currently zero, commenting these lines out as a performance optimization
    //  << "    hue = hue - float(" << center << ");\n"
    //  << "    hue = mix( hue, hue + 6.28318530717959, step( hue, -3.14159265358979));\n"
    //  << "    hue = mix( hue, hue - 6.28318530717959, step( 3.14159265358979, hue));\n"

    ss.newLine() << ss.floatDecl("knot_coord") << " = clamp(2. + hue * float(" << inv_width << "), 0., 4.);";
    ss.newLine() << "int j = int(min(knot_coord, 3.));";
    ss.newLine() << ss.floatDecl("t") << " = knot_coord - float(j);";
    ss.newLine() << ss.float4Decl("monomials") << " = " << ss.float4Const("t*t*t", "t*t", "t", "1.") << ";";
    ss.newLine() << ss.float4Decl("m0") << " = " << ss.float4Const(0.25,  0.00,  0.00,  0.00) << ";";
    ss.newLine() << ss.float4Decl("m1") << " = " << ss.float4Const(-0.75,  0.75,  0.75,  0.25) << ";";
    ss.newLine() << ss.float4Decl("m2") << " = " << ss.float4Const(0.75, -1.50,  0.00,  1.00) << ";";
    ss.newLine() << ss.float4Decl("m3") << " = " << ss.float4Const(-0.25,  0.75, -0.75,  0.25) << ";";
    ss.newLine() << ss.float4Decl("coefs") << " = " << ss.lerp( "m0", "m1", "float(j == 1)") << ";";
    ss.newLine() << "coefs = " << ss.lerp("coefs", "m2", "float(j == 2)") << ";";
    ss.newLine() << "coefs = " << ss.lerp("coefs", "m3", "float(j == 3)") << ";";
    ss.newLine() << ss.floatDecl("f_H") << " = dot(coefs, monomials);";
}

void Add_RedMod_03_Fwd_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.85f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(shaderCreator, ss, 120.f);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("maxval") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";
    ss.newLine() << ss.floatDecl("minval") << " = min( " << pxl << ".rgb.r, min( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";

    ss.newLine() << ss.floatDecl("oldChroma") << " = max(1e-10, maxval - minval);";
    ss.newLine() << ss.float3Decl("delta") << " = " << pxl << ".rgb - minval;";

    ss.newLine() << ss.floatDecl("f_S") << " = ( max(1e-10, maxval) - max(1e-10, minval) ) / max(1e-2, maxval);";

    ss.newLine() << pxl << ".rgb.r = " << pxl << ".rgb.r + f_H * f_S * (" << _pivot
                 << " - " << pxl << ".rgb.r) * " << _1minusScale << ";";

    ss.newLine() << ss.floatDecl("maxval2") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";
    ss.newLine() << ss.floatDecl("newChroma") << " = maxval2 - minval;";
    ss.newLine() << pxl << ".rgb = minval + delta * newChroma / oldChroma;";
}

void Add_RedMod_03_Inv_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.85f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(shaderCreator, ss, 120.f);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << "if (f_H > 0.)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("maxval") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";
    ss.newLine() << ss.floatDecl("minval") << " = min( " << pxl << ".rgb.r, min( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";

    ss.newLine() << ss.floatDecl("oldChroma") << " = max(1e-10, maxval - minval);";
    ss.newLine() << ss.float3Decl("delta") << " = " << pxl << ".rgb - minval;";

    // Note: If f_H == 0, the following generally doesn't change the red value,
    //       but it does for R < 0, hence the need for the if-statement above.
    ss.newLine() << ss.floatDecl("ka") << " = f_H * " << _1minusScale << " - 1.;";
    ss.newLine() << ss.floatDecl("kb") << " = " << pxl << ".rgb.r - f_H * (" << _pivot << " + minval) * "
                 << _1minusScale << ";";
    ss.newLine() << ss.floatDecl("kc") << " = f_H * " << _pivot << " * minval * " << _1minusScale << ";";
    ss.newLine() << pxl << ".rgb.r = ( -kb - sqrt( kb * kb - 4. * ka * kc)) / ( 2. * ka);";

    ss.newLine() << ss.floatDecl("maxval2") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";
    ss.newLine() << ss.floatDecl("newChroma") << " = maxval2 - minval;";
    ss.newLine() << pxl << ".rgb = minval + delta * newChroma / oldChroma;";

    ss.dedent();
    ss.newLine() << "}";
}

void Add_RedMod_10_Fwd_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.82f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(shaderCreator, ss, 135.f);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("maxval") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";
    ss.newLine() << ss.floatDecl("minval") << " = min( " << pxl << ".rgb.r, min( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";

    ss.newLine() << ss.floatDecl("f_S") << " = ( max(1e-10, maxval) - max(1e-10, minval) ) / max(1e-2, maxval);";

    ss.newLine() << pxl << ".rgb.r = " << pxl << ".rgb.r + f_H * f_S * (" << _pivot
                 << " - " << pxl << ".rgb.r) * " << _1minusScale << ";";
}

void Add_RedMod_10_Inv_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.82f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(shaderCreator, ss, 135.f);

    ss.newLine() << "if (f_H > 0.)";
    ss.newLine() << "{";
    ss.indent();

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("minval") << " = min( " << pxl << ".rgb.g, " << pxl << ".rgb.b);";

    // Note: If f_H == 0, the following generally doesn't change the red value
    //       but it does for R < 0, hence the if.
    ss.newLine() << ss.floatDecl("ka") << " = f_H * " << _1minusScale << " - 1.;";
    ss.newLine() << ss.floatDecl("kb") << " = " << pxl << ".rgb.r - f_H * (" << _pivot << " + minval) * "
                 << _1minusScale << ";";
    ss.newLine() << ss.floatDecl("kc") << " = f_H * " << _pivot << " * minval * " << _1minusScale << ";";
    ss.newLine() << pxl << ".rgb.r = ( -kb - sqrt( kb * kb - 4. * ka * kc)) / ( 2. * ka);";

    ss.dedent();
    ss.newLine() << "}";
}

void Add_Glow_03_Fwd_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss, float glowGain, float glowMid)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("chroma") << " = sqrt( " << pxl << ".rgb.b * (" << pxl << ".rgb.b - " << pxl << ".rgb.g)"
                 << " + " << pxl << ".rgb.g * (" << pxl << ".rgb.g - " << pxl << ".rgb.r)"
                 << " + " << pxl << ".rgb.r * (" << pxl << ".rgb.r - " << pxl << ".rgb.b) );";
    ss.newLine() << ss.floatDecl("YC") << " = (" << pxl << ".rgb.b + " << pxl << ".rgb.g + " << pxl << ".rgb.r + 1.75 * chroma) / 3.;";

    ss.newLine() << ss.floatDecl("maxval") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";
    ss.newLine() << ss.floatDecl("minval") << " = min( " << pxl << ".rgb.r, min( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";

    ss.newLine() << ss.floatDecl("sat") << " = ( max(1e-10, maxval) - max(1e-10, minval) ) / max(1e-2, maxval);";

    ss.newLine() << ss.floatDecl("x") << " = (sat - 0.4) * 5.;";
    ss.newLine() << ss.floatDecl("t") << " = max( 0., 1. - 0.5 * abs(x));";
    ss.newLine() << ss.floatDecl("s") << " = 0.5 * (1. + sign(x) * (1. - t * t));";

    ss.newLine() << ss.floatDecl("GlowGain")    << " = " << glowGain << " * s;";
    ss.newLine() << ss.floatDecl("GlowMid")     << " = " << glowMid << ";";
    ss.newLine() << ss.floatDecl("glowGainOut") << " = " << ss.lerp( "GlowGain", "GlowGain * (GlowMid / YC - 0.5)",
                                                                     "float( YC > GlowMid * 2. / 3. )" ) << ";";
    ss.newLine() << "glowGainOut = " << ss.lerp( "glowGainOut", "0.", "float( YC > GlowMid * 2. )" ) << ";";

    ss.newLine() << pxl << ".rgb = " << pxl << ".rgb * glowGainOut + " << pxl << ".rgb;";
}

void Add_Glow_03_Inv_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss, float glowGain, float glowMid)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("chroma") << " = sqrt( " << pxl << ".rgb.b * (" << pxl << ".rgb.b - " << pxl << ".rgb.g)"
                 << " + " << pxl << ".rgb.g * (" << pxl << ".rgb.g - " << pxl << ".rgb.r)"
                 << " + " << pxl << ".rgb.r * (" << pxl << ".rgb.r - " << pxl << ".rgb.b) );";
    ss.newLine() << ss.floatDecl("YC") << " = (" << pxl << ".rgb.b + " << pxl << ".rgb.g + " << pxl << ".rgb.r + 1.75 * chroma) / 3.;";

    ss.newLine() << ss.floatDecl("maxval") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";
    ss.newLine() << ss.floatDecl("minval") << " = min( " << pxl << ".rgb.r, min( " << pxl << ".rgb.g, " << pxl << ".rgb.b));";

    ss.newLine() << ss.floatDecl("sat") << " = ( max(1e-10, maxval) - max(1e-10, minval) ) / max(1e-2, maxval);";

    ss.newLine() << ss.floatDecl("x") << " = (sat - 0.4) * 5.;";
    ss.newLine() << ss.floatDecl("t") << " = max( 0., 1. - 0.5 * abs(x));";
    ss.newLine() << ss.floatDecl("s") << " = 0.5 * (1. + sign(x) * (1. - t * t));";

    ss.newLine() << ss.floatDecl("GlowGain")    << " = " << glowGain << " * s;";
    ss.newLine() << ss.floatDecl("GlowMid")     << " = " << glowMid << ";";
    ss.newLine() << ss.floatDecl("glowGainOut") << " = "
                 << ss.lerp( "-GlowGain / (1. + GlowGain)",
                             "GlowGain * (GlowMid / YC - 0.5) / (GlowGain * 0.5 - 1.)",
                             "float( YC > (1. + GlowGain) * GlowMid * 2. / 3. )" ) << ";";
    ss.newLine() << "glowGainOut = " << ss.lerp( "glowGainOut", "0.", "float( YC > GlowMid * 2. )" ) << ";";

    ss.newLine() << pxl << ".rgb = " << pxl << ".rgb * glowGainOut + " << pxl << ".rgb;";
}

void Add_GamutComp_13_Shader_Compress(GpuShaderText & ss,
                                      const char * dist,
                                      const char * cdist,
                                      float scl,
                                      float thr,
                                      float power)
{
    // Only compress if greater or equal than threshold.
    ss.newLine() << "if (" << dist << " >= " << thr << ")";
    ss.newLine() << "{";
    ss.indent();

    // Normalize distance outside threshold by scale factor.
    ss.newLine() << ss.floatDecl("nd") << " = (" << dist << " - " << thr << ") / " << scl << ";";
    ss.newLine() << ss.floatDecl("p") << " = pow(nd, " << power << ");";
    ss.newLine() << cdist << " = " << thr << " + " << scl << " * nd / (pow(1.0 + p, " << 1.0f / power << "));";

    ss.dedent();
    ss.newLine() << "}"; // if (dist >= thr)
}

void Add_GamutComp_13_Shader_UnCompress(GpuShaderText & ss,
                                        const char * dist,
                                        const char * cdist,
                                        float scl,
                                        float thr,
                                        float power)
{
    // Only compress if greater or equal than threshold, avoid singularity.
    ss.newLine() << "if (" << dist << " >= " << thr << " && " << dist << " < " << thr + scl << " )";
    ss.newLine() << "{";
    ss.indent();

    // Normalize distance outside threshold by scale factor.
    ss.newLine() << ss.floatDecl("nd") << " = (" << dist << " - " << thr << ") / " << scl << ";";
    ss.newLine() << ss.floatDecl("p") << " = pow(nd, " << power << ");";
    ss.newLine() << cdist << " = " << thr << " + " << scl << " * pow(-(p / (p - 1.0)), " << 1.0f / power << ");";

    ss.dedent();
    ss.newLine() << "}"; // if (dist >= thr && dist < thr + scl)
}

template <typename Func>
void Add_GamutComp_13_Shader(GpuShaderText & ss,
                             GpuShaderCreatorRcPtr & sc,
                             float limCyan,
                             float limMagenta,
                             float limYellow,
                             float thrCyan,
                             float thrMagenta,
                             float thrYellow,
                             float power,
                             Func f)
{
    // Precompute scale factor for y = 1 intersect
    auto f_scale = [power](float lim, float thr) {
        return (lim - thr) / std::pow(std::pow((1.0f - thr) / (lim - thr), -power) - 1.0f, 1.0f / power);
    };
    const float scaleCyan      = f_scale(limCyan,    thrCyan);
    const float scaleMagenta   = f_scale(limMagenta, thrMagenta);
    const float scaleYellow    = f_scale(limYellow,  thrYellow);

    const char * pix = sc->getPixelName();

    // Achromatic axis
    ss.newLine() << ss.floatDecl("ach") << " = max( " << pix << ".rgb.r, max( " << pix << ".rgb.g, " << pix << ".rgb.b ) );";

    ss.newLine() << "if ( ach != 0. )";
    ss.newLine() << "{";
    ss.indent();

    // Distance from the achromatic axis for each color component aka inverse rgb ratios.
    ss.newLine() << ss.float3Decl("dist") << " = (ach - " << pix << ".rgb) / abs(ach);";
    ss.newLine() << ss.float3Decl("cdist") << " = dist;";

    f(ss, "dist.x", "cdist.x", scaleCyan,    thrCyan,    power);
    f(ss, "dist.y", "cdist.y", scaleMagenta, thrMagenta, power);
    f(ss, "dist.z", "cdist.z", scaleYellow,  thrYellow,  power);

    // Recalculate rgb from compressed distance and achromatic.
    // Effectively this scales each color component relative to achromatic axis by the compressed distance.
    ss.newLine() << pix << ".rgb = ach - cdist * abs(ach);";

    ss.dedent();
    ss.newLine() << "}"; // if ( ach != 0.0f )
}

void Add_GamutComp_13_Fwd_Shader(GpuShaderText & ss,
                                 GpuShaderCreatorRcPtr & sc,
                                 float limCyan,
                                 float limMagenta,
                                 float limYellow,
                                 float thrCyan,
                                 float thrMagenta,
                                 float thrYellow,
                                 float power)
{
    Add_GamutComp_13_Shader(
        ss,
        sc,
        limCyan,
        limMagenta,
        limYellow,
        thrCyan,
        thrMagenta,
        thrYellow,
        power,
        Add_GamutComp_13_Shader_Compress
    );
}

void Add_GamutComp_13_Inv_Shader(GpuShaderText & ss,
                                 GpuShaderCreatorRcPtr & sc,
                                 float limCyan,
                                 float limMagenta,
                                 float limYellow,
                                 float thrCyan,
                                 float thrMagenta,
                                 float thrYellow,
                                 float power)
{
    Add_GamutComp_13_Shader(
        ss,
        sc,
        limCyan,
        limMagenta,
        limYellow,
        thrCyan,
        thrMagenta,
        thrYellow,
        power,
        Add_GamutComp_13_Shader_UnCompress
    );
}

void _Add_WrapHueChannel_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());
    ss.newLine() << ss.floatDecl("hwrap") << " = " << pxl << ".b;";
    ss.newLine() << "hwrap = hwrap - floor(hwrap / 360.0) * 360.0;";
    ss.newLine() << "hwrap = (hwrap < 0.0) ? hwrap + 360.0 : hwrap;";
    ss.newLine() << pxl << ".b = hwrap;";
}

void _Add_SinCos_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());
    ss.newLine() << ss.floatDecl("h_rad") << " = " << pxl << ".b * " << 3.14159265358979f / 180.0f << ";";
    ss.newLine() << ss.floatDecl("cos_hr") << " = cos(h_rad);";
    ss.newLine() << ss.floatDecl("sin_hr") << " = sin(h_rad);";
}


void _Add_RGB_to_Aab_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.float3Decl("lms") << " = " << ss.mat3fMul(&p.MATRIX_RGB_to_CAM16_c[0], pxl + ".rgb") << ";";

    ss.newLine() << ss.float3Decl("F_L_v") << " = pow(abs(lms), " << ss.float3Const(0.42f) << ");";
    ss.newLine() << ss.float3Decl("rgb_a") << " = (sign(lms) * F_L_v) / ( " << ACES2::cam_nl_offset << " + F_L_v);";

    ss.newLine() << "Aab = " << ss.mat3fMul(&p.MATRIX_cone_response_to_Aab[0], "rgb_a.rgb") << ";";

    ss.dedent();
    ss.newLine() << "}";
}

void _Add_Aab_to_JMh_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << "if (Aab.r <= 0.0)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "JMh.rgb = " << ss.float3Const(0.0) << ";";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.floatDecl("J") << " = " << ACES2::J_scale << " * pow(Aab.r, " << p.cz << ");";

    ss.newLine() << ss.floatDecl("M") << " = (J == 0.0) ? 0.0 : sqrt(Aab.g * Aab.g + Aab.b * Aab.b);";

    ss.newLine() << ss.floatDecl("h") << " = (Aab.g == 0.0) ? 0.0 : " << ss.atan2("Aab.b", "Aab.g") << " * " << 180.0 / 3.14159265358979 << ";";
    ss.newLine() << "h = h - floor(h / 360.0) * 360.0;";
    ss.newLine() << "h = (h < 0.0) ? h + 360.0 : h;";

    ss.newLine() << "JMh.rgb = " << ss.float3Const("J", "M", "h") << ";";
    ss.dedent();
    ss.newLine() << "}";

    ss.dedent();
    ss.newLine() << "}";
}

void _Add_RGB_to_JMh_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("JMh") << ";";    // TODO: leaky abstraction should really be explicit functions
    ss.newLine() << ss.float3Decl("Aab") << ";";    // TODO: leaky abstraction should really be explicit functions

    ss.newLine() << "{";
    ss.indent();

    _Add_RGB_to_Aab_Shader(shaderCreator, ss, p);
    _Add_Aab_to_JMh_Shader(shaderCreator, ss, p);
    
    ss.newLine() << pxl << ".rgb = JMh;";

    ss.dedent();
    ss.newLine() << "}";
}


void _Add_JMh_to_Aab_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << "{";
    ss.indent();


    ss.newLine() << "Aab.r = pow(JMh.r * " << 1.0f / ACES2::J_scale << ", " << p.inv_cz << ");";
    ss.newLine() << "Aab.g = JMh.g * cos_hr;";
    ss.newLine() << "Aab.b = JMh.g * sin_hr;";
    
    ss.dedent();
    ss.newLine() << "}";
}

void _Add_Aab_to_RGB_Shader(
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.float3Decl("rgb_a") << " = " << ss.mat3fMul(&p.MATRIX_Aab_to_cone_response[0], "Aab.rgb") << ";";
    ss.newLine() << ss.float3Decl("rgb_a_lim") << " = min( abs(rgb_a), " << ss.float3Const(0.99f) << " );";
    ss.newLine() << ss.float3Decl("lms") << " = sign(rgb_a) * pow( " << ACES2::cam_nl_offset 
                 << " * rgb_a_lim / (1.0f - rgb_a_lim), " << ss.float3Const(1.f / 0.42f) << ");";
    ss.newLine() << "JMh.rgb = " << ss.mat3fMul(&p.MATRIX_CAM16_c_to_RGB[0], "lms") << ";";

    ss.dedent();
    ss.newLine() << "}";
}

void _Add_JMh_to_RGB_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("JMh") << " = " << pxl << ".rgb;";
    ss.newLine() << ss.float3Decl("Aab") << ";";
    _Add_JMh_to_Aab_Shader(shaderCreator, ss, p);
    _Add_Aab_to_RGB_Shader(ss, p);

    ss.newLine() << pxl << ".rgb = JMh;";
}

std::string _Add_Reach_table(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::Table1D & table)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("reach_m_table_")
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    // Register texture
    GpuShaderDesc::TextureDimensions dimensions = GpuShaderDesc::TEXTURE_1D;
    if (shaderCreator->getLanguage() == GPU_LANGUAGE_GLSL_ES_1_0
        || shaderCreator->getLanguage() == GPU_LANGUAGE_GLSL_ES_3_0
        || !shaderCreator->getAllowTexture1D())
    {
        dimensions = GpuShaderDesc::TEXTURE_2D;
    }

    shaderCreator->addTexture(
        name.c_str(),
        GpuShaderText::getSamplerName(name).c_str(),
        table.total_size,
        1,
        GpuShaderCreator::TEXTURE_RED_CHANNEL,
        dimensions,
        INTERP_NEAREST,
        &(table[0]));


    if (dimensions == GpuShaderDesc::TEXTURE_1D)
    {
        GpuShaderText ss(shaderCreator->getLanguage());
        ss.declareTex1D(name);
        shaderCreator->addToDeclareShaderCode(ss.string().c_str());
    }
    else
    {
        GpuShaderText ss(shaderCreator->getLanguage());
        ss.declareTex2D(name);
        shaderCreator->addToDeclareShaderCode(ss.string().c_str());
    }

    // Sampler function
    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "_sample(float h)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("i_base") << " = floor(h);";
    ss.newLine() << ss.floatDecl("i_lo") << " = i_base + " << table.base_index << ";";
    ss.newLine() << ss.floatDecl("i_hi") << " = i_lo + 1;";

    if (dimensions == GpuShaderDesc::TEXTURE_1D)
    {
        ss.newLine() << ss.floatDecl("lo") << " = " << ss.sampleTex1D(name, "(i_lo + 0.5) / " + std::to_string(table.total_size)) << ".r;";
        ss.newLine() << ss.floatDecl("hi") << " = " << ss.sampleTex1D(name, "(i_hi + 0.5) / " + std::to_string(table.total_size)) << ".r;";
    }
    else
    {
        ss.newLine() << ss.floatDecl("lo") << " = " << ss.sampleTex2D(name, ss.float2Const("(i_lo + 0.5) / " + std::to_string(table.total_size), "0.0")) << ".r;";
        ss.newLine() << ss.floatDecl("hi") << " = " << ss.sampleTex2D(name, ss.float2Const("(i_hi + 0.5) / " + std::to_string(table.total_size), "0.5")) << ".r;";
    }

    ss.newLine() << ss.floatDecl("t") << " = h - i_base;"; // Hardcoded single degree spacing
    ss.newLine() << "return " << ss.lerp("lo", "hi", "t") << ";";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Toe_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    bool invert)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("toe")
            << (invert ? std::string("_inv") : std::string("_fwd"))
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "(float x, float limit, float k1_in, float k2_in)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("k2") << " = max(k2_in, 0.001);";
    ss.newLine() << ss.floatDecl("k1") << " = sqrt(k1_in * k1_in + k2 * k2);";
    ss.newLine() << ss.floatDecl("k3") << " = (limit + k1) / (limit + k2);";

    if (invert)
    {
        ss.newLine() << "return (x > limit) ? x : (x * x + k1 * x) / (k3 * (x + k2));";
    }
    else
    {
        ss.newLine() << "return (x > limit) ? x : 0.5 * (k3 * x - k1 + sqrt((k3 * x - k1) * (k3 * x - k1) + 4.0 * k2 * k3 * x));";
    }

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Tonescale_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    bool invert,
    const ACES2::JMhParams & p,
    const ACES2::ToneScaleParams & t)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("tonescale")
            << (invert ? std::string("_inv") : std::string("_fwd"))
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "(float J)";
    ss.newLine() << "{";
    ss.indent();

    // Tonescale applied in Y (convert to and from J)
    // TODO: Investigate if we can receive negative J here at all. 
    // If not, abs(J) here and the sign(J) at the return may not be needed at all.
    ss.newLine() << ss.floatDecl("A") << " = " << p.A_w_J << " * pow(abs(J) * " << 1.0f / ACES2::J_scale << ", " << p.inv_cz << ");";
    ss.newLine() << ss.floatDecl("Y") << " = pow(( " << ACES2::cam_nl_offset << " * A) / (1.0f - A), " << 1.0 / 0.42 << ");";

    if (invert)
    {
        // Inverse Tonescale applied in Y (convert to and from J)
        ss.newLine() << ss.floatDecl("Y_i") << " = Y / " << double(p.F_L_n) * ACES2::reference_luminance << ";";
 
        ss.newLine() << ss.floatDecl("Z") << " = max(0.0, min(" << t.inverse_limit << ", Y_i));";
        ss.newLine() << ss.floatDecl("ht") << " = 0.5 * (Z + sqrt(Z * (" << 4.0 * t.t_1 << " + Z)));";
        ss.newLine() << ss.floatDecl("Yo") << " = " << double(p.F_L_n) * t.s_2 << " / (pow((" << t.m_2 << " / ht), (" << 1.0 / t.g << ")) - 1.0);";

        ss.newLine() << ss.floatDecl("F_L_Y") << " = pow(abs(Yo), 0.42);";
    }
    else
    {
        // Tonescale applied in Y (convert to and from J)
        ss.newLine() << ss.floatDecl("f") << " = " << t.m_2  << " * pow(Y / (Y + " << double(t.s_2) * p.F_L_n << "), " << t.g << ");";
        ss.newLine() << ss.floatDecl("Y_ts") << " = max(0.0, f * f / (f + " << t.t_1 << "));";
        ss.newLine() << ss.floatDecl("F_L_Y") << " = pow(" << double(p.F_L_n) * ACES2::reference_luminance << " * Y_ts, 0.42);";
    }

    ss.newLine() << ss.floatDecl("J_ts") << " = " << ACES2::J_scale << " * pow((F_L_Y / ( " << ACES2::cam_nl_offset << " + F_L_Y)) * " << p.inv_A_w_J << ", " << p.cz << ");";
    ss.newLine() << "return sign(J) * J_ts;";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

void _Add_ChromaCompressionNorm_Shader(
    GpuShaderText & ss,
    const ACES2::ChromaCompressParams & c)
{
    // Mnorm
    ss.newLine() << ss.floatDecl("Mnorm") << ";";
    ss.newLine() << "{";
    ss.indent();

    // TODO: optimization: can bake weights into terms and convert dotprods to addition. /coz
    ss.newLine() << ss.floatDecl("cos_hr2") << " = 2.0 * cos_hr * cos_hr - 1.0;";
    ss.newLine() << ss.floatDecl("sin_hr2") << " = 2.0 * cos_hr * sin_hr;";
    ss.newLine() << ss.floatDecl("cos_hr3") << " = 4.0 * cos_hr * cos_hr * cos_hr - 3.0 * cos_hr;";
    ss.newLine() << ss.floatDecl("sin_hr3") << " = 3.0 * sin_hr - 4.0 * sin_hr * sin_hr * sin_hr;";
    ss.newLine() << ss.float3Decl("cosines") << " = " <<  ss.float3Const("cos_hr", "cos_hr2", "cos_hr3") <<";";
    ss.newLine() << ss.float3Decl("cosine_weights") << " = " <<  ss.float3Const(11.34072 * c.chroma_compress_scale,
                                                                                16.46899 * c.chroma_compress_scale,
                                                                                 7.88380 * c.chroma_compress_scale) <<";";
    ss.newLine() << ss.float3Decl("sines") << " = " <<  ss.float3Const("sin_hr", "sin_hr2", "sin_hr3") <<";";
    ss.newLine() << ss.float3Decl("sine_weights") << " = " <<  ss.float3Const(14.66441 * c.chroma_compress_scale,
                                                                              -6.37224 * c.chroma_compress_scale,
                                                                               9.19364 * c.chroma_compress_scale) <<";";
    ss.newLine() << "Mnorm = dot(cosines, cosine_weights) + dot(sines, sine_weights) + " << 77.12896 * c.chroma_compress_scale<< ";";

    ss.dedent();
    ss.newLine() << "}";
}

void _Add_Tonescale_Compress_Fwd_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    unsigned resourceIndex,
    const ACES2::SharedCompressionParameters & s,
    const ACES2::ChromaCompressParams & c)
{
    std::string toeName = _Add_Toe_func(shaderCreator, resourceIndex, false);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("J") << " = " << pxl << ".r;";
    ss.newLine() << ss.floatDecl("M") << " = " << pxl << ".g;";
    ss.newLine() << ss.floatDecl("h") << " = " << pxl << ".b;";


    // ChromaCompress
    ss.newLine() << ss.floatDecl("M_cp") << " = M;";

    ss.newLine() << "if (M != 0.0)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("nJ") << " = J_ts / " << s.limit_J_max << ";";
    ss.newLine() << ss.floatDecl("snJ") << " = max(0.0, 1.0 - nJ);";

    _Add_ChromaCompressionNorm_Shader(ss, c);

    ss.newLine() << ss.floatDecl("limit") << " = pow(nJ, " << s.model_gamma_inv << ") * reachMaxM / Mnorm;";
    ss.newLine() << "M_cp = M * pow(J_ts / J, " << s.model_gamma_inv << ");";
    ss.newLine() << "M_cp = M_cp / Mnorm;";

    ss.newLine() << "M_cp = limit - " << toeName << "(limit - M_cp, limit - 0.001, snJ * " << c.sat << ", sqrt(nJ * nJ + " << c.sat_thr << "));";
    ss.newLine() << "M_cp = " << toeName << "(M_cp, limit, nJ * " << c.compr << ", snJ);";
    ss.newLine() << "M_cp = M_cp * Mnorm;";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("J_ts", "M_cp", "h") << ";";
}

void _Add_Tonescale_Compress_Inv_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    unsigned resourceIndex,
    const ACES2::SharedCompressionParameters & s,
    const ACES2::ChromaCompressParams & c)
{
    std::string toeName = _Add_Toe_func(shaderCreator, resourceIndex, true);

    const std::string pxl(shaderCreator->getPixelName());
    
    ss.newLine() << ss.floatDecl("J_ts") << " = " << pxl << ".r;";
    ss.newLine() << ss.floatDecl("M_cp") << " = " << pxl << ".g;";
    ss.newLine() << ss.floatDecl("h") << " = " << pxl << ".b;";

    // ChromaCompress
    ss.newLine() << ss.floatDecl("M") << " = M_cp;";

    ss.newLine() << "if (M_cp != 0.0)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("nJ") << " = J_ts / " << s.limit_J_max << ";";
    ss.newLine() << ss.floatDecl("snJ") << " = max(0.0, 1.0 - nJ);";

    _Add_ChromaCompressionNorm_Shader(ss, c);

    ss.newLine() << ss.floatDecl("limit") << " = pow(nJ, " << s.model_gamma_inv << ") * reachMaxM / Mnorm;";

    ss.newLine() << "M = M_cp / Mnorm;";
    ss.newLine() << "M = " << toeName << "(M, limit, nJ * " << c.compr << ", snJ);";
    ss.newLine() << "M = limit - " << toeName << "(limit - M, limit - 0.001, snJ * " << c.sat << ", sqrt(nJ * nJ + " << c.sat_thr << "));";
    ss.newLine() << "M = M * Mnorm;";
    ss.newLine() << "M = M * pow(J_ts / J, " << -s.model_gamma_inv << ");";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("J", "M", "h") << ";";
}

std::string _Add_Cusp_table(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::GamutCompressParams & g)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("gamut_cusp_table_")
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    // Register texture
    GpuShaderDesc::TextureDimensions dimensions = GpuShaderDesc::TEXTURE_1D;
    if (shaderCreator->getLanguage() == GPU_LANGUAGE_GLSL_ES_1_0
        || shaderCreator->getLanguage() == GPU_LANGUAGE_GLSL_ES_3_0
        || !shaderCreator->getAllowTexture1D())
    {
        dimensions = GpuShaderDesc::TEXTURE_2D;
    }

    shaderCreator->addTexture(
        name.c_str(),
        GpuShaderText::getSamplerName(name).c_str(),
        g.gamut_cusp_table.total_size,
        1,
        GpuShaderCreator::TEXTURE_RGB_CHANNEL,
        dimensions,
        INTERP_NEAREST,
        &(g.gamut_cusp_table[0][0]));

    if (dimensions == GpuShaderDesc::TEXTURE_1D)
    {
        GpuShaderText ss(shaderCreator->getLanguage());
        ss.declareTex1D(name);
        shaderCreator->addToDeclareShaderCode(ss.string().c_str());
    }
    else
    {
        GpuShaderText ss(shaderCreator->getLanguage());
        ss.declareTex2D(name);
        shaderCreator->addToDeclareShaderCode(ss.string().c_str());
    }

    // Sampler function
    GpuShaderText ss(shaderCreator->getLanguage());

    const std::string hues_array_name = name + "_hues_array";
    ss.declareFloatArrayConst(hues_array_name, (int) g.hue_table.total_size, g.hue_table.data());

    ss.newLine() << ss.float3Keyword() << " " << name << "_sample(float h)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.intDecl("i") << " = " << ss.intKeyword() << "(h) + " << g.gamut_cusp_table.base_index << ";";

    ss.newLine() << ss.intDecl("i_lo") << " = " << ss.intKeyword() << "(max("
                 << ss.floatKeyword() << "(" << g.gamut_cusp_table.lower_wrap_index << "), "
                 << ss.floatKeyword() << "(i + " << g.hue_linearity_search_range[0] << ")));";
    ss.newLine() << ss.intDecl("i_hi") << " = " << ss.intKeyword() << "(min("
                 << ss.floatKeyword() << "(" << g.gamut_cusp_table.upper_wrap_index << "), "
                 << ss.floatKeyword() << "(i + " << g.hue_linearity_search_range[1] << ")));";

    ss.newLine() << "while (i_lo + 1 < i_hi)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("hcur") << " = " << hues_array_name << "[i];";

    ss.newLine() << "if (h > hcur)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "i_lo = i;";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "i_hi = i;";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "i = (i_lo + i_hi) / 2;";

    ss.dedent();
    ss.newLine() << "}";

    if (dimensions == GpuShaderDesc::TEXTURE_1D)
    {
        ss.newLine() << ss.float3Decl("lo") << " = " << ss.sampleTex1D(name, std::string("(i_hi - 1 + 0.5) / ") + std::to_string(g.gamut_cusp_table.total_size)) << ".rgb;";
        ss.newLine() << ss.float3Decl("hi") << " = " << ss.sampleTex1D(name, std::string("(i_hi + 0.5) / ") + std::to_string(g.gamut_cusp_table.total_size)) << ".rgb;";
    }
    else
    {
        ss.newLine() << ss.float3Decl("lo") << " = " << ss.sampleTex2D(name, ss.float2Const(std::string("(i_hi - 1 + 0.5) / ") + std::to_string(g.gamut_cusp_table.total_size), "0.5")) << ".rgb;";
        ss.newLine() << ss.float3Decl("hi") << " = " << ss.sampleTex2D(name, ss.float2Const(std::string("(i_hi + 0.5) / ") + std::to_string(g.gamut_cusp_table.total_size), "0.5")) << ".rgb;";
    }

    ss.newLine() << ss.floatDecl("t") << " = (h - " << hues_array_name << "[i_hi - 1]) / "
                                                "(" << hues_array_name << "[i_hi]" << " - " << hues_array_name << "[i_hi - 1]);";
    ss.newLine() << "return " << ss.lerp("lo", "hi", "t") << ";";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Focus_Gain_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::SharedCompressionParameters & s)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("get_focus_gain")
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "(float J, float cuspJ)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("thr") << " = " << ss.lerp("cuspJ", std::to_string(s.limit_J_max), std::to_string(ACES2::focus_gain_blend)) << ";";

    ss.newLine() << "if (J > thr)"; // TODO threshold
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.floatDecl("gain") << " = ( " << s.limit_J_max << " - thr) / max(0.0001, " << s.limit_J_max << " - J);";
    ss.newLine() << "gain = log(gain)/log(10.0);";  // TODO log10(gain) but not all shading languages have log10() would log2(gain)/log2(10) be better? perhaps delegate to GpuShaderText?
    //ss.newLine() << "return pow(gain, " << ACES2::focus_adjust_gain_inv << ") + 1.0;"; // TODO; remove once agreed on change
    ss.newLine() << "return gain * gain + 1.0;";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "return 1.0;";
    ss.dedent();
    ss.newLine() << "}";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Solve_J_Intersect_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::SharedCompressionParameters & s)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("solve_J_intersect")
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "(float J, float M, float focusJ, float slope_gain)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("M_scaled") << " = M / slope_gain;";
    ss.newLine() << ss.floatDecl("a") << " = M_scaled / focusJ;";

    ss.newLine() << "if (J < focusJ)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.floatDecl("b") << " = 1.0 - M_scaled;";
    ss.newLine() << ss.floatDecl("c") << " = -J;";
    ss.newLine() << ss.floatDecl("det") << " =  b * b - 4.f * a * c;";
    ss.newLine() << ss.floatDecl("root") << " =  sqrt(det);";
    ss.newLine() << "return -2.0 * c / (b + root);";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.floatDecl("b") << " = - (1.0 + M_scaled + " << s.limit_J_max << " * a);";
    ss.newLine() << ss.floatDecl("c") << " = " << s.limit_J_max << " * M_scaled + J;";
    ss.newLine() << ss.floatDecl("det") << " =  b * b - 4.f * a * c;";
    ss.newLine() << ss.floatDecl("root") << " =  sqrt(det);";
    ss.newLine() << "return -2.0 * c / (b - root);";
    ss.dedent();
    ss.newLine() << "}";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Find_Gamut_Boundary_Intersection_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::SharedCompressionParameters & s)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("find_gamut_boundary_intersection")
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "(" << ss.float2Keyword()
                 << " JM_cusp, float gamma_top_inv, float gamma_bottom_inv, float J_intersect_source, float J_intersect_cusp, float slope)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("M_boundary_lower") << " = J_intersect_cusp * pow(J_intersect_source / J_intersect_cusp, gamma_bottom_inv) / (JM_cusp.r / JM_cusp.g - slope);";
    ss.newLine() << ss.floatDecl("M_boundary_upper") << " = JM_cusp.g * (" << s.limit_J_max << " - J_intersect_cusp) * pow((" << s.limit_J_max << " - J_intersect_source) / (" << s.limit_J_max << " - J_intersect_cusp), gamma_top_inv) / (slope * JM_cusp.g + " << s.limit_J_max << " - JM_cusp.r);";

    ss.newLine() << ss.floatDecl("smin") << " = 0.0;";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.floatDecl("a") << " = M_boundary_lower;";
    ss.newLine() << ss.floatDecl("b") << " = M_boundary_upper;";
    ss.newLine() << ss.floatDecl("s") << " = " << ACES2::smooth_cusps << " * JM_cusp.g;";

    ss.newLine() << ss.floatDecl("h") << " = max(s - abs(a - b), 0.0) / s;";
    ss.newLine() << "smin = min(a, b) - h * h * h * s * " << (1.0 / 6.0) << ";";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "return smin;";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Compression_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    bool invert)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("remap_M")
            << (invert ? std::string("_inv") : std::string("_fwd"))
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "(float M, float gamut_boundary_M, float reach_boundary_M)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("boundary_ratio") << " = gamut_boundary_M / reach_boundary_M;";
    ss.newLine() << ss.floatDecl("proportion") << " = max(boundary_ratio, " << ACES2::compression_threshold << ");";
    ss.newLine() << ss.floatDecl("threshold") << " = proportion * gamut_boundary_M;";

    ss.newLine() << "if (proportion >= 1.0f || M <= threshold)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "return M;";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << ss.floatDecl("m_offset") << " = M - threshold;";
    ss.newLine() << ss.floatDecl("gamut_offset") << " = gamut_boundary_M - threshold;";
    ss.newLine() << ss.floatDecl("reach_offset") << " = reach_boundary_M - threshold;";

    ss.newLine() << ss.floatDecl("scale") << " = reach_offset / ((reach_offset / gamut_offset) - 1.0f);";
    ss.newLine() << ss.floatDecl("nd") << " = m_offset / scale;";

    if (invert)
    {
        ss.newLine() << "if (nd >= 1.0f)"; // TODO: could be done branchless?
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << "return threshold + scale;";
        ss.dedent();
        ss.newLine() << "}";
        ss.newLine() << "else";
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << "return threshold + scale * -(nd / (nd - 1.0f));";
        ss.dedent();
        ss.newLine() << "}";
    }
    else
    {
        ss.newLine() << "return threshold + scale * nd / (1.0f + nd);";
    }

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Compress_Gamut_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::SharedCompressionParameters & s,
    const ACES2::GamutCompressParams & g,
    const std::string & getFocusGainName,
    const std::string & findGamutBoundaryIntersectionName,
    const std::string & compressionName,
    const std::string & solveJIntersectName)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("gamut_compress")
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.float3Keyword() << " " << name << "(" << ss.float3Keyword() << " JMh, float Jx, " << ss.float3Keyword() << " JMGcusp, float reachMaxM)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("J") << " = JMh.r;";
    ss.newLine() << ss.floatDecl("M") << " = JMh.g;";
    ss.newLine() << ss.floatDecl("h") << " = JMh.b;";

    ss.newLine() << "if (M <= 0.0 || J > " << s.limit_J_max << ")";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "return " << ss.float3Const("J", "0.0", "h") << ";";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.float2Decl("JMcusp") << " = JMGcusp.rg;";

    ss.newLine() << ss.floatDecl("focusJ") << " = " << ss.lerp("JMcusp.r", std::to_string(g.mid_J), std::string("min(1.0, ") + std::to_string(ACES2::cusp_mid_blend) + " - (JMcusp.r / " + std::to_string(s.limit_J_max)) << "));";
    ss.newLine() << ss.floatDecl("slope_gain") << " = " << s.limit_J_max * g.focus_dist << " * " << getFocusGainName << "(Jx, JMcusp.r);";
    ss.newLine() << ss.floatDecl("J_intersect_source") << " = " << solveJIntersectName << "(JMh.r, JMh.g, focusJ, slope_gain);";
    ss.newLine() << ss.floatDecl("gamut_slope") << " = (J_intersect_source < focusJ) ? J_intersect_source : (" << s.limit_J_max << " - J_intersect_source);";
    ss.newLine() << "gamut_slope = gamut_slope * (J_intersect_source - focusJ) / (focusJ * slope_gain);";

    ss.newLine() << ss.floatDecl("gamma_top_inv") << " = JMGcusp.b;";
    ss.newLine() << ss.floatDecl("gamma_bottom_inv") << " = " << g.lower_hull_gamma_inv << ";"; // TODO move to where it is used

    ss.newLine() << ss.floatDecl("J_intersect_cusp") << " = " << solveJIntersectName << "(JMcusp.r, JMcusp.g, focusJ, slope_gain);";
    ss.newLine() << ss.floatDecl("gamutBoundaryM") << " = " << findGamutBoundaryIntersectionName
                 << "(JMcusp, gamma_top_inv, gamma_bottom_inv, J_intersect_source, J_intersect_cusp, gamut_slope);";

    ss.newLine() << "if (gamutBoundaryM <= 0.0)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "return " << ss.float3Const("J", "0.0", "h") << ";";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.floatDecl("reachBoundaryM") << " = " << s.limit_J_max << " * pow(J_intersect_source / " << s.limit_J_max << ",  " << s.model_gamma_inv << ");";
    ss.newLine() << "reachBoundaryM = reachBoundaryM / ((" << s.limit_J_max << " / reachMaxM) - gamut_slope);";

    ss.newLine() << ss.floatDecl("remapped_M") << " = " << compressionName << "(M, gamutBoundaryM, reachBoundaryM);";
    ss.newLine() << ss.floatDecl("remapped_J") << " = J_intersect_source + remapped_M * gamut_slope;";


    ss.newLine() << "return " << ss.float3Const("remapped_J", "remapped_M", "h") << ";";

    ss.dedent();
    ss.newLine() << "}";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

void _Add_Gamut_Compress_Fwd_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    unsigned int resourceIndex,
    const ACES2::SharedCompressionParameters & s,
    const ACES2::GamutCompressParams & g)
{
    std::string cuspName = _Add_Cusp_table(shaderCreator, resourceIndex, g);
    std::string getFocusGainName = _Add_Focus_Gain_func(shaderCreator, resourceIndex, s);
    std::string solveJIntersectName = _Add_Solve_J_Intersect_func(shaderCreator, resourceIndex, s);
    std::string findGamutBoundaryIntersectionName = _Add_Find_Gamut_Boundary_Intersection_func(shaderCreator, resourceIndex, s);
    std::string compressionName = _Add_Compression_func(shaderCreator, resourceIndex, false);
    std::string gamutCompressName = _Add_Compress_Gamut_func(shaderCreator, resourceIndex, s, g, getFocusGainName, findGamutBoundaryIntersectionName, compressionName, solveJIntersectName);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("JMGcusp") << " = " << cuspName << "_sample(" << pxl << ".b);";
    ss.newLine() << pxl << ".rgb = " << gamutCompressName << "(" << pxl << ".rgb, " << pxl << ".r, JMGcusp, reachMaxM);";
}

void _Add_Gamut_Compress_Inv_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    unsigned int resourceIndex,
    const ACES2::SharedCompressionParameters & s,
    const ACES2::GamutCompressParams & g)
{
    std::string cuspName = _Add_Cusp_table(shaderCreator, resourceIndex, g);
    std::string getFocusGainName = _Add_Focus_Gain_func(shaderCreator, resourceIndex, s);
    std::string solveJIntersectName = _Add_Solve_J_Intersect_func(shaderCreator, resourceIndex, s);
    std::string findGamutBoundaryIntersectionName = _Add_Find_Gamut_Boundary_Intersection_func(shaderCreator, resourceIndex, s);
    std::string compressionName = _Add_Compression_func(shaderCreator, resourceIndex, true);
    std::string gamutCompressName = _Add_Compress_Gamut_func(shaderCreator, resourceIndex, s, g, getFocusGainName, findGamutBoundaryIntersectionName, compressionName, solveJIntersectName);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("JMGcusp") << " = " << cuspName << "_sample(" << pxl << ".b);";

    ss.newLine() << ss.floatDecl("Jx") << " = " << pxl << ".r;";
    ss.newLine() << ss.float3Decl("unCompressedJMh") << ";";

    // Analytic inverse below threshold
    ss.newLine() << "if (Jx <= " << ss.lerp("JMGcusp.r", std::to_string(s.limit_J_max), std::to_string(ACES2::focus_gain_blend)) << ")";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "unCompressedJMh = " << gamutCompressName << "(" << pxl << ".rgb, Jx, JMGcusp, reachMaxM);";
    ss.dedent();
    ss.newLine() << "}";
    // Approximation above threshold
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "Jx = " << gamutCompressName << "(" << pxl << ".rgb, Jx, JMGcusp, reachMaxM).r;";
    ss.newLine() << "unCompressedJMh = " << gamutCompressName << "(" << pxl << ".rgb, Jx, JMGcusp, reachMaxM);";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << pxl << ".rgb = unCompressedJMh;";
}

void Add_ACES_OutputTransform_Fwd_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    const float red_x   = (float) params[1];
    const float red_y   = (float) params[2];
    const float green_x = (float) params[3];
    const float green_y = (float) params[4];
    const float blue_x  = (float) params[5];
    const float blue_y  = (float) params[6];
    const float white_x = (float) params[7];
    const float white_y = (float) params[8];

    const Primaries lim_primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    const ACES2::JMhParams pIn = ACES2::init_JMhParams(ACES_AP0::primaries);
    const ACES2::JMhParams pLim = ACES2::init_JMhParams(lim_primaries);
    const ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    const ACES2::JMhParams reachGamut = ACES2::init_JMhParams(ACES_AP1::primaries);
    const ACES2::SharedCompressionParameters s = ACES2::init_SharedCompressionParams(peak_luminance, pIn, reachGamut);
    const ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance, t);
    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, pIn, pLim, t, s, reachGamut);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();

    const std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, s.reach_m_table);
    const std::string tonescaleName_Fwd = _Add_Tonescale_func(shaderCreator, resourceIndex, false, pIn, t);
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << "";
    ss.newLine() << "// Add RGB to JMh";
    ss.newLine() << "";
    _Add_RGB_to_JMh_Shader(shaderCreator, ss, pIn);
    _Add_SinCos_Shader(shaderCreator, ss);


    ss.newLine() << "";
    ss.newLine() << "// Add ToneScale and ChromaCompress (fwd)";
    ss.newLine() << "";

    ss.newLine() << ss.floatDecl("J_ts") << " = " << tonescaleName_Fwd << "(" << pxl << ".r);";

    ss.newLine() << "// Sample tables (fwd)";
    ss.newLine() << ss.floatDecl("reachMaxM") << " = " << reachName << "_sample(" << pxl << ".b);";

    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();
        _Add_Tonescale_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, s, c);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add GamutCompress (fwd)";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_Gamut_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, s, g);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add JMh to RGB";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_JMh_to_RGB_Shader(shaderCreator, ss, pLim);
    ss.dedent();
    ss.newLine() << "}";

}

void Add_ACES_OutputTransform_Inv_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    const float red_x   = (float) params[1];
    const float red_y   = (float) params[2];
    const float green_x = (float) params[3];
    const float green_y = (float) params[4];
    const float blue_x  = (float) params[5];
    const float blue_y  = (float) params[6];
    const float white_x = (float) params[7];
    const float white_y = (float) params[8];

    const Primaries lim_primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    const ACES2::JMhParams pIn = ACES2::init_JMhParams(ACES_AP0::primaries);
    const ACES2::JMhParams pLim = ACES2::init_JMhParams(lim_primaries);
    const ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    const ACES2::JMhParams reachGamut = ACES2::init_JMhParams(ACES_AP1::primaries);
    const ACES2::SharedCompressionParameters s = ACES2::init_SharedCompressionParams(peak_luminance, pIn, reachGamut);
    const ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance, t);
    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, pIn, pLim, t, s, reachGamut);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();
    const std::string pxl(shaderCreator->getPixelName());
    
    std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, s.reach_m_table);
    std::string tonescaleName_Inv = _Add_Tonescale_func(shaderCreator, resourceIndex, true, pIn, t);

    ss.newLine() << "";
    ss.newLine() << "// Add RGB to JMh";
    ss.newLine() << "";
    _Add_RGB_to_JMh_Shader(shaderCreator, ss, pLim);
    _Add_SinCos_Shader(shaderCreator, ss);


    ss.newLine() << ss.floatDecl("reachMaxM") << " = " << reachName << "_sample(" << pxl << ".b);";
    ss.newLine() << "";
    ss.newLine() << "// Add GamutCompress (inv)";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_Gamut_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, s, g);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add ToneScale and ChromaCompress (inv)";
    ss.newLine() << "";
    ss.newLine() << ss.floatDecl("J") << " = " << tonescaleName_Inv << "(" << pxl << ".r);";
    ss.newLine() << "{";
    ss.indent();
        _Add_Tonescale_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, s, c);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add JMh to RGB";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_JMh_to_RGB_Shader(shaderCreator, ss, pIn);
    ss.dedent();
    ss.newLine() << "}";
}

void Add_RGB_to_JMh_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float red_x   = (float) params[0];
    const float red_y   = (float) params[1];
    const float green_x = (float) params[2];
    const float green_y = (float) params[3];
    const float blue_x  = (float) params[4];
    const float blue_y  = (float) params[5];
    const float white_x = (float) params[6];
    const float white_y = (float) params[7];

    const Primaries primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };
    ACES2::JMhParams p = ACES2::init_JMhParams(primaries);

    _Add_RGB_to_JMh_Shader(shaderCreator, ss, p);
}

void Add_JMh_to_RGB_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float red_x   = (float) params[0];
    const float red_y   = (float) params[1];
    const float green_x = (float) params[2];
    const float green_y = (float) params[3];
    const float blue_x  = (float) params[4];
    const float blue_y  = (float) params[5];
    const float white_x = (float) params[6];
    const float white_y = (float) params[7];

    const Primaries primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    ACES2::JMhParams p = ACES2::init_JMhParams(primaries);
    _Add_WrapHueChannel_Shader(shaderCreator, ss);
    _Add_SinCos_Shader(shaderCreator, ss);
    _Add_JMh_to_RGB_Shader(shaderCreator, ss, p);
}

void Add_Tonescale_Compress_Fwd_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    const ACES2::JMhParams p = ACES2::init_JMhParams(ACES_AP0::primaries);
    const ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    const ACES2::JMhParams reachGamut = ACES2::init_JMhParams(ACES_AP1::primaries);
    const ACES2::SharedCompressionParameters s = ACES2::init_SharedCompressionParams(peak_luminance, p, reachGamut);
    const ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance, t);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();
    const std::string pxl(shaderCreator->getPixelName());

    const std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, s.reach_m_table);
    const std::string tonescaleName_Fwd = _Add_Tonescale_func(shaderCreator, resourceIndex, false, p, t);

    _Add_WrapHueChannel_Shader(shaderCreator, ss);
    _Add_SinCos_Shader(shaderCreator, ss);

    ss.newLine() << ss.floatDecl("reachMaxM") << " = " << reachName << "_sample(" << pxl << ".b);";
    ss.newLine() << ss.floatDecl("J_ts") << " = " << tonescaleName_Fwd << "(" << pxl << ".r);";

    _Add_Tonescale_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, s, c);
}

void Add_Tonescale_Compress_Inv_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    const ACES2::JMhParams p = ACES2::init_JMhParams(ACES_AP0::primaries);
    const ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    const ACES2::JMhParams reachGamut = ACES2::init_JMhParams(ACES_AP1::primaries);
    const ACES2::SharedCompressionParameters s = ACES2::init_SharedCompressionParams(peak_luminance, p, reachGamut);
    const ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance, t);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();
    const std::string pxl(shaderCreator->getPixelName());

    const std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, s.reach_m_table);
    std::string tonescaleName_Inv = _Add_Tonescale_func(shaderCreator, resourceIndex, true, p, t);

    _Add_WrapHueChannel_Shader(shaderCreator, ss);
    _Add_SinCos_Shader(shaderCreator, ss);

    ss.newLine() << ss.floatDecl("reachMaxM") << " = " << reachName << "_sample(" << pxl << ".b);";
    ss.newLine() << ss.floatDecl("J") << " = " << tonescaleName_Inv << "(" << pxl << ".r);";

    _Add_Tonescale_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, s, c);
}

void Add_Gamut_Compress_Fwd_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    const float red_x   = (float) params[1];
    const float red_y   = (float) params[2];
    const float green_x = (float) params[3];
    const float green_y = (float) params[4];
    const float blue_x  = (float) params[5];
    const float blue_y  = (float) params[6];
    const float white_x = (float) params[7];
    const float white_y = (float) params[8];

    const Primaries primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    const ACES2::JMhParams pIn = ACES2::init_JMhParams(ACES_AP0::primaries);
    const ACES2::JMhParams pLim = ACES2::init_JMhParams(primaries);
    const ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    const ACES2::JMhParams reachGamut = ACES2::init_JMhParams(ACES_AP1::primaries);
    const ACES2::SharedCompressionParameters s = ACES2::init_SharedCompressionParams(peak_luminance, pIn, reachGamut);
    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, pIn, pLim, t, s, reachGamut); 

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();
    const std::string pxl(shaderCreator->getPixelName());

    const std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, s.reach_m_table);

    _Add_WrapHueChannel_Shader(shaderCreator, ss);
    _Add_SinCos_Shader(shaderCreator, ss);

    ss.newLine() << ss.floatDecl("reachMaxM") << " = " << reachName << "_sample(" << pxl << ".b);";

    _Add_Gamut_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, s, g);
}

void Add_Gamut_Compress_Inv_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    const float red_x   = (float) params[1];
    const float red_y   = (float) params[2];
    const float green_x = (float) params[3];
    const float green_y = (float) params[4];
    const float blue_x  = (float) params[5];
    const float blue_y  = (float) params[6];
    const float white_x = (float) params[7];
    const float white_y = (float) params[8];

    const Primaries primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    const ACES2::JMhParams pIn = ACES2::init_JMhParams(ACES_AP0::primaries);
    const ACES2::JMhParams pLim = ACES2::init_JMhParams(primaries);
    const ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    const ACES2::JMhParams reachGamut = ACES2::init_JMhParams(ACES_AP1::primaries);
    const ACES2::SharedCompressionParameters s = ACES2::init_SharedCompressionParams(peak_luminance, pIn, reachGamut);
    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, pIn, pLim, t, s, reachGamut);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();
    const std::string pxl(shaderCreator->getPixelName());

    const std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, s.reach_m_table);

    _Add_WrapHueChannel_Shader(shaderCreator, ss);
    _Add_SinCos_Shader(shaderCreator, ss);

    ss.newLine() << ss.floatDecl("reachMaxM") << " = " << reachName << "_sample(" << pxl << ".b);";

    _Add_Gamut_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, s, g);
}

void Add_Surround_10_Fwd_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss, float gamma)
{
    const std::string pxl(shaderCreator->getPixelName());

    // TODO: -- add vector inner product to GPUShaderUtils
    ss.newLine() << ss.floatDecl("Y")
                 << " = max( 1e-10, 0.27222871678091454 * " << pxl << ".rgb.r + "
                 <<                "0.67408176581114831 * " << pxl << ".rgb.g + "
                 <<                "0.053689517407937051 * " << pxl << ".rgb.b );";

    ss.newLine() << ss.floatDecl("Ypow_over_Y") << " = pow( Y, " << gamma - 1.f << ");";

    ss.newLine() << pxl << ".rgb = " << pxl << ".rgb * Ypow_over_Y;";
}

void Add_Rec2100_Surround_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss, 
                                 float gamma, bool isForward)
{
    const std::string pxl(shaderCreator->getPixelName());

    float minLum = 1e-4f;
    if (!isForward)
    {
        minLum = powf(minLum, gamma);
        gamma = 1.f / gamma;
    }

    ss.newLine() << ss.floatDecl("Y") 
                 << " = 0.2627 * " << pxl << ".rgb.r + "
                 <<    "0.6780 * " << pxl << ".rgb.g + "
                 <<    "0.0593 * " << pxl << ".rgb.b;";

    ss.newLine() << "Y = max( " << minLum << ", abs(Y) );";

    ss.newLine() << ss.floatDecl("Ypow_over_Y") << " = pow( Y, " << (gamma - 1.f) << ");";

    ss.newLine() << "" << pxl << ".rgb = " << pxl << ".rgb * Ypow_over_Y;";
}

void Add_RGB_TO_HSV(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("minRGB") << " = min( " << pxl << ".rgb.r, min( " << pxl << ".rgb.g, " << pxl << ".rgb.b ) );";
    ss.newLine() << ss.floatDecl("maxRGB") << " = max( " << pxl << ".rgb.r, max( " << pxl << ".rgb.g, " << pxl << ".rgb.b ) );";
    ss.newLine() << ss.floatDecl("val") << " = maxRGB;";

    ss.newLine() << ss.floatDecl("sat") << " = 0.0, hue = 0.0;";
    ss.newLine() << "if (minRGB != maxRGB)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << "if (val != 0.0) sat = (maxRGB - minRGB) / val;";
    ss.newLine() << ss.floatDecl("OneOverMaxMinusMin") << " = 1.0 / (maxRGB - minRGB);";
    ss.newLine() << "if ( maxRGB == " << pxl << ".rgb.r ) hue = (" << pxl << ".rgb.g - " << pxl << ".rgb.b) * OneOverMaxMinusMin;";
    ss.newLine() << "else if ( maxRGB == " << pxl << ".rgb.g ) hue = 2.0 + (" << pxl << ".rgb.b - " << pxl << ".rgb.r) * OneOverMaxMinusMin;";
    ss.newLine() << "else hue = 4.0 + (" << pxl << ".rgb.r - " << pxl << ".rgb.g) * OneOverMaxMinusMin;";
    ss.newLine() << "if ( hue < 0.0 ) hue += 6.0;";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "if ( minRGB < 0.0 ) val += minRGB;";
    ss.newLine() << "if ( -minRGB > maxRGB ) sat = (maxRGB - minRGB) / -minRGB;";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("hue * 1./6.", "sat", "val") << ";";
}

void Add_HSV_TO_RGB(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("Hue") << " = ( " << pxl << ".rgb.r - floor( " << pxl << ".rgb.r ) ) * 6.0;";
    ss.newLine() << ss.floatDecl("Sat") << " = clamp( " << pxl << ".rgb.g, 0., 1.999 );";
    ss.newLine() << ss.floatDecl("Val") << " = " << pxl << ".rgb.b;";

    ss.newLine() << ss.floatDecl("R") << " = abs(Hue - 3.0) - 1.0;";
    ss.newLine() << ss.floatDecl("G") << " = 2.0 - abs(Hue - 2.0);";
    ss.newLine() << ss.floatDecl("B") << " = 2.0 - abs(Hue - 4.0);";
    ss.newLine() << ss.float3Decl("RGB") << " = " << ss.float3Const("R", "G", "B") << ";";
    ss.newLine() << "RGB = clamp( RGB, 0., 1. );";

    ss.newLine() << ss.floatKeyword() << " rgbMax = Val;";
    ss.newLine() << ss.floatKeyword() << " rgbMin = Val * (1.0 - Sat);";

    ss.newLine() << "if ( Sat > 1.0 )";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "rgbMin = Val * (1.0 - Sat) / (2.0 - Sat);";
    ss.newLine() << "rgbMax = Val - rgbMin;";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "if ( Val < 0.0 )";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "rgbMin = Val / (2.0 - Sat);";
    ss.newLine() << "rgbMax = Val - rgbMin;";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "RGB = RGB * (rgbMax - rgbMin) + rgbMin;";

    ss.newLine() << "" << pxl << ".rgb = RGB;";
}

void Add_XYZ_TO_xyY(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("d") << " = " << pxl << ".rgb.r + " << pxl << ".rgb.g + " << pxl << ".rgb.b;";
    ss.newLine() << "d = (d == 0.) ? 0. : 1. / d;";
    ss.newLine() << pxl << ".rgb.b = " << pxl << ".rgb.g;";
    ss.newLine() << pxl << ".rgb.r *= d;";
    ss.newLine() << pxl << ".rgb.g *= d;";
}

void Add_xyY_TO_XYZ(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("d") << " = (" << pxl << ".rgb.g == 0.) ? 0. : 1. / " << pxl << ".rgb.g;";
    ss.newLine() << ss.floatDecl("Y") << " = " << pxl << ".rgb.b;";
    ss.newLine() << pxl << ".rgb.b = Y * (1. - " << pxl << ".rgb.r - " << pxl << ".rgb.g) * d;";
    ss.newLine() << pxl << ".rgb.r *= Y * d;";
    ss.newLine() << pxl << ".rgb.g = Y;";
}

void Add_XYZ_TO_uvY(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("d") << " = " << pxl << ".rgb.r + 15. * " << pxl << ".rgb.g + 3. * " << pxl << ".rgb.b;";
    ss.newLine() << "d = (d == 0.) ? 0. : 1. / d;";
    ss.newLine() << pxl << ".rgb.b = " << pxl << ".rgb.g;";
    ss.newLine() << pxl << ".rgb.r *= 4. * d;";
    ss.newLine() << pxl << ".rgb.g *= 9. * d;";
}

void Add_uvY_TO_XYZ(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("d") << " = (" << pxl << ".rgb.g == 0.) ? 0. : 1. / " << pxl << ".rgb.g;";
    ss.newLine() << ss.floatDecl("Y") << " = " << pxl << ".rgb.b;";
    ss.newLine() << pxl << ".rgb.b = (3./4.) * Y * (4. - " << pxl << ".rgb.r - 6.6666666666666667 * " << pxl << ".rgb.g) * d;";
    ss.newLine() << pxl << ".rgb.r *= (9./4.) * Y * d;";
    ss.newLine() << pxl << ".rgb.g = Y;";
}

void Add_XYZ_TO_LUV(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("d") << " = " << pxl << ".rgb.r + 15. * " << pxl << ".rgb.g + 3. * " << pxl << ".rgb.b;";
    ss.newLine() << "d = (d == 0.) ? 0. : 1. / d;";
    ss.newLine() << ss.floatDecl("u") << " = " << pxl << ".rgb.r * 4. * d;";
    ss.newLine() << ss.floatDecl("v") << " = " << pxl << ".rgb.g * 9. * d;";
    ss.newLine() << ss.floatDecl("Y") << " = " << pxl << ".rgb.g;";

    ss.newLine() << ss.floatDecl("Lstar") << " = " << ss.lerp( "1.16 * pow( max(0., Y), 1./3. ) - 0.16", "9.0329629629629608 * Y",
                                                               "float(Y <= 0.008856451679)" ) << ";";
    ss.newLine() << ss.floatDecl("ustar") << " = 13. * Lstar * (u - 0.19783001);";
    ss.newLine() << ss.floatDecl("vstar") << " = 13. * Lstar * (v - 0.46831999);";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("Lstar", "ustar", "vstar") << ";";
}

void Add_LUV_TO_XYZ(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("Lstar") << " = " << pxl << ".rgb.r;";
    ss.newLine() << ss.floatDecl("d") << " = (Lstar == 0.) ? 0. : 0.076923076923076927 / Lstar;";
    ss.newLine() << ss.floatDecl("u") << " = " << pxl << ".rgb.g * d + 0.19783001;";
    ss.newLine() << ss.floatDecl("v") << " = " << pxl << ".rgb.b * d + 0.46831999;";

    ss.newLine() << ss.floatDecl("tmp") << " = (Lstar + 0.16) * 0.86206896551724144;";
    ss.newLine() << ss.floatDecl("Y") << " = " << ss.lerp( "tmp * tmp * tmp", "0.11070564598794539 * Lstar", "float(Lstar <= 0.08)" ) << ";";

    ss.newLine() << ss.floatDecl("dd") << " = (v == 0.) ? 0. : 0.25 / v;";
    ss.newLine() << pxl << ".rgb.r = 9. * Y * u * dd;";
    ss.newLine() << pxl << ".rgb.b = Y * (12. - 3. * u - 20. * v) * dd;";
    ss.newLine() << pxl << ".rgb.g = Y;";
}


namespace 
{
namespace ST_2084
{
    static constexpr double m1 = 0.25 * 2610. / 4096.;
    static constexpr double m2 = 128. * 2523. / 4096.;
    static constexpr double c2 = 32. * 2413. / 4096.;
    static constexpr double c3 = 32. * 2392. / 4096.;
    static constexpr double c1 = c3 - c2 + 1.;
}
} // anonymous

void Add_LIN_TO_PQ(GpuShaderCreatorRcPtr& shaderCreator, GpuShaderText& ss)
{
    using namespace ST_2084;
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("sign3") << " = sign(" << pxl << ".rgb);";
    ss.newLine() << ss.float3Decl("L") << " = abs(0.01 * " << pxl << ".rgb);";
    ss.newLine() << ss.float3Decl("y") << " = pow(L, " << ss.float3Const(m1) << ");";
    ss.newLine() << ss.float3Decl("ratpoly") << " = (" << ss.float3Const(c1) << " + " << c2 << " * y) / ("
        << ss.float3Const(1.0) << " + " << c3 << " * y);";
    ss.newLine() << pxl << ".rgb = sign3 * pow(ratpoly, " << ss.float3Const(m2) << ");";

    // The sign transfer here is very slightly different than in the CPU path,
    // resulting in a PQ value of 0 at 0 rather than the true value of
    // 0.836^78.84 = 7.36e-07, however, this is well below visual threshold.
}

void Add_PQ_TO_LIN(GpuShaderCreatorRcPtr& shaderCreator, GpuShaderText& ss)
{
    using namespace ST_2084;
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("sign3") << " = sign(" << pxl << ".rgb);";
    ss.newLine() << ss.float3Decl("x") << " = pow(abs(" << pxl << ".rgb), " << ss.float3Const(1.0 / m2) << ");";
    ss.newLine() << pxl << ".rgb = 100. * sign3 * pow(max(" << ss.float3Const(0.0) << ", x - " << ss.float3Const(c1) << ") / ("
        << ss.float3Const(c2) << " - " << c3 << " * x), " << ss.float3Const(1.0 / m1) << ");";
}

void Add_LIN_TO_GAMMA_LOG(
    GpuShaderCreatorRcPtr& shaderCreator, 
    GpuShaderText& ss,
    const FixedFunctionOpData::Params& params)
{
    // Get parameters, baking the log base conversion into 'logSlope'.
    double mirrorPt          = params[0];
    double breakPt           = params[1];
    double gammaSeg_power    = params[2];
    double gammaSeg_slope    = params[3];
    double gammaSeg_off      = params[4];
    double logSeg_base       = params[5];
    double logSeg_logSlope   = params[6] / std::log(logSeg_base);
    double logSeg_logOff     = params[7];
    double logSeg_linSlope   = params[8];
    double logSeg_linOff     = params[9];

    // float mirrorin = in - m_mirror;
    // float E = std::abs(mirrorin) + m_mirror;
    // float Eprime;
    // if (E < m_break)
    //     Eprime = m_gammaSeg.slope * std::pow(E + m_gammaSeg.off, m_gammaSeg.power);
    // else
    //     Eprime = m_logSeg.logSlope * std::log(m_logSeg.linSlope * E + m_logSeg.linOff) + m_logSeg.logOff;
    // out = Eprime * std::copysign(1.0, in);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("mirrorin") << " = " << pxl << ".rgb - " << ss.float3Const(mirrorPt) << ";";
    ss.newLine() << ss.float3Decl("sign3") << " = sign(mirrorin);";
    ss.newLine() << ss.float3Decl("E") << " = abs(mirrorin) + " << ss.float3Const(mirrorPt) << ";";
    ss.newLine() << ss.float3Decl("isAboveBreak") << " = " << ss.float3GreaterThan("E", ss.float3Const(breakPt)) << ";";
    ss.newLine() << ss.float3Decl("isAtOrBelowBreak") << " = " << ss.float3Const(1.0f) << " - isAboveBreak;";

    ss.newLine() << ss.float3Decl("Ep_gamma") << " = " << ss.float3Const(gammaSeg_slope)
                 << " * pow( E - " << ss.float3Const(gammaSeg_off) << ", " << ss.float3Const(gammaSeg_power) << ");";

    // Avoid NaNs by clamping log input below 1 if the branch will not be used.
    ss.newLine() << ss.float3Decl("Ep_clamped") << " = max( isAtOrBelowBreak, E * "
                 << ss.float3Const(logSeg_linSlope) << " + " << ss.float3Const(logSeg_linOff) << " );";
    ss.newLine() << ss.float3Decl("Ep_log") << " = " << ss.float3Const(logSeg_logSlope) << " * log( Ep_clamped ) + "  
                 << ss.float3Const(logSeg_logOff) << ";";

    // Combine log and gamma parts.
    ss.newLine() << pxl << ".rgb = sign3 * (isAboveBreak * Ep_log + ( " << ss.float3Const(1.0f) 
                 << " - isAboveBreak ) * Ep_gamma);";
}

void Add_GAMMA_LOG_TO_LIN(
    GpuShaderCreatorRcPtr& shaderCreator, 
    GpuShaderText& ss,
    const FixedFunctionOpData::Params& params)
{
    // Get parameters, baking the log base conversion into 'logSlope'.
    double mirrorPt          = params[0];
    double breakPt           = params[1];
    double gammaSeg_power    = params[2];
    double gammaSeg_slope    = params[3];
    double gammaSeg_off      = params[4];
    double logSeg_base       = params[5];
    double logSeg_logSlope   = params[6] / std::log(logSeg_base);
    double logSeg_logOff     = params[7];
    double logSeg_linSlope   = params[8];
    double logSeg_linOff     = params[9];

    double primeBreak  = gammaSeg_slope * std::pow(breakPt  + gammaSeg_off, gammaSeg_power);
    double primeMirror = gammaSeg_slope * std::pow(mirrorPt + gammaSeg_off, gammaSeg_power);

    // float mirrorin = in - primeMirror;
    // float Eprime = std::abs(mirrorin) + primeMirror;
    // if (Eprime < m_primeBreak)
    //     E = std::pow(Eprime / m_gammaSeg.slope, 1.0f / m_gammaSeg.power) - m_gammaSeg.off;
    // else
    //     E = (std::exp((Eprime - m_logSeg.logOff) / m_logSeg.logSlope) - m_logSeg.linOff) / m_logSeg.linSlope;
    // out = std::copysign(E, Eprimein);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("mirrorin") << " = " << pxl << ".rgb - " << ss.float3Const(primeMirror) << ";";
    ss.newLine() << ss.float3Decl("sign3") << " = sign(mirrorin);";
    ss.newLine() << ss.float3Decl("Eprime") << " = abs(mirrorin) + " << ss.float3Const(primeMirror) << ";";
    ss.newLine() << ss.float3Decl("isAboveBreak") << " = " << ss.float3GreaterThan("Eprime", ss.float3Const(primeBreak)) << ";";
   
    // Gamma Segment.
    ss.newLine() << ss.float3Decl("E_gamma") << " = pow( Eprime * " << ss.float3Const(1.0/gammaSeg_slope) << ","
        << ss.float3Const(1.0/gammaSeg_power) << ") - " << ss.float3Const(gammaSeg_off) << ";";
    
    // Log Segment.
    ss.newLine() << ss.float3Decl("E_log") << " = (exp((Eprime - " << ss.float3Const(logSeg_logOff) << ") * " 
        << ss.float3Const(1.0/logSeg_logSlope) << ") - " << ss.float3Const(logSeg_linOff) << ") * "
        << ss.float3Const(1.0/logSeg_linSlope) << ";";
    
    // Combine log and gamma parts.
    ss.newLine() << pxl << ".rgb = sign3 * (isAboveBreak * E_log + ( " << ss.float3Const(1.0f) << " - isAboveBreak ) * E_gamma);";
}

void Add_LIN_TO_DOUBLE_LOG(
    GpuShaderCreatorRcPtr& shaderCreator, 
    GpuShaderText& ss, 
    const FixedFunctionOpData::Params& params)
{
    // Get parameters, baking the log base conversion into 'logSlope'.
    double base             = params[0];
    double break1           = params[1];
    double break2           = params[2];
    double logSeg1_logSlope = params[3] / std::log(base);
    double logSeg1_logOff   = params[4];
    double logSeg1_linSlope = params[5];
    double logSeg1_linOff   = params[6];
    double logSeg2_logSlope = params[7] / std::log(base);
    double logSeg2_logOff   = params[8];
    double logSeg2_linSlope = params[9];
    double logSeg2_linOff   = params[10];
    double linSeg_slope     = params[11];
    double linSeg_off       = params[12];

    // Linear segment may not exist or be valid, thus we include the break
    // points in the log segments. Also passing zero or negative value to the
    // log functions are not guarded for, it should be guaranteed by the
    // parameters for the expected working range.
    
    //if (in <= m_break1)
    //    out = m_logSeg1.logSlope * std::log(m_logSeg1.linSlope * x + m_logSeg1.linOff) + m_logSeg1.logOff;
    //else if (in < m_break2)
    //    out = m_linSeg.slope * x + m_linSeg.off;
    //else
    //    out = m_logSeg2.logSlope * std::log(m_logSeg2.linSlope * x + m_logSeg2.linOff) + m_logSeg2.logOff;
    
    const std::string pix(shaderCreator->getPixelName());
    const std::string pix3 = pix + ".rgb";

    ss.newLine() << ss.float3Decl("isSegment1") << " = " << ss.float3GreaterThanEqual(ss.float3Const(break1), pix3) << ";";
    ss.newLine() << ss.float3Decl("isSegment3") << " = " << ss.float3GreaterThanEqual(pix3, ss.float3Const(break2)) << ";";
    ss.newLine() << ss.float3Decl("isSegment2") << " = " << ss.float3Const(1.0f) << " - isSegment1 - isSegment3;";

    // Log Segment 1.
    // TODO: This segment usually handles very dark (even negative) values, thus
    // is rarely hit. As an optimization we can use "any()" to skip this in a
    // branch (needs benchmarking to see if it's worth the effort).
    ss.newLine();
    ss.newLine() << ss.float3Decl("logSeg1") << " = " << 
        pix3 << " * " << ss.float3Const(logSeg1_linSlope) << " + " << ss.float3Const(logSeg1_linOff) << ";";

    // Clamp below 1 to avoid NaNs if the branch will not be used.
    ss.newLine() << "logSeg1 = max( " << ss.float3Const(1.0) << " - isSegment1, logSeg1 );";

    ss.newLine() << "logSeg1 = " << 
        ss.float3Const(logSeg1_logSlope) << " * log( logSeg1 ) + " << ss.float3Const(logSeg1_logOff) << ";";

    // Log Segment 2.
    ss.newLine();
    ss.newLine() << ss.float3Decl("logSeg2") << " = " <<
        pix3 << " * " << ss.float3Const(logSeg2_linSlope) << " + " << ss.float3Const(logSeg2_linOff) << ";";

    // Clamp below 1 to avoid NaNs if the branch will not be used.
    ss.newLine() << "logSeg2 = max( " << ss.float3Const(1.0) << " - isSegment3, logSeg2 );";

    ss.newLine() << "logSeg2 = " <<
        ss.float3Const(logSeg2_logSlope) << " * log( logSeg2 ) + " << ss.float3Const(logSeg2_logOff) << ";";

    // Linear Segment.
    ss.newLine();
    ss.newLine() << ss.float3Decl("linSeg") << "= " << 
        ss.float3Const(linSeg_slope) << " * " << pix3 << " + " << ss.float3Const(linSeg_off) << ";";

    // Combine segments.
    ss.newLine();
    ss.newLine() << pix3 << " = isSegment1 * logSeg1 + isSegment2 * linSeg + isSegment3 * logSeg2;"; 
}

void Add_DOUBLE_LOG_TO_LIN(
    GpuShaderCreatorRcPtr& shaderCreator, 
    GpuShaderText& ss, 
    const FixedFunctionOpData::Params& params)
{
    // Baking the log base conversion into 'logSlope'.
    double base             = params[0];
    double break1           = params[1];
    double break2           = params[2];
    double logSeg1_logSlope = params[3] / std::log(base);
    double logSeg1_logOff   = params[4];
    double logSeg1_linSlope = params[5];
    double logSeg1_linOff   = params[6];
    double logSeg2_logSlope = params[7] / std::log(base);
    double logSeg2_logOff   = params[8];
    double logSeg2_linSlope = params[9];
    double logSeg2_linOff   = params[10];
    double linSeg_slope     = params[11];
    double linSeg_off       = params[12];

    double break1Log = logSeg1_logSlope * std::log(logSeg1_linSlope * break1 + logSeg1_linOff) + logSeg1_logOff;
    double break2Log = logSeg2_logSlope * std::log(logSeg2_linSlope * break2 + logSeg2_linOff) + logSeg2_logOff;

    // if (y <= m_break1Log)
    //     x = (std::exp((y - m_logSeg1.logOff) / m_logSeg1.logSlope) - m_logSeg1.linOff) / m_logSeg1.linSlope;
    // else if (y < m_break2Log)
    //     x = (y - m_linSeg.off) / m_linSeg.slope;
    // else
    //     x = (std::exp((y - m_logSeg2.logOff) / m_logSeg2.logSlope) - m_logSeg2.linOff) / m_logSeg2.linSlope;

    const std::string pix(shaderCreator->getPixelName());
    const std::string pix3 = pix + ".rgb";

    // This assumes the forward function is monotonically increasing.
    ss.newLine() << ss.float3Decl("isSegment1") << " = " << ss.float3GreaterThanEqual(ss.float3Const(break1Log), pix3) << ";";
    ss.newLine() << ss.float3Decl("isSegment3") << " = " << ss.float3GreaterThanEqual(pix3, ss.float3Const(break2Log)) << ";";
    ss.newLine() << ss.float3Decl("isSegment2") << " = " << ss.float3Const(1.0f) << " - isSegment1 - isSegment3;";

    // Log Segment 1.
    // TODO: This segment usually handles very dark (even negative) values, thus
    // is rarely hit. As an optimization we can use "any()" to skip this in a
    // branch (needs benchmarking to see if it's worth the effort).
    ss.newLine();
    ss.newLine() << ss.float3Decl("logSeg1") << " = (" <<
        pix3 << " - " << ss.float3Const(logSeg1_logOff) << ") * " << ss.float3Const(1.0 / logSeg1_logSlope) << ";";
    ss.newLine() << "logSeg1 = (" <<
        "exp(logSeg1) - " << ss.float3Const(logSeg1_linOff) << ") * " << ss.float3Const(1.0 / logSeg1_linSlope) << ";";

    // Log Segment 2.
    ss.newLine();
    ss.newLine() << ss.float3Decl("logSeg2") << " = (" <<
        pix3 << " - " << ss.float3Const(logSeg2_logOff) << ") * " << ss.float3Const(1.0 / logSeg2_logSlope) << ";";
    ss.newLine() << "logSeg2 = (" <<
        "exp(logSeg2) - " << ss.float3Const(logSeg2_linOff) << ") * " << ss.float3Const(1.0 / logSeg2_linSlope) << ";";

    // Linear Segment.
    ss.newLine();
    ss.newLine() << ss.float3Decl("linSeg") << " = (" <<
        pix3 << " - " << ss.float3Const(linSeg_off) << ") * " << ss.float3Const(1.0 / linSeg_slope) << ";";

    // Combine segments.
    ss.newLine();
    ss.newLine() << pix3 << " = isSegment1 * logSeg1 + isSegment2 * linSeg + isSegment3 * logSeg2;";
}

void GetFixedFunctionGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                      ConstFixedFunctionOpDataRcPtr & func)
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add FixedFunction '"
                 << FixedFunctionOpData::ConvertStyleToString(func->getStyle(), true)
                 << "' processing";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();

    switch(func->getStyle())
    {
        case FixedFunctionOpData::ACES_RED_MOD_03_FWD:
        {
            Add_RedMod_03_Fwd_Shader(shaderCreator, ss);
            break;
         }
        case FixedFunctionOpData::ACES_RED_MOD_03_INV:
        {
            Add_RedMod_03_Inv_Shader(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::ACES_RED_MOD_10_FWD:
        {
            Add_RedMod_10_Fwd_Shader(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::ACES_RED_MOD_10_INV:
        {
            Add_RedMod_10_Inv_Shader(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_03_FWD:
        {
            Add_Glow_03_Fwd_Shader(shaderCreator, ss, 0.075f, 0.1f);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_03_INV:
        {
            Add_Glow_03_Inv_Shader(shaderCreator, ss, 0.075f, 0.1f);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_10_FWD:
        {
            // Use 03 renderer with different params.
            Add_Glow_03_Fwd_Shader(shaderCreator, ss, 0.05f, 0.08f);
            break;
        }
        case FixedFunctionOpData::ACES_GLOW_10_INV:
        {
            // Use 03 renderer with different params.
            Add_Glow_03_Inv_Shader(shaderCreator, ss, 0.05f, 0.08f);
            break;
        }
        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD:
        {
            Add_Surround_10_Fwd_Shader(shaderCreator, ss, 0.9811f);
            break;
        }
        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV:
        {
            // Call forward renderer with the inverse gamma.
            Add_Surround_10_Fwd_Shader(shaderCreator, ss, 1.0192640913260627f);
            break;
        }
        case FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD:
        {
            Add_GamutComp_13_Fwd_Shader(
                ss,
                shaderCreator,
                (float) func->getParams()[0],
                (float) func->getParams()[1],
                (float) func->getParams()[2],
                (float) func->getParams()[3],
                (float) func->getParams()[4],
                (float) func->getParams()[5],
                (float) func->getParams()[6]
            );
            break;
        }
        case FixedFunctionOpData::ACES_GAMUT_COMP_13_INV:
        {
            Add_GamutComp_13_Inv_Shader(
                ss,
                shaderCreator,
                (float) func->getParams()[0],
                (float) func->getParams()[1],
                (float) func->getParams()[2],
                (float) func->getParams()[3],
                (float) func->getParams()[4],
                (float) func->getParams()[5],
                (float) func->getParams()[6]
            );
            break;
        }
        case FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD:
        {
            Add_ACES_OutputTransform_Fwd_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV:
        {
            Add_ACES_OutputTransform_Inv_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::ACES_RGB_TO_JMh_20:
        {
            Add_RGB_to_JMh_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::ACES_JMh_TO_RGB_20:
        {
            Add_JMh_to_RGB_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_FWD:
        {
            Add_Tonescale_Compress_Fwd_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_INV:
        {
            Add_Tonescale_Compress_Inv_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_FWD:
        {
            Add_Gamut_Compress_Fwd_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_INV:
        {
            Add_Gamut_Compress_Inv_Shader(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::REC2100_SURROUND_FWD:
        {
            Add_Rec2100_Surround_Shader(shaderCreator, ss, 
                                        (float) func->getParams()[0], true);
            break;
        }
        case FixedFunctionOpData::REC2100_SURROUND_INV:
        {
            Add_Rec2100_Surround_Shader(shaderCreator, ss, 
                                        (float) func->getParams()[0], false);
            break;
        }
        case FixedFunctionOpData::RGB_TO_HSV:
        {
            Add_RGB_TO_HSV(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::HSV_TO_RGB:
        {
            Add_HSV_TO_RGB(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::XYZ_TO_xyY:
        {
            Add_XYZ_TO_xyY(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::xyY_TO_XYZ:
        {
            Add_xyY_TO_XYZ(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::XYZ_TO_uvY:
        {
            Add_XYZ_TO_uvY(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::uvY_TO_XYZ:
        {
            Add_uvY_TO_XYZ(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::XYZ_TO_LUV:
        {
            Add_XYZ_TO_LUV(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::LUV_TO_XYZ:
        {
            Add_LUV_TO_XYZ(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::LIN_TO_PQ:
        {
            Add_LIN_TO_PQ(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::PQ_TO_LIN:
        {
            Add_PQ_TO_LIN(shaderCreator, ss);
            break;
        }
        case FixedFunctionOpData::LIN_TO_GAMMA_LOG:
        {
            Add_LIN_TO_GAMMA_LOG(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::GAMMA_LOG_TO_LIN:
        {
            Add_GAMMA_LOG_TO_LIN(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::LIN_TO_DOUBLE_LOG:
        {
            Add_LIN_TO_DOUBLE_LOG(shaderCreator, ss, func->getParams());
            break;
        }
        case FixedFunctionOpData::DOUBLE_LOG_TO_LIN:
        {
            Add_DOUBLE_LOG_TO_LIN(shaderCreator, ss, func->getParams());
            break;
        }

    }

    ss.dedent();
    ss.newLine() << "}";

    ss.dedent();
    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

} // OCIO_NAMESPACE
