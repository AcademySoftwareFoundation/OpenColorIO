// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FILETRANSFORM_H
#define INCLUDED_OCIO_FILETRANSFORM_H


#include <map>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/noop/NoOps.h"
#include "PrivateTypes.h"
#include "Processor.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
void ClearFileTransformCaches();

class CachedFile
{
public:
    CachedFile() {};
    virtual ~CachedFile() {};
};

typedef OCIO_SHARED_PTR<CachedFile> CachedFileRcPtr;

const int FORMAT_CAPABILITY_NONE = 0;
const int FORMAT_CAPABILITY_READ = 1;
const int FORMAT_CAPABILITY_BAKE = 2;
const int FORMAT_CAPABILITY_WRITE = 4;

struct FormatInfo
{
    std::string name;       // name must be globally unique
    std::string extension;  // extension does not need to be unique
    int capabilities;

    FormatInfo():
        capabilities(FORMAT_CAPABILITY_NONE)
    { }
};

typedef std::vector<FormatInfo> FormatInfoVec;

class FileFormat
{
public:
    virtual ~FileFormat();

    virtual void getFormatInfo(FormatInfoVec & formatInfoVec) const = 0;

    // read an istream. originalFileName is used by parsers that make use
    // of aspects of the file name as part of the parsing.
    // It may be set to an empty string if not known.
    virtual CachedFileRcPtr read(
        std::istream & istream,
        const std::string & originalFileName) const = 0;

    virtual void bake(const Baker & baker,
                        const std::string & formatName,
                        std::ostream & ostream) const;

    virtual void write(const OpRcPtrVec & ops,
                        const FormatMetadataImpl & metadata,
                        const std::string & formatName,
                        std::ostream & ostream) const;

    virtual void buildFileOps(OpRcPtrVec & ops,
                                const Config & config,
                                const ConstContextRcPtr & context,
                                CachedFileRcPtr cachedFile,
                                const FileTransform & fileTransform,
                                TransformDirection dir) const = 0;

    // True if the file is a binary rather than text-based format.
    virtual bool isBinary() const
    {
        return false;
    }

    // For logging purposes.
    std::string getName() const;
private:
    FileFormat& operator= (const FileFormat &);
};

typedef std::map<std::string, FileFormat*> FileFormatMap;
typedef std::vector<FileFormat*> FileFormatVector;
typedef std::map<std::string, FileFormatVector> FileFormatVectorMap;

// TODO: This interface is ugly. What private API is actually appropriate?
// Maybe, instead of exposing the raw formats, we wrap it?
// FileCachePair GetFile(const std::string & filepath) and all
// lookups will move internal.

class FormatRegistry
{
public:
    static FormatRegistry & GetInstance();

    FileFormat* getFileFormatByName(const std::string & name) const;
    void getFileFormatForExtension(const std::string & extension, FileFormatVector & possibleFormats) const;

    int getNumRawFormats() const;
    FileFormat* getRawFormatByIndex(int index) const;

    int getNumFormats(int capability) const;
    const char * getFormatNameByIndex(int capability, int index) const;
    const char * getFormatExtensionByIndex(int capability, int index) const;
private:
    FormatRegistry();
    ~FormatRegistry();

    void registerFileFormat(FileFormat* format);

    FileFormatMap m_formatsByName;
    FileFormatVectorMap m_formatsByExtension;
    FileFormatVector m_rawFormats;

    StringUtils::StringVec m_readFormatNames;
    StringUtils::StringVec m_readFormatExtensions;
    StringUtils::StringVec m_bakeFormatNames;
    StringUtils::StringVec m_bakeFormatExtensions;
    StringUtils::StringVec m_writeFormatNames;
    StringUtils::StringVec m_writeFormatExtensions;
};

// Registry Builders.
FileFormat * CreateFileFormat3DL();
FileFormat * CreateFileFormatCC();
FileFormat * CreateFileFormatCCC();
FileFormat * CreateFileFormatCDL();
FileFormat * CreateFileFormatCLF();
FileFormat * CreateFileFormatCSP();
FileFormat * CreateFileFormatDiscreet1DL();
FileFormat * CreateFileFormatHDL();
FileFormat * CreateFileFormatICC();
FileFormat * CreateFileFormatIridasCube();
FileFormat * CreateFileFormatIridasItx();
FileFormat * CreateFileFormatIridasLook();
FileFormat * CreateFileFormatPandora();
FileFormat * CreateFileFormatResolveCube();
FileFormat * CreateFileFormatSpi1D();
FileFormat * CreateFileFormatSpi3D();
FileFormat * CreateFileFormatSpiMtx();
FileFormat * CreateFileFormatTruelight();
FileFormat * CreateFileFormatVF();

static constexpr char FILEFORMAT_CLF[] = "Academy/ASC Common LUT Format";
static constexpr char FILEFORMAT_CTF[] = "Color Transform Format";

} // namespace OCIO_NAMESPACE

#endif
