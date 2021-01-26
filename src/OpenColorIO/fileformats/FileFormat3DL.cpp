// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "fileformats/FileFormatUtils.h"
#include "MathUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ParseUtils.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"


// Discreet's Flame LUT Format
// Use a loose interpretation of the format to allow other 3D LUTs that look
// similar, but dont strictly adhere to the real definition.

// If line starts with text or # skip it
// If line is a bunch of ints (more than 3) , it's the 1D shaper LUT

// All remaining lines of size 3 int are data
// cube size is determined from num entries
// The bit depth of the shaper LUT and the 3D LUT need not be the same.
/*

Example 1, FLAME
# Comment here
0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023

0 0 0
0 0 100
0 0 200


Example 2, LUSTRE
#Tokens required by applications - do not edit
3DMESH
Mesh 4 12
0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023



0 17 17
0 0 88
0 0 157
9 101 197
0 118 308
...

4092 4094 4094

#Tokens required by applications - do not edit

LUT8
gamma 1.0

In this example, the 3D LUT has an input bit depth of 4 bits and an output 
bit depth of 12 bits. You use the input value to calculate the RGB triplet 
to be 17*17*17 (where 17=(2 to the power of 4)+1, and 4 is the input bit 
depth). The first triplet is the output value at (0,0,0);(0,0,1);...;
(0,0,16) r,g,b coordinates; the second triplet is the output value at 
(0,1,0);(0,1,1);...;(0,1,16) r,g,b coordinates; and so on. You use the output 
bit depth to set the output bit depth range (12 bits or 0-4095).
*/


namespace OCIO_NAMESPACE
{
////////////////////////////////////////////////////////////////

namespace
{
class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile() = default;
    ~LocalCachedFile() = default;

    Lut1DOpDataRcPtr lut1D;
    Lut3DOpDataRcPtr lut3D;
};

typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;



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






// We use the maximum value found in the LUT to infer
// the bit depth.  While this is fugly. We dont believe
// there is a better way, looking at the file, to
// determine this.
//
// Note:  We allow for 2x overshoot in the LUTs.
// As we dont allow for odd bit depths, this isn't a big deal.
// So sizes from 1/2 max - 2x max are valid 
//
// FILE      EXPECTED MAX    CORRECTLY DECODED IF MAX IN THIS RANGE 
// 8-bit     255             [0, 511]      
// 10-bit    1023            [512, 2047]
// 12-bit    4095            [2048, 8191]
// 14-bit    16383           [8192, 32767]
// 16-bit    65535           [32768, 131071+]

int GetLikelyLutBitDepth(int testval)
{
    const int MIN_BIT_DEPTH = 8;
    const int MAX_BIT_DEPTH = 16;

    if(testval < 0) return -1;

    // Only test even bit depths
    for(int bitDepth = MIN_BIT_DEPTH;
        bitDepth <= MAX_BIT_DEPTH; bitDepth+=2)
    {
        int maxcode = static_cast<int>(pow(2.0,bitDepth));
        int adjustedMax = maxcode * 2 - 1;
        if (testval <= adjustedMax)
        {
            // Since 14-bit scaling is not used in practice, if the 
            // max is more than 8192, they are likely 16-bit values.
            if (bitDepth == 14) bitDepth = 16;

            return bitDepth;
        }
    }

    return MAX_BIT_DEPTH;
}

BitDepth GetOCIOBitdepth(int bitdepth)
{
    switch (bitdepth)
    {
    case 8:
        return BIT_DEPTH_UINT8;
    case 10:
        return BIT_DEPTH_UINT10;
    case 12:
        return BIT_DEPTH_UINT12;
    case 16:
        return BIT_DEPTH_UINT16;
    case 14:
    default:
        return BIT_DEPTH_UNKNOWN;
    }
}

int GetMaxValueFromIntegerBitDepth(int bitDepth)
{
    return static_cast<int>( pow(2.0, bitDepth) ) - 1;
}

int GetClampedIntFromNormFloat(float val, float scale)
{
    val = std::min(std::max(0.0f, val), 1.0f) * scale;
    return static_cast<int>(roundf(val));
}

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "flame";
    info.extension = "3dl";
    info.capabilities = (FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_BAKE);
    formatInfoVec.push_back(info);

    FormatInfo info2 = info;
    info2.name = "lustre";
    formatInfoVec.push_back(info2);
}

// The shaper LUT part of the format was never properly documented
// (it is believed to have been introduced in the Kodak version of the
// format but was not used in the Discreet products).  Unfortunately,
// usage in the industry is quite inconsistent and we need to use a
// looser tolerance for what constitutes an identity than we would want
// for most LUTs.  That is why we are not trying to use the Lut1DOp
// isIdentity method here.
bool IsIdentity(const std::vector<int> & rawshaper, BitDepth outBitDepth)
{
    unsigned int dim = (unsigned int)rawshaper.size();
    const float stepValue = (float)GetBitDepthMaxValue(outBitDepth)
        / ((float)dim - 1.0f);

    for (unsigned int i = 0; i < dim; ++i)
    {
        if (fabs((float)i * stepValue - (float)rawshaper[i]) >= 2.0f)
        {
            return false;
        }
    }
    return true;
}

// Try and load the format
// Raise an exception if it can't be loaded.

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & /* fileName unused */,
                                      Interpolation interp) const
{
    std::vector<int> rawshaper;
    std::vector<int> raw3d;

    int lut3dmax = 0;

    // Parse the file 3D LUT data to an int array.
    {
        const int MAX_LINE_SIZE = 4096;
        char lineBuffer[MAX_LINE_SIZE];

        StringUtils::StringVec lineParts;
        std::vector<int> tmpData;

        int lineNumber = 0;

        while(istream.good())
        {
            istream.getline(lineBuffer, MAX_LINE_SIZE);
            ++lineNumber;

            // Strip and split the line.
            lineParts = StringUtils::SplitByWhiteSpaces(StringUtils::Trim(lineBuffer));

            if(lineParts.empty()) continue;
            if (lineParts.size() > 0)
            {
                if (StringUtils::StartsWith(lineParts[0], "#"))
                {
                    continue;
                }
                if (StringUtils::StartsWith(lineParts[0], "<"))
                {
                    // Format error: reject files that could be
                    // formatted as xml.
                    std::ostringstream os;
                    os << "Error parsing .3dl file. ";
                    os << "Not expecting a line starting with \"<\".";
                    os << "Line (" << lineNumber << "): '";
                    os << lineBuffer << "'.";
                    throw Exception(os.str().c_str());
                }
            }

            // If we haven't found a list of ints, continue.
            if (!StringVecToIntVec(tmpData, lineParts))
            {
                // Some keywords are valid (3DMESH, mesh, gamma, LUT*)
                // but others could be format error.
                // To preserve v1 behavior, don't reject them.
                continue;
            }

            // If we've found more than 3 ints, and dont have
            // a shaper LUT yet, we've got it!
            if(tmpData.size()>3)
            {
                if (rawshaper.empty())
                {
                    for (unsigned int i = 0; i < tmpData.size(); ++i)
                    {
                        rawshaper.push_back(tmpData[i]);
                    }
                }
                else
                {
                    // Format error, more than 1 shaper LUT.
                    std::ostringstream os;
                    os << "Error parsing .3dl file. ";
                    os << "Appears to contain more than 1 shaper LUT.";
                    os << "Line (" << lineNumber << "): '";
                    os << lineBuffer << "'.";
                    throw Exception(os.str().c_str());
                }
            }
            // If we've found 3 ints, add it to our 3D LUT.
            else if(tmpData.size() == 3)
            {
                raw3d.push_back(tmpData[0]);
                raw3d.push_back(tmpData[1]);
                raw3d.push_back(tmpData[2]);
                // Find the maximum shaper LUT value to infer bit-depth.
                lut3dmax = std::max(lut3dmax, tmpData[0]);
                lut3dmax = std::max(lut3dmax, tmpData[1]);
                lut3dmax = std::max(lut3dmax, tmpData[2]);
            }
            else
            {
                // Format error, line with 1 or 2 int.
                std::ostringstream os;
                os << "Error parsing .3dl file. ";
                os << "Invalid line with less than 3 values.";
                os << "Line (" << lineNumber << "): '";
                os << lineBuffer << "'.";
                throw Exception(os.str().c_str());
            }
        }
    }

    if(raw3d.empty() && rawshaper.empty())
    {
        std::ostringstream os;
        os << "Error parsing .3dl file. ";
        os << "Does not appear to contain a valid shaper LUT or a 3D LUT.";
        throw Exception(os.str().c_str());
    }

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    // If all we're doing to parse the format is to read in sets of 3 numbers,
    // it's possible that other formats will accidentally be able to be read
    // mistakenly as .3dl files.  We can exclude a huge segment of these mis-reads
    // by screening for files that use float representations.  I.e., if the MAX
    // value of the LUT is a small number (such as <128.0) it's likely not an integer
    // format, and thus not a likely 3DL file.

    const int FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT = 128;

    BitDepth out1DBD = BIT_DEPTH_UNKNOWN;

    // Interpret the shaper LUT
    if(!rawshaper.empty())
    {
        // Find the maximum shaper LUT value to infer bit-depth.
        int shapermax = 0;
        for (auto i : rawshaper)
        {
            shapermax = std::max(shapermax, i);
        }

        if(shapermax<FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT)
        {
            std::ostringstream os;
            os << "Error parsing .3dl file. ";
            os << "The maximum shaper LUT value, " << shapermax;
            os << ", is unreasonably low. This LUT is probably not a .3dl ";
            os << "file, but instead a related format that shares a similar ";
            os << "structure.";

            throw Exception(os.str().c_str());
        }

        int shaperbitdepth = GetLikelyLutBitDepth(shapermax);
        if(shaperbitdepth<0)
        {
            std::ostringstream os;
            os << "Error parsing .3dl file. ";
            os << "The maximum shaper LUT value, " << shapermax;
            os << ", does not correspond to any likely bit depth. ";
            os << "Please confirm source file is valid.";
            throw Exception(os.str().c_str());
        }

        out1DBD = GetOCIOBitdepth(shaperbitdepth);

        if (out1DBD == BIT_DEPTH_UNKNOWN)
        {
            std::ostringstream os;
            os << "Error parsing .3dl file. ";
            os << "The shaper LUT bit depth is not known. ";
            os << "Please confirm source file is valid.";
            throw Exception(os.str().c_str());
        }

        if (!IsIdentity(rawshaper, out1DBD))
        {
            unsigned long length = (unsigned long)rawshaper.size();
            cachedFile->lut1D = std::make_shared<Lut1DOpData>(length);
            if (Lut1DOpData::IsValidInterpolation(interp))
            {
                cachedFile->lut1D->setInterpolation(interp);
            }
            cachedFile->lut1D->setFileOutputBitDepth(out1DBD);

            const float scale = (float)GetBitDepthMaxValue(out1DBD);
            Array & lutArray = cachedFile->lut1D->getArray();
            for (unsigned int i = 0, p = 0; i < rawshaper.size(); ++i)
            {
                for (int j = 0; j < 3; ++j, ++p)
                {
                    lutArray[p] = static_cast<float>(rawshaper[i]) / scale;
                }
            }
        }
    }



    // Interpret the parsed data.
    if(!raw3d.empty())
    {
        // lut3dmax has been stored while reading values.
        if (lut3dmax<FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT)
        {
            std::ostringstream os;
            os << "Error parsing .3dl file.";
            os << "The maximum 3D LUT value, " << lut3dmax;
            os << ", is unreasonably low. This LUT is probably not a .3dl ";
            os << "file, but instead a related format that shares a similar ";
            os << "structure.";

            throw Exception(os.str().c_str());
        }

        int lut3dbitdepth = GetLikelyLutBitDepth(lut3dmax);
        if (lut3dbitdepth<0)
        {
            std::ostringstream os;
            os << "Error parsing .3dl file.";
            os << "The maximum 3D LUT value, " << lut3dmax;
            os << ", does not correspond to any likely bit depth. ";
            os << "Please confirm source file is valid.";
            throw Exception(os.str().c_str());
        }

        // Interpret the int array as a 3D LUT
        int lutEdgeLen = Get3DLutEdgeLenFromNumPixels((int)raw3d.size() / 3);

        // The 3dl format stores the LUT entries in blue-fastest order,
        // which is the same order used by Lut3DOpData, so no
        // transposition of LUT entries is needed in this case.

        BitDepth out3DBD = GetOCIOBitdepth(lut3dbitdepth);

        cachedFile->lut3D = std::make_shared<Lut3DOpData>(lutEdgeLen);
        if (Lut3DOpData::IsValidInterpolation(interp))
        {
            cachedFile->lut3D->setInterpolation(interp);
        }
        cachedFile->lut3D->setFileOutputBitDepth(out3DBD);

        const float scale = (float)GetBitDepthMaxValue(out3DBD);
        Array & lutArray = cachedFile->lut3D->getArray();
        for (size_t i = 0; i < raw3d.size(); ++i)
        {
            lutArray[(unsigned long)i] = static_cast<float>(raw3d[i]) / scale;
        }
    }
    return cachedFile;
}

// 65 -> 6
// 33 -> 5
// 17 -> 4

int CubeDimensionLenToLustreBitDepth(int size)
{
    float logval = logf(static_cast<float>(size-1)) / logf(2.0);
    return static_cast<int>(logval);
}

void LocalFileFormat::bake(const Baker & baker,
                            const std::string & formatName,
                            std::ostream & ostream) const
{
    int DEFAULT_CUBE_SIZE = 0;
    int SHAPER_BIT_DEPTH = 10;
    int CUBE_BIT_DEPTH = 12;


    // NOTE: This code is very old, Lustre and Flame have long been able
    //       to support much larger cube sizes.  Furthermore there is no
    //       need to use the legacy 3dl format since CLF/CTF is supported.
    if(formatName == "lustre")
    {
        DEFAULT_CUBE_SIZE = 33;
    }
    else if(formatName == "flame")
    {
        DEFAULT_CUBE_SIZE = 17;
    }
    else
    {
        std::ostringstream os;
        os << "Unknown 3dl format name, '";
        os << formatName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstConfigRcPtr config = baker.getConfig();

    int cubeSize = baker.getCubeSize();
    if(cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
    cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2

    int shaperSize = baker.getShaperSize();
    if(shaperSize==-1) shaperSize = cubeSize;

    std::vector<float> cubeData;
    cubeData.resize(cubeSize*cubeSize*cubeSize*3);
    GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_BLUE);
    PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);

    // Apply our conversion from the input space to the output space.
    ConstProcessorRcPtr inputToTarget;
    std::string looks = baker.getLooks();
    if (!looks.empty())
    {
        LookTransformRcPtr transform = LookTransform::Create();
        transform->setLooks(looks.c_str());
        transform->setSrc(baker.getInputSpace());
        transform->setDst(baker.getTargetSpace());
        inputToTarget = config->getProcessor(transform, TRANSFORM_DIR_FORWARD);
    }
    else
    {
        inputToTarget = config->getProcessor(baker.getInputSpace(),
            baker.getTargetSpace());
    }
    ConstCPUProcessorRcPtr cpu = inputToTarget->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
    cpu->apply(cubeImg);

    // Write out the file.
    // For for maximum compatibility with other apps, we will
    // not utilize the shaper or output any metadata.

    if(formatName == "lustre")
    {
        int meshInputBitDepth = CubeDimensionLenToLustreBitDepth(cubeSize);
        ostream << "3DMESH\n";
        ostream << "Mesh " << meshInputBitDepth << " " << CUBE_BIT_DEPTH << "\n";
    }

    std::vector<float> shaperData(shaperSize);
    GenerateIdentityLut1D(&shaperData[0], shaperSize, 1);

    float shaperScale = static_cast<float>(
        GetMaxValueFromIntegerBitDepth(SHAPER_BIT_DEPTH));

    for(unsigned int i=0; i<shaperData.size(); ++i)
    {
        if(i != 0) ostream << " ";
        int val = GetClampedIntFromNormFloat(shaperData[i], shaperScale);
        ostream << val;
    }
    ostream << "\n";

    // Write out the 3D Cube.
    float cubeScale = static_cast<float>(
        GetMaxValueFromIntegerBitDepth(CUBE_BIT_DEPTH));
    if(cubeSize < 2)
    {
        throw Exception("Internal cube size exception.");
    }
    for(int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
    {
        int r = GetClampedIntFromNormFloat(cubeData[3*i+0], cubeScale);
        int g = GetClampedIntFromNormFloat(cubeData[3*i+1], cubeScale);
        int b = GetClampedIntFromNormFloat(cubeData[3*i+2], cubeScale);
        ostream << r << " " << g << " " << b << "\n";
    }
    ostream << "\n";

    if(formatName == "lustre")
    {
        ostream << "LUT8\n";
        ostream << "gamma 1.0\n";
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
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    // This should never happen.
    if(!cachedFile || (!cachedFile->lut1D && !cachedFile->lut3D))
    {
        std::ostringstream os;
        os << "Cannot build .3dl Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    // If the FileTransform specifies an interpolation, and it is valid, use it.  If value can't
    // be used for a type of LUT use DEFAULT. It logs a warning if a specified value cannot be used
    // by any LUT in the file. FileTransform interpolation defaults to INTERP_DEFAULT.
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
        if (lut1D)
        {
            CreateLut1DOp(ops, lut1D, newDir);
        }
        if (lut3D)
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }
        break;
    case TRANSFORM_DIR_INVERSE:
        if (lut3D)
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }
        if (lut1D)
        {
            CreateLut1DOp(ops, lut1D, newDir);
        }
        break;
    }
}
}

FileFormat * CreateFileFormat3DL()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE
