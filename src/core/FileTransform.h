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


#ifndef INCLUDED_OCS_FILETRANSFORM_H
#define INCLUDED_OCS_FILETRANSFORM_H

#include <OpenColorSpace/OpenColorSpace.h>

#include "Op.h"

OCS_NAMESPACE_ENTER
{
    class FileTransform::Impl
    {
        public:
        
        Impl();
        /*Impl(const Impl &);*/
        
        ~Impl();
        
        Impl& operator= (const Impl &);
        
        TransformDirection getDirection() const;
        void setDirection(TransformDirection dir);
        
        const char * getSrc() const;
        void setSrc(const char * src);
        
        Interpolation getInterpolation() const;
        void setInterpolation(Interpolation interp);
        
        private:
        
        TransformDirection m_direction;
        std::string m_src;
        Interpolation m_interpolation;
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildFileOps(OpRcPtrVec * opVec,
                      const Config& config,
                      const FileTransform& fileTransform,
                      TransformDirection dir);
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    class CachedFile
    {
    public:
        CachedFile() {};
        virtual ~CachedFile() {};
    };
    
    typedef SharedPtr<CachedFile> CachedFileRcPtr;
    
    class FileFormat
    {
    public:
        virtual ~FileFormat();
        
        virtual std::string GetExtension() const = 0;
        virtual CachedFileRcPtr Load(std::istream & istream) const = 0;
        
        virtual void BuildFileOps(OpRcPtrVec * opVec,
                                  CachedFileRcPtr cachedFile,
                                  const FileTransform& fileTransform,
                                  TransformDirection dir) const = 0;

    private:
        FileFormat& operator= (const FileFormat &);
    };
    
    void RegisterFileFormat(FileFormat* format);

}
OCS_NAMESPACE_EXIT

#endif
