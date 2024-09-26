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

void _Add_RGB_to_JMh_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float3Decl("lms") << " = " << ss.mat3fMul(&p.MATRIX_RGB_to_CAM16[0], pxl + ".rgb") << ";";
    ss.newLine() << "lms = " << "lms * " << ss.float3Const(p.D_RGB[0], p.D_RGB[1], p.D_RGB[2]) << ";";

    ss.newLine() << ss.float3Decl("F_L_v") << " = pow(" << p.F_L << " * abs(lms) / 100.0, " << ss.float3Const(0.42f) << ");";
    ss.newLine() << ss.float3Decl("rgb_a") << " = (400.0 * sign(lms) * F_L_v) / (27.13 + F_L_v);";

    ss.newLine() << ss.floatDecl("A") << " = 2.0 * rgb_a.r + rgb_a.g + 0.05 * rgb_a.b;";
    ss.newLine() << ss.floatDecl("a") << " = rgb_a.r - 12.0 * rgb_a.g / 11.0 + rgb_a.b / 11.0;";
    ss.newLine() << ss.floatDecl("b") << " = (rgb_a.r + rgb_a.g - 2.0 * rgb_a.b) / 9.0;";

    ss.newLine() << ss.floatDecl("J") << " = 100.0 * pow(A / " << p.A_w << ", " << ACES2::surround[1] << " * " << p.z << ");";

    ss.newLine() << ss.floatDecl("M") << " = (J == 0.0) ? 0.0 : 43.0 * " << ACES2::surround[2] << " * sqrt(a * a + b * b);";

    ss.newLine() << ss.floatDecl("h") << " = (a == 0.0) ? 0.0 : " << ss.atan2("b", "a") << " * 180.0 / 3.14159265358979;";
    ss.newLine() << "h = h - floor(h / 360.0) * 360.0;";
    ss.newLine() << "h = (h < 0.0) ? h + 360.0 : h;";

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("J", "M", "h") << ";";
}

void _Add_JMh_to_RGB_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const ACES2::JMhParams & p)
{
    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("h") << " = " << pxl << ".b * 3.14159265358979 / 180.0;";

    ss.newLine() << ss.floatDecl("scale") << " = " << pxl << ".g / (43.0 * " << ACES2::surround[2] << ");";
    ss.newLine() << ss.floatDecl("A") << " = " << p.A_w << " * pow(" << pxl << ".r / 100.0, 1.0 / (" << ACES2::surround[1] << " * " << p.z << "));";
    ss.newLine() << ss.floatDecl("a") << " = scale * cos(h);";
    ss.newLine() << ss.floatDecl("b") << " = scale * sin(h);";

    ss.newLine() << ss.float3Decl("rgb_a") << ";";
    ss.newLine() << "rgb_a.r = (460.0 * A + 451.0 * a + 288.0 *b) / 1403.0;";
    ss.newLine() << "rgb_a.g = (460.0 * A - 891.0 * a - 261.0 *b) / 1403.0;";
    ss.newLine() << "rgb_a.b = (460.0 * A - 220.0 * a - 6300.0 *b) / 1403.0;";

    ss.newLine() << ss.float3Decl("lms") << " = sign(rgb_a) * 100.0 / " << p.F_L << " * pow(27.13 * abs(rgb_a) / (400.0 - abs(rgb_a)), " << ss.float3Const(1.f / 0.42f) << ");";
    ss.newLine() << "lms = " << "lms / " << ss.float3Const(p.D_RGB[0], p.D_RGB[1], p.D_RGB[2]) << ";";

    ss.newLine() << pxl << ".rgb = " << ss.mat3fMul(&p.MATRIX_CAM16_to_RGB[0], "lms") << ";";
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
            << std::string("reach_m_table")
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
        table.size,
        1,
        GpuShaderCreator::TEXTURE_RED_CHANNEL,
        dimensions,
        INTERP_NEAREST,
        &(table.table[0]));


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

    ss.newLine() << ss.floatDecl("hwrap") << " = h;";
    ss.newLine() << "hwrap = hwrap - floor(hwrap / 360.0) * 360.0;";
    ss.newLine() << "hwrap = (hwrap < 0.0) ? hwrap + 360.0 : hwrap;";

    ss.newLine() << ss.floatDecl("i_lo") << " = floor(hwrap);";
    ss.newLine() << ss.floatDecl("i_hi") << " = (i_lo + 1);";
    ss.newLine() << "i_hi = i_hi - floor(i_hi / 360.0) * 360.0;";

    if (dimensions == GpuShaderDesc::TEXTURE_1D)
    {
        ss.newLine() << ss.floatDecl("lo") << " = " << ss.sampleTex1D(name, "(i_lo + 0.5) / 360.0") << ".r;";
        ss.newLine() << ss.floatDecl("hi") << " = " << ss.sampleTex1D(name, "(i_hi + 0.5) / 360.0") << ".r;";
    }
    else
    {
        ss.newLine() << ss.floatDecl("lo") << " = " << ss.sampleTex2D(name, ss.float2Const("(i_lo + 0.5) / 360.0", "0.0")) << ".r;";
        ss.newLine() << ss.floatDecl("hi") << " = " << ss.sampleTex2D(name, ss.float2Const("(i_hi + 0.5) / 360.0", "0.5")) << ".r;";
    }

    ss.newLine() << ss.floatDecl("t") << " = (h - i_lo) / (i_hi - i_lo);";
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

void _Add_Tonescale_Compress_Fwd_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    unsigned resourceIndex,
    const ACES2::JMhParams & p,
    const ACES2::ToneScaleParams & t,
    const ACES2::ChromaCompressParams & c,
    const std::string & reachName)
{
    std::string toeName = _Add_Toe_func(shaderCreator, resourceIndex, false);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("J") << " = " << pxl << ".r;";
    ss.newLine() << ss.floatDecl("M") << " = " << pxl << ".g;";
    ss.newLine() << ss.floatDecl("h") << " = " << pxl << ".b;";

    // Tonescale applied in Y (convert to and from J)
    ss.newLine() << ss.floatDecl("A") << " = " << p.A_w_J << " * pow(abs(J) / 100.0, 1.0 / (" << ACES2::surround[1] << " * " << p.z << "));";
    ss.newLine() << ss.floatDecl("Y") << " = sign(J) * 100.0 / " << p.F_L << " * pow((27.13 * A) / (400.0 - A), 1.0 / 0.42) / 100.0;";

    ss.newLine() << ss.floatDecl("f") << " = " << t.m_2  << " * pow(max(0.0, Y) / (Y + " << t.s_2 << "), " << t.g << ");";
    ss.newLine() << ss.floatDecl("Y_ts") << " = max(0.0, f * f / (f + " << t.t_1 << ")) * " << t.n_r << ";";

    ss.newLine() << ss.floatDecl("F_L_Y") << " = pow(" << p.F_L << " * abs(Y_ts) / 100.0, 0.42);";
    ss.newLine() << ss.floatDecl("J_ts") << " = sign(Y_ts) * 100.0 * pow(((400.0 * F_L_Y) / (27.13 + F_L_Y)) / " << p.A_w_J << ", " << ACES2::surround[1] << " * " << p.z << ");";

    // ChromaCompress
    ss.newLine() << ss.floatDecl("M_cp") << " = M;";

    ss.newLine() << "if (M != 0.0)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("nJ") << " = J_ts / " << c.limit_J_max << ";";
    ss.newLine() << ss.floatDecl("snJ") << " = max(0.0, 1.0 - nJ);";

    // Mnorm
    ss.newLine() << ss.floatDecl("Mnorm") << ";";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("PI") << " = 3.14159265358979;";
    ss.newLine() << ss.floatDecl("h_rad") << " = h / 180.0 * PI;";
    ss.newLine() << ss.floatDecl("a") << " = cos(h_rad);";
    ss.newLine() << ss.floatDecl("b") << " = sin(h_rad);";
    ss.newLine() << ss.floatDecl("cos_hr2") << " = a * a - b * b;";
    ss.newLine() << ss.floatDecl("sin_hr2") << " = 2.0 * a * b;";
    ss.newLine() << ss.floatDecl("cos_hr3") << " = 4.0 * a * a * a - 3.0 * a;";
    ss.newLine() << ss.floatDecl("sin_hr3") << " = 3.0 * b - 4.0 * b * b * b;";
    ss.newLine() << ss.floatDecl("M") << " = 11.34072 * a + 16.46899 * cos_hr2 + 7.88380 * cos_hr3 + 14.66441 * b + -6.37224 * sin_hr2 + 9.19364 * sin_hr3 + 77.12896;";
    ss.newLine() << "Mnorm = M * " << c.chroma_compress_scale << ";";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.floatDecl("reachM") << " = " << reachName << "_sample(h);";
    ss.newLine() << ss.floatDecl("limit") << " = pow(nJ, " << c.model_gamma << ") * reachM / Mnorm;";
    ss.newLine() << "M_cp = M * pow(J_ts / J, " << c.model_gamma << ");";
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
    const ACES2::JMhParams & p,
    const ACES2::ToneScaleParams & t,
    const ACES2::ChromaCompressParams & c,
    const std::string & reachName)
{
    std::string toeName = _Add_Toe_func(shaderCreator, resourceIndex, true);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.floatDecl("J_ts") << " = " << pxl << ".r;";
    ss.newLine() << ss.floatDecl("M_cp") << " = " << pxl << ".g;";
    ss.newLine() << ss.floatDecl("h") << " = " << pxl << ".b;";

    // Inverse Tonescale applied in Y (convert to and from J)
    ss.newLine() << ss.floatDecl("A") << " = " << p.A_w_J << " * pow(abs(J_ts) / 100.0, 1.0 / (" << ACES2::surround[1] << " * " << p.z << "));";
    ss.newLine() << ss.floatDecl("Y_ts") << " = sign(J_ts) * 100.0 / " << p.F_L << " * pow((27.13 * A) / (400.0 - A), 1.0 / 0.42) / 100.0;";

    ss.newLine() << ss.floatDecl("Z") << " = max(0.0, min(" << t.n << " / (" << t.u_2 * t.n_r << "), Y_ts));";
    ss.newLine() << ss.floatDecl("ht") << " = (Z + sqrt(Z * (4.0 * " << t.t_1 << " + Z))) / 2.0;";
    ss.newLine() << ss.floatDecl("Y") << " = " << t.s_2 << " / (pow((" << t.m_2 << " / ht), (1.0 / " << t.g << ")) - 1.0);";

    ss.newLine() << ss.floatDecl("F_L_Y") << " = pow(" << p.F_L << " * abs(Y * 100.0) / 100.0, 0.42);";
    ss.newLine() << ss.floatDecl("J") << " = sign(Y) * 100.0 * pow(((400.0 * F_L_Y) / (27.13 + F_L_Y)) / " << p.A_w_J << ", " << ACES2::surround[1] << " * " << p.z << ");";

    // ChromaCompress
    ss.newLine() << ss.floatDecl("M") << " = M_cp;";

    ss.newLine() << "if (M_cp != 0.0)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("nJ") << " = J_ts / " << c.limit_J_max << ";";
    ss.newLine() << ss.floatDecl("snJ") << " = max(0.0, 1.0 - nJ);";

    // Mnorm
    ss.newLine() << ss.floatDecl("Mnorm") << ";";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("PI") << " = 3.14159265358979;";
    ss.newLine() << ss.floatDecl("h_rad") << " = h / 180.0 * PI;";
    ss.newLine() << ss.floatDecl("a") << " = cos(h_rad);";
    ss.newLine() << ss.floatDecl("b") << " = sin(h_rad);";
    ss.newLine() << ss.floatDecl("cos_hr2") << " = a * a - b * b;";
    ss.newLine() << ss.floatDecl("sin_hr2") << " = 2.0 * a * b;";
    ss.newLine() << ss.floatDecl("cos_hr3") << " = 4.0 * a * a * a - 3.0 * a;";
    ss.newLine() << ss.floatDecl("sin_hr3") << " = 3.0 * b - 4.0 * b * b * b;";
    ss.newLine() << ss.floatDecl("M") << " = 11.34072 * a + 16.46899 * cos_hr2 + 7.88380 * cos_hr3 + 14.66441 * b + -6.37224 * sin_hr2 + 9.19364 * sin_hr3 + 77.12896;";
    ss.newLine() << "Mnorm = M * " << c.chroma_compress_scale << ";";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.floatDecl("reachM") << " = " << reachName << "_sample(h);";
    ss.newLine() << ss.floatDecl("limit") << " = pow(nJ, " << c.model_gamma << ") * reachM / Mnorm;";

    ss.newLine() << "M = M_cp / Mnorm;";
    ss.newLine() << "M = " << toeName << "(M, limit, nJ * " << c.compr << ", snJ);";
    ss.newLine() << "M = limit - " << toeName << "(limit - M, limit - 0.001, snJ * " << c.sat << ", sqrt(nJ * nJ + " << c.sat_thr << "));";
    ss.newLine() << "M = M * Mnorm;";
    ss.newLine() << "M = M * pow(J_ts / J, " << -c.model_gamma << ");";

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
            << std::string("gamut_cusp_table")
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
        &(g.gamut_cusp_table.table[0][0]));

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
    std::vector<float> hues_array(g.gamut_cusp_table.total_size);
    for (int i = 0; i < g.gamut_cusp_table.total_size; ++i)
    {
        hues_array[i] = g.gamut_cusp_table.table[i][2];
    }
    ss.declareFloatArrayConst(hues_array_name, (int) hues_array.size(), hues_array.data());

    ss.newLine() << ss.float2Keyword() << " " << name << "_sample(float h)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("i_lo") << " = 0;";
    ss.newLine() << ss.floatDecl("i_hi") << " = " << g.gamut_cusp_table.base_index + g.gamut_cusp_table.size << ";";

    ss.newLine() << ss.floatDecl("hwrap") << " = h;";
    ss.newLine() << "hwrap = hwrap - floor(hwrap / 360.0) * 360.0;";
    ss.newLine() << "hwrap = (hwrap < 0.0) ? hwrap + 360.0 : hwrap;";
    ss.newLine() << ss.intDecl("i") << " = " << ss.intKeyword() << "(hwrap + " << g.gamut_cusp_table.base_index << ");";

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
    ss.newLine() << "i = " << ss.intKeyword() << "((i_lo + i_hi) / 2.0);";

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

    ss.newLine() << ss.floatDecl("t") << " = (h - lo.b) / (hi.b - lo.b);";
    ss.newLine() << ss.floatDecl("cuspJ") << " = " << ss.lerp("lo.r", "hi.r", "t") << ";";
    ss.newLine() << ss.floatDecl("cuspM") << " = " << ss.lerp("lo.g", "hi.g", "t") << ";";

    ss.newLine() << "return " << ss.float2Const("cuspJ", "cuspM") << ";";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Gamma_table(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::GamutCompressParams & g)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("upper_hull_gamma_table")
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
        GpuShaderCreator::TEXTURE_RED_CHANNEL,
        dimensions,
        INTERP_NEAREST,
        &(g.upper_hull_gamma_table.table[0]));


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

    ss.newLine() << ss.floatDecl("hwrap") << " = h;";
    ss.newLine() << "hwrap = hwrap - floor(hwrap / 360.0) * 360.0;";
    ss.newLine() << "hwrap = (hwrap < 0.0) ? hwrap + 360.0 : hwrap;";

    ss.newLine() << ss.floatDecl("i_lo") << " = floor(hwrap) + " << g.upper_hull_gamma_table.base_index << ";";
    ss.newLine() << ss.floatDecl("i_hi") << " = (i_lo + 1);";
    ss.newLine() << "i_hi = i_hi - floor(i_hi / 360.0) * 360.0;";

    ss.newLine() << ss.floatDecl("base_hue") << " = i_lo - " << g.upper_hull_gamma_table.base_index << ";";

    if (dimensions == GpuShaderDesc::TEXTURE_1D)
    {
        ss.newLine() << ss.floatDecl("lo") << " = " << ss.sampleTex1D(name, std::string("(i_lo + 0.5) / ") + std::to_string(g.upper_hull_gamma_table.total_size)) << ".r;";
        ss.newLine() << ss.floatDecl("hi") << " = " << ss.sampleTex1D(name, std::string("(i_hi + 0.5) / ") + std::to_string(g.upper_hull_gamma_table.total_size)) << ".r;";
    }
    else
    {
        ss.newLine() << ss.floatDecl("lo") << " = " << ss.sampleTex2D(name, ss.float2Const(std::string("(i_lo + 0.5) / ") + std::to_string(g.upper_hull_gamma_table.total_size), "0.5")) << ".r;";
        ss.newLine() << ss.floatDecl("hi") << " = " << ss.sampleTex2D(name, ss.float2Const(std::string("(i_hi + 0.5) / ") + std::to_string(g.upper_hull_gamma_table.total_size), "0.5")) << ".r;";
    }

    ss.newLine() << ss.floatDecl("t") << " = hwrap - base_hue;";
    ss.newLine() << "return " << ss.lerp("lo", "hi", "t") << ";";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Focus_Gain_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::GamutCompressParams & g)
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

    ss.newLine() << ss.floatDecl("thr") << " = " << ss.lerp("cuspJ", std::to_string(g.limit_J_max), std::to_string(ACES2::focus_gain_blend)) << ";";

    ss.newLine() << "if (J > thr)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.floatDecl("gain") << " = ( " << g.limit_J_max << " - thr) / max(0.0001, (" << g.limit_J_max << " - min(" << g.limit_J_max << ", J)));";
    ss.newLine() << "return pow(log(gain)/log(10.0), 1.0 / " << ACES2::focus_adjust_gain << ") + 1.0;";
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
    const ACES2::GamutCompressParams & g)
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

    ss.newLine() << ss.floatDecl("a") << " = " << "M / (focusJ * slope_gain);";
    ss.newLine() << ss.floatDecl("b") << " = 0.0;";
    ss.newLine() << ss.floatDecl("c") << " = 0.0;";
    ss.newLine() << ss.floatDecl("intersectJ") << " = 0.0;";

    ss.newLine() << "if (J < focusJ)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "b = 1.0 - M / slope_gain;";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "b = - (1.0 + M / slope_gain + " << g.limit_J_max << " * M / (focusJ * slope_gain));";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "if (J < focusJ)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "c = -J;";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "c = " << g.limit_J_max << " * M / slope_gain + J;";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.floatDecl("root") << " = sqrt(b * b - 4.0 * a * c);";

    ss.newLine() << "if (J < focusJ)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "intersectJ = 2.0 * c / (-b - root);";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "intersectJ = 2.0 * c / (-b + root);";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "return intersectJ;";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Find_Gamut_Boundary_Intersection_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::GamutCompressParams & g,
    const std::string & solveJIntersectName)
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

    ss.newLine() << ss.float3Keyword() << " " << name << "(" << ss.float3Keyword() << " JMh_s, " << ss.float2Keyword() << " JM_cusp_in, float J_focus, float slope_gain, float gamma_top, float gamma_bottom)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.float2Decl("JM_cusp") << " = " << ss.float2Const("JM_cusp_in.r", std::string("JM_cusp_in.g * (1.0 + ") + std::to_string(ACES2::smooth_m) + " * " + std::to_string(ACES2::smooth_cusps) + ")") << ";";

    ss.newLine() << ss.floatDecl("J_intersect_source") << " = " << solveJIntersectName << "(JMh_s.r, JMh_s.g, J_focus, slope_gain);";
    ss.newLine() << ss.floatDecl("J_intersect_cusp") << " = " << solveJIntersectName << "(JM_cusp.r, JM_cusp.g, J_focus, slope_gain);";

    ss.newLine() << ss.floatDecl("slope") << " = 0.0;";
    ss.newLine() << "if (J_intersect_source < J_focus)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "slope = J_intersect_source * (J_intersect_source - J_focus) / (J_focus * slope_gain);";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "slope = (" << g.limit_J_max << " - J_intersect_source) * (J_intersect_source - J_focus) / (J_focus * slope_gain);";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.floatDecl("M_boundary_lower ") << " = J_intersect_cusp * pow(J_intersect_source / J_intersect_cusp, 1.0 / gamma_bottom) / (JM_cusp.r / JM_cusp.g - slope);";
    ss.newLine() << ss.floatDecl("M_boundary_upper") << " = JM_cusp.g * (" << g.limit_J_max << " - J_intersect_cusp) * pow((" << g.limit_J_max << " - J_intersect_source) / (" << g.limit_J_max << " - J_intersect_cusp), 1.0 / gamma_top) / (slope * JM_cusp.g + " << g.limit_J_max << " - JM_cusp.r);";

    ss.newLine() << ss.floatDecl("smin") << " = 0.0;";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.floatDecl("a") << " = M_boundary_lower / JM_cusp.g;";
    ss.newLine() << ss.floatDecl("b") << " = M_boundary_upper / JM_cusp.g;";
    ss.newLine() << ss.floatDecl("s") << " = " << ACES2::smooth_cusps << ";";

    ss.newLine() << ss.floatDecl("h") << " = max(s - abs(a - b), 0.0) / s;";
    ss.newLine() << "smin = min(a, b) - h * h * h * s * (1.0 / 6.0);";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.floatDecl("M_boundary") << " = JM_cusp.g * smin;";
    ss.newLine() << ss.floatDecl("J_boundary") << "= J_intersect_source + slope * M_boundary;";

    ss.newLine() << "return " << ss.float3Const("J_boundary", "M_boundary", "J_intersect_source") << ";";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Reach_Boundary_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::GamutCompressParams & g,
    const std::string & reachName,
    const std::string & getFocusGainName,
    const std::string & solveJIntersectName)
{
    // Reserve name
    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("get_reach_boundary")
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.float3Keyword() << " " << name << "(float J, float M, float h, " << ss.float2Keyword() << " JMcusp, float focusJ)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("reachMaxM") << " = " << reachName << "_sample(h);";
    ss.newLine() << ss.floatDecl("slope_gain") << " = " << g.limit_J_max << " * " << g.focus_dist << " * " << getFocusGainName << "(J, JMcusp.r);";
    ss.newLine() << ss.floatDecl("intersectJ") << " = " << solveJIntersectName << "(J, M, focusJ, slope_gain);";

    ss.newLine() << ss.floatDecl("slope") << " = 0.0;";
    ss.newLine() << "if (intersectJ < focusJ)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "slope = intersectJ * (intersectJ - focusJ) / (focusJ * slope_gain);";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "slope = (" << g.limit_J_max << " - intersectJ) * (intersectJ - focusJ) / (focusJ * slope_gain);";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.floatDecl("boundary") << " = " << g.limit_J_max << " * pow(intersectJ / " << g.limit_J_max << ", " << g.model_gamma << ") * reachMaxM / (" << g.limit_J_max << " - slope * reachMaxM);";

    ss.newLine() << "return " << ss.float3Const("J", "boundary", "h") << ";";

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
            << std::string("compression")
            << (invert ? std::string("_inv") : std::string("_fwd"))
            << resourceIndex;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine() << ss.floatKeyword() << " " << name << "(float v, float thr, float lim)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("s") << " = (lim - thr) * (1.0 - thr) / (lim - 1.0);";
    ss.newLine() << ss.floatDecl("nd") << " = (v - thr) / s;";


    ss.newLine() << ss.floatDecl("vCompressed") << " = 0.0;";

    if (invert)
    {
        ss.newLine() << "if (v < thr || lim <= 1.0001 || v > thr + s)";
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << "vCompressed = v;";
        ss.dedent();
        ss.newLine() << "}";
        ss.newLine() << "else";
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << "vCompressed = thr + s * (-nd / (nd - 1));";
        ss.dedent();
        ss.newLine() << "}";
    }
    else
    {
        ss.newLine() << "if (v < thr || lim <= 1.0001)";
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << "vCompressed = v;";
        ss.dedent();
        ss.newLine() << "}";
        ss.newLine() << "else";
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << "vCompressed = thr + s * nd / (1.0 + nd);";
        ss.dedent();
        ss.newLine() << "}";
    }

    ss.newLine() << "return vCompressed;";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToHelperShaderCode(ss.string().c_str());

    return name;
}

std::string _Add_Compress_Gamut_func(
    GpuShaderCreatorRcPtr & shaderCreator,
    unsigned resourceIndex,
    const ACES2::GamutCompressParams & g,
    const std::string & cuspName,
    const std::string & getFocusGainName,
    const std::string & gammaName,
    const std::string & findGamutBoundaryIntersectionName,
    const std::string & getReachBoundaryName,
    const std::string & compressionName)
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

    ss.newLine() << ss.float3Keyword() << " " << name << "(" << ss.float3Keyword() << " JMh, float Jx)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.floatDecl("J") << " = JMh.r;";
    ss.newLine() << ss.floatDecl("M") << " = JMh.g;";
    ss.newLine() << ss.floatDecl("h") << " = JMh.b;";

    ss.newLine() << "if (M < 0.0001 || J > " << g.limit_J_max << ")";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "return " << ss.float3Const("J", "0.0", "h") << ";";
    ss.dedent();
    ss.newLine() << "}";
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << ss.float2Decl("project_from") << " = " << ss.float2Const("J", "M") << ";";
    ss.newLine() << ss.float2Decl("JMcusp") << " = " << cuspName << "_sample(h);";

    ss.newLine() << ss.floatDecl("focusJ") << " = " << ss.lerp("JMcusp.r", std::to_string(g.mid_J), std::string("min(1.0, ") + std::to_string(ACES2::cusp_mid_blend) + " - (JMcusp.r / " + std::to_string(g.limit_J_max)) << "));";
    ss.newLine() << ss.floatDecl("slope_gain") << " = " << g.limit_J_max << " * " << g.focus_dist << " * " << getFocusGainName << "(Jx, JMcusp.r);";

    ss.newLine() << ss.floatDecl("gamma_top") << " = " << gammaName << "_sample(h);";
    ss.newLine() << ss.floatDecl("gamma_bottom") << " = " << g.lower_hull_gamma << ";";

    ss.newLine() << ss.float3Decl("boundaryReturn") << " = " << findGamutBoundaryIntersectionName << "(" << ss.float3Const("J", "M", "h") << ", JMcusp, focusJ, slope_gain, gamma_top, gamma_bottom);";
    ss.newLine() << ss.float2Decl("JMboundary") << " = " << ss.float2Const("boundaryReturn.r", "boundaryReturn.g") << ";";
    ss.newLine() << ss.float2Decl("project_to") << " = " << ss.float2Const("boundaryReturn.b", "0.0") << ";";

    ss.newLine() << "if (JMboundary.g <= 0.0)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "return " << ss.float3Const("J", "0.0", "h") << ";";
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << ss.float3Decl("reachBoundary") << " = " << getReachBoundaryName << "(JMboundary.r, JMboundary.g, h, JMcusp, focusJ);";

    ss.newLine() << ss.floatDecl("difference") << " = max(1.0001, reachBoundary.g / JMboundary.g);";
    ss.newLine() << ss.floatDecl("threshold") << " = max(" << ACES2::compression_threshold << ", 1.0 / difference);";

    ss.newLine() << ss.floatDecl("v") << " = project_from.g / JMboundary.g;";
    ss.newLine() << "v = " << compressionName << "(v, threshold, difference);";

    ss.newLine() << ss.float2Decl("JMcompressed") << " = " << ss.float2Const(
        "project_to.r + v * (JMboundary.r - project_to.r)",
        "project_to.g + v * (JMboundary.g - project_to.g)"
    ) << ";";

    ss.newLine() << "return " << ss.float3Const("JMcompressed.r", "JMcompressed.g", "h") << ";";

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
    const ACES2::GamutCompressParams & g,
    const std::string & reachName)
{
    std::string cuspName = _Add_Cusp_table(shaderCreator, resourceIndex, g);
    std::string gammaName = _Add_Gamma_table(shaderCreator, resourceIndex, g);
    std::string getFocusGainName = _Add_Focus_Gain_func(shaderCreator, resourceIndex, g);
    std::string solveJIntersectName = _Add_Solve_J_Intersect_func(shaderCreator, resourceIndex, g);
    std::string findGamutBoundaryIntersectionName = _Add_Find_Gamut_Boundary_Intersection_func(shaderCreator, resourceIndex, g, solveJIntersectName);
    std::string getReachBoundaryName = _Add_Reach_Boundary_func(shaderCreator, resourceIndex, g, reachName, getFocusGainName, solveJIntersectName);
    std::string compressionName = _Add_Compression_func(shaderCreator, resourceIndex, false);
    std::string gamutCompressName = _Add_Compress_Gamut_func(shaderCreator, resourceIndex, g, cuspName, getFocusGainName, gammaName, findGamutBoundaryIntersectionName, getReachBoundaryName, compressionName);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << pxl << ".rgb = " << gamutCompressName << "(" << pxl << ".rgb, " << pxl << ".r);";
}

void _Add_Gamut_Compress_Inv_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    unsigned int resourceIndex,
    const ACES2::GamutCompressParams & g,
    const std::string & reachName)
{
    std::string cuspName = _Add_Cusp_table(shaderCreator, resourceIndex, g);
    std::string gammaName = _Add_Gamma_table(shaderCreator, resourceIndex, g);
    std::string getFocusGainName = _Add_Focus_Gain_func(shaderCreator, resourceIndex, g);
    std::string solveJIntersectName = _Add_Solve_J_Intersect_func(shaderCreator, resourceIndex, g);
    std::string findGamutBoundaryIntersectionName = _Add_Find_Gamut_Boundary_Intersection_func(shaderCreator, resourceIndex, g, solveJIntersectName);
    std::string getReachBoundaryName = _Add_Reach_Boundary_func(shaderCreator, resourceIndex, g, reachName, getFocusGainName, solveJIntersectName);
    std::string compressionName = _Add_Compression_func(shaderCreator, resourceIndex, true);
    std::string gamutCompressName = _Add_Compress_Gamut_func(shaderCreator, resourceIndex, g, cuspName, getFocusGainName, gammaName, findGamutBoundaryIntersectionName, getReachBoundaryName, compressionName);

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float2Decl("JMcusp") << " = " << cuspName << "_sample(" << pxl << ".b);";
    ss.newLine() << ss.floatDecl("Jx") << " = " << pxl << ".r;";
    ss.newLine() << ss.float3Decl("unCompressedJMh") << ";";

    // Analytic inverse below threshold
    ss.newLine() << "if (Jx <= " << ss.lerp("JMcusp.r", std::to_string(g.limit_J_max), std::to_string(ACES2::focus_gain_blend)) << ")";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "unCompressedJMh = " << gamutCompressName << "(" << pxl << ".rgb, Jx);";
    ss.dedent();
    ss.newLine() << "}";
    // Approximation above threshold
    ss.newLine() << "else";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << "Jx = " << gamutCompressName << "(" << pxl << ".rgb, Jx).r;";
    ss.newLine() << "unCompressedJMh = " << gamutCompressName << "(" << pxl << ".rgb, Jx);";
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

    const Primaries in_primaries = ACES_AP0::primaries;

    const Primaries lim_primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    ACES2::JMhParams pIn = ACES2::init_JMhParams(in_primaries);
    ACES2::JMhParams pLim = ACES2::init_JMhParams(lim_primaries);
    ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance);
    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, lim_primaries);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();

    std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, g.reach_m_table);

    ss.newLine() << "";
    ss.newLine() << "// Add RGB to JMh";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_RGB_to_JMh_Shader(shaderCreator, ss, pIn);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add ToneScale and ChromaCompress (fwd)";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_Tonescale_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, pIn, t, c, reachName);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add GamutCompress (fwd)";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_Gamut_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, g, reachName);
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

    const Primaries in_primaries = ACES_AP0::primaries;

    const Primaries lim_primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    ACES2::JMhParams pIn = ACES2::init_JMhParams(in_primaries);
    ACES2::JMhParams pLim = ACES2::init_JMhParams(lim_primaries);
    ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance);
    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, lim_primaries);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();

    std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, c.reach_m_table);

    ss.newLine() << "";
    ss.newLine() << "// Add RGB to JMh";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_RGB_to_JMh_Shader(shaderCreator, ss, pLim);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add GamutCompress (inv)";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_Gamut_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, g, reachName);
    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "";
    ss.newLine() << "// Add ToneScale and ChromaCompress (inv)";
    ss.newLine() << "";
    ss.newLine() << "{";
    ss.indent();
        _Add_Tonescale_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, pIn, t, c, reachName);
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

    _Add_JMh_to_RGB_Shader(shaderCreator, ss, p);
}

void Add_Tonescale_Compress_Fwd_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    ACES2::JMhParams p = ACES2::init_JMhParams(ACES_AP0::primaries);
    ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();

    std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, c.reach_m_table);

    _Add_Tonescale_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, p, t, c, reachName);
}

void Add_Tonescale_Compress_Inv_Shader(
    GpuShaderCreatorRcPtr & shaderCreator,
    GpuShaderText & ss,
    const FixedFunctionOpData::Params & params)
{
    const float peak_luminance = (float) params[0];

    ACES2::JMhParams p = ACES2::init_JMhParams(ACES_AP0::primaries);
    ACES2::ToneScaleParams t = ACES2::init_ToneScaleParams(peak_luminance);
    ACES2::ChromaCompressParams c = ACES2::init_ChromaCompressParams(peak_luminance);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();

    std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, c.reach_m_table);

    _Add_Tonescale_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, p, t, c, reachName);
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

    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, primaries);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();

    std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, g.reach_m_table);

    _Add_Gamut_Compress_Fwd_Shader(shaderCreator, ss, resourceIndex, g, reachName);
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

    const ACES2::GamutCompressParams g = ACES2::init_GamutCompressParams(peak_luminance, primaries);

    unsigned resourceIndex = shaderCreator->getNextResourceIndex();

    std::string reachName = _Add_Reach_table(shaderCreator, resourceIndex, g.reach_m_table);

    _Add_Gamut_Compress_Inv_Shader(shaderCreator, ss, resourceIndex, g, reachName);
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
        }
    }

    ss.dedent();
    ss.newLine() << "}";

    ss.dedent();
    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

} // OCIO_NAMESPACE
