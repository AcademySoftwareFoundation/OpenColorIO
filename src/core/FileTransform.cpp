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
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    FileTransform::FileTransform()
        : m_impl(new FileTransform::Impl)
    {
    
    }
    
    /*
    FileTransform::FileTransform(const FileTransform & rhs)
        : 
        Transform(),
        m_impl(new FileTransform::Impl)
    {
        *m_impl = *rhs.m_impl;
    }
    */
    
    TransformRcPtr FileTransform::createEditableCopy() const
    {
        FileTransformRcPtr transform = FileTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    FileTransform::~FileTransform()
    {
    
    }
    
    FileTransform& FileTransform::operator= (const FileTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection FileTransform::getDirection() const
    {
        return m_impl->getDirection();
    }
    
    void FileTransform::setDirection(TransformDirection dir)
    {
        m_impl->setDirection(dir);
    }
    
    
    const char * FileTransform::getSrc() const
    {
        return m_impl->getSrc();
    }
    
    void FileTransform::setSrc(const char * src)
    {
        m_impl->setSrc(src);
    }
    
    Interpolation FileTransform::getInterpolation() const
    {
        return m_impl->getInterpolation();
    }
    
    void FileTransform::setInterpolation(Interpolation interp)
    {
        m_impl->setInterpolation(interp);
    }
    
    std::ostream& operator<< (std::ostream& os, const FileTransform& t)
    {
        os << "<FileTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << "interpolation=" << InterpolationToString(t.getInterpolation()) << ", ";
        os << "src='" << t.getSrc() << "'";
        os << ">\n";
        
        return os;
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    FileTransform::Impl::Impl() :
        m_direction(TRANSFORM_DIR_FORWARD),
        m_interpolation(INTERP_UNKNOWN)
    { }
    
    /*
    FileTransform::Impl::Impl(const Impl & rhs) :
        m_direction(rhs.m_direction),
        m_src(rhs.m_src),
        m_interpolation(rhs.m_interpolation)
    { }
    */
    
    FileTransform::Impl::~Impl()
    { }
    
    FileTransform::Impl& FileTransform::Impl::operator= (const Impl & rhs)
    {
        m_direction = rhs.m_direction;
        m_src = rhs.m_src;
        m_interpolation = rhs.m_interpolation;
        return *this;
    }
    
    TransformDirection FileTransform::Impl::getDirection() const
    {
        return m_direction;
    }
    
    void FileTransform::Impl::setDirection(TransformDirection dir)
    {
        m_direction = dir;
    }
    
    const char * FileTransform::Impl::getSrc() const
    {
        return m_src.c_str();
    }
    
    void FileTransform::Impl::setSrc(const char * src)
    {
        m_src = src;
    }
    
    Interpolation FileTransform::Impl::getInterpolation() const
    {
        return m_interpolation;
    }
    
    void FileTransform::Impl::setInterpolation(Interpolation interp)
    {
        m_interpolation = interp;
    }
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    FileFormat::~FileFormat()
    {
    
    }
    
    
    namespace
    {
        typedef std::vector<FileFormat*> FormatRegistry;
        
        FormatRegistry & GetFormatRegistry()
        {
            static std::vector<FileFormat*> formats;
            return formats;
        }
        
        
        FileFormat* GetFileFormatForExtension(const std::string & str)
        {
            if(str.empty()) return 0x0;
            
            std::string extension = pystring::lower(str);
            std::string testExtension;
            
            FormatRegistry & formats = GetFormatRegistry();
            
            for(unsigned int i=0; i<formats.size(); ++i)
            {
                FileFormat* format = formats[i];
                testExtension = pystring::lower(format->GetExtension());
                if(extension == testExtension) return format;
            }
            
            return 0x0;
        }
        
        std::string GetFileExtension(const std::string & str)
        {
            std::vector<std::string> parts;
            pystring::rsplit(str, parts, ".", 1);
            if(parts.size() == 2) return parts[1];
            return "";
        }
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
                os << "The specified transform file '";
                os << filepath <<"' could not be opened. ";
                os << "Please confirm the file exists with appropriate read";
                os << " permissions.";
                throw Exception(os.str().c_str());
            }
            
            std::string extension = GetFileExtension(filepath);
            
            FileFormat* format;
            CachedFileRcPtr cachedFile;
            std::string errorText;
            
            // Try the initial format.
            // TODO: actually, if this fails try all that match the specified format
            // (i.e, the million .lut formats)
            format = GetFileFormatForExtension(extension);
            
            if(format)
            {
                try
                {
                    cachedFile = format->Load(filestream);
                }
                catch(std::exception & e)
                {
                    if(errorText.empty()) errorText = e.what();
                    format = 0x0;
                    filestream.seekg( std::ifstream::beg );
                }
            }
            
            // TODO: If that fails, try all formats
            
            // Assert file and cached format exist
            if(!format)
            {
                std::ostringstream os;
                os << "The specified transform file '";
                os << filepath <<"' is not any known file format.";
                throw Exception(os.str().c_str());
            }
            
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "The specified transform file '";
                os << filepath <<"' was not able to be successfully loaded. ";
                os << errorText;
                throw Exception(os.str().c_str());
            }
            
            // Add the result to our cache, return it.
            FileCachePair pair = std::make_pair(format, cachedFile);
            g_fileCache[filepath] = pair;
            return pair;
        }
    }
    
    void BuildFileOps(OpRcPtrVec & ops,
                      const Config& config,
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
        
        std::string resourcepath = config.getResolvedResourcePath();
        std::string filepath = path::join(resourcepath, src);
        
        FileCachePair cachePair = GetFile(filepath);
        
        FileFormat* format = cachePair.first;
        CachedFileRcPtr cachedFile = cachePair.second;
        
        format->BuildFileOps(ops,
                             cachedFile, fileTransform,
                             dir);
    }
}
OCIO_NAMESPACE_EXIT
