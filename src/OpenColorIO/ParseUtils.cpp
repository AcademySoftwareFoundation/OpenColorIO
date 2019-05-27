/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#include <iostream>
#include <set>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
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

    const char * BoolToString(bool val)
    {
        if(val) return "true";
        return "false";
    }
    
    bool BoolFromString(const char * s)
    {
        std::string str = pystring::lower(s);
        if((str == "true") || (str=="yes")) return true;
        return false;
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
        std::string str = pystring::lower(s);
        if(str == "0" || str == "none") return LOGGING_LEVEL_NONE;
        else if(str == "1" || str == "warning") return LOGGING_LEVEL_WARNING;
        else if(str == "2" || str == "info") return LOGGING_LEVEL_INFO;
        else if(str == "3" || str == "debug") return LOGGING_LEVEL_DEBUG;
        return LOGGING_LEVEL_UNKNOWN;
    }
    
    const char * TransformDirectionToString(TransformDirection dir)
    {
        if(dir == TRANSFORM_DIR_FORWARD) return "forward";
        else if(dir == TRANSFORM_DIR_INVERSE) return "inverse";
        return "unknown";
    }
    
    TransformDirection TransformDirectionFromString(const char * s)
    {
        std::string str = pystring::lower(s);
        if(str == "forward") return TRANSFORM_DIR_FORWARD;
        else if(str == "inverse") return TRANSFORM_DIR_INVERSE;
        return TRANSFORM_DIR_UNKNOWN;
    }
    
    TransformDirection CombineTransformDirections(TransformDirection d1,
                                                  TransformDirection d2)
    {
        // Any unknowns always combine to be unknown.
        if(d1 == TRANSFORM_DIR_UNKNOWN || d2 == TRANSFORM_DIR_UNKNOWN)
            return TRANSFORM_DIR_UNKNOWN;
        
        if(d1 == TRANSFORM_DIR_FORWARD && d2 == TRANSFORM_DIR_FORWARD)
            return TRANSFORM_DIR_FORWARD;
        
        if(d1 == TRANSFORM_DIR_INVERSE && d2 == TRANSFORM_DIR_INVERSE)
            return TRANSFORM_DIR_FORWARD;
        
        return TRANSFORM_DIR_INVERSE;
    }
    
    TransformDirection GetInverseTransformDirection(TransformDirection dir)
    {
        if(dir == TRANSFORM_DIR_FORWARD) return TRANSFORM_DIR_INVERSE;
        else if(dir == TRANSFORM_DIR_INVERSE) return TRANSFORM_DIR_FORWARD;
        return TRANSFORM_DIR_UNKNOWN;
    }
    
    const char * ColorSpaceDirectionToString(ColorSpaceDirection dir)
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE) return "to_reference";
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE) return "from_reference";
        return "unknown";
    }
    
    ColorSpaceDirection ColorSpaceDirectionFromString(const char * s)
    {
        std::string str = pystring::lower(s);
        if(str == "to_reference") return COLORSPACE_DIR_TO_REFERENCE;
        else if(str == "from_reference") return COLORSPACE_DIR_FROM_REFERENCE;
        return COLORSPACE_DIR_UNKNOWN;
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
        std::string str = pystring::lower(s);
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
        std::string str = pystring::lower(s);
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
        else if (interp == INTERP_DEFAULT) return "default";
        return "unknown";
    }
    
    Interpolation InterpolationFromString(const char * s)
    {
        std::string str = pystring::lower(s);
        if(str == "nearest") return INTERP_NEAREST;
        else if(str == "linear") return INTERP_LINEAR;
        else if(str == "tetrahedral") return INTERP_TETRAHEDRAL;
        else if(str == "best") return INTERP_BEST;
        return INTERP_UNKNOWN;
    }
    
    const char * GpuLanguageToString(GpuLanguage language)
    {
        if(language == GPU_LANGUAGE_CG) return "cg";
        else if(language == GPU_LANGUAGE_GLSL_1_0)  return "glsl_1.0";
        else if(language == GPU_LANGUAGE_GLSL_1_3)  return "glsl_1.3";
        else if(language == GPU_LANGUAGE_GLSL_4_0)  return "glsl_4.0";
        else if(language == GPU_LANGUAGE_HLSL_DX11) return "hlsl_dx11";
        return "unknown";
    }
    
    GpuLanguage GpuLanguageFromString(const char * s)
    {
        std::string str = pystring::lower(s);
        if(str == "cg") return GPU_LANGUAGE_CG;
        else if(str == "glsl_1.0") return GPU_LANGUAGE_GLSL_1_0;
        else if(str == "glsl_1.3") return GPU_LANGUAGE_GLSL_1_3;
        else if(str == "glsl_4.0") return GPU_LANGUAGE_GLSL_4_0;
        else if(str == "hlsl_dx11") return GPU_LANGUAGE_HLSL_DX11;
        return GPU_LANGUAGE_UNKNOWN;
    }
    
    
    const char * EnvironmentModeToString(EnvironmentMode mode)
    {
        if(mode == ENV_ENVIRONMENT_LOAD_PREDEFINED) return "loadpredefined";
        else if(mode == ENV_ENVIRONMENT_LOAD_ALL) return "loadall";
        return "unknown";
    }
    
    EnvironmentMode EnvironmentModeFromString(const char * s)
    {
        std::string str = pystring::lower(s);
        if(str == "loadpredefined") return ENV_ENVIRONMENT_LOAD_PREDEFINED;
        else if(str == "loadall") return ENV_ENVIRONMENT_LOAD_ALL;
        return ENV_ENVIRONMENT_UNKNOWN;
    }
    
    const char * RangeStyleToString(RangeStyle style)
    {
        if(style == RANGE_NO_CLAMP) return "noClamp";
        else if(style == RANGE_CLAMP) return "Clamp";
        return "Clamp";
    }
    
    RangeStyle RangeStyleFromString(const char * style)
    {
        if(style && *style)
        {
            const std::string str = pystring::lower(style);
            if(str == "noclamp") return RANGE_NO_CLAMP;
            else if(str == "clamp") return RANGE_CLAMP;
        }

        std::string msg("Wrong Range style: ");
        msg += (style && *style) ? style : "<null>";

        throw Exception(msg.c_str());
        return RANGE_CLAMP;
    }

    const char * FixedFunctionStyleToString(FixedFunctionStyle style)
    {
        switch(style)
        {
            case FIXED_FUNCTION_ACES_RED_MOD_03:     return "ACES_RedMod03";      break;
            case FIXED_FUNCTION_ACES_RED_MOD_10:     return "ACES_RedMod10";      break;
            case FIXED_FUNCTION_ACES_GLOW_03:        return "ACES_Glow03";        break;
            case FIXED_FUNCTION_ACES_GLOW_10:        return "ACES_Glow10";        break;
            case FIXED_FUNCTION_ACES_DARK_TO_DIM_10: return "ACES_DarkToDim10";   break;
            case FIXED_FUNCTION_REC2100_SURROUND:    return "REC2100_Surround";   break;
        }

        // Default style is meaningless.
        throw Exception("Unknown Fixed FunctionOp style");
        return nullptr;
    }

    FixedFunctionStyle FixedFunctionStyleFromString(const char * style)
    {
        std::string str = pystring::lower(style);

        if(str == "aces_redmod03")           return FIXED_FUNCTION_ACES_RED_MOD_03;
        else if(str == "aces_redmod10")      return FIXED_FUNCTION_ACES_RED_MOD_10;
        else if(str == "aces_glow03")        return FIXED_FUNCTION_ACES_GLOW_03;
        else if(str == "aces_glow10")        return FIXED_FUNCTION_ACES_GLOW_10;
        else if(str == "aces_darktodim10")   return FIXED_FUNCTION_ACES_DARK_TO_DIM_10;
        else if(str == "rec2100_surround")   return FIXED_FUNCTION_REC2100_SURROUND;

        // Default style is meaningless.
        std::stringstream ss;
        ss << "Unknown Fixed FunctionOp style: " << style;

        throw Exception(ss.str().c_str());
        return FIXED_FUNCTION_ACES_RED_MOD_03;
    }

    namespace
    {
        static constexpr const char * EC_STYLE_VIDEO = "video";
        static constexpr const char * EC_STYLE_LOGARITHMIC = "log";
        static constexpr const char * EC_STYLE_LINEAR = "linear";
    }

    const char * ExposureContrastStyleToString(ExposureContrastStyle style)
    {
        switch (style)
        {
        case EXPOSURE_CONTRAST_VIDEO:       return EC_STYLE_VIDEO;       break;
        case EXPOSURE_CONTRAST_LOGARITHMIC: return EC_STYLE_LOGARITHMIC; break;
        case EXPOSURE_CONTRAST_LINEAR:      return EC_STYLE_LINEAR;      break;
        }

        // Default style is meaningless.
        throw Exception("Unknown exposure contrast style");
        return nullptr;
    }
    
    ExposureContrastStyle ExposureContrastStyleFromString(const char * style)
    {
        std::string str = pystring::lower(style);

        if      (str == EC_STYLE_LINEAR)      return EXPOSURE_CONTRAST_LINEAR;
        else if (str == EC_STYLE_VIDEO)       return EXPOSURE_CONTRAST_VIDEO;
        else if (str == EC_STYLE_LOGARITHMIC) return EXPOSURE_CONTRAST_LOGARITHMIC;

        // Default style is meaningless.
        std::stringstream ss;
        ss << "Unknown exposure contrast style: " << style;

        throw Exception(ss.str().c_str());
        return EXPOSURE_CONTRAST_LINEAR;
    }

    const char * ROLE_DEFAULT = "default";
    const char * ROLE_REFERENCE = "reference";
    const char * ROLE_DATA = "data";
    const char * ROLE_COLOR_PICKING = "color_picking";
    const char * ROLE_SCENE_LINEAR = "scene_linear";
    const char * ROLE_COMPOSITING_LOG = "compositing_log";
    const char * ROLE_COLOR_TIMING = "color_timing";
    const char * ROLE_TEXTURE_PAINT = "texture_paint";
    const char * ROLE_MATTE_PAINT = "matte_paint";

    namespace
    {
        const int FLOAT_DECIMALS = 7;
        const int DOUBLE_DECIMALS = 16;
    }

    std::string FloatToString(float value)
    {
        std::ostringstream pretty;
        pretty.precision(FLOAT_DECIMALS);
        pretty << value;
        return pretty.str();
    }
    
    std::string FloatVecToString(const float * fval, unsigned int size)
    {
        if(size<=0) return "";
        
        std::ostringstream pretty;
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
        
        std::istringstream inputStringstream(str);
        float x;
        if(!(inputStringstream >> x))
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
        char c=0;
        if (!(i >> *ival) || (failIfLeftoverChars && i.get(c))) return false;
        return true;
    }
    
    
    std::string DoubleToString(double value)
    {
        std::ostringstream pretty;
        pretty.precision(DOUBLE_DECIMALS);
        pretty << value;
        return pretty.str();
    }
    
    
    bool StringVecToFloatVec(std::vector<float> &floatArray,
                             const std::vector<std::string> &lineParts)
    {
        floatArray.resize(lineParts.size());
        
        for(unsigned int i=0; i<lineParts.size(); i++)
        {
            std::istringstream inputStringstream(lineParts[i]);
            float x;
            if(!(inputStringstream >> x))
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
    bool StringVecToIntVec(std::vector<int> &intArray,
                           const std::vector<std::string> &lineParts)
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
    
    // read the next non empty line, and store it in 'line'
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
            if(!pystring::strip(line).empty())
            {
                return true;
            }
        }
        
        line = "";
        return false;
    }
    
    
    bool StrEqualsCaseIgnore(const std::string & a, const std::string & b)
    {
        return (pystring::lower(a) == pystring::lower(b));
    }
    
    // If a ',' is in the string, split on it
    // If a ':' is in the string, split on it
    // Otherwise, assume a single string.
    // Also, strip whitespace from all parts.
    
    void SplitStringEnvStyle(std::vector<std::string> & outputvec, const char * str)
    {
        if(!str) return;
        
        std::string s = pystring::strip(str);
        if(pystring::find(s, ",") > -1)
        {
            pystring::split(s, outputvec, ",");
        }
        else if(pystring::find(s, ":") > -1)
        {
            pystring::split(s, outputvec, ":");
        }
        else
        {
            outputvec.push_back(s);
        }
        
        for(unsigned int i=0; i<outputvec.size(); ++i)
        {
            outputvec[i] = pystring::strip(outputvec[i]);
        }
    }
    
    std::string JoinStringEnvStyle(const std::vector<std::string> & outputvec)
    {
        return pystring::join(", ", outputvec);
    }
    
    // Return a vector of strings that are both in vec1 and vec2.
    // Case is ignored to find strings.
    // Ordering and capitalization from vec1 are preserved.
    std::vector<std::string> IntersectStringVecsCaseIgnore(const std::vector<std::string> & vec1,
                                                           const std::vector<std::string> & vec2)
    {
        std::vector<std::string> newvec;
        std::set<std::string> allvalues;
        
        // Seed the set with all values from vec2
        for(unsigned int i=0; i<vec2.size(); ++i)
        {
            allvalues.insert(pystring::lower(vec2[i]));
        }
        
        for(unsigned int i=0; i<vec1.size(); ++i)
        {
            std::string key = pystring::lower(vec1[i]);
            if(allvalues.find(key) != allvalues.end())
            {
                newvec.push_back(vec1[i]);
            }
        }
        
        return newvec;
    }
    
    
    int FindInStringVecCaseIgnore(const std::vector<std::string> & vec, const std::string & str)
    {
        std::string teststr = pystring::lower(str);
        for(unsigned int i=0; i<vec.size(); ++i)
        {
            if(pystring::lower(vec[i]) == teststr) return static_cast<int>(i);
        }
        
        return -1;
    }
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "unittest.h"

OIIO_ADD_TEST(ParseUtils, XMLText)
{
    const std::string in("abc \" def ' ghi < jkl > mnop & efg");
    const std::string ref("abc &quot; def &apos; ghi &lt; jkl &gt; mnop &amp; efg");

    std::string out1 = OCIO::ConvertSpecialCharToXmlToken(in);
    OIIO_CHECK_EQUAL(out1, ref);
}

OIIO_ADD_TEST(ParseUtils, BoolString)
{
    std::string resStr = OCIO::BoolToString(true);
    OIIO_CHECK_EQUAL("true", resStr);

    resStr = OCIO::BoolToString(false);
    OIIO_CHECK_EQUAL("false", resStr);

    bool resBool = OCIO::BoolFromString("yes");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("Yes");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("YES");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("YeS");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("yEs");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("true");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("TRUE");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("True");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("tRUe");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("tRUE");
    OIIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("yes ");
    OIIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString(" true ");
    OIIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("false");
    OIIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("");
    OIIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("no");
    OIIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("valid");
    OIIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("success");
    OIIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("anything");
    OIIO_CHECK_EQUAL(false, resBool);
}

OIIO_ADD_TEST(ParseUtils, TransformDirection)
{
    std::string resStr;
    resStr = OCIO::TransformDirectionToString(OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL("forward", resStr);
    resStr = OCIO::TransformDirectionToString(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL("inverse", resStr);
    resStr = OCIO::TransformDirectionToString(OCIO::TRANSFORM_DIR_UNKNOWN);
    OIIO_CHECK_EQUAL("unknown", resStr);

    OCIO::TransformDirection resDir;
    resDir = OCIO::TransformDirectionFromString("forward");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::TransformDirectionFromString("inverse");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    resDir = OCIO::TransformDirectionFromString("Forward");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::TransformDirectionFromString("Inverse");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    resDir = OCIO::TransformDirectionFromString("FORWARD");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::TransformDirectionFromString("INVERSE");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    resDir = OCIO::TransformDirectionFromString("unknown");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_UNKNOWN, resDir);
    resDir = OCIO::TransformDirectionFromString("");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_UNKNOWN, resDir);
    resDir = OCIO::TransformDirectionFromString("anything");
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_UNKNOWN, resDir);

    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_UNKNOWN,
        OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_UNKNOWN, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_INVERSE,
        OCIO::TRANSFORM_DIR_UNKNOWN);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_UNKNOWN, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_UNKNOWN,
        OCIO::TRANSFORM_DIR_UNKNOWN);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_UNKNOWN, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_INVERSE,
        OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_FORWARD,
        OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_INVERSE,
        OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_FORWARD,
        OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);

    resDir = OCIO::GetInverseTransformDirection(OCIO::TRANSFORM_DIR_UNKNOWN);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_UNKNOWN, resDir);
    resDir = OCIO::GetInverseTransformDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::GetInverseTransformDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
}

OIIO_ADD_TEST(ParseUtils, ColorSpace)
{
    std::string resStr;
    resStr = OCIO::ColorSpaceDirectionToString(
        OCIO::COLORSPACE_DIR_TO_REFERENCE);
    OIIO_CHECK_EQUAL("to_reference", resStr);
    resStr = OCIO::ColorSpaceDirectionToString(
        OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    OIIO_CHECK_EQUAL("from_reference", resStr);
    resStr = OCIO::ColorSpaceDirectionToString(
        OCIO::COLORSPACE_DIR_UNKNOWN);
    OIIO_CHECK_EQUAL("unknown", resStr);

    OCIO::ColorSpaceDirection resCSD;
    resCSD = OCIO::ColorSpaceDirectionFromString("to_reference");
    OIIO_CHECK_EQUAL(OCIO::COLORSPACE_DIR_TO_REFERENCE, resCSD);
    resCSD = OCIO::ColorSpaceDirectionFromString("from_reference");
    OIIO_CHECK_EQUAL(OCIO::COLORSPACE_DIR_FROM_REFERENCE, resCSD);
    resCSD = OCIO::ColorSpaceDirectionFromString("unkwon");
    OIIO_CHECK_EQUAL(OCIO::COLORSPACE_DIR_UNKNOWN, resCSD);
    resCSD = OCIO::ColorSpaceDirectionFromString("");
    OIIO_CHECK_EQUAL(OCIO::COLORSPACE_DIR_UNKNOWN, resCSD);
}

OIIO_ADD_TEST(ParseUtils, BitDepth)
{
    std::string resStr;
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL("8ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL("10ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL("12ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT14);
    OIIO_CHECK_EQUAL("14ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT16);
    OIIO_CHECK_EQUAL("16ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT32);
    OIIO_CHECK_EQUAL("32ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL("16f", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL("32f", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UNKNOWN);
    OIIO_CHECK_EQUAL("unknown", resStr);
    
    OCIO::BitDepth resBD;
    resBD = OCIO::BitDepthFromString("8ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("8Ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("8UI");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("8uI");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("10ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT10, resBD);
    resBD = OCIO::BitDepthFromString("12ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT12, resBD);
    resBD = OCIO::BitDepthFromString("14ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT14, resBD);
    resBD = OCIO::BitDepthFromString("16ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT16, resBD);
    resBD = OCIO::BitDepthFromString("32ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT32, resBD);
    resBD = OCIO::BitDepthFromString("16f");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F16, resBD);
    resBD = OCIO::BitDepthFromString("32f");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F32, resBD);
    resBD = OCIO::BitDepthFromString("7ui");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UNKNOWN, resBD);
    resBD = OCIO::BitDepthFromString("unknown");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UNKNOWN, resBD);
    resBD = OCIO::BitDepthFromString("");
    OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UNKNOWN, resBD);


    bool resBool;
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(true, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(true, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT14);
    OIIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT16);
    OIIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT32);
    OIIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UNKNOWN);
    OIIO_CHECK_EQUAL(false, resBool);

    int resInt;
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(8, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(10, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(12, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT14);
    OIIO_CHECK_EQUAL(14, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT16);
    OIIO_CHECK_EQUAL(16, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT32);
    OIIO_CHECK_EQUAL(32, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(0, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(0, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UNKNOWN);
    OIIO_CHECK_EQUAL(0, resInt);

}

OIIO_ADD_TEST(ParseUtils, StringToInt)
{
    int ival = 0;
    bool success = false;
    
    success = OCIO::StringToInt(&ival, "", false);
    OIIO_CHECK_EQUAL(success, false);
    
    success = OCIO::StringToInt(&ival, "9", false);
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(ival, 9);
    
    success = OCIO::StringToInt(&ival, " 10 ", false);
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(ival, 10);
    
    success = OCIO::StringToInt(&ival, " 101", true);
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(ival, 101);
    
    success = OCIO::StringToInt(&ival, " 11x ", false);
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(ival, 11);
    
    success = OCIO::StringToInt(&ival, " 12x ", true);
    OIIO_CHECK_EQUAL(success, false);
    
    success = OCIO::StringToInt(&ival, "13", true);
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(ival, 13);
    
    success = OCIO::StringToInt(&ival, "-14", true);
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(ival, -14);
    
    success = OCIO::StringToInt(&ival, "x-15", false);
    OIIO_CHECK_EQUAL(success, false);
    
    success = OCIO::StringToInt(&ival, "x-16", false);
    OIIO_CHECK_EQUAL(success, false);
}

OIIO_ADD_TEST(ParseUtils, StringToFloat)
{
    float fval = 0;
    bool success = false;

    success = OCIO::StringToFloat(&fval, "");
    OIIO_CHECK_EQUAL(success, false);

    success = OCIO::StringToFloat(&fval, "1.0");
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval, "1");
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval, "a1");
    OIIO_CHECK_EQUAL(success, false);

    success = OCIO::StringToFloat(&fval,
        "1 do we really want this to succeed?");
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval, "1Success");
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval,
        "1.0000000000000000000000000000000000000000000001");
    OIIO_CHECK_EQUAL(success, true);
    OIIO_CHECK_EQUAL(fval, 1.0f);
}

OIIO_ADD_TEST(ParseUtils, FloatDouble)
{
    std::string resStr;
    resStr = OCIO::FloatToString(0.0f);
    OIIO_CHECK_EQUAL("0", resStr);

    resStr = OCIO::FloatToString(0.1111001f);
    OIIO_CHECK_EQUAL("0.1111001", resStr);

    resStr = OCIO::FloatToString(0.11000001f);
    OIIO_CHECK_EQUAL("0.11", resStr);

    resStr = OCIO::DoubleToString(0.11000001);
    OIIO_CHECK_EQUAL("0.11000001", resStr);

    resStr = OCIO::DoubleToString(0.1100000000000001);
    OIIO_CHECK_EQUAL("0.1100000000000001", resStr);

    resStr = OCIO::DoubleToString(0.11000000000000001);
    OIIO_CHECK_EQUAL("0.11", resStr);
}

OIIO_ADD_TEST(ParseUtils, StringVecToIntVec)
{
    std::vector<int> intArray;
    std::vector<std::string> lineParts;
    bool success = OCIO::StringVecToIntVec(intArray, lineParts);
    OIIO_CHECK_EQUAL(true, success);
    OIIO_CHECK_EQUAL(0, intArray.size());

    lineParts.push_back("42");
    lineParts.push_back("");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OIIO_CHECK_EQUAL(false, success);
    OIIO_CHECK_EQUAL(2, intArray.size());

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42");
    lineParts.push_back("0");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OIIO_CHECK_EQUAL(true, success);
    OIIO_CHECK_EQUAL(2, intArray.size());
    OIIO_CHECK_EQUAL(42, intArray[0]);
    OIIO_CHECK_EQUAL(0, intArray[1]);

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42");
    lineParts.push_back("021");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OIIO_CHECK_EQUAL(true, success);
    OIIO_CHECK_EQUAL(2, intArray.size());
    OIIO_CHECK_EQUAL(42, intArray[0]);
    OIIO_CHECK_EQUAL(21, intArray[1]);

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42");
    lineParts.push_back("0x21");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OIIO_CHECK_EQUAL(false, success);
    OIIO_CHECK_EQUAL(2, intArray.size());

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42u");
    lineParts.push_back("21");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OIIO_CHECK_EQUAL(false, success);
    OIIO_CHECK_EQUAL(2, intArray.size());
}

OIIO_ADD_TEST(ParseUtils, SplitStringEnvStyle)
{
    std::vector<std::string> outputvec;
    OCIO::SplitStringEnvStyle(outputvec, "This:is:a:test");
    OIIO_CHECK_EQUAL(4, outputvec.size());
    OIIO_CHECK_EQUAL("This", outputvec[0]);
    OIIO_CHECK_EQUAL("is", outputvec[1]);
    OIIO_CHECK_EQUAL("a", outputvec[2]);
    OIIO_CHECK_EQUAL("test", outputvec[3]);
    outputvec.clear();
    OCIO::SplitStringEnvStyle(outputvec, "   This  : is   :   a:   test  ");
    OIIO_CHECK_EQUAL(4, outputvec.size());
    OIIO_CHECK_EQUAL("This", outputvec[0]);
    OIIO_CHECK_EQUAL("is", outputvec[1]);
    OIIO_CHECK_EQUAL("a", outputvec[2]);
    OIIO_CHECK_EQUAL("test", outputvec[3]);
    outputvec.clear();
    OCIO::SplitStringEnvStyle(outputvec, "   This  , is   ,   a,   test  ");
    OIIO_CHECK_EQUAL(4, outputvec.size());
    OIIO_CHECK_EQUAL("This", outputvec[0]);
    OIIO_CHECK_EQUAL("is", outputvec[1]);
    OIIO_CHECK_EQUAL("a", outputvec[2]);
    OIIO_CHECK_EQUAL("test", outputvec[3]);
    outputvec.clear();
    OCIO::SplitStringEnvStyle(outputvec, "This:is   ,   a:test  ");
    OIIO_CHECK_EQUAL(2, outputvec.size());
    OIIO_CHECK_EQUAL("This:is", outputvec[0]);
    OIIO_CHECK_EQUAL("a:test", outputvec[1]);
    outputvec.clear();
    OCIO::SplitStringEnvStyle(outputvec, ",,");
    OIIO_CHECK_EQUAL(3, outputvec.size());
    OIIO_CHECK_EQUAL("", outputvec[0]);
    OIIO_CHECK_EQUAL("", outputvec[1]);
    OIIO_CHECK_EQUAL("", outputvec[2]);
}

OIIO_ADD_TEST(ParseUtils, IntersectStringVecsCaseIgnore)
{
    std::vector<std::string> source1;
    std::vector<std::string> source2;
    source1.push_back("111");
    source1.push_back("This");
    source1.push_back("is");
    source1.push_back("222");
    source1.push_back("a");
    source1.push_back("test");

    source2.push_back("333");
    source2.push_back("TesT");
    source2.push_back("this");
    source2.push_back("444");
    source2.push_back("a");
    source2.push_back("IS");

    std::vector<std::string> resInter = OCIO::IntersectStringVecsCaseIgnore(source1, source2);
    OIIO_CHECK_EQUAL(4, resInter.size());
    OIIO_CHECK_EQUAL("This", resInter[0]);
    OIIO_CHECK_EQUAL("is", resInter[1]);
    OIIO_CHECK_EQUAL("a", resInter[2]);
    OIIO_CHECK_EQUAL("test", resInter[3]);
}

#endif // OCIO_UNIT_TEST
