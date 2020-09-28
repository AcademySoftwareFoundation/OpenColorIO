// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ops/fixedfunction/FixedFunctionOpGPU.h"


namespace OCIO_NAMESPACE
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
    ss.newLine() << ss.float4Decl("monomials") << " = " << ss.float4Const("t*t*t", "t*t", "t", "1.") << ";";
    ss.newLine() << ss.float4Decl("m0") << " = " << ss.float4Const(0.25,  0.00,  0.00,  0.00) << ";";
    ss.newLine() << ss.float4Decl("m1") << " = " << ss.float4Const(-0.75,  0.75,  0.75,  0.25) << ";";
    ss.newLine() << ss.float4Decl("m2") << " = " << ss.float4Const(0.75, -1.50,  0.00,  1.00) << ";";
    ss.newLine() << ss.float4Decl("m3") << " = " << ss.float4Const(-0.25,  0.75, -0.75,  0.25) << ";";
    ss.newLine() << ss.float4Decl("coefs") << " = " << ss.lerp( "m0", "m1", "float(j == 1)") << ";";
    ss.newLine() << "coefs = " << ss.lerp("coefs", "m2", "float(j == 2)") << ";";
    ss.newLine() << "coefs = " << ss.lerp("coefs", "m3", "float(j == 3)") << ";";
    ss.newLine() << "float f_H = dot(coefs, monomials);";
}

void Add_RedMod_03_Fwd_Shader(GpuShaderText & ss)
{
    const float _1minusScale = 1.f - 0.85f;  // (1. - scale) from the original ctl code
    const float _pivot = 0.03f;

    Add_hue_weight_shader(ss, 120.f);

    ss.newLine() << ss.float3Decl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.float3Decl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";
    ss.newLine() << "float oldChroma = max(1e-10, maxval.r - minval.r);";
    ss.newLine() << ss.float3Decl("delta") << " = outColor.rgb - minval;";

    ss.newLine() << "float f_S = ( max(1e-10, maxval.r) - max(1e-10, minval.r) ) / max(1e-2, maxval.r);";

    ss.newLine() << "outColor.r = outColor.r + f_H * f_S * (" << _pivot
                 << " - outColor.r) * " << _1minusScale << ";";

    ss.newLine() << ss.float3Decl("maxval2") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
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

    ss.newLine() << ss.float3Decl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.float3Decl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";
    ss.newLine() << "float oldChroma = max(1e-10, maxval.r - minval.r);";
    ss.newLine() << ss.float3Decl("delta") << " = outColor.rgb - minval;";

    // Note: If f_H == 0, the following generally doesn't change the red value,
    //       but it does for R < 0, hence the need for the if-statement above.
    ss.newLine() << "float ka = f_H * " << _1minusScale << " - 1.;";
    ss.newLine() << "float kb = outColor.r - f_H * (" << _pivot << " + minval.r) * "
                 << _1minusScale << ";";
    ss.newLine() << "float kc = f_H * " << _pivot << " * minval.r * " << _1minusScale << ";";
    ss.newLine() << "outColor.r = ( -kb - sqrt( kb * kb - 4. * ka * kc)) / ( 2. * ka);";

    ss.newLine() << ss.float3Decl("maxval2") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
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

    ss.newLine() << ss.float3Decl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.float3Decl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";
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

    ss.newLine() << ss.float3Decl("minval") << " = min( outColor.gbr, outColor.brg);";

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
    ss.newLine() << ss.float3Decl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.float3Decl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";

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
    ss.newLine() << ss.float3Decl("maxval") << " = max( outColor.rgb, max( outColor.gbr, outColor.brg));";
    ss.newLine() << ss.float3Decl("minval") << " = min( outColor.rgb, min( outColor.gbr, outColor.brg));";

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

void Add_RGB_TO_HSV(GpuShaderText & ss)
{
    ss.newLine() << "float minRGB = min( outColor.x, min( outColor.y, outColor.z ) );";
    ss.newLine() << "float maxRGB = max( outColor.x, max( outColor.y, outColor.z ) );";
    ss.newLine() << "float val = maxRGB;";

    ss.newLine() << "float sat = 0.0, hue = 0.0;";
    ss.newLine() << "if (minRGB != maxRGB)";
    ss.newLine() << "{";
    ss.indent();

    ss.newLine() << "if (val != 0.0) sat = (maxRGB - minRGB) / val;";
    ss.newLine() << "float OneOverMaxMinusMin = 1.0 / (maxRGB - minRGB);";
    ss.newLine() << "if ( maxRGB == outColor.r ) hue = (outColor.g - outColor.b) * OneOverMaxMinusMin;";
    ss.newLine() << "else if ( maxRGB == outColor.g ) hue = 2.0 + (outColor.b - outColor.r) * OneOverMaxMinusMin;";
    ss.newLine() << "else hue = 4.0 + (outColor.r - outColor.g) * OneOverMaxMinusMin;";
    ss.newLine() << "if ( hue < 0.0 ) hue += 6.0;";

    ss.dedent();
    ss.newLine() << "}";

    ss.newLine() << "if ( minRGB < 0.0 ) val += minRGB;";
    ss.newLine() << "if ( -minRGB > maxRGB ) sat = (maxRGB - minRGB) / -minRGB;";

    ss.newLine() << "outColor.r = hue * 1./6.; outColor.g = sat; outColor.b = val;";
}

void Add_HSV_TO_RGB(GpuShaderText & ss)
{
    ss.newLine() << "float Hue = ( outColor.x - floor( outColor.x ) ) * 6.0;";
    ss.newLine() << "float Sat = clamp( outColor.y, 0., 1.999 );";
    ss.newLine() << "float Val = outColor.z;";

    ss.newLine() << "float R = abs(Hue - 3.0) - 1.0;";
    ss.newLine() << "float G = 2.0 - abs(Hue - 2.0);";
    ss.newLine() << "float B = 2.0 - abs(Hue - 4.0);";
    ss.newLine() << ss.float3Decl("RGB") << " = " << ss.float3Const("R", "G", "B") << ";";
    ss.newLine() << "RGB = clamp( RGB, 0., 1. );";

    ss.newLine() << "float rgbMax = Val;";
    ss.newLine() << "float rgbMin = Val * (1.0 - Sat);";

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

    ss.newLine() << "outColor.rgb = RGB;";
}

void Add_XYZ_TO_xyY(GpuShaderText & ss)
{
    ss.newLine() << "float d = outColor.r + outColor.g + outColor.b;";
    ss.newLine() << "d = (d == 0.) ? 0. : 1. / d;";
    ss.newLine() << "outColor.b = outColor.g;";
    ss.newLine() << "outColor.r *= d;";
    ss.newLine() << "outColor.g *= d;";
}

void Add_xyY_TO_XYZ(GpuShaderText & ss)
{
    ss.newLine() << "float d = (outColor.g == 0.) ? 0. : 1. / outColor.g;";
    ss.newLine() << "float Y = outColor.b;";
    ss.newLine() << "outColor.b = Y * (1. - outColor.r - outColor.g) * d;";
    ss.newLine() << "outColor.r *= Y * d;";
    ss.newLine() << "outColor.g = Y;";
}

void Add_XYZ_TO_uvY(GpuShaderText & ss)
{
    ss.newLine() << "float d = outColor.r + 15. * outColor.g + 3. * outColor.b;";
    ss.newLine() << "d = (d == 0.) ? 0. : 1. / d;";
    ss.newLine() << "outColor.b = outColor.g;";
    ss.newLine() << "outColor.r *= 4. * d;";
    ss.newLine() << "outColor.g *= 9. * d;";
}

void Add_uvY_TO_XYZ(GpuShaderText & ss)
{
    ss.newLine() << "float d = (outColor.g == 0.) ? 0. : 1. / outColor.g;";
    ss.newLine() << "float Y = outColor.b;";
    ss.newLine() << "outColor.b = (3./4.) * Y * (4. - outColor.r - 6.6666666666666667 * outColor.g) * d;";
    ss.newLine() << "outColor.r *= (9./4.) * Y * d;";
    ss.newLine() << "outColor.g = Y;";
}

void Add_XYZ_TO_LUV(GpuShaderText & ss)
{
    ss.newLine() << "float d = outColor.r + 15. * outColor.g + 3. * outColor.b;";
    ss.newLine() << "d = (d == 0.) ? 0. : 1. / d;";
    ss.newLine() << "float u = outColor.r * 4. * d;";
    ss.newLine() << "float v = outColor.g * 9. * d;";
    ss.newLine() << "float Y = outColor.g;";

    ss.newLine() << "float Lstar = " << ss.lerp( "1.16 * pow( max(0., Y), 1./3. ) - 0.16", "9.0329629629629608 * Y",
                                                 "float(Y <= 0.008856451679)" ) << ";";
    ss.newLine() << "float ustar = 13. * Lstar * (u - 0.19783001);";
    ss.newLine() << "float vstar = 13. * Lstar * (v - 0.46831999);";

    ss.newLine() << "outColor.r = Lstar; outColor.g = ustar; outColor.b = vstar;";
}

void Add_LUV_TO_XYZ(GpuShaderText & ss)
{
    ss.newLine() << "float Lstar = outColor.r;";
    ss.newLine() << "float d = (Lstar == 0.) ? 0. : 0.076923076923076927 / Lstar;";
    ss.newLine() << "float u = outColor.g * d + 0.19783001;";
    ss.newLine() << "float v = outColor.b * d + 0.46831999;";

    ss.newLine() << "float tmp = (Lstar + 0.16) * 0.86206896551724144;";
    ss.newLine() << "float Y = " << ss.lerp( "tmp*tmp*tmp", "0.11070564598794539 * Lstar", "float(Lstar <= 0.08)" ) << ";";

    ss.newLine() << "float dd = (v == 0.) ? 0. : 0.25 / v;";
    ss.newLine() << "outColor.r = 9. * Y * u * dd;";
    ss.newLine() << "outColor.b = Y * (12. - 3. * u - 20. * v) * dd;";
    ss.newLine() << "outColor.g = Y;";
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
        case FixedFunctionOpData::REC2100_SURROUND_FWD:
        {
            Add_Surround_Shader(ss, (float) func->getParams()[0]);
            break;
        }
        case FixedFunctionOpData::REC2100_SURROUND_INV:
        {
            Add_Surround_Shader(ss, (float)(1. / func->getParams()[0]));
            break;
        }
        case FixedFunctionOpData::RGB_TO_HSV:
        {
            Add_RGB_TO_HSV(ss);
            break;
        }
        case FixedFunctionOpData::HSV_TO_RGB:
        {
            Add_HSV_TO_RGB(ss);
            break;
        }
        case FixedFunctionOpData::XYZ_TO_xyY:
        {
            Add_XYZ_TO_xyY(ss);
            break;
        }
        case FixedFunctionOpData::xyY_TO_XYZ:
        {
            Add_xyY_TO_XYZ(ss);
            break;
        }
        case FixedFunctionOpData::XYZ_TO_uvY:
        {
            Add_XYZ_TO_uvY(ss);
            break;
        }
        case FixedFunctionOpData::uvY_TO_XYZ:
        {
            Add_uvY_TO_XYZ(ss);
            break;
        }
        case FixedFunctionOpData::XYZ_TO_LUV:
        {
            Add_XYZ_TO_LUV(ss);
            break;
        }
        case FixedFunctionOpData::LUV_TO_XYZ:
        {
            Add_LUV_TO_XYZ(ss);
        }
    }

    ss.dedent();
    ss.newLine() << "}";

    ss.dedent();
    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

} // OCIO_NAMESPACE
