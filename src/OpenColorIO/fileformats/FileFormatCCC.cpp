// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <map>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "fileformats/FormatMetadata.h"
#include "transforms/CDLTransform.h"
#include "transforms/FileTransform.h"
#include "OpBuilders.h"
#include "ParseUtils.h"


namespace OCIO_NAMESPACE
{

namespace
{
class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile () 
        : metadata()
    {
    }
    ~LocalCachedFile() = default;

    CDLTransformMap transformMap;
    CDLTransformVec transformVec;
    // Descriptive element children of <ColorCorrectionCollection> are
    // stored here.  Descriptive elements of SOPNode and SatNode are
    // stored in the transforms.
    FormatMetadataImpl metadata;
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
    info.name = "ColorCorrectionCollection";
    info.extension = "ccc";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);
}

// Try and load the format
// Raise an exception if it can't be loaded.

CachedFileRcPtr LocalFileFormat::read(
    std::istream & istream,
    const std::string & fileName) const
{
    CDLParser parser(fileName);
    parser.parse(istream);

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    parser.getCDLTransforms(cachedFile->transformMap,
                            cachedFile->transformVec,
                            cachedFile->metadata);

    return cachedFile;
}

void LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                   const Config & config,
                                   const ConstContextRcPtr & context,
                                   CachedFileRcPtr untypedCachedFile,
                                   const FileTransform & fileTransform,
                                   TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    // This should never happen.
    if (!cachedFile)
    {
        std::ostringstream os;
        os << "Cannot build .ccc Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    TransformDirection newDir = CombineTransformDirections(dir, fileTransform.getDirection());
    if (newDir == TRANSFORM_DIR_UNKNOWN)
    {
        std::ostringstream os;
        os << "Cannot build ASC FileTransform,";
        os << " unspecified transform direction.";
        throw Exception(os.str().c_str());
    }

    // Below this point, we should throw ExceptionMissingFile on
    // errors rather than Exception
    // This is because we've verified that the ccc file is valid,
    // at now we're only querying whether the specified cccid can
    // be found.
    //
    // Using ExceptionMissingFile enables the missing looks fallback
    // mechanism to function properly.
    // At the time ExceptionMissingFile was named, we errently assumed
    // a 1:1 relationship between files and color corrections, which is
    // not true for .ccc files.
    //
    // In a future OCIO release, it may be more appropriate to
    // rename ExceptionMissingFile -> ExceptionMissingCorrection.
    // But either way, it's what we should throw below.

    std::string cccid = fileTransform.getCCCId();
    cccid = context->resolveStringVar(cccid.c_str());

    if (cccid.empty())
    {
        std::ostringstream os;
        os << "You must specify which cccid to load from the ccc file";
        os << " (either by name or index).";
        throw ExceptionMissingFile(os.str().c_str());
    }

    bool success=false;

    const auto fileCDLStyle = fileTransform.getCDLStyle();

    // Try to parse the cccid as a string id
    CDLTransformMap::const_iterator iter = cachedFile->transformMap.find(cccid);
    if (iter != cachedFile->transformMap.end())
    {
        auto cdl = iter->second;
        if (fileCDLStyle != CDL_TRANSFORM_DEFAULT)
        {
            cdl = OCIO_DYNAMIC_POINTER_CAST<CDLTransform>(cdl->createEditableCopy());
            cdl->setStyle(fileCDLStyle);
        }
        success = true;
        BuildCDLOp(ops, config, *cdl, newDir);
    }

    // Try to parse the cccid as an integer index
    // We want to be strict, so fail if leftover chars in the parse.
    if (!success)
    {
        int cccindex=0;
        if (StringToInt(&cccindex, cccid.c_str(), true))
        {
            int maxindex = ((int)cachedFile->transformVec.size())-1;
            if (cccindex<0 || cccindex>maxindex)
            {
                std::ostringstream os;
                os << "The specified cccindex " << cccindex;
                os << " is outside the valid range for this file [0,";
                os << maxindex << "]";
                throw ExceptionMissingFile(os.str().c_str());
            }

            auto cdl = cachedFile->transformVec[cccindex];
            if (fileCDLStyle != CDL_TRANSFORM_DEFAULT)
            {
                cdl = OCIO_DYNAMIC_POINTER_CAST<CDLTransform>(cdl->createEditableCopy());
                cdl->setStyle(fileCDLStyle);
            }
            success = true;
            BuildCDLOp(ops, config, *cdl, newDir);
        }
    }

    if (!success)
    {
        std::ostringstream os;
        os << "You must specify a valid cccid to load from the ccc file";
        os << " (either by name or index). id='" << cccid << "' ";
        os << "is not found in the file, and is not parsable as an ";
        os << "integer index.";
        throw ExceptionMissingFile(os.str().c_str());
    }
}
}

FileFormat * CreateFileFormatCCC()
{
    return new LocalFileFormat();
}
} // namespace OCIO_NAMESPACE

