// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

/*

    Houdini LUTs
    http://www.sidefx.com/docs/hdk11.0/hdk_io_lut.html

    Types:
      - 1D LUT (partial support)
      - 3D LUT
      - 3D LUT with 1D Prelut

      TODO:
        - Add support for other 1D types (R, G, B, A, RGB, RGBA, All)
          we only support type 'C' atm.
        - Add support for 'Sampling' tag

*/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FileFormatUtils.h"
#include "MathUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/matrix/MatrixOp.h"
#include "ParseUtils.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
namespace
{
// HDL parser helpers

// HDL headers/LUT's are shoved into these datatypes
typedef std::map<std::string, StringUtils::StringVec > StringToStringVecMap;
typedef std::map<std::string, std::vector<float> > StringToFloatVecMap;

void
readHeaders(StringToStringVecMap& headers,
            std::istream& istream)
{
    std::string line;
    while(nextline(istream, line))
    {
        // Remove trailing/leading whitespace, lower-case and
        // split into words
        StringUtils::StringVec chunks 
            = StringUtils::SplitByWhiteSpaces(StringUtils::Lower(StringUtils::Trim(line)));

        // Skip empty lines
        if(chunks.empty()) continue;

        // Stop looking for headers at the "LUT:" line
        if(chunks[0] == "lut:") break;

        // Use first index as key, and remove it from the value
        const std::string key = chunks[0];
        chunks.erase(chunks.begin());

        headers[key] = chunks;
    }
}

// Try to grab key (e.g "version") from headers. Throws
// exception if not found, or if number of chunks in value is
// not between min_vals and max_vals (e.g the "length" key
// must exist, and must have either 1 or 2 values)
StringUtils::StringVec
findHeaderItem(StringToStringVecMap& headers,
                const std::string key,
                const unsigned int min_vals,
                const unsigned int max_vals)
{
    StringToStringVecMap::iterator iter;
    iter = headers.find(key);

    // Error if key is not found
    if(iter == headers.end())
    {
        std::ostringstream os;
        os << "'" << key << "' line not found";
        throw Exception(os.str().c_str());
    }

    // Error if incorrect number of values is found
    if(iter->second.size() < min_vals ||
        iter->second.size() > max_vals)
    {
        std::ostringstream os;
        os << "Incorrect number of chunks (" << iter->second.size() << ")";
        os << " after '" << key << "' line, expected ";

        if(min_vals == max_vals)
        {
            os << min_vals;
        }
        else
        {
            os << "between " << min_vals << " and " << max_vals;
        }

        throw Exception(os.str().c_str());
    }

    return iter->second;
}

// Simple wrapper to call findHeaderItem with a fixed number
// of values (e.g "version" should have a single value)
StringUtils::StringVec
findHeaderItem(StringToStringVecMap& chunks,
                const std::string key,
                const unsigned int numvals)
{
    return findHeaderItem(chunks, key, numvals, numvals);
}

// Crudely parse LUT's - doesn't do any length checking etc,
// just grabs a series of floats for Pre{...}, 3d{...} etc
// Does some basic error checking, but there are situations
// were it could incorrectly accept broken data (like
// "Pre{0.0\n1.0}blah"), but hopefully none where it misses
// data
void
readLuts(std::istream& istream,
            StringToFloatVecMap& lutValues)
{
    // State variables
    bool inlut = false;
    std::string lutname;

    std::string word;

    while(istream >> word)
    {
        if(!inlut)
        {
            if(word == "{")
            {
                // Lone "{" is for a 3D
                inlut = true;
                lutname = "3d";
            }
            else
            {
                // Named LUT, e.g "Pre {"
                inlut = true;
                lutname = StringUtils::Lower(word);

                // Ensure next word is "{"
                std::string nextword;
                istream >> nextword;
                if(nextword != "{")
                {
                    std::ostringstream os;
                    os << "Malformed LUT - Unknown word '";
                    os << word << "' after LUT name '";
                    os << nextword << "'";
                    throw Exception(os.str().c_str());
                }
            }
        }
        else if(word == "}")
        {
            // end of LUT
            inlut = false;
            lutname = "";
        }
        else if(inlut)
        {
            // StringToFloat was far slower, for 787456 values:
            // - StringToFloat took 3879 (avg nanoseconds per value)
            // - stdtod took 169 nanoseconds
            char* endptr = 0;
            float v = static_cast<float>(strtod(word.c_str(), &endptr));

            if(!*endptr)
            {
                // Since each word should contain a single
                // float value, the pointer should be null
                lutValues[lutname].push_back(v);
            }
            else
            {
                // stdtod endptr still contained stuff,
                // meaning an invalid float value
                std::ostringstream os;
                os << "Invalid float value in " << lutname;
                os << " LUT, '" << word << "'";
                throw Exception(os.str().c_str());
            }
        }
        else
        {
            std::ostringstream os;
            os << "Unexpected word, possibly a value outside";
            os <<" a LUT {} block. Word was '" << word << "'";
            throw Exception(os.str().c_str());

        }
    }
}

} // end anonymous "HDL parser helpers" namespace

namespace
{
class CachedFileHDL : public CachedFile
{
public:
    CachedFileHDL ()
        : hdlversion("unknown")
        , hdlformat("unknown")
        , hdltype("unknown")
    {
    }
    ~CachedFileHDL() = default;

    void setLUT1D(const std::vector<float> & values, Interpolation interp)
    {
        auto lutSize = static_cast<unsigned long>(values.size());
        lut1D = std::make_shared<Lut1DOpData>(lutSize);
        if (Lut1DOpData::IsValidInterpolation(interp))
        {
            lut1D->setInterpolation(interp);
        }
        lut1D->setFileOutputBitDepth(BIT_DEPTH_F32);

        auto & lutArray = lut1D->getArray();
        for (unsigned long i = 0; i < lutSize; ++i)
        {
            lutArray[3 * i + 0] = values[i];
            lutArray[3 * i + 1] = values[i];
            lutArray[3 * i + 2] = values[i];
        }
    }

    std::string hdlversion;
    std::string hdlformat;
    std::string hdltype;
    float from_min = 0.0f;
    float from_max = 1.0f;
    float to_min = 0.0f;
    float to_max = 1.0f;
    float hdlblack = 0.0f;
    float hdlwhite = 1.0f;

    Lut1DOpDataRcPtr lut1D;
    Lut3DOpDataRcPtr lut3D;
};
typedef OCIO_SHARED_PTR<CachedFileHDL> CachedFileHDLRcPtr;

class LocalFileFormat : public FileFormat
{
public:
    LocalFileFormat() = default;
    ~LocalFileFormat() = default;

    void getFormatInfo(FormatInfoVec & formatInfoVec) const override;

    CachedFileRcPtr read(std::istream & istream,
                         const std::string & fileName,
                         Interpolation interp) const override;

    void bake(const Baker & baker,
                const std::string & formatName,
                std::ostream & ostream) const override;

    void buildFileOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        CachedFileRcPtr untypedCachedFile,
                        const FileTransform & fileTransform,
                        TransformDirection dir) const override;
};

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "houdini";
    info.extension = "lut";
    info.capabilities = FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_BAKE;
    formatInfoVec.push_back(info);
}

CachedFileRcPtr
LocalFileFormat::read(std::istream & istream,
                      const std::string & /* fileName unused */,
                      Interpolation interp) const
{

    // this shouldn't happen
    if (!istream)
        throw Exception ("file stream empty when trying to read Houdini LUT");

    //
    CachedFileHDLRcPtr cachedFile = CachedFileHDLRcPtr (new CachedFileHDL ());

    Lut3DOpDataRcPtr lut3d_ptr;

    // Parse headers into key-value pairs
    StringToStringVecMap header_chunks;
    StringToStringVecMap::iterator iter;

    // Read headers, ending after the "LUT:" line
    readHeaders(header_chunks, istream);

    // Grab useful values from headers
    StringUtils::StringVec value;

    // "Version 3" - format version (currently one version
    // number per LUT type)
    value = findHeaderItem(header_chunks, "version", 1);
    cachedFile->hdlversion = value[0];

    // "Format any" - bit depth of image the LUT should be
    // applied to (this is basically ignored)
    value = findHeaderItem(header_chunks, "format", 1);
    cachedFile->hdlformat = value[0];

    // "Type 3d" - type of LUT
    {
        value = findHeaderItem(header_chunks, "type", 1);

        cachedFile->hdltype = value[0];
    }

    // "From 0.0 1.0" - range of input values
    {
        float from_min = 0.0f;
        float from_max = 1.0f;
        value = findHeaderItem(header_chunks, "from", 2);

        if(!StringToFloat(&from_min, value[0].c_str()) ||
            !StringToFloat(&from_max, value[1].c_str()))
        {
            std::ostringstream os;
            os << "Invalid float value(s) on 'From' line, '";
            os << value[0] << "' and '"  << value[1] << "'";
            throw Exception(os.str().c_str());
        }
        cachedFile->from_min = from_min;
        cachedFile->from_max = from_max;
    }


    // "To 0.0 1.0" - range of values in LUT (e.g "0 255"
    // to specify values as 8-bit numbers, usually "0 1")
    {
        float to_min, to_max;

        value = findHeaderItem(header_chunks, "to", 2);

        if(!StringToFloat(&to_min, value[0].c_str()) ||
            !StringToFloat(&to_max, value[1].c_str()))
        {
            std::ostringstream os;
            os << "Invalid float value(s) on 'To' line, '";
            os << value[0] << "' and '"  << value[1] << "'";
            throw Exception(os.str().c_str());
        }
        cachedFile->to_min = to_min;
        cachedFile->to_max = to_max;
    }

    // "Black 0" and "White 1" - obsolete options, should be 0
    // and 1

    {
        value = findHeaderItem(header_chunks, "black", 1);

        float black;

        if(!StringToFloat(&black, value[0].c_str()))
        {
            std::ostringstream os;
            os << "Invalid float value on 'Black' line, '";
            os << value[0] << "'";
            throw Exception(os.str().c_str());
        }
        cachedFile->hdlblack = black;
    }

    {
        value = findHeaderItem(header_chunks, "white", 1);

        float white;

        if(!StringToFloat(&white, value[0].c_str()))
        {
            std::ostringstream os;
            os << "Invalid float value on 'White' line, '";
            os << value[0] << "'";
            throw Exception(os.str().c_str());
        }
        cachedFile->hdlwhite = white;
    }


    // Verify type is valid and supported - used to handle
    // length sensibly, and checking the LUT later
    {
        std::string ltype = cachedFile->hdltype;
        if(ltype != "3d" && ltype != "3d+1d" && ltype != "c")
        {
            std::ostringstream os;
            os << "Unsupported Houdini LUT type: '" << ltype << "'";
            throw Exception(os.str().c_str());
        }
    }


    // "Length 2" or "Length 2 5" - either "[cube size]", or "[cube
    // size] [prelut size]"
    int size_3d = -1;
    int size_prelut = -1;
    int size_1d = -1;

    {
        std::vector<int> lut_sizes;

        value = findHeaderItem(header_chunks, "length", 1, 2);
        for(unsigned int i = 0; i < value.size(); ++i)
        {
            int tmpsize = -1;
            if(!StringToInt(&tmpsize, value[i].c_str()))
            {
                std::ostringstream os;
                os << "Invalid integer on 'Length' line: ";
                os << "'" << value[0] << "'";
                throw Exception(os.str().c_str());
            }
            lut_sizes.push_back(tmpsize);
        }

        if(cachedFile->hdltype == "3d" || cachedFile->hdltype == "3d+1d")
        {
            // Set cube size
            size_3d = lut_sizes[0];

            lut3d_ptr = std::make_shared<Lut3DOpData>(lut_sizes[0]);
            if (Lut3DOpData::IsValidInterpolation(interp))
            {
                lut3d_ptr->setInterpolation(interp);
            }
            lut3d_ptr->setFileOutputBitDepth(BIT_DEPTH_F32);
        }

        if(cachedFile->hdltype == "c")
        {
            size_1d = lut_sizes[0];
        }

        if(cachedFile->hdltype == "3d+1d")
        {
            size_prelut = lut_sizes[1];
        }
    }

    // Read stuff after "LUT:"
    StringToFloatVecMap lut_data;
    readLuts(istream, lut_data);

    //
    StringToFloatVecMap::iterator lut_iter;

    if(cachedFile->hdltype == "3d+1d")
    {
        // Read prelut, and bind onto cachedFile
        lut_iter = lut_data.find("pre");
        if(lut_iter == lut_data.end())
        {
            std::ostringstream os;
            os << "3D+1D LUT should contain Pre{} LUT section";
            throw Exception(os.str().c_str());
        }

        if(size_prelut != static_cast<int>(lut_iter->second.size()))
        {
            std::ostringstream os;
            os << "Pre{} LUT was " << lut_iter->second.size();
            os << " values long, expected " << size_prelut << " values";
            throw Exception(os.str().c_str());
        }

        cachedFile->setLUT1D(lut_iter->second, interp);
    }

    if(cachedFile->hdltype == "3d" ||
        cachedFile->hdltype == "3d+1d")
    {
        // Bind 3D LUT to lut3d_ptr, along with some
        // slightly-elabourate error messages

        lut_iter = lut_data.find("3d");
        if(lut_iter == lut_data.end())
        {
            std::ostringstream os;
            os << "3D LUT section not found";
            throw Exception(os.str().c_str());
        }

        int size_3d_cubed = size_3d * size_3d * size_3d;

        if(size_3d_cubed * 3 != static_cast<int>(lut_iter->second.size()))
        {
            int foundsize = static_cast<int>(lut_iter->second.size());
            int foundlines = foundsize / 3;

            std::ostringstream os;
            os << "3D LUT contains incorrect number of values. ";
            os << "Contained " << foundsize << " values ";
            os << "(" << foundlines << " lines), ";
            os << "expected " << (size_3d_cubed*3) << " values ";
            os << "(" << size_3d_cubed << " lines)";
            throw Exception(os.str().c_str());
        }

        lut3d_ptr->setArrayFromRedFastestOrder(lut_iter->second);

        // Bind to cachedFile
        cachedFile->lut3D = lut3d_ptr;
    }

    if(cachedFile->hdltype == "c")
    {
        // Bind simple 1D RGB LUT
        lut_iter = lut_data.find("rgb");
        if(lut_iter == lut_data.end())
        {
            std::ostringstream os;
            os << "3D+1D LUT should contain Pre{} LUT section";
            throw Exception(os.str().c_str());
        }

        if(size_1d != static_cast<int>(lut_iter->second.size()))
        {
            std::ostringstream os;
            os << "RGB{} LUT was " << lut_iter->second.size();
            os << " values long, expected " << size_1d << " values";
            throw Exception(os.str().c_str());
        }

        cachedFile->setLUT1D(lut_iter->second, interp);
    }

    return cachedFile;
}

void LocalFileFormat::bake(const Baker & baker,
                            const std::string & formatName,
                            std::ostream & ostream) const
{

    if(formatName != "houdini")
    {
        std::ostringstream os;
        os << "Unknown hdl format name, '";
        os << formatName << "'.";
        throw Exception(os.str().c_str());
    }

    // Get config
    ConstConfigRcPtr config = baker.getConfig();

    // setup the floating point precision
    ostream.setf(std::ios::fixed, std::ios::floatfield);
    ostream.precision(6);

    // Default sizes
    const int DEFAULT_SHAPER_SIZE = 1024;
    // MPlay produces bad results with 32^3 cube (in a way
    // that looks more quantised than even "nearest"
    // interpolation in OCIOFileTransform)
    const int DEFAULT_CUBE_SIZE = 64;
    const int DEFAULT_1D_SIZE = 1024;

    // Get configured sizes
    int cubeSize = baker.getCubeSize();
    int shaperSize = baker.getShaperSize();
    // FIXME: Misusing cube size to set 1D LUT size, as it seemed
    // slightly less confusing than using the shaper LUT size
    int onedSize = baker.getCubeSize();

    // Check defaults and cube size
    if(cubeSize == -1) cubeSize = DEFAULT_CUBE_SIZE;
    if(cubeSize < 0) cubeSize = DEFAULT_CUBE_SIZE;
    if(cubeSize<2)
    {
        std::ostringstream os;
        os << "Cube size must be 2 or larger (was " << cubeSize << ")";
        throw Exception(os.str().c_str());
    }

    // ..and same for shaper size
    if(shaperSize<0) shaperSize = DEFAULT_SHAPER_SIZE;
    if(shaperSize<2)
    {
        std::ostringstream os;
        os << "A shaper space ('" << baker.getShaperSpace() << "') has";
        os << " been specified, so the shaper size must be 2 or larger";
        throw Exception(os.str().c_str());
    }

    // ..and finally, for the 1D LUT size
    if(onedSize == -1) onedSize = DEFAULT_1D_SIZE;
    if(onedSize < 2)
    {
        std::ostringstream os;
        os << "1D LUT size must be higher than 2 (was " << onedSize << ")";
        throw Exception(os.str().c_str());
    }

    // Version numbers
    const int HDL_1D = 1; // 1D LUT version number
    const int HDL_3D = 2; // 3D LUT version number
    const int HDL_3D1D = 3; // 3D LUT with 1D prelut

    // Get spaces from baker
    const std::string shaperSpace = baker.getShaperSpace();
    const std::string inputSpace = baker.getInputSpace();
    const std::string targetSpace = baker.getTargetSpace();
    const std::string looks = baker.getLooks();

    // Determine required LUT type
    ConstProcessorRcPtr inputToTargetProc;
    if (!looks.empty())
    {
        LookTransformRcPtr transform = LookTransform::Create();
        transform->setLooks(looks.c_str());
        transform->setSrc(inputSpace.c_str());
        transform->setDst(targetSpace.c_str());
        inputToTargetProc = config->getProcessor(transform, TRANSFORM_DIR_FORWARD);
    }
    else
    {
        inputToTargetProc = config->getProcessor(
            inputSpace.c_str(),
            targetSpace.c_str());
    }

    int required_lut = -1;

    if(inputToTargetProc->hasChannelCrosstalk())
    {
        if(shaperSpace.empty())
        {
            // Has crosstalk, but no prelut, so need 3D LUT
            required_lut = HDL_3D;
        }
        else
        {
            // Crosstalk with shaper-space
            required_lut = HDL_3D1D;
        }
    }
    else
    {
        // No crosstalk
        required_lut = HDL_1D;
    }

    if(required_lut == -1)
    {
        // Unnecessary paranoia
        throw Exception(
            "Internal logic error, LUT type was not determined");
    }

    // Make prelut
    std::vector<float> prelutData;

    float fromInStart = 0; // for "From:" part of header
    float fromInEnd = 1;

    if(required_lut == HDL_3D1D)
    {
        // TODO: Later we only grab the green channel for the prelut,
        // should ensure the prelut is monochromatic somehow?

        ConstProcessorRcPtr inputToShaperProc = config->getProcessor(
            inputSpace.c_str(),
            shaperSpace.c_str());

        if(inputToShaperProc->hasChannelCrosstalk())
        {
            // TODO: Automatically turn shaper into
            // non-crosstalked version?
            std::ostringstream os;
            os << "The specified shaperSpace, '" << baker.getShaperSpace();
            os << "' has channel crosstalk, which is not appropriate for";
            os << " shapers. Please select an alternate shaper space or";
            os << " omit this option.";
            throw Exception(os.str().c_str());
        }

        // Calculate min/max value
        {
            // Get input value of 1.0 in shaper space, as this
            // is the highest value that is transformed by the
            // cube (e.g for a generic lin-to-log transform,
            // what the log value 1.0 is in linear).
            ConstCPUProcessorRcPtr shaperToInputProc = config->getProcessor(
                shaperSpace.c_str(),
                inputSpace.c_str())->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);

            float minval[3] = {0.0f, 0.0f, 0.0f};
            float maxval[3] = {1.0f, 1.0f, 1.0f};

            shaperToInputProc->applyRGB(minval);
            shaperToInputProc->applyRGB(maxval);

            // Grab green channel, as this is the one used later
            fromInStart = minval[1];
            fromInEnd = maxval[1];
        }

        // Generate the identity prelut values, then apply the transform.
        // Prelut is linearly sampled from fromInStart to fromInEnd
        prelutData.resize(shaperSize*3);

        for (int i = 0; i < shaperSize; ++i)
        {
            const float x = (float)(double(i) / double(shaperSize - 1));
            float cur_value = lerpf(fromInStart, fromInEnd, x);

            prelutData[3*i+0] = cur_value;
            prelutData[3*i+1] = cur_value;
            prelutData[3*i+2] = cur_value;
        }

        PackedImageDesc prelutImg(&prelutData[0], shaperSize, 1, 3);

        ConstCPUProcessorRcPtr cpu = inputToShaperProc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
        cpu->apply(prelutImg);
    }

    // TODO: Do same "auto prelut" input-space allocation as FileFormatCSP?

    // Make 3D LUT
    std::vector<float> cubeData;
    if(required_lut == HDL_3D || required_lut == HDL_3D1D)
    {
        cubeData.resize(cubeSize*cubeSize*cubeSize*3);

        GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
        PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);

        ConstProcessorRcPtr cubeProc;
        if(required_lut == HDL_3D1D)
        {
            // Prelut goes from input-to-shaper, so cube goes from shaper-to-target
            if (!looks.empty())
            {
                LookTransformRcPtr transform = LookTransform::Create();
                transform->setLooks(looks.c_str());
                transform->setSrc(shaperSpace.c_str());
                transform->setDst(targetSpace.c_str());
                cubeProc = config->getProcessor(transform, TRANSFORM_DIR_FORWARD);
            }
            else
            {
                cubeProc = config->getProcessor(shaperSpace.c_str(), targetSpace.c_str());
            }
        }
        else
        {
            // No prelut, so cube goes from input-to-target
            cubeProc = inputToTargetProc;
        }

        ConstCPUProcessorRcPtr cpu = cubeProc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
        cpu->apply(cubeImg);
    }


    // Make 1D LUT
    std::vector<float> onedData;
    if(required_lut == HDL_1D)
    {
        onedData.resize(onedSize * 3);

        GenerateIdentityLut1D(&onedData[0], onedSize, 3);
        PackedImageDesc onedImg(&onedData[0], onedSize, 1, 3);
        ConstCPUProcessorRcPtr cpu = inputToTargetProc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
        cpu->apply(onedImg);
    }


    // Write the file contents
    ostream << "Version\t\t" << required_lut << "\n";
    ostream << "Format\t\t" << "any" << "\n";

    ostream << "Type\t\t";
    if(required_lut == HDL_1D)
        ostream << "RGB";
    if(required_lut == HDL_3D)
        ostream << "3D";
    if(required_lut == HDL_3D1D)
        ostream << "3D+1D";
    ostream << "\n";

    ostream << "From\t\t" << fromInStart << " " << fromInEnd << "\n";
    ostream << "To\t\t" << 0.0f << " " << 1.0f << "\n";
    ostream << "Black\t\t" << 0.0f << "\n";
    ostream << "White\t\t" << 1.0f << "\n";

    if(required_lut == HDL_3D1D)
        ostream << "Length\t\t" << cubeSize << " " << shaperSize << "\n";
    if(required_lut == HDL_3D)
        ostream << "Length\t\t" << cubeSize << "\n";
    if(required_lut == HDL_1D)
        ostream << "Length\t\t" << onedSize << "\n";

    ostream << "LUT:\n";

    // Write prelut
    if(required_lut == HDL_3D1D)
    {
        ostream << "Pre {\n";
        for(int i=0; i < shaperSize; ++i)
        {
            // Grab green channel from RGB prelut
            ostream << "\t" << prelutData[i*3+1] << "\n";
        }
        ostream << "}\n";
    }

    // Write "3D {" part of output of 3D+1D LUT
    if(required_lut == HDL_3D1D)
    {
        ostream << "3D {\n";
    }

    // Write the slightly-different "{" without line for the 3D-only LUT
    if(required_lut == HDL_3D)
    {
        ostream << " {\n";
    }

    // Write the cube data after the "{"
    if(required_lut == HDL_3D || required_lut == HDL_3D1D)
    {
        for(int i=0; i < cubeSize*cubeSize*cubeSize; ++i)
        {
            // TODO: Original baker code clamped values to
            // 1.0, was this necessary/desirable?

            ostream << "\t" << cubeData[3*i+0];
            ostream << " "  << cubeData[3*i+1];
            ostream << " "  << cubeData[3*i+2] << "\n";
        }

        // Write closing "}"
        ostream << " }\n";
    }

    // Write out channels for 1D LUT
    if(required_lut == HDL_1D)
    {
        ostream << "R {\n";
        for(int i=0; i < onedSize; ++i)
            ostream << "\t" << onedData[i*3+0] << "\n";
        ostream << "}\n";

        ostream << "G {\n";
        for(int i=0; i < onedSize; ++i)
            ostream << "\t" << onedData[i*3+1] << "\n";
        ostream << "}\n";

        ostream << "B {\n";
        for(int i=0; i < onedSize; ++i)
            ostream << "\t" << onedData[i*3+2] << "\n";
        ostream << "}\n";
    }
}

void
LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                            const Config & /*config*/,
                            const ConstContextRcPtr & /*context*/,
                            CachedFileRcPtr untypedCachedFile,
                            const FileTransform & fileTransform,
                            TransformDirection dir) const
{

    CachedFileHDLRcPtr cachedFile = DynamicPtrCast<CachedFileHDL>(untypedCachedFile);

    // This should never happen.
    if(!cachedFile || (!cachedFile->lut1D && !cachedFile->lut3D))
    {
        std::ostringstream os;
        os << "Cannot build Houdini Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    const auto fileInterp = fileTransform.getInterpolation();

    bool fileInterpUsed = false;
    auto lut1D = HandleLUT1D(cachedFile->lut1D, fileInterp, fileInterpUsed);
    auto lut3D = HandleLUT3D(cachedFile->lut3D, fileInterp, fileInterpUsed);

    if (!fileInterpUsed)
    {
        LogWarningInterpolationNotUsed(fileInterp, fileTransform);
    }

    switch (newDir)
    {
    case TRANSFORM_DIR_FORWARD:
    {
        if (cachedFile->hdltype == "c")
        {
            CreateMinMaxOp(ops, cachedFile->from_min, cachedFile->from_max, newDir);
            CreateLut1DOp(ops, lut1D, newDir);
        }
        else if (cachedFile->hdltype == "3d")
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }
        else if (cachedFile->hdltype == "3d+1d")
        {
            CreateMinMaxOp(ops, cachedFile->from_min, cachedFile->from_max, newDir);
            CreateLut1DOp(ops, lut1D, newDir);
            CreateLut3DOp(ops, lut3D, newDir);
        }
        else
        {
            throw Exception("Unhandled hdltype while creating forward ops");
        }
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        if (cachedFile->hdltype == "c")
        {
            CreateLut1DOp(ops, lut1D, newDir);
            CreateMinMaxOp(ops, cachedFile->from_min, cachedFile->from_max, newDir);
        }
        else if (cachedFile->hdltype == "3d")
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }
        else if (cachedFile->hdltype == "3d+1d")
        {
            CreateLut3DOp(ops, lut3D, newDir);
            CreateLut1DOp(ops, lut1D, newDir);
            CreateMinMaxOp(ops, cachedFile->from_min, cachedFile->from_max, newDir);
        }
        else
        {
            throw Exception("Unhandled hdltype while creating reverse ops");
        }
        break;
    }
    }
}
}

FileFormat * CreateFileFormatHDL()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE

