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

#include "HashUtils.h"
#include "md5/md5.h"

#include <sstream>
#include <iostream>

OCIO_NAMESPACE_ENTER
{
    std::string CacheIDHash(const char * array, int size)
    {
        md5_state_t state;
        md5_byte_t digest[16];
        
        md5_init(&state);
        md5_append(&state, (const md5_byte_t *)array, size);
        md5_finish(&state, digest);
        
        return GetPrintableHash(digest);
    }
    
    std::string GetPrintableHash(const md5_byte_t * digest)
    {
        static char charmap[] = "0123456789abcdef";
        
        char printableResult[34];
        char *ptr = printableResult;
        
        // build a printable string from unprintable chars.  first character
        // of hashed cache IDs is '$', to later check if it's already been hashed
        *ptr++ = '$';
        for (int i=0;i<16;++i)
        {
            *ptr++ = charmap[(digest[i] & 0x0F)];
            *ptr++ = charmap[(digest[i] >> 4)];
        }
        *ptr++ = 0;
        
        return std::string(printableResult);
    }
}
OCIO_NAMESPACE_EXIT
