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
#include "Mutex.h"
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
        return FormatRegistry::GetInstance().getNumFormats(FILE_FORMAT_READ);
    }
    
    const char * FileTransform::getFormatNameByIndex(int index)
    {
        FileFormat* format = FormatRegistry::GetInstance().getFormatByIndex(FILE_FORMAT_READ, index);
        if(format)
        {
            return format->GetName().c_str();
        }
        
        return "";
    }
    
    std::ostream& operator<< (std::ostream& os, const FileTransform& t)
    {
        os << "<FileTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << "interpolation=" << InterpolationToString(t.getInterpolation()) << ", ";
        os << "src='" << t.getSrc() << "'";
        os << "cccid='" << t.getCCCId() << "'";
        os << ">\n";
        
        return os;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    // NOTE: You must be mindful when editing this function.
    //       FormatRegistry::GetInstance().registerFileFormat(...) is called
    //       during static initialization, so this full code path
    //       must be resiliant to the static initialization order 'fiasco'
    //
    //       See
    //       http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.14
    //       http://stackoverflow.com/questions/335369/finding-c-static-initialization-order-problems
    //       for more info.
    //
    //       In our case, we avoid it by only have POD types as statics,
    //       and constructing on first use.  If we were to do something
    //       more clever, such as using a static mutex to guard GetInstance(),
    //       this would be fragile to compilation unit ordering.
    
    FormatRegistry & FormatRegistry::GetInstance()
    {
        static FormatRegistry* g_formatRegistry = new FormatRegistry();
        return *g_formatRegistry;
    }
    
    FormatRegistry::FormatRegistry()
    {
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
    
    int FormatRegistry::getNumFormats() const
    {
        return static_cast<int>(m_formatsByName.size());
    }
    
    FileFormat* FormatRegistry::getFormatByIndex(int index) const
    {
        if(index>=0 && index<getNumFormats())
        {
            FileFormatMap::const_iterator iter = m_formatsByName.begin();
            for(int i=0; i<index; ++i) ++iter;
            return iter->second;
        }
        
        return NULL;
    }
    
    
    int FormatRegistry::getNumFormats(FileFormatFeature feature) const
    {
        int count = 0;
        for(FileFormatMap::const_iterator iter = m_formatsByName.begin();
            iter != m_formatsByName.end();
            ++iter)
        {
            if(iter->second->Supports(feature))
            {
                count++;
            }
        }
        
        return count;
    }
    
    FileFormat* FormatRegistry::getFormatByIndex(FileFormatFeature feature, int index) const
    {
        int count = 0;
        for(FileFormatMap::const_iterator iter = m_formatsByName.begin();
            iter != m_formatsByName.end();
            ++iter)
        {
            if(iter->second->Supports(feature))
            {
                if(count == index)
                {
                    return iter->second;
                }
                
                count++;
            }
        }
        
        return NULL;
    }
    
    void FormatRegistry::registerFileFormat(FileFormat* format)
    {
        std::string formatName = pystring::lower(format->GetName());
        if(getFileFormatByName(formatName))
        {
            std::ostringstream os;
            os << "Cannot register multiple file formats named, '";
            os << formatName << "'.";
            throw Exception(os.str().c_str());
        }
        
        m_formatsByName[formatName] = format;
        
        // For now, dont worry if multiple formats register the same extension
        // TODO: keep track of all of em! (make the value a vector)
        m_formatsByExtension[pystring::lower(format->GetExtension())] = format;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    FileFormat::~FileFormat()
    {
    
    }
    
    void FileFormat::Write(TransformData & /*data*/,
        std::ostream & /*ostream*/) const
    {
        std::ostringstream os;
        os << "Format " << GetName() << " does not support writing.";
        throw Exception(os.str().c_str());
    }
    
    namespace
    {
        typedef std::pair<FileFormat*, CachedFileRcPtr> FileCachePair;
        typedef std::map<std::string, FileCachePair> FileCacheMap;
        
        // TODO: provide mechanism for flushing cache
        
        FileCacheMap g_fileCache;
        Mutex g_fileCacheLock;
        
        // Get the FileFormat, CachedFilePtr
        // or throw an exception.
        
        FileCachePair GetFile(const std::string & filepath)
        {
            // Acquire fileCache mutex
            AutoMutex lock(g_fileCacheLock);
            
            FileCacheMap::iterator iter = g_fileCache.find(filepath);
            if(iter != g_fileCache.end())
            {
                return iter->second;
            }
            
            // We did not find the file in the cache; let's read it.
            
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
            std::string extension = GetExtension(filepath);
            FileFormat * primaryFormat = FormatRegistry::GetInstance().getFileFormatForExtension(extension);
            
            if(primaryFormat)
            {
                try
                {
                    CachedFileRcPtr cachedFile = primaryFormat->Load(filestream);
                    
                    // Add the result to our cache, return it.
                    FileCachePair pair = std::make_pair(primaryFormat, cachedFile);
                    g_fileCache[filepath] = pair;
                    
                    return pair;
                }
                catch(std::exception & e)
                {
                    primaryErrorText = e.what();
                    filestream.seekg( std::ifstream::beg );
                }
            }
            
            // If this fails, try all other formats
            FormatRegistry & formats = FormatRegistry::GetInstance();
            
            for(int findex = 0; findex<formats.getNumFormats(); ++findex)
            {
                FileFormat * localFormat = formats.getFormatByIndex(findex);
                
                // Dont bother trying the primaryFormat twice.
                if(localFormat == primaryFormat) continue;
                
                try
                {
                    CachedFileRcPtr cachedFile = localFormat->Load(filestream);
                    
                    // Add the result to our cache, return it.
                    FileCachePair pair = std::make_pair(localFormat, cachedFile);
                    g_fileCache[filepath] = pair;
                    return pair;
                }
                catch(std::exception & e)
                {
                    filestream.seekg( std::ifstream::beg );
                }
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
        
        FileCachePair cachePair = GetFile(filepath);
        FileFormat* format = cachePair.first;
        CachedFileRcPtr cachedFile = cachePair.second;
        
        format->BuildFileOps(ops,
                             config, context,
                             cachedFile, fileTransform,
                             dir);
    }
}
OCIO_NAMESPACE_EXIT
