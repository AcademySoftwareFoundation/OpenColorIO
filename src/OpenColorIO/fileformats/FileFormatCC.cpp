// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "fileformats/cdl/CDLWriter.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "fileformats/xmlutils/XMLWriterUtils.h"
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
    {
        m_transform = CDLTransformImpl::Create();
    };

    ~LocalCachedFile() = default;

    GroupTransformRcPtr getCDLGroup() const override
    {
        auto group = GroupTransform::Create();
        group->appendTransform(m_transform);
        return group;
    }

    CDLTransformImplRcPtr m_transform;
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
    info.name = FILEFORMAT_COLOR_CORRECTION;
    info.extension = "cc";
    info.capabilities = FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE;
    formatInfoVec.push_back(info);
}

// Try and load the format
// Raise an exception if it can't be loaded.

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation /*interp*/) const
{
    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
    
    CDLParser parser(fileName);
    try
    {
        parser.parse(istream);
        parser.getCDLTransform(cachedFile->m_transform);
    }
    catch(Exception & e)
    {
        std::ostringstream os;
        os << "Error parsing .cc file. ";
        os << "Does not appear to contain a valid ASC CDL XML:";
        os << e.what();
        throw Exception(os.str().c_str());
    }
    if (!parser.isCC())
    {
        std::ostringstream os;
        os << "File '" << fileName << "' is not a .cc file.";
        throw Exception(os.str().c_str());
    }

    return cachedFile;
}

void LocalFileFormat::write(const ConstConfigRcPtr & /*config*/,
                            const ConstContextRcPtr & /*context*/,
                            const GroupTransform & group,
                            const std::string & formatName,
                            std::ostream & ostream) const
{
    if (group.getNumTransforms() != 1)
    {
        throw Exception("CDL write: there should be a single CDL.");
    }
    auto cdl = OCIO_DYNAMIC_POINTER_CAST<const CDLTransform>(group.getTransform(0));
    if (!cdl)
    {
        throw Exception("CDL write: only CDL can be written.");
    }

    XmlFormatter fmt(ostream);
    Write(fmt, cdl);
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

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    CDLTransformRcPtr cdl = cachedFile->m_transform;
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

