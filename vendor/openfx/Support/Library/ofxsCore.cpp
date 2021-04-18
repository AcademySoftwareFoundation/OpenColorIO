/*
OFX Support Library, a library that skins the OFX plug-in API with C++ classes.
Copyright (C) 2004-2005 The Open Effects Association Ltd
Author Bruno Nicoletti bruno@thefoundry.co.uk

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name The Open Effects Association Ltd, nor the names of its 
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The Open Effects Association Ltd
1 Wardour St
London W1D 6PA
England


*/
#include "ofxsSupportPrivate.h"
#ifdef DEBUG
#include <iostream>
#if defined(__APPLE__) || defined(linux)
#include <execinfo.h>
#include <stdlib.h>
#endif
#endif

#include "ofxsMemory.h"

namespace OFX {
  /** @brief Throws an @ref OFX::Exception depending on the status flag passed in */
  void throwSuiteStatusException(OfxStatus stat)
  {
    switch (stat) 
    {
    case kOfxStatOK :
    case kOfxStatReplyYes :
    case kOfxStatReplyNo :
    case kOfxStatReplyDefault :
      break;

    case kOfxStatErrMemory :
      throw std::bad_alloc();

    default :
#    ifdef DEBUG
      std::cout << "Threw suite exception!" << std::endl;
#     if defined(__APPLE__) || defined(linux)
      void* callstack[128];
      int i, frames = backtrace(callstack, 128);
      char** strs = backtrace_symbols(callstack, frames);
      for (i = 0; i < frames; ++i) {
          std::cout << strs[i] << std::endl;
      }
      free(strs);
#     endif
#    endif
      throw OFX::Exception::Suite(stat);
    }
  }

  void throwHostMissingSuiteException(std::string name)
  {
#  ifdef DEBUG
    std::cout << "Threw suite exception! Host missing '" << name << "' suite." << std::endl;
#   if defined(__APPLE__) || defined(linux)
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        std::cout << strs[i] << std::endl;
    }
    free(strs);
#   endif
#  endif
    throw OFX::Exception::Suite(kOfxStatErrUnsupported);
  }


  /** @brief maps status to a string */
  const char* mapStatusToString(OfxStatus stat)
  {
    switch(stat) 
    {    
    case kOfxStatOK             : return "kOfxStatOK";
    case kOfxStatFailed         : return "kOfxStatFailed";
    case kOfxStatErrFatal       : return "kOfxStatErrFatal";
    case kOfxStatErrUnknown     : return "kOfxStatErrUnknown";
    case kOfxStatErrMissingHostFeature : return "kOfxStatErrMissingHostFeature";
    case kOfxStatErrUnsupported : return "kOfxStatErrUnsupported";
    case kOfxStatErrExists      : return "kOfxStatErrExists";
    case kOfxStatErrFormat      : return "kOfxStatErrFormat";
    case kOfxStatErrMemory      : return "kOfxStatErrMemory";
    case kOfxStatErrBadHandle   : return "kOfxStatErrBadHandle";
    case kOfxStatErrBadIndex    : return "kOfxStatErrBadIndex";
    case kOfxStatErrValue       : return "kOfxStatErrValue";
    case kOfxStatReplyYes       : return "kOfxStatReplyYes";
    case kOfxStatReplyNo        : return "kOfxStatReplyNo";
    case kOfxStatReplyDefault   : return "kOfxStatReplyDefault";
    case kOfxStatErrImageFormat : return "kOfxStatErrImageFormat";
    }
    return "UNKNOWN STATUS CODE";
  }


  /** @brief namespace for memory allocation that is done via wrapping the ofx memory suite */
  namespace Memory {
    /** @brief allocate n bytes, returns a pointer to it */
    void *allocate(size_t nBytes, ImageEffect *effect)
    {
      void *data = 0;
      OfxStatus stat = OFX::Private::gMemorySuite->memoryAlloc((void *)(effect ? effect->getHandle() : 0), nBytes, &data);
      if(stat != kOfxStatOK)
        throw std::bad_alloc();
      return data;
    }

    /** @brief free n previously allocated memory */
    void free(void *ptr) noexcept
    {
      if(ptr)
        // note we are ignore errors, this could be bad, but we don't throw on a destruction
        OFX::Private::gMemorySuite->memoryFree(ptr);            
    }

  };

}; // namespace OFX

