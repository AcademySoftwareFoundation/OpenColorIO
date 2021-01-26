// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <map>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "fileformats/cdl/CDLWriter.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "fileformats/xmlutils/XMLWriterUtils.h"
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
        : m_metadata()
    {
    }
    ~LocalCachedFile() = default;

    GroupTransformRcPtr getCDLGroup() const override
    {
        auto group = GroupTransform::Create();
        for (const auto & cdl : m_transformVec)
        {
            group->appendTransform(cdl);
        }
        group->getFormatMetadata() = m_metadata;
        return group;
    }

    CDLTransformMap m_transformMap;
    CDLTransformVec m_transformVec;
    // Descriptive element children of <ColorCorrectionCollection> are
    // stored here.  Descriptive elements of SOPNode and SatNode are
    // stored in the transforms.
    FormatMetadataImpl m_metadata;
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

    void write(const ConstConfigRcPtr & config,
               const ConstContextRcPtr & context,
               const GroupTransform & group,
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
    info.name = FILEFORMAT_COLOR_CORRECTION_COLLECTION;
    info.extension = "ccc";
    info.capabilities = FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE;
    formatInfoVec.push_back(info);
}

// Try and load the format
// Raise an exception if it can't be loaded.

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation /*interp*/) const
{
    CDLParser parser(fileName);
    parser.parse(istream);

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    parser.getCDLTransforms(cachedFile->m_transformMap,
                            cachedFile->m_transformVec,
                            cachedFile->m_metadata);

    return cachedFile;
}

void LocalFileFormat::write(const ConstConfigRcPtr & /*config*/,
                            const ConstContextRcPtr & /*context*/,
                            const GroupTransform & group,
                            const std::string & formatName,
                            std::ostream & ostream) const
{
    const auto numCDL = group.getNumTransforms();
    if (!numCDL)
    {
        std::ostringstream os;
        os << "Write to " << formatName << ": there should be at least one CDL.";
        throw Exception(os.str().c_str());
    }
    for (int i = 0; i < numCDL; ++i)
    {
        auto cdl = OCIO_DYNAMIC_POINTER_CAST<const CDLTransform>(group.getTransform(i));
        if (!cdl)
        {
            std::ostringstream os;
            os << "Write to " << formatName << ": only CDL can be written.";
            throw Exception(os.str().c_str());
        }
    }

    XmlFormatter fmt(ostream);
    XmlFormatter::Attributes attributes;
    attributes.push_back(XmlFormatter::Attribute("xmlns", "urn:ASC:CDL:v1.01"));
    fmt.writeStartTag(CDL_TAG_COLOR_CORRECTION_COLLECTION, attributes);
    {
        XmlScopeIndent scopeIndent(fmt);

        StringUtils::StringVec mainDesc;
        StringUtils::StringVec inputDesc;
        StringUtils::StringVec viewingDesc;
        StringUtils::StringVec sopDesc;
        StringUtils::StringVec satDesc;
        const auto & metadata = group.getFormatMetadata();
        ExtractCDLMetadata(metadata, mainDesc, inputDesc, viewingDesc, sopDesc, satDesc);
        WriteStrings(fmt, TAG_DESCRIPTION, mainDesc);
        WriteStrings(fmt, METADATA_INPUT_DESCRIPTION, inputDesc);
        WriteStrings(fmt, METADATA_VIEWING_DESCRIPTION, viewingDesc);

        for (int i = 0; i < numCDL; ++i)
        {
            auto cdl = OCIO_DYNAMIC_POINTER_CAST<const CDLTransform>(group.getTransform(i));
            Write(fmt, cdl);
        }
    }
    fmt.writeEndTag(CDL_TAG_COLOR_CORRECTION_COLLECTION);
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

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

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

    bool success=false;

    const auto fileCDLStyle = fileTransform.getCDLStyle();

    // Try to parse the cccid as a string id
    CDLTransformMap::const_iterator iter = cachedFile->m_transformMap.find(cccid);
    if (iter != cachedFile->m_transformMap.end())
    {
        CDLTransformRcPtr cdl = iter->second;
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
        // Use 0 for empty string.
        int cccindex=0;
        if (StringToInt(&cccindex, cccid.c_str(), true))
        {
            int maxindex = ((int)cachedFile->m_transformVec.size())-1;
            if (cccindex<0 || cccindex>maxindex)
            {
                std::ostringstream os;
                os << "The specified cccindex " << cccindex;
                os << " is outside the valid range for this file [0,";
                os << maxindex << "]";
                throw ExceptionMissingFile(os.str().c_str());
            }

            CDLTransformRcPtr cdl = cachedFile->m_transformVec[cccindex];
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

