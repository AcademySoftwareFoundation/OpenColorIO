// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ParseUtils.h"
#include "Platform.h"
#include "utils/StringUtils.h"
#include "utils/NumberUtils.h"


namespace OCIO_NAMESPACE
{

struct Element
{
    std::string str;
    char c;
};

static struct Element elts[]
    = { { "&quot;", '"' },
        { "&apos;", '\''},
        { "&lt;",   '<' },
        { "&gt;",   '>' },
        { "&amp;",  '&' },
        { "",       ' ' } };

std::string ConvertSpecialCharToXmlToken(const std::string& str)
{
    std::string res;
    for (std::string::const_iterator it(str.begin());
        it != str.end();
        it++)
    {
        bool found = false;
        for (unsigned idx = 0; !elts[idx].str.empty(); ++idx)
        {
            const Element& elt = elts[idx];
            if (*it == elt.c)
            {
                found = true;
                res += elt.str;
                break;
            }
        }
        if (!found)
        {
            res += *it;
        }
    }
    return res;
}

std::string ConvertXmlTokenToSpecialChar(const std::string & str)
{
    std::string res;
    for (std::string::const_iterator it(str.begin()); it != str.end(); it++)
    {
        switch (*it)
        {
        case '&':
        {
            unsigned idx = 0;
            for (; !elts[idx].str.empty(); ++idx)
            {
                const Element& elt = elts[idx];
                const size_t length = elt.str.length();
                if (0 == strncmp(&(*it), elt.str.c_str(), length))
                {
                    res += elt.c;
                    it += (length - 1); // -1 because for loop will +1
                    break;
                }
            }

            if (elts[idx].str.empty())
            {
                std::ostringstream oss;
                oss << "Unknown XML tag:" << std::string(&(*it));
                throw Exception(oss.str().c_str());
            }
            break;
        }
        default:
        {
            res += *it;
            break;
        }
        }
    }
    return res;
}

const char * BoolToString(bool val)
{
    return val ? "true" : "false";
}

bool BoolFromString(const char * s)
{
    const std::string str = StringUtils::Lower(s ? s : "");

    return (str == "true") || (str == "yes");
}

const char * LoggingLevelToString(LoggingLevel level)
{
    if(level == LOGGING_LEVEL_NONE) return "none";
    else if(level == LOGGING_LEVEL_WARNING) return "warning";
    else if(level == LOGGING_LEVEL_INFO) return "info";
    else if(level == LOGGING_LEVEL_DEBUG) return "debug";
    return "unknown";
}

LoggingLevel LoggingLevelFromString(const char * s)
{
    const std::string str = StringUtils::Lower(s ? s : "");

    if(str == "0" || str == "none") return LOGGING_LEVEL_NONE;
    else if(str == "1" || str == "warning") return LOGGING_LEVEL_WARNING;
    else if(str == "2" || str == "info") return LOGGING_LEVEL_INFO;
    else if(str == "3" || str == "debug") return LOGGING_LEVEL_DEBUG;
    return LOGGING_LEVEL_UNKNOWN;
}

const char * TransformDirectionToString(TransformDirection dir)
{
    if(dir == TRANSFORM_DIR_FORWARD) return "forward";
    // TRANSFORM_DIR_INVERSE
    return "inverse";
}

TransformDirection TransformDirectionFromString(const char * s)
{
    const char * p = (s ? s : "");
    const std::string str = StringUtils::Lower(p);

    if(str == "forward") return TRANSFORM_DIR_FORWARD;
    else if(str == "inverse") return TRANSFORM_DIR_INVERSE;

    std::ostringstream oss;
    oss << "Unrecognized transform direction: '" << p << "'.";
    throw Exception(oss.str().c_str());
}

TransformDirection CombineTransformDirections(TransformDirection d1, TransformDirection d2)
{
    if(d1 == TRANSFORM_DIR_FORWARD && d2 == TRANSFORM_DIR_FORWARD)
        return TRANSFORM_DIR_FORWARD;

    if(d1 == TRANSFORM_DIR_INVERSE && d2 == TRANSFORM_DIR_INVERSE)
        return TRANSFORM_DIR_FORWARD;

    return TRANSFORM_DIR_INVERSE;
}

TransformDirection GetInverseTransformDirection(TransformDirection dir)
{
    if(dir == TRANSFORM_DIR_FORWARD) return TRANSFORM_DIR_INVERSE;
    // TRANSFORM_DIR_INVERSE
    return TRANSFORM_DIR_FORWARD;
}

const char * BitDepthToString(BitDepth bitDepth)
{
    if(bitDepth == BIT_DEPTH_UINT8) return "8ui";
    else if(bitDepth == BIT_DEPTH_UINT10) return "10ui";
    else if(bitDepth == BIT_DEPTH_UINT12) return "12ui";
    else if(bitDepth == BIT_DEPTH_UINT14) return "14ui";
    else if(bitDepth == BIT_DEPTH_UINT16) return "16ui";
    else if(bitDepth == BIT_DEPTH_UINT32) return "32ui";
    else if(bitDepth == BIT_DEPTH_F16) return "16f";
    else if(bitDepth == BIT_DEPTH_F32) return "32f";
    return "unknown";
}

BitDepth BitDepthFromString(const char * s)
{
    const std::string str = StringUtils::Lower(s ? s : "");

    if(str == "8ui") return BIT_DEPTH_UINT8;
    else if(str == "10ui") return BIT_DEPTH_UINT10;
    else if(str == "12ui") return BIT_DEPTH_UINT12;
    else if(str == "14ui") return BIT_DEPTH_UINT14;
    else if(str == "16ui") return BIT_DEPTH_UINT16;
    else if(str == "32ui") return BIT_DEPTH_UINT32;
    else if(str == "16f") return BIT_DEPTH_F16;
    else if(str == "32f") return BIT_DEPTH_F32;
    return BIT_DEPTH_UNKNOWN;
}

bool BitDepthIsFloat(BitDepth bitDepth)
{
    if(bitDepth == BIT_DEPTH_F16) return true;
    else if(bitDepth == BIT_DEPTH_F32) return true;
    return false;
}

int BitDepthToInt(BitDepth bitDepth)
{
    if(bitDepth == BIT_DEPTH_UINT8) return 8;
    else if(bitDepth == BIT_DEPTH_UINT10) return 10;
    else if(bitDepth == BIT_DEPTH_UINT12) return 12;
    else if(bitDepth == BIT_DEPTH_UINT14) return 14;
    else if(bitDepth == BIT_DEPTH_UINT16) return 16;
    else if(bitDepth == BIT_DEPTH_UINT32) return 32;

    return 0;
}

const char * AllocationToString(Allocation alloc)
{
    if(alloc == ALLOCATION_UNIFORM) return "uniform";
    else if(alloc == ALLOCATION_LG2) return "lg2";
    return "unknown";
}

Allocation AllocationFromString(const char * s)
{
    const std::string str = StringUtils::Lower(s ? s : "");

    if(str == "uniform") return ALLOCATION_UNIFORM;
    else if(str == "lg2") return ALLOCATION_LG2;
    return ALLOCATION_UNKNOWN;
}

const char * InterpolationToString(Interpolation interp)
{
    if(interp == INTERP_NEAREST) return "nearest";
    else if(interp == INTERP_LINEAR) return "linear";
    else if(interp == INTERP_TETRAHEDRAL) return "tetrahedral";
    else if(interp == INTERP_BEST) return "best";
    else if(interp == INTERP_DEFAULT) return "default";
    // INTERP_CUBIC is not implemented yet, but the string may be useful for error messages.
    else if(interp == INTERP_CUBIC) return "cubic";
    return "unknown";
}

Interpolation InterpolationFromString(const char * s)
{
    const std::string str = StringUtils::Lower(s ? s : "");

    if(str == "nearest") return INTERP_NEAREST;
    else if(str == "linear") return INTERP_LINEAR;
    else if(str == "tetrahedral") return INTERP_TETRAHEDRAL;
    else if(str == "best") return INTERP_BEST;
    else if(str == "cubic") return INTERP_CUBIC;
    return INTERP_UNKNOWN;
}

const char * GpuLanguageToString(GpuLanguage language)
{
    switch(language)
    {
        case GPU_LANGUAGE_CG:           return "cg";
        case GPU_LANGUAGE_GLSL_1_2:     return "glsl_1.2";
        case GPU_LANGUAGE_GLSL_1_3:     return "glsl_1.3";
        case GPU_LANGUAGE_GLSL_4_0:     return "glsl_4.0";
        case GPU_LANGUAGE_GLSL_ES_1_0:  return "glsl_es_1.0";
        case GPU_LANGUAGE_GLSL_ES_3_0:  return "glsl_es_3.0";
        case GPU_LANGUAGE_HLSL_SM_5_0:  return "hlsl_sm_5.0";
        case GPU_LANGUAGE_MSL_2_0:      return "msl_2";
        case LANGUAGE_OSL_1:            return "osl_1";
    }

    throw Exception("Unsupported GPU shader language.");
}

GpuLanguage GpuLanguageFromString(const char * s)
{
    const char * p = (s ? s : "");
    const std::string str = StringUtils::Lower(p);

    if(str == "cg") return GPU_LANGUAGE_CG;
    else if(str == "glsl_1.2")    return GPU_LANGUAGE_GLSL_1_2;
    else if(str == "glsl_1.3")    return GPU_LANGUAGE_GLSL_1_3;
    else if(str == "glsl_4.0")    return GPU_LANGUAGE_GLSL_4_0;
    else if(str == "glsl_es_1.0") return GPU_LANGUAGE_GLSL_ES_1_0;
    else if(str == "glsl_es_3.0") return GPU_LANGUAGE_GLSL_ES_3_0;
    else if(str == "hlsl_sm_5.0") return GPU_LANGUAGE_HLSL_SM_5_0;
    else if(str == "osl_1")       return LANGUAGE_OSL_1;
    else if(str == "msl_2")       return GPU_LANGUAGE_MSL_2_0;

    std::ostringstream oss;
    oss << "Unsupported GPU shader language: '" << p << "'.";
    throw Exception(oss.str().c_str());
}

const char * EnvironmentModeToString(EnvironmentMode mode)
{
    if(mode == ENV_ENVIRONMENT_LOAD_PREDEFINED) return "loadpredefined";
    else if(mode == ENV_ENVIRONMENT_LOAD_ALL) return "loadall";
    return "unknown";
}

EnvironmentMode EnvironmentModeFromString(const char * s)
{
    const std::string str = StringUtils::Lower(s ? s : "");

    if(str == "loadpredefined") return ENV_ENVIRONMENT_LOAD_PREDEFINED;
    else if(str == "loadall") return ENV_ENVIRONMENT_LOAD_ALL;
    return ENV_ENVIRONMENT_UNKNOWN;
}

const char * CDLStyleToString(CDLStyle style)
{
    if      (style == CDL_ASC)      return "asc";
    else if (style == CDL_NO_CLAMP) return "noClamp";
    return "asc";
}

CDLStyle CDLStyleFromString(const char * style)
{
    const char * p = (style ? style : "");
    const std::string str = StringUtils::Lower(p);

    if      (str == "asc")     return CDL_ASC;
    else if (str == "noclamp") return CDL_NO_CLAMP;

    std::ostringstream oss;
    oss << "Wrong CDL style: '" << p << "'.";
    throw Exception(oss.str().c_str());
}

const char * RangeStyleToString(RangeStyle style)
{
    if(style == RANGE_NO_CLAMP) return "noClamp";
    else if(style == RANGE_CLAMP) return "Clamp";
    return "Clamp";
}

RangeStyle RangeStyleFromString(const char * style)
{
    const char * p = (style ? style : "");
    const std::string str = StringUtils::Lower(p);

    if (str == "noclamp") return RANGE_NO_CLAMP;
    else if (str == "clamp") return RANGE_CLAMP;

    std::ostringstream oss;
    oss << "Wrong Range style '" << p << "'.";
    throw Exception(oss.str().c_str());
}

const char * FixedFunctionStyleToString(FixedFunctionStyle style)
{
    switch(style)
    {
        case FIXED_FUNCTION_ACES_RED_MOD_03:            return "ACES_RedMod03";
        case FIXED_FUNCTION_ACES_RED_MOD_10:            return "ACES_RedMod10";
        case FIXED_FUNCTION_ACES_GLOW_03:               return "ACES_Glow03";
        case FIXED_FUNCTION_ACES_GLOW_10:               return "ACES_Glow10";
        case FIXED_FUNCTION_ACES_DARK_TO_DIM_10:        return "ACES_DarkToDim10";
        case FIXED_FUNCTION_ACES_GAMUT_COMP_13:         return "ACES_GamutComp13";
        case FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20:   return "ACES2_OutputTransform";
        case FIXED_FUNCTION_ACES_RGB_TO_JMH_20:         return "ACES2_RGB_TO_JMh";
        case FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20: return "ACES2_TonescaleCompress";
        case FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20:     return "ACES2_GamutCompress";
        case FIXED_FUNCTION_REC2100_SURROUND:           return "REC2100_Surround";
        case FIXED_FUNCTION_RGB_TO_HSV:                 return "RGB_TO_HSV";
        case FIXED_FUNCTION_XYZ_TO_xyY:                 return "XYZ_TO_xyY";
        case FIXED_FUNCTION_XYZ_TO_uvY:                 return "XYZ_TO_uvY";
        case FIXED_FUNCTION_XYZ_TO_LUV:                 return "XYZ_TO_LUV";
        case FIXED_FUNCTION_LIN_TO_PQ:                  return "Lin_TO_PQ";
        case FIXED_FUNCTION_LIN_TO_GAMMA_LOG:           return "Lin_TO_GammaLog";
        case FIXED_FUNCTION_LIN_TO_DOUBLE_LOG:          return "Lin_TO_DoubleLog";
        case FIXED_FUNCTION_ACES_GAMUTMAP_02:
        case FIXED_FUNCTION_ACES_GAMUTMAP_07:
            throw Exception("Unimplemented fixed function types: "
                            "FIXED_FUNCTION_ACES_GAMUTMAP_02, "
                            "FIXED_FUNCTION_ACES_GAMUTMAP_07.");
    }

    // Default style is meaningless.
    throw Exception("Unknown Fixed FunctionOp style");
}

FixedFunctionStyle FixedFunctionStyleFromString(const char * style)
{
    const char * p = (style ? style : "");
    const std::string str = StringUtils::Lower(p);

    if(str == "aces_redmod03")                return FIXED_FUNCTION_ACES_RED_MOD_03;
    else if(str == "aces_redmod10")           return FIXED_FUNCTION_ACES_RED_MOD_10;
    else if(str == "aces_glow03")             return FIXED_FUNCTION_ACES_GLOW_03;
    else if(str == "aces_glow10")             return FIXED_FUNCTION_ACES_GLOW_10;
    else if(str == "aces_darktodim10")        return FIXED_FUNCTION_ACES_DARK_TO_DIM_10;
    else if(str == "aces_gamutcomp13")        return FIXED_FUNCTION_ACES_GAMUT_COMP_13;
    else if(str == "aces2_outputtransform")   return FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20;
    else if(str == "aces2_rgb_to_jmh")        return FIXED_FUNCTION_ACES_RGB_TO_JMH_20;
    else if(str == "aces2_tonescalecompress") return FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20;
    else if(str == "aces2_gamutcompress")     return FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20;
    else if(str == "rec2100_surround")        return FIXED_FUNCTION_REC2100_SURROUND;
    else if(str == "rgb_to_hsv")              return FIXED_FUNCTION_RGB_TO_HSV;
    else if(str == "xyz_to_xyy")              return FIXED_FUNCTION_XYZ_TO_xyY;
    else if(str == "xyz_to_uvy")              return FIXED_FUNCTION_XYZ_TO_uvY;
    else if(str == "xyz_to_luv")              return FIXED_FUNCTION_XYZ_TO_LUV;
    else if(str == "lin_to_pq")               return FIXED_FUNCTION_LIN_TO_PQ;
    else if(str == "lin_to_gammalog")         return FIXED_FUNCTION_LIN_TO_GAMMA_LOG;
    else if(str == "lin_to_doublelog")        return FIXED_FUNCTION_LIN_TO_DOUBLE_LOG;

    // Default style is meaningless.
    std::stringstream ss;
    ss << "Unknown Fixed FunctionOp style: '" << p << "'.";
    throw Exception(ss.str().c_str());
}

namespace
{
static constexpr char GRADING_STYLE_LINEAR[]      = "linear";
static constexpr char GRADING_STYLE_LOGARITHMIC[] = "log";
static constexpr char GRADING_STYLE_VIDEO[]       = "video";
}

const char * GradingStyleToString(GradingStyle style)
{
    switch (style)
    {
    case GRADING_LIN:    return GRADING_STYLE_LINEAR;
    case GRADING_LOG:    return GRADING_STYLE_LOGARITHMIC;
    case GRADING_VIDEO:  return GRADING_STYLE_VIDEO;
    }

    // Default style is meaningless.
    throw Exception("Unknown grading style");
}

GradingStyle GradingStyleFromString(const char * style)
{
    const char * p = (style ? style : "");
    const std::string str = StringUtils::Lower(p);

    if      (str == GRADING_STYLE_LINEAR)      return GRADING_LIN;
    else if (str == GRADING_STYLE_LOGARITHMIC) return GRADING_LOG;
    else if (str == GRADING_STYLE_VIDEO)       return GRADING_VIDEO;

    // Default style is meaningless.
    std::stringstream ss;
    ss << "Unknown grading style: '" << p << "'.";
    throw Exception(ss.str().c_str());
}

namespace
{
static constexpr char EC_STYLE_VIDEO[]       = "video";
static constexpr char EC_STYLE_LOGARITHMIC[] = "log";
static constexpr char EC_STYLE_LINEAR[]      = "linear";
}

const char * ExposureContrastStyleToString(ExposureContrastStyle style)
{
    switch (style)
    {
    case EXPOSURE_CONTRAST_VIDEO:       return EC_STYLE_VIDEO;
    case EXPOSURE_CONTRAST_LOGARITHMIC: return EC_STYLE_LOGARITHMIC;
    case EXPOSURE_CONTRAST_LINEAR:      return EC_STYLE_LINEAR;
    }

    // Default style is meaningless.
    throw Exception("Unknown exposure contrast style");
}

ExposureContrastStyle ExposureContrastStyleFromString(const char * style)
{
    const char * p = (style ? style : "");
    const std::string str = StringUtils::Lower(p);

    if      (str == EC_STYLE_LINEAR)      return EXPOSURE_CONTRAST_LINEAR;
    else if (str == EC_STYLE_VIDEO)       return EXPOSURE_CONTRAST_VIDEO;
    else if (str == EC_STYLE_LOGARITHMIC) return EXPOSURE_CONTRAST_LOGARITHMIC;

    // Default style is meaningless.
    std::stringstream ss;
    ss << "Unknown exposure contrast style: '" << p << "'.";
    throw Exception(ss.str().c_str());
}


namespace
{
static constexpr char NEGATIVE_STYLE_CLAMP[]     = "clamp";
static constexpr char NEGATIVE_STYLE_LINEAR[]    = "linear";
static constexpr char NEGATIVE_STYLE_MIRROR[]    = "mirror";
static constexpr char NEGATIVE_STYLE_PASS_THRU[] = "pass_thru";
}

const char * NegativeStyleToString(NegativeStyle style)
{
    switch (style)
    {
    case NEGATIVE_CLAMP:     return NEGATIVE_STYLE_CLAMP;
    case NEGATIVE_MIRROR:    return NEGATIVE_STYLE_MIRROR;
    case NEGATIVE_PASS_THRU: return NEGATIVE_STYLE_PASS_THRU;
    case NEGATIVE_LINEAR:    return NEGATIVE_STYLE_LINEAR;
    }

    throw Exception("Unknown exponent style");
}

NegativeStyle NegativeStyleFromString(const char * style)
{
    const char * p = (style ? style : "");
    const std::string str = StringUtils::Lower(p);

    if (str == NEGATIVE_STYLE_MIRROR)         return NEGATIVE_MIRROR;
    else if (str == NEGATIVE_STYLE_PASS_THRU) return NEGATIVE_PASS_THRU;
    else if (str == NEGATIVE_STYLE_CLAMP)     return NEGATIVE_CLAMP;
    else if (str == NEGATIVE_STYLE_LINEAR)    return NEGATIVE_LINEAR;

    std::stringstream ss;
    ss << "Unknown exponent style: '" << p << "'.";
    throw Exception(ss.str().c_str());
}

// Define variables declared in OpenColorTypes.h.
const char * ROLE_DEFAULT             = "default";
const char * ROLE_REFERENCE           = "reference";
const char * ROLE_DATA                = "data";
const char * ROLE_COLOR_PICKING       = "color_picking";
const char * ROLE_SCENE_LINEAR        = "scene_linear";
const char * ROLE_COMPOSITING_LOG     = "compositing_log";
const char * ROLE_COLOR_TIMING        = "color_timing";
const char * ROLE_TEXTURE_PAINT       = "texture_paint";
const char * ROLE_MATTE_PAINT         = "matte_paint";
const char * ROLE_RENDERING           = "rendering";
const char * ROLE_INTERCHANGE_SCENE   = "aces_interchange";
const char * ROLE_INTERCHANGE_DISPLAY = "cie_xyz_d65_interchange";

namespace
{
static constexpr int FLOAT_DECIMALS  = 7;
static constexpr int DOUBLE_DECIMALS = 16;
}

std::string FloatToString(float value)
{
    std::ostringstream pretty;
    pretty.imbue(std::locale::classic());
    pretty.precision(FLOAT_DECIMALS);
    pretty << value;
    return pretty.str();
}

std::string FloatVecToString(const float * fval, unsigned int size)
{
    if(size<=0) return "";

    std::ostringstream pretty;
    pretty.imbue(std::locale::classic());
    pretty.precision(FLOAT_DECIMALS);
    for(unsigned int i=0; i<size; ++i)
    {
        if(i!=0) pretty << " ";
        pretty << fval[i];
    }

    return pretty.str();
}

bool StringToFloat(float * fval, const char * str)
{
    if(!str) return false;

    float x = NAN;
    const auto result = NumberUtils::from_chars(str, str + strlen(str), x);
    if (result.ec != std::errc())
    {
        return false;
    }

    if(fval) *fval = x;
    return true;
}

bool StringToInt(int * ival, const char * str, bool failIfLeftoverChars)
{
    if(!str) return false;
    if(!ival) return false;

    std::istringstream i(str);
    i.imbue(std::locale::classic());
    char c=0;
    if (!(i >> *ival) || (failIfLeftoverChars && i.get(c))) return false;
    return true;
}

std::string DoubleToString(double value)
{
    std::ostringstream pretty;
    pretty.imbue(std::locale::classic());
    pretty.precision(DOUBLE_DECIMALS);
    pretty << value;
    return pretty.str();
}

std::string DoubleVecToString(const double * val, unsigned int size)
{
    if (size <= 0) return "";

    std::ostringstream pretty;
    pretty.imbue(std::locale::classic());
    pretty.precision(DOUBLE_DECIMALS);
    for (unsigned int i = 0; i<size; ++i)
    {
        if (i != 0) pretty << " ";
        pretty << val[i];
    }

    return pretty.str();
}

bool StringVecToFloatVec(std::vector<float> &floatArray, const StringUtils::StringVec &lineParts)
{
    floatArray.resize(lineParts.size());

    for(unsigned int i=0; i<lineParts.size(); i++)
    {
        float x = NAN;
        const char *str = lineParts[i].c_str();
        const auto result = NumberUtils::from_chars(str, str + lineParts[i].size(), x);
        if (result.ec != std::errc())
        {
            return false;
        }
        floatArray[i] = x;
    }

    return true;
}

// This will resize intArray to the size of lineParts.
// Returns true if all lineParts have been recognized as int.
// Content of intArray will be unknown if function returns false.
bool StringVecToIntVec(std::vector<int> &intArray, const StringUtils::StringVec &lineParts)
{
    intArray.resize(lineParts.size());

    for(unsigned int i=0; i<lineParts.size(); i++)
    {
        int x;
        // When reading a vector of string as int, ints that
        // are followed by other characters (ex. "3d") are
        // not considered as int.
        if ( !StringToInt(&x, lineParts[i].c_str(), true) )
        {
            return false;
        }
        intArray[i] = x;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////

// read the next non-empty line, and store it in 'line'
// return 'true' on success

bool nextline(std::istream &istream, std::string &line)
{
    while ( istream.good() )
    {
        std::getline(istream, line);
        if(line.size() > 0 && line[line.size() - 1] == '\r')
        {
            line.resize(line.size() - 1);
        }
        if(!StringUtils::IsEmptyOrWhiteSpace(line))
        {
            return true;
        }
    }

    line.clear();
    return false;
}

bool StrEqualsCaseIgnore(const std::string & a, const std::string & b)
{
    return 0 == Platform::Strcasecmp(a.c_str(), b.c_str());
}

// Find the end of a name from a list contained in a string.
// The elements of the list are separated by the character defined by the sep argument.
// A name can be surrounded by quotes to enable names including the sep symbol, in 
// other words, to prevent it from being used as a separator.
static int FindEndOfName(const std::string & s, size_t start, char sep)
{
    int currentPos = static_cast<int>(start);
    int nameEndPos = currentPos;
    bool isEndFound = false;
    
    std::string symbols = "\"";
    symbols += sep;

    while ( !isEndFound )
    {
        nameEndPos =  static_cast<int>( s.find_first_of( symbols, currentPos ) );
        if ( nameEndPos == static_cast<int>(std::string::npos) )
        {
            // Reached the end of the list.
            nameEndPos =  static_cast<int>( s.size() );
            isEndFound = true;
        } 
        else if ( s[nameEndPos] == '"' )
        {
            // Found a quote, need to find the next one.
            nameEndPos =  static_cast<int>( s.find_first_of("\"", nameEndPos + 1) );
            if ( nameEndPos == (int)std::string::npos )
            {
                // Reached the end of the list instead.
                std::ostringstream os;
                os << "The string '" << s << "' is not correctly formatted. " <<
                "It is missing a closing quote.";
                throw Exception(os.str().c_str());
            }
            else
            {
                // Found the second quote, need to continue the search for a symbol
                // separating the elements (: or ,).
                currentPos = nameEndPos + 1;
            }
        }
        else if( s[nameEndPos] == sep )
        {
            // Found a symbol separating the elements, stop here.
            isEndFound = true;
        }
    }

    return nameEndPos;
}

StringUtils::StringVec SplitStringEnvStyle(const std::string & str)
{
    const std::string s = StringUtils::Trim(str);
    if( s.size() == 0 ) 
    {
        // Look parsing always wants a result, even if an empty string.
        return { "" };
    }

    StringUtils::StringVec outputvec;
    auto foundComma = s.find_first_of(",");
    auto foundColon = s.find_first_of(":");

    if( foundComma != std::string::npos || 
        foundColon != std::string::npos )
    {
        int currentPos = 0;
        while( s.size() > 0 && 
               currentPos <= (int)s.size() )
        {
            int nameEndPos = FindEndOfName( s, 
                                            currentPos, 
                                            foundComma != std::string::npos ? ',' : ':' );
            if(nameEndPos > currentPos)
            {
                outputvec.push_back(s.substr(currentPos, nameEndPos - currentPos));
                currentPos = nameEndPos + 1;
            }
            else
            {
                outputvec.push_back("");
                currentPos += 1;
            }
        }
    }
    else
    {
        // If there is no comma or colon, consider the string as a single element.
        outputvec.push_back(s);
    }

    for ( auto & val : outputvec )
    {
        const std::string trimmedValue = StringUtils::Trim(val);

        // If the trimmed value is surrounded by quotes, remove them.
        if( trimmedValue.size() > 1 && 
            trimmedValue[0] == '"' && 
            trimmedValue[trimmedValue.size() - 1] == '"' )
        {
            val = trimmedValue.substr(1, trimmedValue.size() - 2);
        }
        else
        {
            val = trimmedValue;
        }
    }
    
    return outputvec;
}

std::string JoinStringEnvStyle(const StringUtils::StringVec & outputvec)
{
    std::string result;
    const int nElement = static_cast<int>(outputvec.size());
    if( nElement == 0 )
    {
        return "";
    }

    // Check if the value contains a symbol that could be interpreted as a separator
    // and if it is not already surrounded by quotes.
    const std::string& firstValue = outputvec[0];
    if( firstValue.find_first_of(",:") != std::string::npos &&
        firstValue.size() > 1 &&
        firstValue[0] != '"' &&
        firstValue[firstValue.size() - 1] != '"' )
    {
        result += "\"" + firstValue + "\"";
    }
    else
    {
        result += firstValue;
    }

    for( int i = 1; i < nElement; ++i )
    {
        if( outputvec[i].find_first_of(",:") != std::string::npos &&
            outputvec[i].size() > 1 &&
            outputvec[i][0] != '"' &&
            outputvec[i][outputvec[i].size() - 1] != '"' )
        {
            result += ", \"" + outputvec[i] + "\"";
        }
        else
        {
            result += ", " + outputvec[i];
        }
    }

    return result;
}

// Return a vector of strings that are both in vec1 and vec2.
// Case is ignored to find strings.
// Ordering and capitalization from vec1 are preserved.
StringUtils::StringVec IntersectStringVecsCaseIgnore(const StringUtils::StringVec & vec1,
                                                     const StringUtils::StringVec & vec2)
{
    StringUtils::StringVec newvec;
    std::set<std::string> allvalues;

    // Seed the set with all values from vec2
    for (const auto & val : vec2)
    {
        allvalues.insert(StringUtils::Lower(val));
    }

    for (const auto & val : vec1)
    {
        const std::string key = StringUtils::Lower(val);
        if(allvalues.find(key) != allvalues.end())
        {
            newvec.push_back(val);
        }
    }

    return newvec;
}

int FindInStringVecCaseIgnore(const StringUtils::StringVec & vec, const std::string & str)
{
    const std::string teststr = StringUtils::Lower(str);
    for(unsigned int i=0; i<vec.size(); ++i)
    {
        if(StringUtils::Lower(vec[i]) == teststr) return static_cast<int>(i);
    }

    return -1;
}
} // namespace OCIO_NAMESPACE
