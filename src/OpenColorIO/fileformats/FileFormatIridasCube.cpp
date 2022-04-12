// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FileFormatUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/matrix/MatrixOp.h"
#include "BakingUtils.h"
#include "ParseUtils.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"
#include "utils/NumberUtils.h"


/*

http://doc.iridas.com/index.php/LUT_Formats

#comments start with '#'
#title is currently ignored, but it's not an error to enter one
TITLE "title"

#LUT_1D_SIZE M or
#LUT_3D_SIZE M
#where M is the size of the texture
#a 3D texture has the size M x M x M
#e.g. LUT_3D_SIZE 16 creates a 16 x 16 x 16 3D texture
LUT_3D_SIZE 2

#Default input value range (domain) is 0.0 (black) to 1.0 (white)
#Specify other min/max values to map the cube to any custom input
#range you wish to use, for example if you're working with HDR data
DOMAIN_MIN 0.0 0.0 0.0
DOMAIN_MAX 1.0 1.0 1.0

#for 1D textures, the data is simply a list of floating point values,
#three per line, in RGB order
#for 3D textures, the data is also RGB, and ordered in such a way
#that the red coordinate changes fastest, then the green coordinate,
#and finally, the blue coordinate changes slowest:
0.0 0.0 0.0
1.0 0.0 0.0
0.0 1.0 0.0
1.0 1.0 0.0
0.0 0.0 1.0
1.0 0.0 1.0
0.0 1.0 1.0
1.0 1.0 1.0

#Note that the LUT data is not limited to any particular range
#and can contain values under 0.0 and over 1.0
#The processing application might however still clip the
#output values to the 0.0 - 1.0 range, depending on the internal
#precision of that application's pipeline
#IRIDAS applications generally use a floating point pipeline
#with little or no clipping

#A LUT may contain a 1D or 3D LUT but not both.

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

    Lut1DOpDataRcPtr lut1D;
    Lut3DOpDataRcPtr lut3D;
    float domain_min[3]{ 0.0f, 0.0f, 0.0f };
    float domain_max[3]{ 1.0f, 1.0f, 1.0f };
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
							  
private:
    static void ThrowErrorMessage(const std::string & error,
                                    const std::string & fileName,
                                    int line,
                                    const std::string & lineContent);
};

void LocalFileFormat::ThrowErrorMessage(const std::string & error,
                                        const std::string & fileName,
                                        int line,
                                        const std::string & lineContent)
{
    std::ostringstream os;
    os << "Error parsing Iridas .cube file (";
    os << fileName;
    os << ").  ";
    if (-1 != line)
    {
        os << "At line (" << line << "): '";
        os << lineContent << "'.  ";
    }
    os << error;

    throw Exception(os.str().c_str());
}

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "iridas_cube";
    info.extension = "cube";
    info.capabilities = FormatCapabilityFlags(FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_BAKE);
    info.bake_capabilities = FormatBakeFlags(FORMAT_BAKE_CAPABILITY_3DLUT);
    formatInfoVec.push_back(info);
}

CachedFileRcPtr
LocalFileFormat::read(std::istream & istream,
                      const std::string & fileName,
                      Interpolation interp) const
{
    // this shouldn't happen
    if(!istream)
    {
        throw Exception ("File stream empty when trying to read Iridas .cube LUT");
    }

    // Parse the file
    std::vector<float> raw;

    int size3d = 0;
    int size1d = 0;

    bool in1d = false;
    bool in3d = false;

    float domain_min[] = { 0.0f, 0.0f, 0.0f };
    float domain_max[] = { 1.0f, 1.0f, 1.0f };

    {
        std::string line;
        int lineNumber = 0;
        char endTok;
        bool entriesStarted = false;

        while(!entriesStarted && nextline(istream, line))
        {
            ++lineNumber;
            // All lines starting with '#' are comments
            if (StringUtils::StartsWith(line,"#")) continue;

            line = StringUtils::Lower(StringUtils::Trim(line));

            if (line.empty()) continue;

            if (StringUtils::StartsWith(line, "title"))
            {
                // Optional, and currently unhandled
            }
            else if (StringUtils::StartsWith(line, "lut_1d_size"))
            {
#ifdef _WIN32
                if (sscanf_s(line.c_str(), "lut_1d_size %d %c", &size1d, &endTok, 1) != 1)
#else
                if (sscanf(line.c_str(), "lut_1d_size %d %c", &size1d, &endTok) != 1)
#endif
                {
                    ThrowErrorMessage(
                        "Malformed 'LUT_1D_SIZE' tag.",
                        fileName,
                        lineNumber,
                        line);
                }

                raw.reserve(3*size1d);
                in1d = true;
            }
            else if (StringUtils::StartsWith(line, "lut_2d_size"))
            {
                ThrowErrorMessage(
                    "Unsupported tag: 'LUT_2D_SIZE'.",
                    fileName,
                    lineNumber,
                    line);
            }
            else if (StringUtils::StartsWith(line, "lut_3d_size"))
            {
#ifdef _WIN32
                if (sscanf_s(line.c_str(), "lut_3d_size %d %c", &size3d, &endTok, 1) != 1)
#else
                if (sscanf(line.c_str(), "lut_3d_size %d %c", &size3d, &endTok) != 1)
#endif
                {
                    ThrowErrorMessage(
                        "Malformed 'LUT_3D_SIZE' tag.",
                        fileName,
                        lineNumber,
                        line);
                }

                raw.reserve(3*size3d*size3d*size3d);
                in3d = true;
            }
            else if (StringUtils::StartsWith(line, "domain_min"))
            {
                char domainMinR[64] = "";
                char domainMinG[64] = "";
                char domainMinB[64] = "";

#ifdef _WIN32
                if (sscanf_s(line.c_str(), "domain_min %s %s %s %c", domainMinR, 64, domainMinG, 64, domainMinB, 64, &endTok, 1) != 3)
#else
                if (sscanf(line.c_str(), "domain_min %s %s %s %c", domainMinR, domainMinG, domainMinB, &endTok) != 3)
#endif
                {
                    ThrowErrorMessage(
                        "Malformed 'DOMAIN_MIN' tag.",
                        fileName,
                        lineNumber,
                        line);
                }
                else
                {
                    const auto fromMinRAnswer = NumberUtils::from_chars(domainMinR, domainMinR + 64, domain_min[0]);
                    const auto fromMinGAnswer = NumberUtils::from_chars(domainMinG, domainMinG + 64, domain_min[1]);
                    const auto fromMinBAnswer = NumberUtils::from_chars(domainMinB, domainMinB + 64, domain_min[2]);

                    if (fromMinRAnswer.ec != std::errc() || fromMinGAnswer.ec != std::errc() || fromMinBAnswer.ec != std::errc())
                    {
                        ThrowErrorMessage(
                            "Invalid 'DOMAIN_MIN' Tag",
                            fileName,
                            lineNumber,
                            line);
                    }
                }
            }
            else if (StringUtils::StartsWith(line, "domain_max"))
            {
                char domainMaxR[64] = "";
                char domainMaxG[64] = "";
                char domainMaxB[64] = "";

#ifdef _WIN32
                if (sscanf_s(line.c_str(), "domain_max %s %s %s %c", domainMaxR, 64, domainMaxG, 64, domainMaxB, 64, &endTok, 1) != 3)
#else
                if (sscanf(line.c_str(), "domain_max %s %s %s %c", domainMaxR, domainMaxG, domainMaxB, &endTok) != 3)
#endif
                {
                    ThrowErrorMessage(
                        "Malformed 'DOMAIN_MAX' tag.",
                        fileName,
                        lineNumber,
                        line);
                }
                else
                {
                    const auto fromMaxRAnswer = NumberUtils::from_chars(domainMaxR, domainMaxR + 64, domain_max[0]);
                    const auto fromMaxGAnswer = NumberUtils::from_chars(domainMaxG, domainMaxG + 64, domain_max[1]);
                    const auto fromMaxBAnswer = NumberUtils::from_chars(domainMaxB, domainMaxB + 64, domain_max[2]);

                    if (fromMaxRAnswer.ec != std::errc() || fromMaxGAnswer.ec != std::errc() || fromMaxBAnswer.ec != std::errc())
                    {
                        ThrowErrorMessage(
                            "Invalid 'DOMAIN_MAX' Tag",
                            fileName,
                            lineNumber,
                            line);
                    }
                }
            }
            else
            {
                entriesStarted = true;
            }
        }

        do
        {
            line = StringUtils::Trim(line);

            // All lines starting with '#' are comments
            if (StringUtils::StartsWith(line,"#")) continue;

            if (line.empty()) continue;

            char valR[64] = "";
            char valG[64] = "";
            char valB[64] = "";

#ifdef _WIN32
            if (sscanf_s(line.c_str(), "%s %s %s %c", valR, 64, valG, 64, valB, 64, &endTok, 1) != 3)
#else
            if (sscanf(line.c_str(), "%s %s %s %c", valR, valG, valB, &endTok) != 3)
#endif
            {
                // It must be a float triple!
                ThrowErrorMessage(
                    "Malformed color triples specified.",
                    fileName,
                    lineNumber,
                    line);
            }
            else
            {
                float r = NAN;
                float g = NAN;
                float b = NAN;

                const auto rAnswer = NumberUtils::from_chars(valR, valR + 64, r);
                const auto gAnswer = NumberUtils::from_chars(valG, valG + 64, g);
                const auto bAnswer = NumberUtils::from_chars(valB, valB + 64, b);

                if (rAnswer.ec != std::errc() || gAnswer.ec != std::errc() || bAnswer.ec != std::errc())
                {
                    ThrowErrorMessage(
                        "Invalid color triples",
                        fileName,
                        lineNumber,
                        line);
                }

                raw.push_back(r);
                raw.push_back(g);
                raw.push_back(b);
            }

            ++lineNumber;
        }
        while (nextline(istream, line));
    }

    // Interpret the parsed data, validate LUT sizes.

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    if(in1d)
    {
        if(size1d != static_cast<int>(raw.size()/3))
        {
            std::ostringstream os;
            os << "Incorrect number of lut1d entries. ";
            os << "Found " << raw.size() / 3;
            os << ", expected " << size1d << ".";
            ThrowErrorMessage(
                os.str().c_str(),
                fileName, -1, "");
        }

        // Reformat 1D data
        if(size1d>0)
        {
            memcpy(cachedFile->domain_min, domain_min, 3 * sizeof(float));
            memcpy(cachedFile->domain_max, domain_max, 3 * sizeof(float));

            const auto lutLenght = static_cast<unsigned long>(size1d);
            cachedFile->lut1D = std::make_shared<Lut1DOpData>(lutLenght);
            if (Lut1DOpData::IsValidInterpolation(interp))
            {
                cachedFile->lut1D->setInterpolation(interp);
            }

            cachedFile->lut1D->setFileOutputBitDepth(BIT_DEPTH_F32);

            auto & lutArray = cachedFile->lut1D->getArray();

            const auto numVals = lutArray.getNumValues();
            for(unsigned long i = 0; i < numVals; ++i)
            {
                lutArray[i] = raw[i];
            }
        }
    }
    else if(in3d)
    {
        if(size3d*size3d*size3d 
            != static_cast<int>(raw.size()/3))
        {
            std::ostringstream os;
            os << "Incorrect number of 3D LUT entries. ";
            os << "Found " << raw.size() / 3 << ", expected ";
            os << size3d * size3d * size3d << ".";
            ThrowErrorMessage(
                os.str().c_str(),
                fileName, -1, "");
        }

        // Reformat 3D data
        memcpy(cachedFile->domain_min, domain_min, 3*sizeof(float));
        memcpy(cachedFile->domain_max, domain_max, 3*sizeof(float));
        cachedFile->lut3D = std::make_shared<Lut3DOpData>(size3d);
        if (Lut3DOpData::IsValidInterpolation(interp))
        {
            cachedFile->lut3D->setInterpolation(interp);
        }
        cachedFile->lut3D->setFileOutputBitDepth(BIT_DEPTH_F32);
        cachedFile->lut3D->setArrayFromRedFastestOrder(raw);
    }
    else
    {
        ThrowErrorMessage(
            "LUT type (1D/3D) unspecified.",
            fileName, -1, "");
    }

    return cachedFile;
}

void LocalFileFormat::bake(const Baker & baker,
                           const std::string & formatName,
                           std::ostream & ostream) const
{

    static const int DEFAULT_CUBE_SIZE = 32;

    if(formatName != "iridas_cube")
    {
        std::ostringstream os;
        os << "Unknown cube format name, '";
        os << formatName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstConfigRcPtr config = baker.getConfig();

    int cubeSize = baker.getCubeSize();
    if(cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
    cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2

    std::vector<float> cubeData;
    cubeData.resize(cubeSize*cubeSize*cubeSize*3);
    GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
    PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);
    ConstCPUProcessorRcPtr inputToTarget = GetInputToTargetProcessor(baker);
    inputToTarget->apply(cubeImg);

    const auto & metadata = baker.getFormatMetadata();
    const auto nb = metadata.getNumChildrenElements();
    for (int i = 0; i < nb; ++i)
    {
        const auto & child = metadata.getChildElement(i);
        ostream << "# " << child.getElementValue() << "\n";
    }
    if (nb > 0)
    {
        ostream << "\n";
    }

    ostream << "LUT_3D_SIZE " << cubeSize << "\n";

    // Set to a fixed 6 decimal precision
    ostream.setf(std::ios::fixed, std::ios::floatfield);
    ostream.precision(6);
    for(int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
    {
        ostream << cubeData[3*i+0] << " "
                << cubeData[3*i+1] << " "
                << cubeData[3*i+2] << "\n";
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
        os << "Cannot build Iridas .cube Op. Invalid cache type.";
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

    const double dmin[]{ cachedFile->domain_min[0], cachedFile->domain_min[1], cachedFile->domain_min[2] };
    const double dmax[]{ cachedFile->domain_max[0], cachedFile->domain_max[1], cachedFile->domain_max[2] };

    switch (newDir)
    {
    case TRANSFORM_DIR_FORWARD:
    {
        CreateMinMaxOp(ops, dmin, dmax, newDir);
        if(lut1D)
        {
            CreateLut1DOp(ops, lut1D, newDir);
        }
        else if(lut3D)
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        if(lut3D)
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }
        else if(lut1D)
        {
            CreateLut1DOp(ops, lut1D, newDir);
        }
        CreateMinMaxOp(ops, dmin, dmax, newDir);
        break;
    }
    }
}
}

FileFormat * CreateFileFormatIridasCube()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE
