// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "transforms/FileTransform.h"
#include "OpBuilders.h"

namespace OCIO_NAMESPACE
{

namespace
{
class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile ()
    {
        transform = CDLTransform::Create();
    };

    ~LocalCachedFile() = default;

    CDLTransformRcPtr transform;
};

typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;



class LocalFileFormat : public FileFormat
{
public:
    LocalFileFormat() = default;
    ~LocalFileFormat() = default;

    void getFormatInfo(FormatInfoVec & formatInfoVec) const override;

    CachedFileRcPtr read(std::istream & istream,
                         const std::string & fileName) const override;

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
    info.name = "ColorCorrection";
    info.extension = "cc";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);
}

// Try and load the format
// Raise an exception if it can't be loaded.

CachedFileRcPtr LocalFileFormat::read(
    std::istream & istream,
    const std::string & fileName) const
{
    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
    
    try
    {
        CDLParser parser(fileName);
        parser.parse(istream);
        parser.getCDLTransform(cachedFile->transform);
    }
    catch(Exception & e)
    {
        std::ostringstream os;
        os << "Error parsing .cc file. ";
        os << "Does not appear to contain a valid ASC CDL XML:";
        os << e.what();
        throw Exception(os.str().c_str());
    }

    return cachedFile;
}

void
LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                              const Config & config,
                              const ConstContextRcPtr & /*context*/,
                              CachedFileRcPtr untypedCachedFile,
                              const FileTransform& fileTransform,
                              TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    // This should never happen.
    if (!cachedFile)
    {
        std::ostringstream os;
        os << "Cannot build .cc Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    TransformDirection newDir = CombineTransformDirections(dir, fileTransform.getDirection());
    if (newDir == TRANSFORM_DIR_UNKNOWN)
    {
        std::ostringstream os;
        os << "Cannot build file format transform,";
        os << " unspecified transform direction.";
        throw Exception(os.str().c_str());
    }

    auto cdl = cachedFile->transform;
    const auto fileCDLStyle = fileTransform.getCDLStyle();
    if (fileCDLStyle != CDL_TRANSFORM_DEFAULT)
    {
        cdl = OCIO_DYNAMIC_POINTER_CAST<CDLTransform>(cdl->createEditableCopy());
        cdl->setStyle(fileCDLStyle);
    }

    BuildCDLOp(ops, config, *cdl, newDir);
}
}

FileFormat * CreateFileFormatCC()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE

