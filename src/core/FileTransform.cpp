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
    
    FileFormat::~FileFormat()
    {
    
    }
    
    FormatRegistry & GetFormatRegistry()
    {
        static std::vector<FileFormat*> formats;
        return formats;
    }
    
    FileFormat* GetFileFormat(const std::string & name)
    {
        if(name.empty()) return 0x0;
        std::string namelower = pystring::lower(name);
        FormatRegistry & formats = GetFormatRegistry();
        for(unsigned int findex=0; findex<formats.size(); ++findex)
        {
            FileFormat* format = formats[findex];
            std::string name = pystring::lower(format->GetName());
            if(namelower == name)
            {
                return format;
            }
        }
        return 0x0;
    }
    
    FileFormat* GetFileFormatForExtension(const std::string & str)
    {
        if(str.empty()) return 0x0;
        
        std::string extension = pystring::lower(str);
        
        FormatRegistry & formats = GetFormatRegistry();
        
        for(unsigned int findex=0; findex<formats.size(); ++findex)
        {
            FileFormat* format = formats[findex];
            std::string testExtension = pystring::lower(format->GetExtension());
            if(extension == testExtension)
            {
                return format;
            }
        }
        
        return 0x0;
    }
    
    void RegisterFileFormat(FileFormat* format)
    {
        FormatRegistry & formats = GetFormatRegistry();
        formats.push_back(format);
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
            FileFormat * primaryFormat = GetFileFormatForExtension(extension);
            
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
            FormatRegistry & formats = GetFormatRegistry();
            
            for(unsigned int findex = 0; findex<formats.size(); ++findex)
            {
                // Dont bother trying the primaryFormat twice.
                if(formats[findex] == primaryFormat) continue;
                
                try
                {
                    CachedFileRcPtr cachedFile = formats[findex]->Load(filestream);
                    
                    // Add the result to our cache, return it.
                    FileCachePair pair = std::make_pair(formats[findex], cachedFile);
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
