// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace
{
    void check_param_bounds(const std::string & name, double val, double low, double high)
    {
        if (val < low || val > high)
        {
            std::stringstream ss;
            ss << "Parameter " << val << " (" << name << ") is outside valid range [" << low << "," << high << "]";
            throw Exception(ss.str().c_str());
        }
    };

    void check_param_no_frac(const std::string & name, double val)
    {
        if (floor(val) != val)
        {
            std::stringstream ss;
            ss << "Parameter " << val << " (" << name << ") cannot include any fractional component";
            throw Exception(ss.str().c_str());
        }
    };
}

namespace DefaultValues
{
const int FLOAT_DECIMALS = 7;
}

constexpr char ACES_RED_MOD_03_FWD_STR[]            = "RedMod03Fwd";
constexpr char ACES_RED_MOD_03_REV_STR[]            = "RedMod03Rev";
constexpr char ACES_RED_MOD_10_FWD_STR[]            = "RedMod10Fwd";
constexpr char ACES_RED_MOD_10_REV_STR[]            = "RedMod10Rev";
constexpr char ACES_GLOW_03_FWD_STR[]               = "Glow03Fwd";
constexpr char ACES_GLOW_03_REV_STR[]               = "Glow03Rev";
constexpr char ACES_GLOW_10_FWD_STR[]               = "Glow10Fwd";
constexpr char ACES_GLOW_10_REV_STR[]               = "Glow10Rev";
constexpr char ACES_DARK_TO_DIM_10_STR[]            = "DarkToDim10";
constexpr char ACES_DIM_TO_DARK_10_STR[]            = "DimToDark10";
constexpr char ACES_GAMUT_COMP_13_FWD_STR[]         = "GamutComp13Fwd";
constexpr char ACES_GAMUT_COMP_13_REV_STR[]         = "GamutComp13Rev";
constexpr char ACES_OUTPUT_TRANSFORM_20_FWD_STR[]   = "ACESOutputTransform20Fwd";
constexpr char ACES_OUTPUT_TRANSFORM_20_INV_STR[]   = "ACESOutputTransform20Inv";
constexpr char ACES_RGB_TO_JMh_20_STR[]             = "RGB_TO_JMh_20";
constexpr char ACES_JMh_TO_RGB_20_STR[]             = "JMh_TO_RGB_20";
constexpr char ACES_TONESCALE_COMPRESS_20_FWD_STR[] = "ToneScaleCompress20Fwd";
constexpr char ACES_TONESCALE_COMPRESS_20_INV_STR[] = "ToneScaleCompress20Inv";
constexpr char ACES_GAMUT_COMPRESS_20_FWD_STR[]     = "GamutCompress20Fwd";
constexpr char ACES_GAMUT_COMPRESS_20_INV_STR[]     = "GamutCompress20Inv";
constexpr char SURROUND_STR[]                       = "Surround"; // Old name for Rec2100SurroundFwd
constexpr char REC_2100_SURROUND_FWD_STR[]          = "Rec2100SurroundFwd";
constexpr char REC_2100_SURROUND_REV_STR[]          = "Rec2100SurroundRev";
constexpr char RGB_TO_HSV_STR[]                     = "RGB_TO_HSV";
constexpr char HSV_TO_RGB_STR[]                     = "HSV_TO_RGB";
constexpr char XYZ_TO_xyY_STR[]                     = "XYZ_TO_xyY";
constexpr char xyY_TO_XYZ_STR[]                     = "xyY_TO_XYZ";
constexpr char XYZ_TO_uvY_STR[]                     = "XYZ_TO_uvY";
constexpr char uvY_TO_XYZ_STR[]                     = "uvY_TO_XYZ";
constexpr char XYZ_TO_LUV_STR[]                     = "XYZ_TO_LUV";
constexpr char LUV_TO_XYZ_STR[]                     = "LUV_TO_XYZ";
constexpr char LIN_TO_PQ_STR[]                      = "Lin_TO_PQ";
constexpr char PQ_TO_LIN_STR[]                      = "PQ_TO_Lin";
constexpr char LIN_TO_GAMMA_LOG_STR[]               = "Lin_TO_GammaLog";
constexpr char GAMMA_LOG_TO_LIN_STR[]               = "GammaLog_TO_Lin";
constexpr char LIN_TO_DOUBLE_LOG_STR[]              = "Lin_TO_DoubleLog";
constexpr char DOUBLE_LOG_TO_LIN_STR[]              = "DoubleLog_TO_Lin";


// NOTE: Converts the enumeration value to its string representation (i.e. CLF reader).
//       It could add details for error reporting.
//
// Convert internal OpData style enum to CTF attribute string.  Set 'detailed' to true to get
// a more verbose human readable string
const char * FixedFunctionOpData::ConvertStyleToString(Style style, bool detailed)
{
    switch(style)
    {
        case ACES_RED_MOD_03_FWD:
            return detailed ? "ACES_RedMod03 (Forward)"    : ACES_RED_MOD_03_FWD_STR;
        case ACES_RED_MOD_03_INV:
            return detailed ? "ACES_RedMod03 (Inverse)"    : ACES_RED_MOD_03_REV_STR;
        case ACES_RED_MOD_10_FWD:
            return detailed ? "ACES_RedMod10 (Forward)"    : ACES_RED_MOD_10_FWD_STR;
        case ACES_RED_MOD_10_INV:
            return detailed ? "ACES_RedMod10 (Inverse)"    : ACES_RED_MOD_10_REV_STR;
        case ACES_GLOW_03_FWD:
            return detailed ? "ACES_Glow03 (Forward)"      : ACES_GLOW_03_FWD_STR;
        case ACES_GLOW_03_INV:
            return detailed ? "ACES_Glow03 (Inverse)"      : ACES_GLOW_03_REV_STR;
        case ACES_GLOW_10_FWD:
            return detailed ? "ACES_Glow10 (Forward)"      : ACES_GLOW_10_FWD_STR;
        case ACES_GLOW_10_INV:
            return detailed ? "ACES_Glow10 (Inverse)"      : ACES_GLOW_10_REV_STR;
        case ACES_DARK_TO_DIM_10_FWD:
            return detailed ? "ACES_DarkToDim10 (Forward)" : ACES_DARK_TO_DIM_10_STR;
        case ACES_DARK_TO_DIM_10_INV:
            return detailed ? "ACES_DarkToDim10 (Inverse)" : ACES_DIM_TO_DARK_10_STR;
        case ACES_GAMUT_COMP_13_FWD:
            return detailed ? "ACES_GamutComp13 (Forward)" : ACES_GAMUT_COMP_13_FWD_STR;
        case ACES_GAMUT_COMP_13_INV:
            return detailed ? "ACES_GamutComp13 (Inverse)" : ACES_GAMUT_COMP_13_REV_STR;
        case ACES_OUTPUT_TRANSFORM_20_FWD:
            return detailed ? "ACES_OutputTransform20 (Forward)" : ACES_OUTPUT_TRANSFORM_20_FWD_STR;
        case ACES_OUTPUT_TRANSFORM_20_INV:
            return detailed ? "ACES_OutputTransform20 (Inverse)" : ACES_OUTPUT_TRANSFORM_20_INV_STR;
        case ACES_RGB_TO_JMh_20:
            return ACES_RGB_TO_JMh_20_STR;
        case ACES_JMh_TO_RGB_20:
            return ACES_JMh_TO_RGB_20_STR;
        case ACES_TONESCALE_COMPRESS_20_FWD:
            return detailed ? "ACES_ToneScaleCompress20 (Forward)" : ACES_TONESCALE_COMPRESS_20_FWD_STR;
        case ACES_TONESCALE_COMPRESS_20_INV:
            return detailed ? "ACES_ToneScaleCompress20 (Inverse)" : ACES_TONESCALE_COMPRESS_20_INV_STR;
        case ACES_GAMUT_COMPRESS_20_FWD:
            return detailed ? "ACES_GamutCompress20 (Forward)" : ACES_GAMUT_COMPRESS_20_FWD_STR;
        case ACES_GAMUT_COMPRESS_20_INV:
            return detailed ? "ACES_GamutCompress20 (Inverse)" : ACES_GAMUT_COMPRESS_20_INV_STR;
        case REC2100_SURROUND_FWD:
            return detailed ? "REC2100_Surround (Forward)" : REC_2100_SURROUND_FWD_STR;
        case REC2100_SURROUND_INV:
            return detailed ? "REC2100_Surround (Inverse)" : REC_2100_SURROUND_REV_STR;
        case RGB_TO_HSV:
            return RGB_TO_HSV_STR;
        case HSV_TO_RGB:
            return HSV_TO_RGB_STR;
        case XYZ_TO_xyY:
            return XYZ_TO_xyY_STR;
        case xyY_TO_XYZ:
            return xyY_TO_XYZ_STR;
        case XYZ_TO_uvY:
            return XYZ_TO_uvY_STR;
        case uvY_TO_XYZ:
            return uvY_TO_XYZ_STR;
        case XYZ_TO_LUV:
            return XYZ_TO_LUV_STR;
        case LUV_TO_XYZ:
            return LUV_TO_XYZ_STR;
        case LIN_TO_PQ:
            return LIN_TO_PQ_STR;
        case PQ_TO_LIN:
          return PQ_TO_LIN_STR;
        case LIN_TO_GAMMA_LOG:
            return LIN_TO_GAMMA_LOG_STR;
        case GAMMA_LOG_TO_LIN:
            return GAMMA_LOG_TO_LIN_STR;
        case LIN_TO_DOUBLE_LOG:
            return LIN_TO_DOUBLE_LOG_STR;
        case DOUBLE_LOG_TO_LIN:
            return DOUBLE_LOG_TO_LIN_STR;
    }

    std::stringstream ss("Unknown FixedFunction style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

// Convert CTF attribute string into OpData style enum.
FixedFunctionOpData::Style FixedFunctionOpData::GetStyle(const char * name)
{
    if (name && *name)
    {
        if (0 == Platform::Strcasecmp(name, ACES_RED_MOD_03_FWD_STR))
        {
            return ACES_RED_MOD_03_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_RED_MOD_03_REV_STR))
        {
            return ACES_RED_MOD_03_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_RED_MOD_10_FWD_STR))
        {
            return ACES_RED_MOD_10_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_RED_MOD_10_REV_STR))
        {
            return ACES_RED_MOD_10_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GLOW_03_FWD_STR))
        {
            return ACES_GLOW_03_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GLOW_03_REV_STR))
        {
            return ACES_GLOW_03_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GLOW_10_FWD_STR))
        {
            return ACES_GLOW_10_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GLOW_10_REV_STR))
        {
            return ACES_GLOW_10_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_DARK_TO_DIM_10_STR))
        {
            return ACES_DARK_TO_DIM_10_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_DIM_TO_DARK_10_STR))
        {
            return ACES_DARK_TO_DIM_10_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GAMUT_COMP_13_FWD_STR))
        {
            return ACES_GAMUT_COMP_13_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GAMUT_COMP_13_REV_STR))
        {
            return ACES_GAMUT_COMP_13_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_OUTPUT_TRANSFORM_20_FWD_STR))
        {
            return ACES_OUTPUT_TRANSFORM_20_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_OUTPUT_TRANSFORM_20_INV_STR))
        {
            return ACES_OUTPUT_TRANSFORM_20_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_RGB_TO_JMh_20_STR))
        {
            return ACES_RGB_TO_JMh_20;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_JMh_TO_RGB_20_STR))
        {
            return ACES_JMh_TO_RGB_20;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_TONESCALE_COMPRESS_20_FWD_STR))
        {
            return ACES_TONESCALE_COMPRESS_20_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_TONESCALE_COMPRESS_20_INV_STR))
        {
            return ACES_TONESCALE_COMPRESS_20_INV;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GAMUT_COMPRESS_20_FWD_STR))
        {
            return ACES_GAMUT_COMPRESS_20_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, ACES_GAMUT_COMPRESS_20_INV_STR))
        {
            return ACES_GAMUT_COMPRESS_20_INV;
        }
        else if (0 == Platform::Strcasecmp(name, SURROUND_STR) ||
                 0 == Platform::Strcasecmp(name, REC_2100_SURROUND_FWD_STR))
        {
            return REC2100_SURROUND_FWD;
        }
        else if (0 == Platform::Strcasecmp(name, REC_2100_SURROUND_REV_STR))
        {
            return REC2100_SURROUND_INV;
        }
        else if (0 == Platform::Strcasecmp(name, RGB_TO_HSV_STR))
        {
            return RGB_TO_HSV;
        }
        else if (0 == Platform::Strcasecmp(name, HSV_TO_RGB_STR))
        {
            return HSV_TO_RGB;
        }
        else if (0 == Platform::Strcasecmp(name, XYZ_TO_xyY_STR))
        {
            return XYZ_TO_xyY;
        }
        else if (0 == Platform::Strcasecmp(name, xyY_TO_XYZ_STR))
        {
            return xyY_TO_XYZ;
        }
        else if (0 == Platform::Strcasecmp(name, XYZ_TO_uvY_STR))
        {
            return XYZ_TO_uvY;
        }
        else if (0 == Platform::Strcasecmp(name, uvY_TO_XYZ_STR))
        {
            return uvY_TO_XYZ;
        }
        else if (0 == Platform::Strcasecmp(name, XYZ_TO_LUV_STR))
        {
            return XYZ_TO_LUV;
        }
        else if (0 == Platform::Strcasecmp(name, LUV_TO_XYZ_STR))
        {
            return LUV_TO_XYZ;
        }
        else if (0 == Platform::Strcasecmp(name, LIN_TO_PQ_STR))
        {
            return LIN_TO_PQ;
        }
        else if (0 == Platform::Strcasecmp(name, PQ_TO_LIN_STR))
        {
            return PQ_TO_LIN;
        }
        else if (0 == Platform::Strcasecmp(name, LIN_TO_GAMMA_LOG_STR))
        {
            return LIN_TO_GAMMA_LOG;
        }
        else if (0 == Platform::Strcasecmp(name, GAMMA_LOG_TO_LIN_STR))
        {
            return GAMMA_LOG_TO_LIN;
        }
        else if (0 == Platform::Strcasecmp(name, LIN_TO_DOUBLE_LOG_STR))
        {
            return LIN_TO_DOUBLE_LOG;
        }
        else if (0 == Platform::Strcasecmp(name, DOUBLE_LOG_TO_LIN_STR))
        {
            return DOUBLE_LOG_TO_LIN;
        }
    }

    std::string st("Unknown FixedFunction style: ");
    st += name;

    throw Exception(st.c_str());
}

// Combine the Transform style and direction into the internal OpData style.
FixedFunctionOpData::Style FixedFunctionOpData::ConvertStyle(FixedFunctionStyle style,
                                                             TransformDirection dir)
{
    const bool isForward = dir == TRANSFORM_DIR_FORWARD;

    switch (style)
    {
        case FIXED_FUNCTION_ACES_RED_MOD_03:
        {
            return isForward ? FixedFunctionOpData::ACES_RED_MOD_03_FWD :
                               FixedFunctionOpData::ACES_RED_MOD_03_INV;
        }
        case FIXED_FUNCTION_ACES_RED_MOD_10:
        {
            return isForward ? FixedFunctionOpData::ACES_RED_MOD_10_FWD :
                               FixedFunctionOpData::ACES_RED_MOD_10_INV;
        }
        case FIXED_FUNCTION_ACES_GLOW_03:
        {
            return isForward ? FixedFunctionOpData::ACES_GLOW_03_FWD :
                               FixedFunctionOpData::ACES_GLOW_03_INV;
        }
        case FIXED_FUNCTION_ACES_GLOW_10:
        {
            return isForward ? FixedFunctionOpData::ACES_GLOW_10_FWD :
                               FixedFunctionOpData::ACES_GLOW_10_INV;
        }
        case FIXED_FUNCTION_ACES_DARK_TO_DIM_10:
        {
            return isForward ? FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD :
                               FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV;
        }
        case FIXED_FUNCTION_ACES_GAMUT_COMP_13:
        {
            return isForward ? FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD :
                               FixedFunctionOpData::ACES_GAMUT_COMP_13_INV;
        }
        case FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20:
        {
            return isForward ? FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD :
                               FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV;
        }
        case FIXED_FUNCTION_ACES_RGB_TO_JMH_20:
        {
            return isForward ? FixedFunctionOpData::ACES_RGB_TO_JMh_20 :
                               FixedFunctionOpData::ACES_JMh_TO_RGB_20;
        }
        case FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20:
        {
            return isForward ? FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_FWD :
                               FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_INV;
        }
        case FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20:
        {
            return isForward ? FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_FWD :
                               FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_INV;
        }
        case FIXED_FUNCTION_REC2100_SURROUND:
        {
            return isForward ? FixedFunctionOpData::REC2100_SURROUND_FWD :
                               FixedFunctionOpData::REC2100_SURROUND_INV;
        }
        case FIXED_FUNCTION_RGB_TO_HSV:
        {
            return FixedFunctionOpData::RGB_TO_HSV;
        }
        case FIXED_FUNCTION_XYZ_TO_xyY:
        {
            return FixedFunctionOpData::XYZ_TO_xyY;
        }
        case FIXED_FUNCTION_XYZ_TO_uvY:
        {
            return FixedFunctionOpData::XYZ_TO_uvY;
        }
        case FIXED_FUNCTION_XYZ_TO_LUV:
        {
            return FixedFunctionOpData::XYZ_TO_LUV;
        }
        case FIXED_FUNCTION_ACES_GAMUTMAP_02:
        case FIXED_FUNCTION_ACES_GAMUTMAP_07:
        {
            throw Exception("Unimplemented fixed function types: "
                            "FIXED_FUNCTION_ACES_GAMUTMAP_02, "
                            "FIXED_FUNCTION_ACES_GAMUTMAP_07.");
        }
        case FIXED_FUNCTION_LIN_TO_PQ: 
        {
            return isForward ? FixedFunctionOpData::LIN_TO_PQ:
                               FixedFunctionOpData::PQ_TO_LIN;
        }
        case FIXED_FUNCTION_LIN_TO_GAMMA_LOG:
        {
            return isForward ? FixedFunctionOpData::LIN_TO_GAMMA_LOG:
                               FixedFunctionOpData::GAMMA_LOG_TO_LIN;
        }
        case FIXED_FUNCTION_LIN_TO_DOUBLE_LOG:
        {
            return isForward ? FixedFunctionOpData::LIN_TO_DOUBLE_LOG:
                               FixedFunctionOpData::DOUBLE_LOG_TO_LIN;
        }
    }

    std::stringstream ss("Unknown FixedFunction transform style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

// Convert internal OpData style to Transform style.
FixedFunctionStyle FixedFunctionOpData::ConvertStyle(FixedFunctionOpData::Style style)
{
    switch (style)
    {
    case FixedFunctionOpData::ACES_RED_MOD_03_FWD:
    case FixedFunctionOpData::ACES_RED_MOD_03_INV:
        return FIXED_FUNCTION_ACES_RED_MOD_03;

    case FixedFunctionOpData::ACES_RED_MOD_10_FWD:
    case FixedFunctionOpData::ACES_RED_MOD_10_INV:
        return FIXED_FUNCTION_ACES_RED_MOD_10;

    case FixedFunctionOpData::ACES_GLOW_03_FWD:
    case FixedFunctionOpData::ACES_GLOW_03_INV:
        return FIXED_FUNCTION_ACES_GLOW_03;

    case FixedFunctionOpData::ACES_GLOW_10_FWD:
    case FixedFunctionOpData::ACES_GLOW_10_INV:
        return FIXED_FUNCTION_ACES_GLOW_10;

    case FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD:
    case FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV:
        return FIXED_FUNCTION_ACES_DARK_TO_DIM_10;

    case FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD:
    case FixedFunctionOpData::ACES_GAMUT_COMP_13_INV:
        return FIXED_FUNCTION_ACES_GAMUT_COMP_13;

    case FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD:
    case FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV:
        return FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20;

    case FixedFunctionOpData::ACES_RGB_TO_JMh_20:
    case FixedFunctionOpData::ACES_JMh_TO_RGB_20:
        return FIXED_FUNCTION_ACES_RGB_TO_JMH_20;

    case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_FWD:
    case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_INV:
        return FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20;

    case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_FWD:
    case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_INV:
        return FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20;

    case FixedFunctionOpData::REC2100_SURROUND_FWD:
    case FixedFunctionOpData::REC2100_SURROUND_INV:
        return FIXED_FUNCTION_REC2100_SURROUND;

    case FixedFunctionOpData::RGB_TO_HSV:
    case FixedFunctionOpData::HSV_TO_RGB:
        return FIXED_FUNCTION_RGB_TO_HSV;

    case FixedFunctionOpData::XYZ_TO_xyY:
    case FixedFunctionOpData::xyY_TO_XYZ:
        return FIXED_FUNCTION_XYZ_TO_xyY;

    case FixedFunctionOpData::XYZ_TO_uvY:
    case FixedFunctionOpData::uvY_TO_XYZ:
        return FIXED_FUNCTION_XYZ_TO_uvY;

    case FixedFunctionOpData::XYZ_TO_LUV:
    case FixedFunctionOpData::LUV_TO_XYZ:
        return FIXED_FUNCTION_XYZ_TO_LUV;

    case FixedFunctionOpData::LIN_TO_PQ:
    case FixedFunctionOpData::PQ_TO_LIN:
        return FIXED_FUNCTION_LIN_TO_PQ;
    
    case FixedFunctionOpData::LIN_TO_GAMMA_LOG:
    case FixedFunctionOpData::GAMMA_LOG_TO_LIN:
        return FIXED_FUNCTION_LIN_TO_GAMMA_LOG;

    case FixedFunctionOpData::LIN_TO_DOUBLE_LOG:
    case FixedFunctionOpData::DOUBLE_LOG_TO_LIN:
        return FIXED_FUNCTION_LIN_TO_DOUBLE_LOG;
    }

    std::stringstream ss("Unknown FixedFunction style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

FixedFunctionOpData::FixedFunctionOpData(Style style)
    :   OpData()
    ,   m_style(style)
{
    validate();
}

FixedFunctionOpData::FixedFunctionOpData(Style style, const Params & params)
    :   OpData()
    ,   m_style(style)
    ,   m_params(params)
{
    validate();
}

FixedFunctionOpData::~FixedFunctionOpData()
{
}

FixedFunctionOpDataRcPtr FixedFunctionOpData::clone() const
{
    auto clone = std::make_shared<FixedFunctionOpData>(getStyle(), getParams());
    clone->getFormatMetadata() = getFormatMetadata();
    return clone;
}

void FixedFunctionOpData::validate() const
{
    if (m_style==ACES_GAMUT_COMP_13_FWD || m_style == ACES_GAMUT_COMP_13_INV)
    {
        if (m_params.size() != 7)
        {
            std::stringstream ss;
            ss  << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have seven parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }

        const double lim_cyan    = m_params[0];
        const double lim_magenta = m_params[1];
        const double lim_yellow  = m_params[2];
        const double thr_cyan    = m_params[3];
        const double thr_magenta = m_params[4];
        const double thr_yellow  = m_params[5];
        const double power       = m_params[6];

        // Clamped to the smallest increment above 1 in half float precision for numerical stability.
        static constexpr double lim_low_bound = 1.001;
        static constexpr double lim_hi_bound  = 65504.0;
        check_param_bounds("lim_cyan",    lim_cyan,    lim_low_bound, lim_hi_bound);
        check_param_bounds("lim_magenta", lim_magenta, lim_low_bound, lim_hi_bound);
        check_param_bounds("lim_yellow",  lim_yellow,  lim_low_bound, lim_hi_bound);

        static constexpr double thr_low_bound = 0.0;
        // Clamped to the smallest increment below 1 in half float precision for numerical stability.
        static constexpr double thr_hi_bound  = 0.9995;
        check_param_bounds("thr_cyan",    thr_cyan,    thr_low_bound, thr_hi_bound);
        check_param_bounds("thr_magenta", thr_magenta, thr_low_bound, thr_hi_bound);
        check_param_bounds("thr_yellow",  thr_yellow,  thr_low_bound, thr_hi_bound);

        static constexpr double pwr_low_bound = 1.0;
        static constexpr double pwr_hi_bound  = 65504.0;
        check_param_bounds("power",       power,       pwr_low_bound, pwr_hi_bound);
    }
    else if (m_style == ACES_OUTPUT_TRANSFORM_20_FWD || m_style == ACES_OUTPUT_TRANSFORM_20_INV)
    {
        if (m_params.size() != 9)
        {
            std::stringstream ss;
            ss  << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have 9 parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }

        const double peak_luminance = m_params[0];
        check_param_bounds("peak_luminance", peak_luminance, 1, 10000);
        check_param_no_frac("peak_luminance", peak_luminance);
    }
    else if (m_style == ACES_RGB_TO_JMh_20 || m_style == ACES_JMh_TO_RGB_20)
    {
        if (m_params.size() != 8)
        {
            std::stringstream ss;
            ss  << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have 8 parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }
    }
    else if (m_style == ACES_TONESCALE_COMPRESS_20_FWD || m_style == ACES_TONESCALE_COMPRESS_20_INV)
    {
        if (m_params.size() != 1)
        {
            std::stringstream ss;
            ss  << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have 1 parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }

        const double peak_luminance = m_params[0];
        check_param_bounds("peak_luminance", peak_luminance, 1, 10000);
        check_param_no_frac("peak_luminance", peak_luminance);
    }
    else if (m_style == ACES_GAMUT_COMPRESS_20_FWD || m_style == ACES_GAMUT_COMPRESS_20_INV)
    {
        if (m_params.size() != 9)
        {
            std::stringstream ss;
            ss  << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have 9 parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }

        const double peak_luminance = m_params[0];
        check_param_bounds("peak_luminance", peak_luminance, 1, 10000);
        check_param_no_frac("peak_luminance", peak_luminance);
    }
    else if (m_style==REC2100_SURROUND_FWD || m_style == REC2100_SURROUND_INV)
    {
        if (m_params.size() != 1)
        {
            std::stringstream ss;
            ss  << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have one parameter but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }

        const double p = m_params[0];
        const double low_bound = 0.01;
        const double hi_bound  = 100.;

        if (p < low_bound)
        {
            std::stringstream ss;
            ss << "Parameter " << p << " is less than lower bound " << low_bound;
            throw Exception(ss.str().c_str());
        }
        else if (p > hi_bound)
        {
            std::stringstream ss;
            ss << "Parameter " << p << " is greater than upper bound " << hi_bound;
            throw Exception(ss.str().c_str());
        }
    }
    else if (m_style == DOUBLE_LOG_TO_LIN || m_style == LIN_TO_DOUBLE_LOG)
    {
        if (m_params.size() != 13)
        {
            std::stringstream ss;
            ss << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have 13 parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }

        double base              = m_params[0];
        double break1            = m_params[1];
        double break2            = m_params[2];
        // TODO: Add additional checks on the remaining params.
        // double logSeg1_logSlope  = m_params[3];
        // double logSeg1_logOff    = m_params[4];
        // double logSeg1_linSlope  = m_params[5];
        // double logSeg1_linOff    = m_params[6];
        // double logSeg2_logSlope  = m_params[7];
        // double logSeg2_logOff    = m_params[8];
        // double logSeg2_linSlope  = m_params[9];
        // double logSeg2_linOff    = m_params[10];
        // double linSeg_slope      = m_params[11];
        // double linSeg_off        = m_params[12];

        // Check log base.
        if(base <= 0.0)
        {
            std::stringstream ss;
            ss << "Log base " << base << " is not greater than zero.";
            throw Exception(ss.str().c_str());
        }

        // Check break point order.
        if(break1 > break2)
        {
            std::stringstream ss;
            ss << "First break point " << break1 << " is larger than the second break point " << break2 << ".";
            throw Exception(ss.str().c_str());
        }
    }
    else if (m_style == LIN_TO_GAMMA_LOG || m_style == GAMMA_LOG_TO_LIN)
    {
        if (m_params.size() != 10)
        {
            std::stringstream ss;
            ss << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have 10 parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }

        double mirrorPt          = m_params[0];
        double breakPt           = m_params[1];
        double gammaSeg_power    = m_params[2];
        // TODO: Add additional checks on the remaining params.
        // double gammaSeg_slope    = m_params[3];
        // double gammaSeg_off      = m_params[4];
        double logSeg_base       = m_params[5];
        // double logSeg_logSlope   = m_params[6];
        // double logSeg_logOff     = m_params[7];
        // double logSeg_linSlope   = m_params[8];
        // double logSeg_linOff     = m_params[9];

        // Check log base.
        if (logSeg_base <= 0.0)
        {
            std::stringstream ss;
            ss << "Log base " << logSeg_base << " is not greater than zero.";
            throw Exception(ss.str().c_str());
        }

        // Check mirror and break point order.
        if (mirrorPt >= breakPt)
        {
            std::stringstream ss;
            ss << "Mirror point " << mirrorPt << " is not smaller than the break point " << breakPt << ".";
            throw Exception(ss.str().c_str());
        }

        // Check gamma.
        if (gammaSeg_power == 0.0)
        {
            std::stringstream ss;
            ss << "Gamma power is zero.";
            throw Exception(ss.str().c_str());
        }
    }
    else
    {
        if (m_params.size()!=0)
        {
            std::stringstream ss;
            ss  << "The style '" << ConvertStyleToString(m_style, true)
                << "' must have zero parameters but "
                << m_params.size() << " found.";
            throw Exception(ss.str().c_str());
        }
    }
}

bool FixedFunctionOpData::isInverse(ConstFixedFunctionOpDataRcPtr & r) const
{
    const auto thisStyle = getStyle();
    if (REC2100_SURROUND_FWD == thisStyle || REC2100_SURROUND_INV == thisStyle)
    {
        // Check for the case where the styles are the same but the parameter is inverted.
        if (thisStyle == r->getStyle())
        {
            return m_params[0] == 1. / r->m_params[0];
        }
    }
    return *r == *inverse();
}

void FixedFunctionOpData::invert() noexcept
{
    // NB: The following assumes the op has already been validated.

    switch(getStyle())
    {
        case ACES_RED_MOD_03_FWD:
        {
            setStyle(ACES_RED_MOD_03_INV);
            break;
        }
        case ACES_RED_MOD_03_INV:
        {
            setStyle(ACES_RED_MOD_03_FWD);
            break;
        }
        case ACES_RED_MOD_10_FWD:
        {
            setStyle(ACES_RED_MOD_10_INV);
            break;
        }
        case ACES_RED_MOD_10_INV:
        {
            setStyle(ACES_RED_MOD_10_FWD);
            break;
        }
        case ACES_GLOW_03_FWD:
        {
            setStyle(ACES_GLOW_03_INV);
            break;
        }
        case ACES_GLOW_03_INV:
        {
            setStyle(ACES_GLOW_03_FWD);
            break;
        }
        case ACES_GLOW_10_FWD:
        {
            setStyle(ACES_GLOW_10_INV);
            break;
        }
        case ACES_GLOW_10_INV:
        {
            setStyle(ACES_GLOW_10_FWD);
            break;
        }
        case ACES_DARK_TO_DIM_10_FWD:
        {
            setStyle(ACES_DARK_TO_DIM_10_INV);
            break;
        }
        case ACES_DARK_TO_DIM_10_INV:
        {
            setStyle(ACES_DARK_TO_DIM_10_FWD);
            break;
        }
        case ACES_GAMUT_COMP_13_FWD:
        {
            setStyle(ACES_GAMUT_COMP_13_INV);
            break;
        }
        case ACES_GAMUT_COMP_13_INV:
        {
            setStyle(ACES_GAMUT_COMP_13_FWD);
            break;
        }
        case ACES_OUTPUT_TRANSFORM_20_FWD:
        {
            setStyle(ACES_OUTPUT_TRANSFORM_20_INV);
            break;
        }
        case ACES_OUTPUT_TRANSFORM_20_INV:
        {
            setStyle(ACES_OUTPUT_TRANSFORM_20_FWD);
            break;
        }
        case ACES_RGB_TO_JMh_20:
        {
            setStyle(ACES_JMh_TO_RGB_20);
            break;
        }
        case ACES_JMh_TO_RGB_20:
        {
            setStyle(ACES_RGB_TO_JMh_20);
            break;
        }
        case ACES_TONESCALE_COMPRESS_20_FWD:
        {
            setStyle(ACES_TONESCALE_COMPRESS_20_INV);
            break;
        }
        case ACES_TONESCALE_COMPRESS_20_INV:
        {
            setStyle(ACES_TONESCALE_COMPRESS_20_FWD);
            break;
        }
        case ACES_GAMUT_COMPRESS_20_FWD:
        {
            setStyle(ACES_GAMUT_COMPRESS_20_INV);
            break;
        }
        case ACES_GAMUT_COMPRESS_20_INV:
        {
            setStyle(ACES_GAMUT_COMPRESS_20_FWD);
            break;
        }

        case REC2100_SURROUND_FWD:
        {
            setStyle(REC2100_SURROUND_INV);
            break;
        }
        case REC2100_SURROUND_INV:
        {
            setStyle(REC2100_SURROUND_FWD);
            break;
        }

        case RGB_TO_HSV:
        {
            setStyle(HSV_TO_RGB);
            break;
        }
        case HSV_TO_RGB:
        {
            setStyle(RGB_TO_HSV);
            break;
        }

        case XYZ_TO_xyY:
        {
            setStyle(xyY_TO_XYZ);
            break;
        }
        case xyY_TO_XYZ:
        {
            setStyle(XYZ_TO_xyY);
            break;
        }

        case XYZ_TO_uvY:
        {
            setStyle(uvY_TO_XYZ);
            break;
        }
        case uvY_TO_XYZ:
        {
            setStyle(XYZ_TO_uvY);
            break;
        }

        case XYZ_TO_LUV:
        {
            setStyle(LUV_TO_XYZ);
            break;
        }
        case LUV_TO_XYZ:
        {
            setStyle(XYZ_TO_LUV);
            break;
        }

        case LIN_TO_PQ:
        {
            setStyle(PQ_TO_LIN);
            break;
        }
        case PQ_TO_LIN:
        {
            setStyle(LIN_TO_PQ);
            break;
        }

        case LIN_TO_GAMMA_LOG:
        {
            setStyle(GAMMA_LOG_TO_LIN);
            break;
        }
        case GAMMA_LOG_TO_LIN:
        {
            setStyle(LIN_TO_GAMMA_LOG);
            break;
        }

        case LIN_TO_DOUBLE_LOG:
        {
            setStyle(DOUBLE_LOG_TO_LIN);
            break;
        }
        case DOUBLE_LOG_TO_LIN:
        {
            setStyle(LIN_TO_DOUBLE_LOG);
            break;
        }
    }

    // Note that any existing metadata could become stale at this point but
    // trying to update it is also challenging since inverse() is sometimes
    // called even during the creation of new ops.
}

FixedFunctionOpDataRcPtr FixedFunctionOpData::inverse() const
{
    FixedFunctionOpDataRcPtr func = clone();
    func->invert();
    return func;
}

// Convert internal OpData style into Transform direction.
TransformDirection FixedFunctionOpData::getDirection() const noexcept
{
    switch (m_style)
    {
    case FixedFunctionOpData::ACES_RED_MOD_03_FWD:
    case FixedFunctionOpData::ACES_RED_MOD_10_FWD:
    case FixedFunctionOpData::ACES_GLOW_03_FWD:
    case FixedFunctionOpData::ACES_GLOW_10_FWD:
    case FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD:
    case FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD:
    case FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD:
    case FixedFunctionOpData::ACES_RGB_TO_JMh_20:
    case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_FWD:
    case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_FWD:
    case FixedFunctionOpData::REC2100_SURROUND_FWD:
    case FixedFunctionOpData::RGB_TO_HSV:
    case FixedFunctionOpData::XYZ_TO_xyY:
    case FixedFunctionOpData::XYZ_TO_uvY:
    case FixedFunctionOpData::XYZ_TO_LUV:
    case FixedFunctionOpData::LIN_TO_PQ:
    case FixedFunctionOpData::LIN_TO_GAMMA_LOG:
    case FixedFunctionOpData::LIN_TO_DOUBLE_LOG:
        return TRANSFORM_DIR_FORWARD;

    case FixedFunctionOpData::ACES_RED_MOD_03_INV:
    case FixedFunctionOpData::ACES_RED_MOD_10_INV:
    case FixedFunctionOpData::ACES_GLOW_03_INV:
    case FixedFunctionOpData::ACES_GLOW_10_INV:
    case FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV:
    case FixedFunctionOpData::ACES_GAMUT_COMP_13_INV:
    case FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV:
    case FixedFunctionOpData::ACES_JMh_TO_RGB_20:
    case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_INV:
    case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_INV:
    case FixedFunctionOpData::REC2100_SURROUND_INV:
    case FixedFunctionOpData::HSV_TO_RGB:
    case FixedFunctionOpData::xyY_TO_XYZ:
    case FixedFunctionOpData::uvY_TO_XYZ:
    case FixedFunctionOpData::LUV_TO_XYZ:
    case FixedFunctionOpData::PQ_TO_LIN:
    case FixedFunctionOpData::GAMMA_LOG_TO_LIN:
    case FixedFunctionOpData::DOUBLE_LOG_TO_LIN:
        return TRANSFORM_DIR_INVERSE;
    }
    return TRANSFORM_DIR_FORWARD;
}

void FixedFunctionOpData::setDirection(TransformDirection dir) noexcept
{
    if (getDirection() != dir)
    {
        invert();
    }
}

bool FixedFunctionOpData::equals(const OpData & other) const
{
    if (!OpData::equals(other)) return false;

    const FixedFunctionOpData* fop = static_cast<const FixedFunctionOpData*>(&other);

    return getStyle() == fop->getStyle() && getParams() == fop->getParams();
}

std::string FixedFunctionOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << ConvertStyleToString(m_style, true);

    for (const auto & param : m_params)
    {
        cacheIDStream << " " << param;
    }

    return cacheIDStream.str();
}

bool operator==(const FixedFunctionOpData & lhs, const FixedFunctionOpData & rhs)
{
    return lhs.equals(rhs);
}

} // namespace OCIO_NAMESPACE
