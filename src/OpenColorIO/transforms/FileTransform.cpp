/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Logging.h"
#include "Mutex.h"
#include "ops/NoOp/NoOps.h"
#include "PathUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    FileTransformRcPtr FileTransform::Create()
    {
        return FileTransformRcPtr(new FileTransform(), &deleter);
    }
    
    void FileTransform::deleter(FileTransform* t)
    {
        delete t;
    }
    
    
    class FileTransform::Impl
    {
    public:
        TransformDirection dir_;
        std::string src_;
        std::string cccid_;
        Interpolation interp_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            interp_(INTERP_UNKNOWN)
        { }

        Impl(const Impl &) = delete;

        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;
                src_ = rhs.src_;
                cccid_ = rhs.cccid_;
                interp_ = rhs.interp_;
            }
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    FileTransform::FileTransform()
        : m_impl(new FileTransform::Impl)
    {
    }
    
    TransformRcPtr FileTransform::createEditableCopy() const
    {
        FileTransformRcPtr transform = FileTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    FileTransform::~FileTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    FileTransform& FileTransform::operator= (const FileTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection FileTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void FileTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }

    void FileTransform::validate() const
    {
        Transform::validate();

        if (getImpl()->src_.empty())
        {
            throw Exception("FileTransform: empty file path");
        }
    }

    const char * FileTransform::getSrc() const
    {
        return getImpl()->src_.c_str();
    }
    
    void FileTransform::setSrc(const char * src)
    {
        getImpl()->src_ = src;
    }
    
    const char * FileTransform::getCCCId() const
    {
        return getImpl()->cccid_.c_str();
    }
    
    void FileTransform::setCCCId(const char * cccid)
    {
        getImpl()->cccid_ = cccid;
    }
    
    Interpolation FileTransform::getInterpolation() const
    {
        return getImpl()->interp_;
    }
    
    void FileTransform::setInterpolation(Interpolation interp)
    {
        getImpl()->interp_ = interp;
    }
    
    int FileTransform::getNumFormats()
    {
        return FormatRegistry::GetInstance().getNumFormats(
            FORMAT_CAPABILITY_READ);
    }
    
    const char * FileTransform::getFormatNameByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatNameByIndex(
            FORMAT_CAPABILITY_READ, index);
    }
    
    const char * FileTransform::getFormatExtensionByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatExtensionByIndex(
            FORMAT_CAPABILITY_READ, index);
    }
    
    std::ostream& operator<< (std::ostream& os, const FileTransform& t)
    {
        os << "<FileTransform ";
        os << "direction=";
        os << TransformDirectionToString(t.getDirection()) << ", ";
        os << "interpolation=" << InterpolationToString(t.getInterpolation());
        os << ", src=" << t.getSrc() << ", ";
        os << "cccid=" << t.getCCCId();
        os << ">";
        
        return os;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    // NOTE: You must be mindful when editing this function.
    //       to be resiliant to the static initialization order 'fiasco'
    //
    //       See
    //       http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.14
    //       http://stackoverflow.com/questions/335369/finding-c-static-initialization-order-problems
    //       for more info.
    
    namespace
    {
        FormatRegistry* g_formatRegistry = NULL;
        Mutex g_formatRegistryLock;
    }
    
    FormatRegistry & FormatRegistry::GetInstance()
    {
        AutoMutex lock(g_formatRegistryLock);
        
        if(!g_formatRegistry)
        {
            g_formatRegistry = new FormatRegistry();
        }
        
        return *g_formatRegistry;
    }
    
    FormatRegistry::FormatRegistry()
    {
        registerFileFormat(CreateFileFormat3DL());
        registerFileFormat(CreateFileFormatCC());
        registerFileFormat(CreateFileFormatCCC());
        registerFileFormat(CreateFileFormatCDL());
        registerFileFormat(CreateFileFormatCLF());
        registerFileFormat(CreateFileFormatCSP());
        registerFileFormat(CreateFileFormatDiscreet1DL());
        registerFileFormat(CreateFileFormatHDL());
        registerFileFormat(CreateFileFormatICC());
        registerFileFormat(CreateFileFormatIridasCube());
        registerFileFormat(CreateFileFormatIridasItx());
        registerFileFormat(CreateFileFormatIridasLook());
        registerFileFormat(CreateFileFormatPandora());
        registerFileFormat(CreateFileFormatResolveCube());
        registerFileFormat(CreateFileFormatSpi1D());
        registerFileFormat(CreateFileFormatSpi3D());
        registerFileFormat(CreateFileFormatSpiMtx());
        registerFileFormat(CreateFileFormatTruelight());
        registerFileFormat(CreateFileFormatVF());
    }
    
    FormatRegistry::~FormatRegistry()
    {
    }
    
    FileFormat* FormatRegistry::getFileFormatByName(
        const std::string & name) const
    {
        FileFormatMap::const_iterator iter = m_formatsByName.find(
            pystring::lower(name));
        if(iter != m_formatsByName.end())
            return iter->second;
        return NULL;
    }
    
    void FormatRegistry::getFileFormatForExtension(
        const std::string & extension,
        FileFormatVector & possibleFormats) const
    {
        FileFormatVectorMap::const_iterator iter = m_formatsByExtension.find(
            pystring::lower(extension));
        if(iter != m_formatsByExtension.end())
            possibleFormats = iter->second;
    }
    
    void FormatRegistry::registerFileFormat(FileFormat* format)
    {
        FormatInfoVec formatInfoVec;
        format->GetFormatInfo(formatInfoVec);
        
        if(formatInfoVec.empty())
        {
            std::ostringstream os;
            os << "FileFormat Registry error. ";
            os << "A file format did not provide the required format info.";
            throw Exception(os.str().c_str());
        }
        
        for(unsigned int i=0; i<formatInfoVec.size(); ++i)
        {
            if(formatInfoVec[i].capabilities == FORMAT_CAPABILITY_NONE)
            {
                std::ostringstream os;
                os << "FileFormat Registry error. ";
                os << "A file format does not define either";
                os << " reading or writing.";
                throw Exception(os.str().c_str());
            }
            
            if(getFileFormatByName(formatInfoVec[i].name))
            {
                std::ostringstream os;
                os << "Cannot register multiple file formats named, '";
                os << formatInfoVec[i].name << "'.";
                throw Exception(os.str().c_str());
            }
            
            m_formatsByName[pystring::lower(formatInfoVec[i].name)] = format;
            
            m_formatsByExtension[formatInfoVec[i].extension].push_back(format);
            
            if(formatInfoVec[i].capabilities & FORMAT_CAPABILITY_READ)
            {
                m_readFormatNames.push_back(formatInfoVec[i].name);
                m_readFormatExtensions.push_back(formatInfoVec[i].extension);
            }
            
            if(formatInfoVec[i].capabilities & FORMAT_CAPABILITY_WRITE)
            {
                m_writeFormatNames.push_back(formatInfoVec[i].name);
                m_writeFormatExtensions.push_back(formatInfoVec[i].extension);
            }
        }
        
        m_rawFormats.push_back(format);
    }
    
    int FormatRegistry::getNumRawFormats() const
    {
        return static_cast<int>(m_rawFormats.size());
    }
    
    FileFormat* FormatRegistry::getRawFormatByIndex(int index) const
    {
        if(index<0 || index>=getNumRawFormats())
        {
            return NULL;
        }
        
        return m_rawFormats[index];
    }
    
    int FormatRegistry::getNumFormats(int capability) const
    {
        if(capability == FORMAT_CAPABILITY_READ)
        {
            return static_cast<int>(m_readFormatNames.size());
        }
        else if(capability == FORMAT_CAPABILITY_WRITE)
        {
            return static_cast<int>(m_writeFormatNames.size());
        }
        return 0;
    }
    
    const char * FormatRegistry::getFormatNameByIndex(
        int capability, int index) const
    {
        if(capability == FORMAT_CAPABILITY_READ)
        {
            if(index<0 || index>=static_cast<int>(m_readFormatNames.size()))
            {
                return "";
            }
            return m_readFormatNames[index].c_str();
        }
        else if(capability == FORMAT_CAPABILITY_WRITE)
        {
            if(index<0 || index>=static_cast<int>(m_writeFormatNames.size()))
            {
                return "";
            }
            return m_writeFormatNames[index].c_str();
        }
        return "";
    }
    
    const char * FormatRegistry::getFormatExtensionByIndex(
        int capability, int index) const
    {
        if(capability == FORMAT_CAPABILITY_READ)
        {
            if(index<0 
                || index>=static_cast<int>(m_readFormatExtensions.size()))
            {
                return "";
            }
            return m_readFormatExtensions[index].c_str();
        }
        else if(capability == FORMAT_CAPABILITY_WRITE)
        {
            if(index<0 
                || index>=static_cast<int>(m_writeFormatExtensions.size()))
            {
                return "";
            }
            return m_writeFormatExtensions[index].c_str();
        }
        return "";
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    FileFormat::~FileFormat()
    {
    
    }
    
    std::string FileFormat::getName() const
    {
        FormatInfoVec infoVec;
        GetFormatInfo(infoVec);
        if(infoVec.size()>0)
        {
            return infoVec[0].name;
        }
        return "Unknown Format";
    }
        
    
    
    void FileFormat::Write(const Baker & /*baker*/,
                           const std::string & formatName,
                           std::ostream & /*ostream*/) const
    {
        std::ostringstream os;
        os << "Format " << formatName << " does not support writing.";
        throw Exception(os.str().c_str());
    }
    
    namespace
    {
    
        void LoadFileUncached(FileFormat * & returnFormat,
            CachedFileRcPtr & returnCachedFile,
            const std::string & filepath)
        {
            returnFormat = NULL;
            
            {
                std::ostringstream os;
                os << "Opening " << filepath;
                LogDebug(os.str());
            }
            
            // Try the initial format.
            std::string primaryErrorText;
            std::string root, extension;
            pystring::os::path::splitext(root, extension, filepath);
            // remove the leading '.'
            extension = pystring::replace(extension,".","",1);

            FormatRegistry & formatRegistry = FormatRegistry::GetInstance();
            
            FileFormatVector possibleFormats;
            formatRegistry.getFileFormatForExtension(
                extension, possibleFormats);
            FileFormatVector::const_iterator endFormat = possibleFormats.end();
            FileFormatVector::const_iterator itFormat =
                possibleFormats.begin();
            while(itFormat != endFormat)
            {

                FileFormat * tryFormat = *itFormat;
                std::ifstream filestream;
                try
                {
                    // Open the filePath
                    filestream.open(
                        filepath.c_str(),
                        tryFormat->IsBinary()
                            ? std::ios_base::binary : std::ios_base::in);
                    if (!filestream.good())
                    {
                        std::ostringstream os;
                        os << "The specified FileTransform srcfile, '";
                        os << filepath << "', could not be opened. ";
                        os << "Please confirm the file exists with ";
                        os << "appropriate read permissions.";
                        throw Exception(os.str().c_str());
                    }

                    CachedFileRcPtr cachedFile = tryFormat->Read(
                        filestream,
                        filepath);
                    
                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Loaded primary format ";
                        os << tryFormat->getName();
                        LogDebug(os.str());
                    }
                    
                    returnFormat = tryFormat;
                    returnCachedFile = cachedFile;
                    filestream.close();
                    return;
                }
                catch(std::exception & e)
                {
                    if (filestream.is_open())
                    {
                        filestream.close();
                    }

                    primaryErrorText += tryFormat->getName();
                    primaryErrorText += " failed with: '";
                    primaryErrorText = e.what();
                    primaryErrorText += "'.  ";

                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Failed primary format ";
                        os << tryFormat->getName();
                        os << ":  " << e.what();
                        LogDebug(os.str());
                    }
                }
                ++itFormat;
            }
            
            // If this fails, try all other formats
            CachedFileRcPtr cachedFile;
            FileFormat * altFormat = NULL;
            
            for(int findex = 0;
                findex<formatRegistry.getNumRawFormats();
                ++findex)
            {
                altFormat = formatRegistry.getRawFormatByIndex(findex);
                
                // Do not try primary formats twice.
                FileFormatVector::const_iterator itAlt = std::find(
                    possibleFormats.begin(), possibleFormats.end(), altFormat);
                if(itAlt != endFormat)
                    continue;
                
                std::ifstream filestream;
                try
                {
                    filestream.open(filepath.c_str(), altFormat->IsBinary()
                        ? std::ios_base::binary : std::ios_base::in);
                    if (!filestream.good())
                    {
                        std::ostringstream os;
                        os << "The specified FileTransform srcfile, '";
                        os << filepath << "', could not be opened. ";
                        os << "Please confirm the file exists with ";
                        os << "appropriate read";
                        os << " permissions.";
                        throw Exception(os.str().c_str());
                    }

                    cachedFile = altFormat->Read(filestream, filepath);
                    
                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Loaded alt format ";
                        os << altFormat->getName();
                        LogDebug(os.str());
                    }
                    
                    returnFormat = altFormat;
                    returnCachedFile = cachedFile;
                    filestream.close();
                    return;
                }
                catch(std::exception & e)
                {
                    if (filestream.is_open())
                    {
                        filestream.close();
                    }

                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Failed alt format ";
                        os << altFormat->getName();
                        os << ":  " << e.what();
                        LogDebug(os.str());
                    }
                }
            }
            
            // No formats succeeded. Error out with a sensible message.
            std::ostringstream os;
            os << "The specified transform file '";
            os << filepath << "' could not be loaded.  ";

            if (IsDebugLoggingEnabled())
            {
                os << "(Refer to debug log for errors from all formats). ";
            }
            else
            {
                os << "(Enable debug log for errors from all formats). ";
            }

            if(!possibleFormats.empty())
            {
                os << "All formats have been tried including ";
                os << "formats registered for the given extension. ";
                os << "These formats gave the following errors: ";
                os << primaryErrorText;
            }

            throw Exception(os.str().c_str());
        }
        
        // We mutex both the main map and each item individually, so that
        // the potentially slow file access wont block other lookups to already
        // existing items. (Loads of the *same* file will mutually block though)
        
        struct FileCacheResult
        {
            Mutex mutex;
            FileFormat * format;
            bool ready;
            bool error;
            CachedFileRcPtr cachedFile;
            std::string exceptionText;
            
            FileCacheResult():
                format(NULL),
                ready(false),
                error(false)
            {}
        };
        
        typedef OCIO_SHARED_PTR<FileCacheResult> FileCacheResultPtr;
        typedef std::map<std::string, FileCacheResultPtr> FileCacheMap;
        
        FileCacheMap g_fileCache;
        Mutex g_fileCacheLock;
        
    } // namespace

    void GetCachedFileAndFormat(FileFormat * & format,
                                CachedFileRcPtr & cachedFile,
                                const std::string & filepath)
    {
        // Load the file cache ptr from the global map
        FileCacheResultPtr result;
        {
            AutoMutex lock(g_fileCacheLock);
            FileCacheMap::iterator iter = g_fileCache.find(filepath);
            if (iter != g_fileCache.end())
            {
                result = iter->second;
            }
            else
            {
                result = FileCacheResultPtr(new FileCacheResult);
                g_fileCache[filepath] = result;
            }
        }

        // If this file has already been loaded, return
        // the result immediately

        AutoMutex lock(result->mutex);
        if (!result->ready)
        {
            result->ready = true;
            result->error = false;

            try
            {
                LoadFileUncached(result->format,
                    result->cachedFile,
                    filepath);
            }
            catch (std::exception & e)
            {
                result->error = true;
                result->exceptionText = e.what();
            }
            catch (...)
            {
                result->error = true;
                std::ostringstream os;
                os << "An unknown error occurred in LoadFileUncached, ";
                os << filepath;
                result->exceptionText = os.str();
            }
        }

        if (result->error)
        {
            throw Exception(result->exceptionText.c_str());
        }
        else
        {
            format = result->format;
            cachedFile = result->cachedFile;
        }

        if (!format)
        {
            std::ostringstream os;
            os << "The specified file load ";
            os << filepath << " appeared to succeed, but no format ";
            os << "was returned.";
            throw Exception(os.str().c_str());
        }

        if (!cachedFile.get())
        {
            std::ostringstream os;
            os << "The specified file load ";
            os << filepath << " appeared to succeed, but no cachedFile ";
            os << "was returned.";
            throw Exception(os.str().c_str());
        }
    }

    void ClearFileTransformCaches()
    {
        AutoMutex lock(g_fileCacheLock);
        g_fileCache.clear();
    }
    
    void BuildFileTransformOps(OpRcPtrVec & ops,
                               const Config& config,
                               const ConstContextRcPtr & context,
                               const FileTransform& fileTransform,
                               TransformDirection dir)
    {
        std::string src = fileTransform.getSrc();
        if(src.empty())
        {
            std::ostringstream os;
            os << "The transform file has not been specified.";
            throw Exception(os.str().c_str());
        }
        
        std::string filepath = context->resolveFileLocation(src.c_str());

        // Verify the recursion is valid, FileNoOp is added for each file.
        for (ConstOpRcPtr&& op : ops)
        {
            ConstOpDataRcPtr data = op->data();
            auto fileData = DynamicPtrCast<const FileNoOpData>(data);
            if (fileData)
            {
                // Error if file is still being loaded and is the same as the
                // one about to be loaded.
                if (!fileData->getComplete() &&
                    Platform::Strcasecmp(fileData->getPath().c_str(),
                                         filepath.c_str()) == 0)
                {
                    std::ostringstream os;
                    os << "Reference to: " << filepath;
                    os << " is creating a recursion.";

                    throw Exception(os.str().c_str());
                }
            }
        }

        FileFormat* format = NULL;
        CachedFileRcPtr cachedFile;

        try
        {
            GetCachedFileAndFormat(format, cachedFile, filepath);
            // Add FileNoOp and keep track of it.
            CreateFileNoOp(ops, filepath);
            ConstOpRcPtr fileNoOp = ops.back();

            // CTF implementation of FileFormat::BuildFileOps might call
            // BuildFileTransformOps for References.
            format->BuildFileOps(ops,
                                 config, context,
                                 cachedFile, fileTransform,
                                 dir);

            // File has been loaded completely. It may now be referenced again.
            ConstOpDataRcPtr data = fileNoOp->data();
            auto fileData = DynamicPtrCast<const FileNoOpData>(data);
            if (fileData)
            {
                fileData->setComplete();
            }
        }
        catch (Exception & e)
        {
            std::ostringstream err;
            err << "The transform file: " << filepath;
            err << " failed while loading ops with this error: ";
            err << e.what();
            throw Exception(err.str().c_str());
        }
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include <algorithm>
#include "unittest.h"
#include "UnitTestUtils.h"

OIIO_ADD_TEST(FileTransform, LoadFileOK)
{
    OCIO::ConstProcessorRcPtr proc;

    // Discreet 1D LUT.
    const std::string discreetLut("logtolin_8to8.lut");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(discreetLut));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // Houdini 1D LUT.
    const std::string houdiniLut("houdini.lut");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(houdiniLut));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // Discreet 3D LUT file.
    const std::string discree3DtLut("discreet-3d-lut.3dl");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(discree3DtLut));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // 3D LUT file.
    const std::string crosstalk3DtLut("crosstalk.3dl");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(crosstalk3DtLut));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // Lustre 3D LUT file.
    const std::string lustre3DtLut("lustre_33x33x33.3dl");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(lustre3DtLut));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // Autodesk color transform format.
    const std::string ctfTransform("matrix_example4x4.ctf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfTransform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // Academy/ASC common LUT format.
    const std::string clfRangeTransform("range.clf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfRangeTransform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // Academy/ASC common LUT format.
    const std::string clfMatTransform("matrix_example.clf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfMatTransform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    // Test other types of CLF/CTF elements.
    const std::string clfCdlTransform("cdl_clamp_fwd.clf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfCdlTransform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string clfLut1Transform("lut1d_example.clf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfLut1Transform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string clfLut3Transform("lut3d_2x2x2_32f_32f.clf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfLut3Transform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctFFfTransform("ff_aces_redmod.ctf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctFFfTransform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfGammaTransform("gamma_test1.ctf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfGammaTransform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfLogTransform("log_logtolin.ctf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfLogTransform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfInvLut1Transform("lut1d_inv.ctf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfInvLut1Transform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfInvLut3Transform("lut3d_example_Inv.ctf");
    OIIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfInvLut3Transform));
    OIIO_CHECK_ASSERT(!proc->isNoOp());
}

OIIO_ADD_TEST(FileTransform, LoadFileFail)
{
    // Legacy Lustre 1D LUT files. Similar to supported formats but actually
    // are different formats.
    // Test that they are correctly recognized as unreadable.
    {
        const std::string lustreOldLut("legacy_slog_to_log_v3_lustre.lut");
        OIIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(lustreOldLut),
                              OCIO::Exception, "could not be loaded");
    }

    {
        const std::string lustreOldLut("legacy_flmlk_desat.lut");
        OIIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(lustreOldLut),
                              OCIO::Exception, "could not be loaded");
    }

    // Invalid file.
    const std::string unKnown("error_unknown_format.txt");
    OIIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(unKnown),
                          OCIO::Exception, "could not be loaded");

    // Missing file.
    const std::string missing("missing.file");
    OIIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(missing),
                          OCIO::Exception, "could not be located");
}

bool FormatNameFoundByExtension(const std::string & extension, const std::string & formatName)
{
    bool foundIt = false;
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();

    OCIO::FileFormatVector possibleFormats;
    formatRegistry.getFileFormatForExtension(extension, possibleFormats);
    OCIO::FileFormatVector::const_iterator endFormat = possibleFormats.end();
    OCIO::FileFormatVector::const_iterator itFormat = possibleFormats.begin();
    while (itFormat != endFormat && !foundIt)
    {
        OCIO::FileFormat * tryFormat = *itFormat;

        if (formatName == tryFormat->getName())
            foundIt = true;

        ++itFormat;
    }
    return foundIt;
}

bool FormatExtensionFoundByName(const std::string & extension, const std::string & formatName)
{
    bool foundIt = false;
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();

    OCIO::FileFormat * fileFormat = formatRegistry.getFileFormatByName(formatName);
    if (fileFormat)
    {
        OCIO::FormatInfoVec formatInfoVec;
        fileFormat->GetFormatInfo(formatInfoVec);

        for (unsigned int i = 0; i < formatInfoVec.size() && !foundIt; ++i)
        {
            if (extension == formatInfoVec[i].extension)
                foundIt = true;

        }
    }
    return foundIt;
}

OIIO_ADD_TEST(FileTransform, AllFormats)
{
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();
    OIIO_CHECK_EQUAL(19, formatRegistry.getNumRawFormats());
    OIIO_CHECK_EQUAL(23, formatRegistry.getNumFormats(OCIO::FORMAT_CAPABILITY_READ));
    OIIO_CHECK_EQUAL(8, formatRegistry.getNumFormats(OCIO::FORMAT_CAPABILITY_WRITE));

    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("3dl", "flame"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("cc", "ColorCorrection"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("ccc", "ColorCorrectionCollection"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("cdl", "ColorDecisionList"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("clf", "Academy/ASC Common LUT Format"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("csp", "cinespace"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("cub", "truelight"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("cube", "iridas_cube"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("cube", "resolve_cube"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("itx", "iridas_itx"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("icc", "International Color Consortium profile"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("look", "iridas_look"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("lut", "houdini"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("lut", "Discreet 1D LUT"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("mga", "pandora_mga"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("spi1d", "spi1d"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("spi3d", "spi3d"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("spimtx", "spimtx"));
    OIIO_CHECK_ASSERT(FormatNameFoundByExtension("vf", "nukevf"));
    // When a FileFormat handles 2 "formats" it declares both names
    // but only exposes one name using the getName() function.
    OIIO_CHECK_ASSERT(!FormatNameFoundByExtension("3dl", "lustre"));
    OIIO_CHECK_ASSERT(!FormatNameFoundByExtension("m3d", "pandora_m3d"));
    OIIO_CHECK_ASSERT(!FormatNameFoundByExtension("icm", "Image Color Matching"));
    OIIO_CHECK_ASSERT(!FormatNameFoundByExtension("ctf", "Color Transform Format"));

    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("3dl", "flame"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("3dl", "lustre"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("cc", "ColorCorrection"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("ccc", "ColorCorrectionCollection"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("cdl", "ColorDecisionList"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("clf", "Academy/ASC Common LUT Format"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("ctf", "Color Transform Format"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("csp", "cinespace"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("cub", "truelight"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("cube", "iridas_cube"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("cube", "resolve_cube"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("itx", "iridas_itx"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("icc", "International Color Consortium profile"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("icm", "International Color Consortium profile"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("look", "iridas_look"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("lut", "houdini"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("lut", "Discreet 1D LUT"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("m3d", "pandora_m3d"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("mga", "pandora_mga"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("spi1d", "spi1d"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("spi3d", "spi3d"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("spimtx", "spimtx"));
    OIIO_CHECK_ASSERT(FormatExtensionFoundByName("vf", "nukevf"));
}

void ValidateFormatByIndex(OCIO::FormatRegistry &reg, int cap)
{
    int numFormat = reg.getNumFormats(cap);

    // Check out of bounds access
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatNameByIndex(cap, -1), ""));
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatExtensionByIndex(cap, -1), ""));
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatNameByIndex(cap, numFormat), ""));
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatExtensionByIndex(cap, numFormat), ""));

    // Check valid access
    for (int i = 0; i < numFormat; ++i) {
        OIIO_CHECK_NE(0, OCIO::Platform::Strcasecmp(reg.getFormatNameByIndex(cap, i), ""));
        OIIO_CHECK_NE(0, OCIO::Platform::Strcasecmp(reg.getFormatExtensionByIndex(cap, i), ""));
    }
}

OIIO_ADD_TEST(FileTransform, FormatByIndex)
{
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();
    ValidateFormatByIndex(formatRegistry, OCIO::FORMAT_CAPABILITY_WRITE);
    ValidateFormatByIndex(formatRegistry, OCIO::FORMAT_CAPABILITY_READ);
}

OIIO_ADD_TEST(FileTransform, Validate)
{
    OCIO::FileTransformRcPtr tr = OCIO::FileTransform::Create();

    tr->setSrc("lut3d_17x17x17_32f_12i.clf");
    OIIO_CHECK_NO_THROW(tr->validate());

    tr->setSrc("");
    OIIO_CHECK_THROW(tr->validate(), OCIO::Exception);
}


#endif
