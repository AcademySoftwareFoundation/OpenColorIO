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

#include "GroupTransform.h"
#include "FileTransform.h"
#include "Op.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    Op::~Op()
    { }
    
    
    
    void BuildOps(OpRcPtrVec * opVec,
                  const Config& config,
                  const ConstTransformRcPtr& transform,
                  TransformDirection dir)
    {
        if(ConstGroupTransformRcPtr groupTransform = \
            DynamicPtrCast<const GroupTransform>(transform))
        {
            BuildGroupOps(opVec, config, *groupTransform, dir);
        }
        else if(ConstFileTransformRcPtr fileTransform = \
            DynamicPtrCast<const FileTransform>(transform))
        {
            BuildFileOps(opVec, config, *fileTransform, dir);
        }
        else
        {
            std::ostringstream os;
            os << "Unknown transform type for Op Creation.";
            throw OCIOException(os.str().c_str());
        }
    }
    
    /*
    std::ostream& operator<< (std::ostream& os, const Transform& transform)
    {
        if(dynamic_cast<const GroupTransform*>(t))
        {
            os << *(dynamic_cast<const GroupTransform*>(t));
        }
        else if(dynamic_cast<const FileTransform*>(t))
        {
            os << *(dynamic_cast<const FileTransform*>(t));
        }
        else
        {
            os << "Unknown transform type for serialization.";
        }
        
        return os;
    }
    */
}
OCIO_NAMESPACE_EXIT
