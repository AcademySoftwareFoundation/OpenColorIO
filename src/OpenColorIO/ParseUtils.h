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


#ifndef INCLUDED_OCIO_PARSEUTILS_H
#define INCLUDED_OCIO_PARSEUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"

#include <sstream>
#include <string>
#include <vector>

OCIO_NAMESPACE_ENTER
{
    
    std::string FloatToString(float fval);
    std::string FloatVecToString(const float * fval, unsigned int size);
    
    std::string DoubleToString(double value);
    
    bool StringToFloat(float * fval, const char * str);
    bool StringToInt(int * ival, const char * str, bool failIfLeftoverChars=false);
    
    bool StringVecToFloatVec(std::vector<float> & floatArray,
                             const std::vector<std::string> & lineParts);
    
    bool StringVecToIntVec(std::vector<int> & intArray,
                           const std::vector<std::string> & lineParts);
    
    //////////////////////////////////////////////////////////////////////////
    
    // read the next non empty line, and store it in 'line'
    // return 'true' on success
    
    bool nextline(std::istream &istream, std::string &line);
    
    bool StrEqualsCaseIgnore(const std::string & a, const std::string & b);
    
    // If a ',' is in the string, split on it
    // If a ':' is in the string, split on it
    // Otherwise, assume a single string.
    // Also, strip whitespace from all parts.
    
    void SplitStringEnvStyle(std::vector<std::string> & outputvec, const char * str);
    
    // Join on ','
    std::string JoinStringEnvStyle(const std::vector<std::string> & outputvec);
    
    // Ordering and capitalization from vec1 is preserved
    std::vector<std::string> IntersectStringVecsCaseIgnore(const std::vector<std::string> & vec1,
                                                           const std::vector<std::string> & vec2);
    
    // Find the index of the specified string, ignoring case.
    // return -1 if not found.
    
    int FindInStringVecCaseIgnore(const std::vector<std::string> & vec, const std::string & str);
    
}
OCIO_NAMESPACE_EXIT

#endif
