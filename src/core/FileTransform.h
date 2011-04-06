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


#ifndef INCLUDED_OCIO_FILETRANSFORM_H
#define INCLUDED_OCIO_FILETRANSFORM_H

#include <map>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "Processor.h"

OCIO_NAMESPACE_ENTER
{
    class CachedFile
    {
    public:
        CachedFile() {};
        virtual ~CachedFile() {};
    };
    
    typedef OCIO_SHARED_PTR<CachedFile> CachedFileRcPtr;
    
    struct TransformData
    {
        float minlum[3];
        float maxlum[3];
        size_t shaperSize;
        std::vector<float> shaper_ident;
        std::vector<float> shaper_encode;
        std::vector<float> shaper_decode;
        //std::vector<float> lookup1D;
        size_t lookup3DSize;
        std::vector<float> lookup3D;
    };
    
    enum FileFormatFeature
    {
        FILE_FORMAT_READ = 0,
        FILE_FORMAT_WRITE = 1
    };
    
    void ClearFileTransformCaches();
    
    class FileFormat
    {
    public:
        virtual ~FileFormat();
        
        virtual std::string GetName() const = 0;
        virtual std::string GetExtension() const = 0;
        
        virtual bool Supports(const FileFormatFeature & feature) const = 0;
        
        virtual CachedFileRcPtr Load(std::istream & istream) const = 0;
        
        virtual void Write(TransformData & data, std::ostream & ostream) const;
        
        virtual void BuildFileOps(OpRcPtrVec & ops,
                                  const Config& config,
                                  const ConstContextRcPtr & context,
                                  CachedFileRcPtr cachedFile,
                                  const FileTransform& fileTransform,
                                  TransformDirection dir) const = 0;
    private:
        FileFormat& operator= (const FileFormat &);
    };
    
    typedef std::map<std::string, FileFormat*> FileFormatMap;
    
    class FormatRegistry
    {
    public:
        static FormatRegistry & GetInstance();
        
        FileFormat* getFileFormatByName(const std::string & name) const;
        FileFormat* getFileFormatForExtension(const std::string & extension) const;
        
        int getNumFormats() const;
        FileFormat* getFormatByIndex(int index) const;
        
        int getNumFormats(FileFormatFeature feature) const;
        FileFormat* getFormatByIndex(FileFormatFeature feature, int index) const;
        
        void registerFileFormat(FileFormat* format);
    
    private:
        FormatRegistry();
        ~FormatRegistry();
        
        FileFormatMap m_formatsByName;
        FileFormatMap m_formatsByExtension;
    };
}
OCIO_NAMESPACE_EXIT

#endif
