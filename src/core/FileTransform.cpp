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

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Logging.h"
#include "Mutex.h"
#include "NoOps.h"
#include "PathUtils.h"
#include "pystring/pystring.h"

#include <fstream>
#include <map>
#include <sstream>

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
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            src_ = rhs.src_;
            cccid_ = rhs.cccid_;
            interp_ = rhs.interp_;
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
        *m_impl = *rhs.m_impl;
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
        return FormatRegistry::GetInstance().getNumFormats(FORMAT_CAPABILITY_READ);
    }
    
    const char * FileTransform::getFormatNameByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatNameByIndex(FORMAT_CAPABILITY_READ, index);
    }
    
    const char * FileTransform::getFormatExtensionByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatExtensionByIndex(FORMAT_CAPABILITY_READ, index);
    }
    
    std::ostream& operator<< (std::ostream& os, const FileTransform& t)
    {
        os << "<FileTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << "interpolation=" << InterpolationToString(t.getInterpolation()) << ", ";
        os << "src='" << t.getSrc() << "', ";
        os << "cccid='" << t.getCCCId() << "'";
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
        registerFileFormat(CreateFileFormatCCC());
        registerFileFormat(CreateFileFormatCC());
        registerFileFormat(CreateFileFormatCSP());
        registerFileFormat(CreateFileFormatHDL());
        registerFileFormat(CreateFileFormatIridasItx());
        registerFileFormat(CreateFileFormatIridasCube());
        registerFileFormat(CreateFileFormatIridasLook());
        registerFileFormat(CreateFileFormatPandora());
        registerFileFormat(CreateFileFormatSpi1D());
        registerFileFormat(CreateFileFormatSpi3D());
        registerFileFormat(CreateFileFormatSpiMtx());
        registerFileFormat(CreateFileFormatTruelight());
        registerFileFormat(CreateFileFormatVF());
    }
    
    FormatRegistry::~FormatRegistry()
    {
    }
    
    FileFormat* FormatRegistry::getFileFormatByName(const std::string & name) const
    {
        FileFormatMap::const_iterator iter = m_formatsByName.find(
            pystring::lower(name));
        if(iter != m_formatsByName.end())
            return iter->second;
        return NULL;
    }
    
    FileFormat* FormatRegistry::getFileFormatForExtension(const std::string & extension) const
    {
        FileFormatMap::const_iterator iter = m_formatsByExtension.find(
            pystring::lower(extension));
        if(iter != m_formatsByExtension.end())
            return iter->second;
        return NULL;
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
                os << "A file format does not define either reading or writing.";
                throw Exception(os.str().c_str());
            }
            
            if(getFileFormatByName(formatInfoVec[i].name))
            {
                std::ostringstream os;
                os << "Cannot register multiple file formats named, '";
                os << formatInfoVec[i].name << "'.";
                throw Exception(os.str().c_str());
            }
            
            m_formatsByName[formatInfoVec[i].name] = format;
            
            // For now, dont worry if multiple formats register the same extension
            // TODO: keep track of all of em! (make the value a vector)
            m_formatsByExtension[formatInfoVec[i].extension] = format;
            
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
    
    const char * FormatRegistry::getFormatNameByIndex(int capability, int index) const
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
            if(index<0 || index>=static_cast<int>(m_readFormatNames.size()))
            {
                return "";
            }
            return m_writeFormatNames[index].c_str();
        }
        return "";
    }
    
    const char * FormatRegistry::getFormatExtensionByIndex(int capability, int index) const
    {
        if(capability == FORMAT_CAPABILITY_READ)
        {
            if(index<0 || index>=static_cast<int>(m_readFormatExtensions.size()))
            {
                return "";
            }
            return m_readFormatExtensions[index].c_str();
        }
        else if(capability == FORMAT_CAPABILITY_WRITE)
        {
            if(index<0 || index>=static_cast<int>(m_writeFormatExtensions.size()))
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
            
            // Open the filePath
            std::ifstream filestream;
            filestream.open(filepath.c_str(), std::ios_base::in);
            if (!filestream.good())
            {
                std::ostringstream os;
                os << "The specified FileTransform srcfile, '";
                os << filepath <<"', could not be opened. ";
                os << "Please confirm the file exists with appropriate read";
                os << " permissions.";
                throw Exception(os.str().c_str());
            }
            
            // Try the initial format.
            std::string primaryErrorText;
            std::string root, extension;
            pystring::os::path::splitext(root, extension, filepath);
            extension = pystring::replace(extension,".","",1); // remove the leading '.'
            
            FormatRegistry & formatRegistry = FormatRegistry::GetInstance();
            
            FileFormat * primaryFormat = 
                formatRegistry.getFileFormatForExtension(extension);
            if(primaryFormat)
            {
                try
                {
                    CachedFileRcPtr cachedFile = primaryFormat->Read(filestream);
                    
                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Loaded primary format ";
                        os << primaryFormat->getName();
                        LogDebug(os.str());
                    }
                    
                    returnFormat = primaryFormat;
                    returnCachedFile = cachedFile;
                    return;
                }
                catch(std::exception & e)
                {
                    primaryErrorText = e.what();
                    
                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Failed primary format ";
                        os << primaryFormat->getName();
                        os << ":  " << e.what();
                        LogDebug(os.str());
                    }
                }
            }
            
            filestream.clear();
            filestream.seekg( std::ifstream::beg );
            
            // If this fails, try all other formats
            CachedFileRcPtr cachedFile;
            FileFormat * altFormat = NULL;
            
            for(int findex = 0;
                findex<formatRegistry.getNumRawFormats();
                ++findex)
            {
                altFormat = formatRegistry.getRawFormatByIndex(findex);
                
                // Dont bother trying the primaryFormat twice.
                if(altFormat == primaryFormat) continue;
                
                try
                {
                    cachedFile = altFormat->Read(filestream);
                    
                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Loaded alt format ";
                        os << altFormat->getName();
                        LogDebug(os.str());
                    }
                    
                    returnFormat = altFormat;
                    returnCachedFile = cachedFile;
                    return;
                }
                catch(std::exception & e)
                {
                    if(IsDebugLoggingEnabled())
                    {
                        std::ostringstream os;
                        os << "    Failed alt format ";
                        os << altFormat->getName();
                        os << ":  " << e.what();
                        LogDebug(os.str());
                    }
                }
                
                filestream.clear();
                filestream.seekg( std::ifstream::beg );
            }
            
            // No formats succeeded. Error out with a sensible message.
            if(primaryFormat)
            {
                std::ostringstream os;
                os << "The specified transform file '";
                os << filepath <<"' could not be loaded. ";
                os << primaryErrorText;
                
                throw Exception(os.str().c_str());
            }
            else
            {
                std::ostringstream os;
                os << "The specified transform file '";
                os << filepath <<"' does not appear to be a valid, known LUT file format.";
                throw Exception(os.str().c_str());
            }
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
        
        void GetCachedFileAndFormat(
            FileFormat * & format, CachedFileRcPtr & cachedFile,
            const std::string & filepath)
        {
            // Load the file cache ptr from the global map
            FileCacheResultPtr result;
            {
                AutoMutex lock(g_fileCacheLock);
                FileCacheMap::iterator iter = g_fileCache.find(filepath);
                if(iter != g_fileCache.end())
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
            if(!result->ready)
            {
                result->ready = true;
                result->error = false;
                
                try
                {
                    LoadFileUncached(result->format,
                                     result->cachedFile,
                                     filepath);
                }
                catch(std::exception & e)
                {
                    result->error = true;
                    result->exceptionText = e.what();
                }
                catch(...)
                {
                    result->error = true;
                    std::ostringstream os;
                    os << "An unknown error occurred in LoadFileUncached, ";
                    os << filepath;
                    result->exceptionText = os.str();
                }
            }
            
            if(result->error)
            {
                throw Exception(result->exceptionText.c_str());
            }
            else
            {
                format = result->format;
                cachedFile = result->cachedFile;
            }
        }
    } // namespace
    
    void ClearFileTransformCaches()
    {
        AutoMutex lock(g_fileCacheLock);
        g_fileCache.clear();
    }
    
    void BuildFileOps(OpRcPtrVec & ops,
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
        CreateFileNoOp(ops, filepath);
        
        FileFormat* format = NULL;
        CachedFileRcPtr cachedFile;
        
        GetCachedFileAndFormat(format, cachedFile, filepath);
        if(!format)
        {
            std::ostringstream os;
            os << "The specified file load ";
            os << filepath << " appeared to succeed, but no format ";
            os << "was returned.";
            throw Exception(os.str().c_str());
        }
        
        if(!cachedFile.get())
        {
            std::ostringstream os;
            os << "The specified file load ";
            os << filepath << " appeared to succeed, but no cachedFile ";
            os << "was returned.";
            throw Exception(os.str().c_str());
        }
        
        format->BuildFileOps(ops,
                             config, context,
                             cachedFile, fileTransform,
                             dir);
    }
}
OCIO_NAMESPACE_EXIT
