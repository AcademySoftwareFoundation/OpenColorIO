// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FileFormatUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/matrix/MatrixOp.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"


/*
Version 1
From -7.5 3.7555555555555555
Components 1
Length 4096
{
        0.031525943963232252
        0.045645604561056156
	...
}

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

    Lut1DOpDataRcPtr lut;
    float from_min = 0.0f;
    float from_max = 1.0f;
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
                                  int line,
                                  const std::string & lineContent);
};

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "spi1d";
    info.extension = "spi1d";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);
}

// Try and load the format.
// Raise an exception if it can't be loaded.

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & /*fileName*/,
                                      Interpolation interp) const
{
    // Parse Header Info.
    int lut_size = -1;
    float from_min = 0.0;
    float from_max = 1.0;
    int version = -1;
    int components = -1;

    const int MAX_LINE_SIZE = 4096;
    char lineBuffer[MAX_LINE_SIZE];
    int currentLine = 0;

    // PARSE HEADER INFO
    {
        std::string headerLine("");
        do
        {
            istream.getline(lineBuffer, MAX_LINE_SIZE);
            ++currentLine;
            headerLine = std::string(lineBuffer);

            if(StringUtils::StartsWith(headerLine, "Version"))
            {
                // " " in format means any number of spaces (white space,
                // new line, tab) including 0 of them.
                // "Version1" is valid.
                if (sscanf(lineBuffer, "Version %d", &version) != 1)
                {
                    ThrowErrorMessage("Invalid 'Version' Tag", currentLine, headerLine);
                }
                else if (version != 1)
                {
                    ThrowErrorMessage("Only format version 1 supported", currentLine, headerLine);
                }
            }
            else if(StringUtils::StartsWith(headerLine, "From"))
            {
                if (sscanf(lineBuffer, "From %f %f", &from_min, &from_max) != 2)
                {
                    ThrowErrorMessage("Invalid 'From' Tag", currentLine, headerLine);
                }
            }
            else if(StringUtils::StartsWith(headerLine, "Components"))
            {
                if (sscanf(lineBuffer, "Components %d", &components) != 1)
                {
                    ThrowErrorMessage("Invalid 'Components' Tag", currentLine, headerLine);
                }
            }
            else if(StringUtils::StartsWith(headerLine, "Length"))
            {
                if (sscanf(lineBuffer, "Length %d", &lut_size) != 1)
                {
                    ThrowErrorMessage("Invalid 'Length' Tag", currentLine, headerLine);
                }
            }
        }
        while (istream.good() && !StringUtils::StartsWith(headerLine,"{"));
    }

    if (version == -1)
    {
        ThrowErrorMessage("Could not find 'Version' Tag", -1, "");
    }
    if (lut_size == -1)
    {
        ThrowErrorMessage("Could not find 'Length' Tag", -1, "");
    }
    if (components == -1)
    {
        ThrowErrorMessage("Could not find 'Components' Tag", -1, "");
    }
    if (components < 0 || components>3)
    {
        ThrowErrorMessage("Components must be [1,2,3]", -1, "");
    }

    Lut1DOpDataRcPtr lut1d = std::make_shared<Lut1DOpData>(lut_size);
    if (Lut1DOpData::IsValidInterpolation(interp))
    {
        lut1d->setInterpolation(interp);
    }

    lut1d->setFileOutputBitDepth(BIT_DEPTH_F32);
    Array & lutArray = lut1d->getArray();
    unsigned long i = 0;
    {
        istream.getline(lineBuffer, MAX_LINE_SIZE);
        ++currentLine;

        int lineCount=0;

        StringUtils::StringVec inputLUT;
        std::vector<float> values;

        while (istream.good())
        {
            const std::string line = StringUtils::Trim(std::string(lineBuffer));
            if (Platform::Strcasecmp(line.c_str(), "}") == 0)
            {
                break;
            }

            if (line.length() != 0)
            {
                inputLUT = StringUtils::SplitByWhiteSpaces(StringUtils::Trim(lineBuffer));
                values.clear();
                if (!StringVecToFloatVec(values, inputLUT)
                    || components != (int)values.size())
                {
                    std::ostringstream os;
                    os << "Malformed LUT line. Expecting a ";
                    os << components << " components entry.";

                    ThrowErrorMessage("Malformed LUT line", currentLine, line);
                }

                if (lineCount >= lut_size)
                {
                    ThrowErrorMessage("Too many entries found", currentLine, "");
                }

                // If 1 component is specified, use x1 x1 x1.
                if (components == 1)
                {
                    lutArray[i]     = values[0];
                    lutArray[i + 1] = values[0];
                    lutArray[i + 2] = values[0];
                    i += 3;
                    ++lineCount;
                }
                // If 2 components are specified, use x1 x2 0.0.
                else if (components == 2)
                {
                    lutArray[i]     = values[0];
                    lutArray[i + 1] = values[1];
                    lutArray[i + 2] = 0.0f;
                    i += 3;
                    ++lineCount;
                }
                // If 3 component is specified, use x1 x2 x3.
                else if (components == 3)
                {
                    lutArray[i]     = values[0];
                    lutArray[i + 1] = values[1];
                    lutArray[i + 2] = values[2];
                    i += 3;
                    ++lineCount;
                }

                // No other case, components is in [1..3].
            }

            istream.getline(lineBuffer, MAX_LINE_SIZE);
            ++currentLine;
        }

        if (lineCount != lut_size)
        {
            ThrowErrorMessage("Not enough entries found", currentLine, "");
        }
    }

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
    cachedFile->lut = lut1d;
    cachedFile->from_min = from_min;
    cachedFile->from_max = from_max;
    return cachedFile;
}

void LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                    const Config & /*config*/,
                                    const ConstContextRcPtr & /*context*/,
                                    CachedFileRcPtr untypedCachedFile,
                                    const FileTransform& fileTransform,
                                    TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    if(!cachedFile || !cachedFile->lut) // This should never happen.
    {
        std::ostringstream os;
        os << "Cannot build Spi1D Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    const double min[3] = { cachedFile->from_min,
                            cachedFile->from_min,
                            cachedFile->from_min };

    const double max[3] = { cachedFile->from_max,
                            cachedFile->from_max,
                            cachedFile->from_max };

    const auto fileInterp = fileTransform.getInterpolation();

    bool fileInterpUsed = false;
    Lut1DOpDataRcPtr lut = HandleLUT1D(cachedFile->lut, fileInterp, fileInterpUsed);

    if (!fileInterpUsed)
    {
        LogWarningInterpolationNotUsed(fileInterp, fileTransform);
    }

    switch (newDir)
    {
    case TRANSFORM_DIR_FORWARD:
        CreateMinMaxOp(ops, min, max, TRANSFORM_DIR_FORWARD);
        CreateLut1DOp(ops, lut, TRANSFORM_DIR_FORWARD);
        break;
    case TRANSFORM_DIR_INVERSE:
        CreateLut1DOp(ops, lut, TRANSFORM_DIR_INVERSE);
        CreateMinMaxOp(ops, min, max, TRANSFORM_DIR_INVERSE);
        break;
    }
}

void LocalFileFormat::ThrowErrorMessage(const std::string & error,
                                        int line,
                                        const std::string & lineContent)
{
    std::ostringstream os;
    if (-1 != line)
    {
        os << "At line " << line << ": ";
    }
    os << error;
    if (-1 != line && !lineContent.empty())
    {
        os << " (" << lineContent << ")";
    }

    throw Exception(os.str().c_str());
}
}

FileFormat * CreateFileFormatSpi1D()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE

