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


#ifndef INCLUDED_OCIO_CTFOPELT_H
#define INCLUDED_OCIO_CTFOPELT_H

#include "CTFContainerElt.h"
#include "CTFTransform.h"
#include "../opdata/OpData.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the CTF sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{


    // Base class for the OpData Element
    class OpElt : public ContainerElt
    {
    public:
        // Constructor
        OpElt();
        
        // Destructor
        ~OpElt();

        // Set the current context
        void setContext(const std::string& name,
                        const TransformPtr& pTransform,
                        unsigned xmlLineNumber,
                        const std::string& xmlFile
        );

        // Get the element's identifier
        const std::string& getIdentifier() const;

        // Get the associated OpData
        virtual OpData::OpData* getOp() const = 0;

        // Append a description string
        void appendDescription(const std::string& desc);

        // Start the parsing of the element
        void start(const char **atts);
        
        // End the parsing of the element
        virtual void end();

        // Get the element's type name
        const std::string& getTypeName() const;

        // Get the right reader using its type and
        // the xml transform version
        // - type is the container type
        // - version is the requested version
        static OpElt* getReader(OpData::OpData::OpType type,
                                const Version& version);

    protected:
        // Convert the bit depth string to its enum's value
        static BitDepth getBitDepth(const std::string& str);

    protected:
        TransformPtr m_transform;  // The parent
    };


} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif
