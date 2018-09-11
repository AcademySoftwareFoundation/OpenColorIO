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
    const int FORMAT_CAPABILITY_WRITE = 2;
    const int FORMAT_CAPABILITY_ALL = (FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE);
    
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
        
        virtual void GetFormatInfo(FormatInfoVec & formatInfoVec) const = 0;
        
        // read an istream. originalFileName is used by parsers that make use
        // of aspects of the file name as part of the parsing.
        // It may be set to an empty string if not known.
        virtual CachedFileRcPtr Read(
            std::istream & istream,
            const std::string & originalFileName) const = 0;
        
        virtual void Write(const Baker & baker,
                           const std::string & formatName,
                           std::ostream & ostream) const;
        
        virtual void BuildFileOps(OpRcPtrVec & ops,
                                  const Config & config,
                                  const ConstContextRcPtr & context,
                                  CachedFileRcPtr cachedFile,
                                  const FileTransform & fileTransform,
                                  TransformDirection dir) const = 0;
        
        // True if the file is a binary rather than text-based format.
        virtual bool IsBinary() const
        {
            return false;
        }

        // For logging purposes
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
    // lookups will move internal
    
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
        
        typedef std::vector<std::string> StringVec;
        StringVec m_readFormatNames;
        StringVec m_readFormatExtensions;
        StringVec m_writeFormatNames;
        StringVec m_writeFormatExtensions;
    };
    
    // Registry Builders
    FileFormat * CreateFileFormat3DL();
    FileFormat * CreateFileFormatCCC();
    FileFormat * CreateFileFormatCDL();
    FileFormat * CreateFileFormatCC();
    FileFormat * CreateFileFormatCSP();
    FileFormat * CreateFileFormatHDL();
    FileFormat * CreateFileFormatICC();
    FileFormat * CreateFileFormatDiscreet1DL();
    FileFormat * CreateFileFormatIridasItx();
    FileFormat * CreateFileFormatIridasCube();
    FileFormat * CreateFileFormatIridasLook();
    FileFormat * CreateFileFormatPandora();
    FileFormat * CreateFileFormatSpi1D();
    FileFormat * CreateFileFormatSpi3D();
    FileFormat * CreateFileFormatSpiMtx();
    FileFormat * CreateFileFormatTruelight();
    FileFormat * CreateFileFormatVF();
    FileFormat * CreateFileFormatResolveCube();
    
}
OCIO_NAMESPACE_EXIT

#endif
