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

#include "CTFDummyElt.h"
#include <sstream>
#include "../Logging.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

DummyElt::DummyParent::DummyParent(const Element* pParent)
    : ContainerElt(pParent ? pParent->getName() : "",
                   pParent ? pParent->getXmLineNumber() : 0,
                   pParent ? pParent->getXmlFile() : "")
{
}

DummyElt::DummyParent::~DummyParent()
{
}

const std::string& DummyElt::DummyParent::getIdentifier() const
{
    static const std::string identifier = "Unknown";
    return identifier;
}

void DummyElt::DummyParent::appendDescription(const std::string&)
{
}

void DummyElt::DummyParent::start(const char **)
{
}

void DummyElt::DummyParent::end()
{
}

const std::string& DummyElt::DummyParent::getTypeName() const
{
    return getIdentifier();
}

DummyElt::DummyElt(const std::string& name,
                   const Element* pParent,
                   unsigned xmlLineNumber,
                   const std::string& xmlFile,
                   const char* msg)
    : PlainElt(name, new DummyParent(pParent), xmlLineNumber, xmlFile)
{
    std::ostringstream oss;
    oss << "Ignore element '" << getName().c_str();
    oss << "' (line " << getXmLineNumber() << ") ";
    oss << "where its parent is '" << getParent()->getName().c_str();
    oss << "' (line " << getParent()->getXmLineNumber() << ") ";
    oss << (msg ? msg : "");
    oss << ": " << getXmlFile().c_str();

    LogDebug(oss.str());
}

DummyElt::~DummyElt()
{
    delete getParent();
}

void DummyElt::start(const char **)
{
}

void DummyElt::end()
{
}

const std::string& DummyElt::getIdentifier() const
{
    static std::string emptyName;
    return emptyName;
}

void DummyElt::setRawData(const char* str, size_t len, unsigned)
{
    m_rawData.push_back(std::string(str, len));
}

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

