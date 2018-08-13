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

#include "CTFInputDescriptorElt.h"
#include "CTFTransformElt.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

InputDescriptorElt::InputDescriptorElt(const std::string& name,
                                       ContainerElt* pParent,
                                       unsigned xmlLineNumber,
                                       const std::string& xmlFile)
    : PlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

InputDescriptorElt::~InputDescriptorElt()
{
}

void InputDescriptorElt::start(const char **)
{
}

void InputDescriptorElt::end()
{
}

void InputDescriptorElt::setRawData(const char* str,
                                    size_t len,
                                    unsigned /*xmlLine*/)
{
    TransformElt* pTransform
        = dynamic_cast<TransformElt*>(getParent());

    std::string s = pTransform->getTransform()->getInputDescriptor();
    s += std::string(str, len);

    pTransform->getTransform()->setInputDescriptor(s);
}


} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

