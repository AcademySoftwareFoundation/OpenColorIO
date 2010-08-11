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


#ifndef INCLUDED_OCIO_CDLTRANSFORM_H
#define INCLUDED_OCIO_CDLTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "Processor.h"

#include "tinyxml/tinyxml.h"

OCIO_NAMESPACE_ENTER
{
    class CDLTransform::Impl
    {
        public:
        
        Impl();
        ~Impl();
        
        Impl& operator= (const Impl &);
        
        TransformDirection getDirection() const;
        void setDirection(TransformDirection dir);
        
        const char * getXML() const;
        void setXML(const char * xml);
        
        // Throw an exception if the CDL is in any way invalid
        void sanityCheck() const;
        
        bool isNoOp() const;
        
        void setSlope(const float * rgb);
        void getSlope(float * rgb) const;
        
        void setOffset(const float * rgb);
        void getOffset(float * rgb) const;
        
        void setPower(const float * rgb);
        void getPower(float * rgb) const;
        
        void setSOP(const float * vec9);
        void getSOP(float * vec9) const;
        
        void setSat(float sat);
        float getSat() const;
        
        void getSatLumaCoefs(float * rgb) const;
        
        void setID(const char * id);
        const char * getID() const;
        
        void setDescription(const char * desc);
        const char * getDescription() const;
        
        private:
        
        void setSOPVec(const float * rgb, const char * name);
        void getSOPVec(float * rgb, const char * name) const;
        
        TransformDirection m_direction;
        
        TiXmlDocument * m_doc;
        mutable std::string m_xml;
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildCDLOps(LocalProcessor & processor,
                     const Config & config,
                     const CDLTransform & cdlTransform,
                     TransformDirection dir);
}
OCIO_NAMESPACE_EXIT

#endif
