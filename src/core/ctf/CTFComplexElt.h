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


#ifndef INCLUDED_OCIO_CTFCOMPLEXELT_H
#define INCLUDED_OCIO_CTFCOMPLEXELT_H

#include "CTFContainerElt.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the CTF sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

// Base class for nested elements
class ComplexElt : public ContainerElt
{
public:
    // Constructor
    ComplexElt(const std::string& name,
                ContainerElt* pParent,
                unsigned xmlLineNumber,
                const std::string& xmlFile)
        : ContainerElt(name, xmlLineNumber, xmlFile)
        , m_parent(pParent)
    {
    }

    // Destructor
    ~ComplexElt()
    {
    }

    ContainerElt* getParent() const
    {
        return m_parent;
    }

    // Get the element's identifier
    const std::string& getIdentifier() const
    {
        return getName();
    }

    // Get the element's type name
    const std::string& getTypeName() const
    {
        return getName();
    }

    // Append a description string
    void appendDescription(const std::string& /*desc*/)
    {
    }

private:
    // No default contructor
    ComplexElt();

    ContainerElt* m_parent; // The element's parent
};

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif
