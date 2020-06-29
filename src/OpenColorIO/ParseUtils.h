// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PARSEUTILS_H
#define INCLUDED_OCIO_PARSEUTILS_H


#include <sstream>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
// Prepares a string to be inserted in an XML document by escaping
// characters that may not appear directly in XML.
// (Note: eXpat does the inverse conversion automatically.)
std::string ConvertSpecialCharToXmlToken(const std::string& str);

std::string ConvertXmlTokenToSpecialChar(const std::string & str);

std::string FloatToString(float fval);
std::string FloatVecToString(const float * fval, unsigned int size);

std::string DoubleToString(double value);
std::string DoubleVecToString(const double * fval, unsigned int size);

bool StringToFloat(float * fval, const char * str);
bool StringToInt(int * ival, const char * str, bool failIfLeftoverChars=false);

bool StringVecToFloatVec(std::vector<float> & floatArray,
                         const StringUtils::StringVec & lineParts);

bool StringVecToIntVec(std::vector<int> & intArray,
                       const StringUtils::StringVec & lineParts);

//////////////////////////////////////////////////////////////////////////

// read the next non-empty line, and store it in 'line'
// return 'true' on success

bool nextline(std::istream &istream, std::string &line);

bool StrEqualsCaseIgnore(const std::string & a, const std::string & b);

// If a ',' is in the string, split on it
// If a ':' is in the string, split on it
// Otherwise, assume a single string.
// Also, strip whitespace from all parts.
StringUtils::StringVec SplitStringEnvStyle(const std::string & str);

// Join on ','
std::string JoinStringEnvStyle(const StringUtils::StringVec & outputvec);

// Ordering and capitalization from vec1 is preserved
StringUtils::StringVec IntersectStringVecsCaseIgnore(const StringUtils::StringVec & vec1,
                                        const StringUtils::StringVec & vec2);

// Find the index of the specified string, ignoring case.
// return -1 if not found.

int FindInStringVecCaseIgnore(const StringUtils::StringVec & vec, const std::string & str);

} // namespace OCIO_NAMESPACE

#endif
