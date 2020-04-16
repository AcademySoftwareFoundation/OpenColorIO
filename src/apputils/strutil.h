/*
  Copyright 2008 Larry Gritz and the other authors and contributors.
  All Rights Reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the software's owners nor the names of its
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

  (This is the Modified BSD License)
*/


/////////////////////////////////////////////////////////////////////////
/// @file  strutil.h
///
/// @brief String-related utilities, all in namespace Strutil.
/////////////////////////////////////////////////////////////////////////



#ifndef INCLUDED_OCIO_STRUTIL_H
#define INCLUDED_OCIO_STRUTIL_H

#include <cstdarg>
#include <string>
#include <cstring>
#include <map>


#ifndef OPENCOLORIO_PRINTF_ARGS
#   ifndef __GNUC__
#       define __attribute__(x)
#   endif
    // Enable printf-like warnings with gcc by attaching
    // OPENIMAGEIO_PRINTF_ARGS to printf-like functions.  Eg:
    //
    // void foo (const char* fmt, ...) OPENCOLORIO_PRINTF_ARGS(1,2);
    //
    // The arguments specify the positions of the format string and the
    // beginning of the varargs parameter list respectively.
    //
    // For member functions with arguments like the example above, you need
    // OPENCOLORIO_PRINTF_ARGS(2,3) instead.  (gcc includes the implicit this
    // pointer when it counts member function arguments.)
#   define OPENCOLORIO_PRINTF_ARGS(fmtarg_pos, vararg_pos) \
        __attribute__ ((format (printf, fmtarg_pos, vararg_pos) ))
#endif



/// @namespace Strutil
///
/// @brief     String-related utilities.
namespace Strutil
{

/// Return a std::string formatted from printf-like arguments -- passed
/// already as a va_list.
std::string vformat (const char *fmt, va_list ap)
                                         OPENCOLORIO_PRINTF_ARGS(1,0);

}  // namespace Strutil


#endif // INCLUDED_OCIO_STRUTIL_H
