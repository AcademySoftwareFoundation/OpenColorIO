// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FileFormatUtils.h"
#include "ops/lut3d/Lut3DOp.h"
#include "Platform.h"
#include "BakingUtils.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"
#include "utils/NumberUtils.h"


/*
SPILUT 1.0
3 3
32 32 32
0 0 0 0.0132509 0.0158522 0.0156622
0 0 1 0.0136178 0.018843 0.033921
0 0 2 0.0136487 0.0240918 0.0563014
0 0 3 0.015706 0.0303061 0.0774135

... entries can be in any order
... after the expected number of entries is found, file can contain anything
*/

namespace OCIO_NAMESPACE
{

namespace
{
class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile() = default;
    ~LocalCachedFile() = default;

    Lut3DOpDataRcPtr lut;
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

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "spi3d";
    info.extension = "spi3d";
    info.capabilities = FormatCapabilityFlags(FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_BAKE);
    info.bake_capabilities = FormatBakeFlags(FORMAT_BAKE_CAPABILITY_3DLUT);
    formatInfoVec.push_back(info);
}

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation interp) const
{
    const int MAX_LINE_SIZE = 4096;
    char lineBuffer[MAX_LINE_SIZE];

    // Read header information
    istream.getline(lineBuffer, MAX_LINE_SIZE);
    if(!StringUtils::StartsWith(StringUtils::Lower(lineBuffer), "spilut"))
    {
        std::ostringstream os;
        os << "Error parsing .spi3d file (";
        os << fileName;
        os << ").  ";
        os << "LUT does not appear to be valid spilut format. ";
        os << "Expected 'SPILUT'.  Found: '" << lineBuffer << "'.";
        throw Exception(os.str().c_str());
    }

    // TODO: Assert 2nd line is 3 3
    istream.getline(lineBuffer, MAX_LINE_SIZE);

    // Get LUT Size
    int rSize = 0, gSize = 0, bSize = 0;
    istream.getline(lineBuffer, MAX_LINE_SIZE);
    if (3 != sscanf(lineBuffer, "%d %d %d", &rSize, &gSize, &bSize))
    {
        std::ostringstream os;
        os << "Error parsing .spi3d file (";
        os << fileName;
        os << "). ";
        os << "Error while reading LUT size. Found: '";
        os << lineBuffer << "'.";
        throw Exception(os.str().c_str());
    }

    // TODO: Support nonuniformly sized LUTs.
    if (rSize != gSize || rSize != bSize)
    {
        std::ostringstream os;
        os << "Error parsing .spi3d file (";
        os << fileName;
        os << "). ";
        os << "LUT size should be the same for all components. Found: '";
        os << lineBuffer << "'.";
        throw Exception(os.str().c_str());
    }

    Lut3DOpDataRcPtr lut3d = std::make_shared<Lut3DOpData>((unsigned long)rSize);
    if (Lut3DOpData::IsValidInterpolation(interp))
    {
        lut3d->setInterpolation(interp);
    }
    lut3d->setFileOutputBitDepth(BIT_DEPTH_F32);

    // Parse table
    int index = 0;
    int rIndex, gIndex, bIndex;
    float redValue = NAN, greenValue = NAN, blueValue = NAN;

    int entriesRemaining = rSize * gSize * bSize;
    Array & lutArray = lut3d->getArray();
    unsigned long numVal = lutArray.getNumValues();
    std::vector<bool> indexDefined(numVal, false);
    while (istream.good() && entriesRemaining > 0)
    {
        istream.getline(lineBuffer, MAX_LINE_SIZE);

        char redValueS[64] = "";
        char greenValueS[64] = "";
        char blueValueS[64] = "";

#ifdef _WIN32
        if (sscanf(lineBuffer,
            "%d %d %d %s %s %s",
            &rIndex, &gIndex, &bIndex,
            redValueS, 64,
            greenValueS, 64,
            blueValueS, 64) == 6)
#else
        if (sscanf(lineBuffer, "%d %d %d %s %s %s",
            &rIndex, &gIndex, &bIndex,
            redValueS, greenValueS, blueValueS) == 6)
#endif
        {
            const auto redValueAnswer = NumberUtils::from_chars(redValueS, redValueS + 64, redValue);
            const auto greenValueAnswer = NumberUtils::from_chars(greenValueS, greenValueS + 64, greenValue);
            const auto blueValueAnswer = NumberUtils::from_chars(blueValueS, blueValueS + 64, blueValue);

            if (redValueAnswer.ec != std::errc()
                || greenValueAnswer.ec != std::errc()
                || blueValueAnswer.ec != std::errc())
            {
                std::ostringstream os;
                os << "Error parsing .spi3d file (";
                os << fileName;
                os << "). ";
                os << "Data is invalid. ";
                os << "A color value is specified (";
                os << redValueS << " " << greenValueS << " " << blueValueS;
                os << ") that cannot be parsed as a floating-point triplet.";
                throw Exception(os.str().c_str());
            }

            bool invalidIndex = false;
            if (rIndex < 0 || rIndex >= rSize
                || gIndex < 0 || gIndex >= gSize
                || bIndex < 0 || bIndex >= bSize)
            {
                invalidIndex = true;
            }
            else
            {
                index = GetLut3DIndex_BlueFast(rIndex, gIndex, bIndex,
                                                rSize, gSize, bSize);
                if (index < 0 || index >= (int)numVal)
                {
                    invalidIndex = true;
                }

            }

            if (invalidIndex)
            {
                std::ostringstream os;
                os << "Error parsing .spi3d file (";
                os << fileName;
                os << "). ";
                os << "Data is invalid. ";
                os << "A LUT entry is specified (";
                os << rIndex << " " << gIndex << " " << bIndex;
                os << ") that falls outside of the cube.";
                throw Exception(os.str().c_str());
            }

            lutArray[index+0] = redValue;
            lutArray[index+1] = greenValue;
            lutArray[index+2] = blueValue;
            if (! indexDefined[index])
            {
                entriesRemaining--;
                indexDefined[index] = true;
            }
            else
            {
                std::ostringstream os;
                os << "Error parsing .spi3d file (";
                os << fileName;
                os << "). ";
                os << "Data is invalid. ";
                os << "A LUT entry is specified multiple times (";
                os << rIndex << " " << gIndex << " " << bIndex;
                os <<  ").";  
                throw Exception(os.str().c_str());
            }
        }
    }

    // Have we fully populated the table?
    if (entriesRemaining > 0)
    {
        std::ostringstream os;
        os << "Error parsing .spi3d file (";
        os << fileName;
        os << "). ";
        os << "Not enough entries found.";
        throw Exception(os.str().c_str());
    }

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
    cachedFile->lut = lut3d;
    return cachedFile;
}

void LocalFileFormat::bake(const Baker & baker,
                           const std::string & formatName,
                           std::ostream & ostream) const
{

    static const int DEFAULT_CUBE_SIZE = 32;

    if(formatName != "spi3d")
    {
        std::ostringstream os;
        os << "Unknown spi format name, '";
        os << formatName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstConfigRcPtr config = baker.getConfig();

    int cubeSize = baker.getCubeSize();
    if(cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
    cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2

    std::vector<float> cubeData;
    cubeData.resize(cubeSize*cubeSize*cubeSize*3);
    GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_BLUE);
    PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);
    ConstCPUProcessorRcPtr inputToTarget = GetInputToTargetProcessor(baker);
    inputToTarget->apply(cubeImg);

    ostream << "SPILUT 1.0\n";
    ostream << "3 3\n";
    ostream << cubeSize << " " << cubeSize << " " << cubeSize << "\n";

    // Set to a fixed 6 decimal precision
    ostream.setf(std::ios::fixed, std::ios::floatfield);
    ostream.precision(6);
    for(int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
    {
        ostream << ((i / cubeSize) / cubeSize) % cubeSize << " "
                << (i / cubeSize) % cubeSize << " "
                << i % cubeSize << " "
                << cubeData[3*i+0] << " "
                << cubeData[3*i+1] << " "
                << cubeData[3*i+2] << "\n";
    }
}

void LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                    const Config & /*config*/,
                                    const ConstContextRcPtr & /*context*/,
                                    CachedFileRcPtr untypedCachedFile,
                                    const FileTransform & fileTransform,
                                    TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    if(!cachedFile || !cachedFile->lut) // This should never happen.
    {
        std::ostringstream os;
        os << "Cannot build Spi3D Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    const auto fileInterp = fileTransform.getInterpolation();

    bool fileInterpUsed = false;
    auto lut = HandleLUT3D(cachedFile->lut, fileInterp, fileInterpUsed);

    if (!fileInterpUsed)
    {
        LogWarningInterpolationNotUsed(fileInterp, fileTransform);
    }

    CreateLut3DOp(ops, lut, newDir);
}
}

FileFormat * CreateFileFormatSpi3D()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE
