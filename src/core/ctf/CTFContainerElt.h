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


#ifndef INCLUDED_OCIO_CTFCONTAINERELT_H
#define INCLUDED_OCIO_CTFCONTAINERELT_H

#include <OpenColorIO/OpenColorIO.h>

#include "CTFElement.h"
#include "CTFReaderVersion.h"

OCIO_NAMESPACE_ENTER
{


// Private namespace to the CTF sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

// Base class for element that could contain sub-elements
class ContainerElt : public Element
{
public:
    // Constructor
    // - name is the element's name
    // - xmlLineNumber is the element's location in the XML file
    // - xmlFile is the element's XML file name
    ContainerElt(const std::string& name,
                    unsigned xmlLineNumber,
                    const std::string& xmlFile)
        : Element(name, xmlLineNumber, xmlFile)
    {
    }

    // Destructor
    virtual ~ContainerElt()
    {
    }

    // Is it a container which means if it can hold other elements
    bool isContainer() const
    {
        return true;
    }

    // Append a description string
    virtual void appendDescription(const std::string& desc) = 0;

    // Get the current xml transform version
    virtual const Version & getVersion() const
    {
        return CTF_PROCESS_LIST_VERSION;
    }

private:
    // No default Constructor
    ContainerElt();


};

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif
