// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <cstring>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FileFormatUtils.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/matrix/MatrixOp.h"
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
    LocalCachedFile() = default;
    ~LocalCachedFile() = default;

    Lut3DOpDataRcPtr lut3D;
    double m44[16]{ 0 };
    bool useMatrix = false;
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
    os << "Error parsing Nuke .vf file (";
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
    info.name = "nukevf";
    info.extension = "vf";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);
}

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation interp) const
{
    // this shouldn't happen
    if(!istream)
    {
        throw Exception ("File stream empty when trying to read .vf LUT");
    }

    // Validate the file type
    std::string line;
    int lineNumber = 1;
    if(!nextline(istream, line) ||
        !StringUtils::StartsWith(StringUtils::Lower(line), "#inventor"))
    {
        ThrowErrorMessage("Expecting '#Inventor V2.1 ascii'.",
            fileName, lineNumber, line);
    }

    // Parse the file
    std::vector<float> raw3d;
    int size3d[] = { 0, 0, 0 };
    std::vector<float> global_transform;

    {
        StringUtils::StringVec parts;
        std::vector<float> tmpfloats;

        bool in3d = false;

        while(nextline(istream, line))
        {
            ++lineNumber;

            // Strip, lowercase, and split the line
            parts = StringUtils::SplitByWhiteSpaces(StringUtils::Lower(StringUtils::Trim(line)));

            if(parts.empty()) continue;

            if(StringUtils::StartsWith(parts[0],"#")) continue;

            if(!in3d)
            {
                if(parts[0] == "grid_size")
                {
                    if(parts.size() != 4 || 
                        !StringToInt( &size3d[0], parts[1].c_str()) ||
                        !StringToInt( &size3d[1], parts[2].c_str()) ||
                        !StringToInt( &size3d[2], parts[3].c_str()))
                    {
                        ThrowErrorMessage(
                            "Malformed grid_size tag.",
                            fileName, lineNumber, line);
                    }

                    // TODO: Support nununiformly sized LUTs.
                    if (size3d[0] != size3d[1] ||
                        size3d[0] != size3d[2])
                    {
                        std::ostringstream os;
                        os << "Only equal grid size LUTs are supported. Found ";
                        os << "grid size: " << size3d[0] << " x ";
                        os << size3d[1] << " x " << size3d[2] << ".";
                        ThrowErrorMessage(
                            os.str(),
                            fileName, lineNumber, line);
                    }

                    raw3d.reserve(3*size3d[0]*size3d[1]*size3d[2]);
                }
                else if(parts[0] == "global_transform")
                {
                    if(parts.size() != 17)
                    {
                        ThrowErrorMessage(
                            "Malformed global_transform tag. "
                            "16 floats expected.",
                            fileName, lineNumber, line);
                    }

                    parts.erase(parts.begin()); // Drop the 1st entry. (the tag)
                    if(!StringVecToFloatVec(global_transform, parts) || global_transform.size() != 16)
                    {
                        ThrowErrorMessage(
                            "Malformed global_transform tag. "
                            "Could not convert to float array.",
                            fileName, lineNumber, line);
                    }
                }
                // TODO: element_size (aka scale3)
                // TODO: world_origin (aka translate3)
                else if(parts[0] == "data")
                {
                    in3d = true;
                }
            }
            else // (in3d)
            {
                if(StringVecToFloatVec(tmpfloats, parts) && (tmpfloats.size() == 3))
                {
                    raw3d.push_back(tmpfloats[0]);
                    raw3d.push_back(tmpfloats[1]);
                    raw3d.push_back(tmpfloats[2]);
                }
            }
        }
    }

    // Interpret the parsed data, validate LUT sizes
    if(size3d[0]*size3d[1]*size3d[2] != static_cast<int>(raw3d.size()/3))
    {
        std::ostringstream os;
        os << "Incorrect number of 3D LUT entries. ";
        os << "Found " << raw3d.size()/3 << ", expected " << size3d[0]*size3d[1]*size3d[2] << ".";
        ThrowErrorMessage(
            os.str().c_str(),
            fileName, -1, "");
    }

    if(size3d[0]*size3d[1]*size3d[2] == 0)
    {
        ThrowErrorMessage(
            "No 3D LUT entries found.",
            fileName, -1, "");
    }

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    // Setup the global matrix.
    // (Nuke pre-scales this by the 3D LUT size, so we must undo that here)
    if(global_transform.size() == 16)
    {
        for(int i=0; i<4; ++i)
        {
            global_transform[4*i+0] *= static_cast<float>(size3d[0]);
            global_transform[4*i+1] *= static_cast<float>(size3d[1]);
            global_transform[4*i+2] *= static_cast<float>(size3d[2]);
            cachedFile->m44[4*i+0] = global_transform[4*i+0];
            cachedFile->m44[4*i+1] = global_transform[4*i+1];
            cachedFile->m44[4*i+2] = global_transform[4*i+2];
            cachedFile->m44[4*i+3] = global_transform[4*i+3];
        }

        cachedFile->useMatrix = true;
    }

    // Copy raw3d vector into LutOpData object.
    cachedFile->lut3D = std::make_shared<Lut3DOpData>(size3d[0]);
    if (Lut3DOpData::IsValidInterpolation(interp))
    {
        cachedFile->lut3D->setInterpolation(interp);
    }

    cachedFile->lut3D->setFileOutputBitDepth(BIT_DEPTH_F32);
    // LUT in file is blue fastest.
    cachedFile->lut3D->getArray().getValues() = raw3d;

    return cachedFile;
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
    if(!cachedFile)
    {
        std::ostringstream os;
        os << "Cannot build .vf Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    const auto fileInterp = fileTransform.getInterpolation();

    Lut3DOpDataRcPtr lut3D;

    if (cachedFile->lut3D)
    {
        bool fileInterpUsed = false;
        lut3D = HandleLUT3D(cachedFile->lut3D, fileInterp, fileInterpUsed);

        if (!fileInterpUsed)
        {
            LogWarningInterpolationNotUsed(fileInterp, fileTransform);
        }
    }

    switch (newDir)
    {
    case TRANSFORM_DIR_FORWARD:
        if (cachedFile->useMatrix)
        {
            CreateMatrixOp(ops, cachedFile->m44, newDir);
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

        if (cachedFile->useMatrix)
        {
            CreateMatrixOp(ops, cachedFile->m44, newDir);
        }
        break;
    }
}
}

FileFormat * CreateFileFormatVF()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE

