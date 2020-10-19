// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "fileformats/FileFormatUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ParseUtils.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
namespace
{
class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile () = default;
    ~LocalCachedFile() = default;

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
    os << "Error parsing Pandora LUT file (";
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
    info.name = "pandora_mga";
    info.extension = "mga";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);

    FormatInfo info2;
    info2.name = "pandora_m3d";
    info2.extension = "m3d";
    info2.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info2);
}

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation interp) const
{
    // this shouldn't happen
    if(!istream)
    {
        throw Exception ("File stream empty when trying "
                            "to read Pandora LUT");
    }

    // Validate the file type
    std::string line;

    // Parse the file
    std::string format;
    int lutEdgeLen = 0;
    int outputBitDepthMaxValue = 0;
    std::vector<int> raw3d;

    {
        StringUtils::StringVec parts;
        std::vector<int> tmpints;
        bool inLut3d = false;
        int lineNumber = 0;

        while(nextline(istream, line))
        {
            ++lineNumber;

            // Strip, lowercase, and split the line
            parts = StringUtils::SplitByWhiteSpaces(StringUtils::Lower(StringUtils::Trim(line)));
            if(parts.empty()) continue;

            // Skip all lines starting with '#'
            if(StringUtils::StartsWith(parts[0],"#")) continue;

            if(parts[0] == "channel")
            {
                if(parts.size() != 2 
                    || StringUtils::Lower(parts[1]) != "3d")
                {
                    ThrowErrorMessage(
                        "Only 3D LUTs are currently supported "
                        "(channel: 3d).",
                        fileName,
                        lineNumber,
                        line);
                }
            }
            else if(parts[0] == "in")
            {
                int inval = 0;
                if(parts.size() != 2
                    || !StringToInt( &inval, parts[1].c_str()))
                {
                    ThrowErrorMessage(
                        "Malformed 'in' tag.",
                        fileName,
                        lineNumber,
                        line);
                }
                raw3d.reserve(inval*3);
                lutEdgeLen = Get3DLutEdgeLenFromNumPixels(inval);
            }
            else if(parts[0] == "out")
            {
                if(parts.size() != 2 
                    || !StringToInt(&outputBitDepthMaxValue,
                                    parts[1].c_str()))
                {
                    ThrowErrorMessage(
                        "Malformed 'out' tag.",
                        fileName,
                        lineNumber,
                        line);
                }
            }
            else if(parts[0] == "format")
            {
                if(parts.size() != 2 
                    || StringUtils::Lower(parts[1]) != "lut")
                {
                    ThrowErrorMessage(
                        "Only LUTs are currently supported "
                        "(format: lut).",
                        fileName,
                        lineNumber,
                        line);
                }
            }
            else if(parts[0] == "values")
            {
                if(parts.size() != 4 || 
                    StringUtils::Lower(parts[1]) != "red" ||
                    StringUtils::Lower(parts[2]) != "green" ||
                    StringUtils::Lower(parts[3]) != "blue")
                {
                    ThrowErrorMessage(
                        "Only rgb LUTs are currently supported "
                        "(values: red green blue).",
                        fileName,
                        lineNumber,
                        line);
                }

                inLut3d = true;
            }
            else if(inLut3d)
            {
                if(!StringVecToIntVec(tmpints, parts) || tmpints.size() != 4)
                {
                    ThrowErrorMessage(
                        "Expected to find 4 integers.",
                        fileName,
                        lineNumber,
                        line);
                }

                raw3d.push_back(tmpints[1]);
                raw3d.push_back(tmpints[2]);
                raw3d.push_back(tmpints[3]);
            }
        }
    }

    // Interpret the parsed data, validate LUT sizes
    if(lutEdgeLen*lutEdgeLen*lutEdgeLen != static_cast<int>(raw3d.size()/3))
    {
        std::ostringstream os;
        os << "Incorrect number of 3D LUT entries. ";
        os << "Found " << raw3d.size() / 3 << ", expected ";
        os << lutEdgeLen*lutEdgeLen*lutEdgeLen << ".";
        ThrowErrorMessage(
            os.str().c_str(),
            fileName, -1, "");
    }

    if(lutEdgeLen*lutEdgeLen*lutEdgeLen == 0)
    {
        ThrowErrorMessage(
            "No 3D LUT entries found.",
            fileName, -1, "");
    }

    if(outputBitDepthMaxValue <= 0)
    {
        ThrowErrorMessage(
            "A valid 'out' tag was not found.",
            fileName, -1, "");
    }

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    // Copy raw3d vector into LutOpData object.
    cachedFile->lut3D = std::make_shared<Lut3DOpData>(lutEdgeLen);
    if (Lut3DOpData::IsValidInterpolation(interp))
    {
        cachedFile->lut3D->setInterpolation(interp);
    }

    BitDepth fileBD = GetBitdepthFromMaxValue(outputBitDepthMaxValue);
    cachedFile->lut3D->setFileOutputBitDepth(fileBD);

    auto & lutArray = cachedFile->lut3D->getArray();

    float scale = 1.0f / ((float)outputBitDepthMaxValue - 1.0f);

    // lutArray and LUT in file are blue fastest.
    for (size_t i = 0; i < raw3d.size(); ++i)
    {
        lutArray[(unsigned long)i] = static_cast<float>(raw3d[i]) * scale;
    }

    return cachedFile;
}

void
LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                const Config& /*config*/,
                                const ConstContextRcPtr & /*context*/,
                                CachedFileRcPtr untypedCachedFile,
                                const FileTransform& fileTransform,
                                TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    // This should never happen.
    if(!cachedFile || !cachedFile->lut3D)
    {
        std::ostringstream os;
        os << "Cannot build Pandora LUT. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    const auto fileInterp = fileTransform.getInterpolation();

    bool fileInterpUsed = false;
    auto lut3D = HandleLUT3D(cachedFile->lut3D, fileInterp, fileInterpUsed);

    if (!fileInterpUsed)
    {
        LogWarningInterpolationNotUsed(fileInterp, fileTransform);
    }

    CreateLut3DOp(ops, lut3D, newDir);
}
}

FileFormat * CreateFileFormatPandora()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE

