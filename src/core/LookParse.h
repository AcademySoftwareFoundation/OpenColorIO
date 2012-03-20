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


#ifndef INCLUDED_OCIO_PARSED_LOOK_H
#define INCLUDED_OCIO_PARSED_LOOK_H

#include <OpenColorIO/OpenColorIO.h>

#include <vector>

OCIO_NAMESPACE_ENTER
{
    // Looks parse structures
    // This is contains a list, where each option entry corresponds to
    // an "or" separated looks token list.
    // I.e, " +cc,-onset | +cc " parses to TWO options: (+cc,-onset), (+cc)
    
    class LookParseResult
    {
        public:
        struct Token
        {
            std::string name;
            TransformDirection dir;
            
            Token():
                dir(TRANSFORM_DIR_FORWARD) {}
            
            void parse(const std::string & str);
            void serialize(std::ostream & os) const;
        };
        
        typedef std::vector<Token> Tokens;
        
        static void serialize(std::ostream & os, const Tokens & tokens);
        
        typedef std::vector<Tokens> Options;
        
        const Options & parse(const std::string & looksstr);
        
        const Options & getOptions() const;
        bool empty() const;
        
        void reverse();
        
        private:
        Options m_options;
    };
    
}
OCIO_NAMESPACE_EXIT

#endif
