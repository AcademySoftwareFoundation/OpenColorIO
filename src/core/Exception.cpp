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

OCIO_NAMESPACE_ENTER
{
  
    Exception::Exception(const char * msg) throw()
    : std::exception(),
      msg_(msg)
    {}

    Exception::Exception(const Exception& e) throw()
    : std::exception(),
      msg_(e.msg_)
    {}

    //*** operator=
    Exception& Exception::operator=(const Exception& e) throw()
    {
        msg_ = e.msg_;
        return *this;
    }

    //*** ~Exception
    Exception::~Exception() throw()
    {
    }

    //*** what
    const char* Exception::what() const throw()
    {
        return msg_.c_str();
    }

  
  
  
    ExceptionMissingFile::ExceptionMissingFile(const char * msg) throw()
    : Exception(msg)
    {}

    ExceptionMissingFile::ExceptionMissingFile(const ExceptionMissingFile& e) throw()
    : Exception(e)
    {}

}
OCIO_NAMESPACE_EXIT
