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
#include "Lut1DOp.h"
#include "Lut3DOp.h"

/*
*/

OCIO_NAMESPACE_ENTER
{

class CachedFileCSP : public CachedFile
{
    
public:
    
    CachedFileCSP ()
    {
        csptype = "unknown";
        metadata = "none";
        //prelut = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
        lut1D = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
        lut3D = OCIO_SHARED_PTR<Lut3D>(new Lut3D());
    };
    ~CachedFileCSP() {};
    
    std::string csptype;
    std::string metadata;
    //OCIO_SHARED_PTR<Lut1D> prelut;
    OCIO_SHARED_PTR<Lut1D> lut1D;
    OCIO_SHARED_PTR<Lut3D> lut3D;
    
};
typedef OCIO_SHARED_PTR<CachedFileCSP> CachedFileCSPRcPtr;

class FileFormatCSP : public FileFormat
{
    
public:
    
    ~FileFormatCSP() {};
    
    virtual std::string
    GetExtension () const;
    
    virtual CachedFileRcPtr
    Load (std::istream & istream) const;
    
    virtual void
    BuildFileOps(OpRcPtrVec & ops,
                 CachedFileRcPtr untypedCachedFile,
                 const FileTransform& fileTransform,
                 TransformDirection dir) const;
    
};

struct AutoRegisterCSP
{
    AutoRegisterCSP() { RegisterFileFormat(new FileFormatCSP); }
};
static AutoRegisterCSP cspReg;

}
OCIO_NAMESPACE_EXIT
