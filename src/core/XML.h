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


#ifndef INCLUDED_OCIO_XML_H
#define INCLUDED_OCIO_XML_H

#include <sstream>
#include <iostream>
#include <tinyxml2.h>

OCIO_NAMESPACE_ENTER
{
    typedef tinyxml2::XMLDocument TiXmlDocument;
    typedef tinyxml2::XMLElement TiXmlElement;
    typedef tinyxml2::XMLText TiXmlText;
    typedef tinyxml2::XMLNode TiXmlNode;
    typedef tinyxml2::XMLHandle TiXmlHandle;
    typedef tinyxml2::XMLPrinter TiXmlPrinter;
    
    namespace XML {
        inline std::string Error(TiXmlDocument* doc, std::ostringstream& os, bool del) {
            os << "XML Parse Error " << doc->ErrorName() << ' ';
            if (const char* err1 = doc->GetErrorStr1())
                os << err1 << ' ';
            if (const char* err2 = doc->GetErrorStr2())
                os << err2 << ' ';
            // os << "(line " << doc->GetErrorLineNum() << ")";
            if (del) delete doc;
            return os.str();
        }

        inline TiXmlDocument* Parse(const char* str, const char* type, const char* src = 0) {
            if (!type || !type[0]) {
                std::ostringstream os;
                os << "Error loading ";
                if (type) os << type << " ";
                os << "xml.";
                if (src) {
                    os << " The specified source file, '";
                    os << src << "' appears to be empty.";
                }
                throw Exception(os);
            }
            tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
            if (doc->Parse(str) != tinyxml2::XML_SUCCESS) {
                std::ostringstream os;
                Error(doc, os, true);
                throw Exception(os);
            }
          return doc;
        }
        
        inline TiXmlDocument* Parse(std::istream & istream, const char* type, const char* src = 0) {
            // Read the file into a string.
            std::ostringstream rawdata;
            rawdata << istream.rdbuf();
            return Parse(rawdata.str().c_str(), type, src);
        }
    }
}
OCIO_NAMESPACE_EXIT

#endif
