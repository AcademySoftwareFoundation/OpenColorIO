// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

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
    LocalCachedFile()
    {
        memset(m44, 0, 16*sizeof(double));
        memset(offset4, 0, 4*sizeof(double));
    };
    ~LocalCachedFile() {};

    double m44[16];
    double offset4[4];
};

typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;

class LocalFileFormat : public FileFormat
{
public:

    ~LocalFileFormat() {};

    void getFormatInfo(FormatInfoVec & formatInfoVec) const override;

    CachedFileRcPtr read(std::istream & istream,
                         const std::string & fileName,
                         Interpolation interp) const override;

    void buildFileOps(OpRcPtrVec & ops,
                        const Config& config,
                        const ConstContextRcPtr & context,
                        CachedFileRcPtr untypedCachedFile,
                        const FileTransform& fileTransform,
                        TransformDirection dir) const override;
};

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "spimtx";
    info.extension = "spimtx";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);
}

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation /*interp*/) const
{

    // Read the entire file.
    std::ostringstream fileStream;

    {
        const int MAX_LINE_SIZE = 4096;
        char lineBuffer[MAX_LINE_SIZE];

        while (istream.good())
        {
            istream.getline(lineBuffer, MAX_LINE_SIZE);
            fileStream << std::string(lineBuffer) << " ";
        }
    }

    // Turn it into parts
    const StringUtils::StringVec lineParts 
        = StringUtils::SplitByWhiteSpaces(StringUtils::Trim(fileStream.str()));

    if(lineParts.size() != 12)
    {
        std::ostringstream os;
        os << "Error parsing .spimtx file (";
        os << fileName << "). ";
        os << "File must contain 12 float entries. ";
        os << lineParts.size() << " found.";
        throw Exception(os.str().c_str());
    }

    // Turn the parts into floats
    std::vector<float> floatArray;
    if(!StringVecToFloatVec(floatArray, lineParts))
    {
        std::ostringstream os;
        os << "Error parsing .spimtx file (";
        os << fileName << "). ";
        os << "File must contain all float entries. ";
        throw Exception(os.str().c_str());
    }


    // Put the bits in the right place
    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    cachedFile->m44[0] =  floatArray[0];
    cachedFile->m44[1] =  floatArray[1];
    cachedFile->m44[2] =  floatArray[2];
    cachedFile->m44[3] =  0.0;

    cachedFile->m44[4] =  floatArray[4];
    cachedFile->m44[5] =  floatArray[5];
    cachedFile->m44[6] =  floatArray[6];
    cachedFile->m44[7] =  0.0;

    cachedFile->m44[8] =  floatArray[8];
    cachedFile->m44[9] =  floatArray[9];
    cachedFile->m44[10] = floatArray[10];
    cachedFile->m44[11] = 0.0;

    cachedFile->m44[12] = 0.0;
    cachedFile->m44[13] = 0.0;
    cachedFile->m44[14] = 0.0;
    cachedFile->m44[15] = 1.0;

    cachedFile->offset4[0] = floatArray[3] / 65535.0;
    cachedFile->offset4[1] = floatArray[7] / 65535.0;
    cachedFile->offset4[2] = floatArray[11] / 65535.0;
    cachedFile->offset4[3] = 0.0;

    return cachedFile;
}

void LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                    const Config& /*config*/,
                                    const ConstContextRcPtr & /*context*/,
                                    CachedFileRcPtr untypedCachedFile,
                                    const FileTransform& fileTransform,
                                    TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    if(!cachedFile) // This should never happen.
    {
        std::ostringstream os;
        os << "Cannot build SpiMtx Ops. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    CreateMatrixOffsetOp(ops, cachedFile->m44, cachedFile->offset4, newDir);
}
}

FileFormat * CreateFileFormatSpiMtx()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE

