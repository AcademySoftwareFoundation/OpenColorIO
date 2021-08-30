// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/fixedfunction/FixedFunctionOpGPU.h"


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

void Add_Surround_Shader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & ss, float gamma)
{
    const std::string pxl(shaderCreator->getPixelName());

    // TODO: -- add vector inner product to GPUShaderUtils
    ss.newLine() << ss.floatDecl("Y") 
                 << " = max( 1e-4, 0.2627 * " << pxl << ".rgb.r + "
                 <<               "0.6780 * " << pxl << ".rgb.g + "
                 <<               "0.0593 * " << pxl << ".rgb.b );";

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
        case FixedFunctionOpData::REC2100_SURROUND_FWD:
        {
            Add_Surround_Shader(shaderCreator, ss, (float) func->getParams()[0]);
            break;
        }
        case FixedFunctionOpData::REC2100_SURROUND_INV:
        {
            Add_Surround_Shader(shaderCreator, ss, (float)(1. / func->getParams()[0]));
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
