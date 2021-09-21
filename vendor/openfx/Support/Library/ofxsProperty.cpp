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

using namespace OFX::Private;

namespace OFX {

  static
  void throwPropertyException(OfxStatus stat,
    const std::string &propName)
  {
    switch (stat) 
    {
    case kOfxStatOK :
    case kOfxStatReplyYes :
    case kOfxStatReplyNo :
    case kOfxStatReplyDefault :
      // Throw nothing!
      break;

    case kOfxStatErrUnknown :
    case kOfxStatErrUnsupported : // unsupported implies unknow here
      if(OFX::PropertySet::getThrowOnUnsupportedProperties()) // are we suppressing this?
        throw OFX::Exception::PropertyUnknownToHost(propName.c_str());
      break;

    case kOfxStatErrMemory :
      throw std::bad_alloc();
      break;

    case kOfxStatErrValue :
      throw  OFX::Exception::PropertyValueIllegalToHost(propName.c_str());
      break;

    case kOfxStatErrBadHandle :
    case kOfxStatErrBadIndex :
    default :
      throwSuiteStatusException(stat);
      break;
    }
  }

  /** @brief are we logging property get/set */
  int PropertySet::_gPropLogging = 1;

  /** @brief Do we throw an exception if a host returns 'unsupported' when setting a property */
  bool PropertySet::_gThrowOnUnsupported = true;

  /** @brief Virtual destructor */
  PropertySet::~PropertySet() {}

  /** @brief, returns the dimension of the given property from this property set */
  int PropertySet::propGetDimension(const char* property, bool throwOnFailure) const
  {
    assert(_propHandle != 0);
    int dimension = 0;
    OfxStatus stat = gPropSuite->propGetDimension(_propHandle, property, &dimension);
    Log::error(stat != kOfxStatOK, "Failed on fetching dimension for property %s, host returned status %s.", property, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) 
      Log::print("Fetched dimension of property %s, returned %d.",  property, dimension);

    return dimension;
  }

  /** @brief, resets the property to it's default value */
  void PropertySet::propReset(const char* property)
  {
    assert(_propHandle != 0);
    OfxStatus stat = gPropSuite->propReset(_propHandle, property);
    Log::error(stat != kOfxStatOK, "Failed on reseting property %s to its defaults, host returned status %s.", property, mapStatusToString(stat));
    throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Reset property %s.",  property);
  }

  /** @brief, Set a single dimension pointer property */
  void PropertySet::propSetPointer(const char* property, void *value, int idx, bool throwOnFailure)
  {
    assert(_propHandle != 0);
    OfxStatus stat = gPropSuite->propSetPointer(_propHandle, property, idx, value);
    OFX::Log::error(stat != kOfxStatOK, "Failed on setting pointer property %s[%d] to %p, host returned status %s;", 
      property, idx, value, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property);  

    if(_gPropLogging > 0) Log::print("Set pointer property %s[%d] to be %p.",  property, idx, value);
  }

  /** @brief, Set a single dimension string property */
  void PropertySet::propSetString(const char* property, const std::string &value, int idx, bool throwOnFailure)
  {
    assert(_propHandle != 0);
    OfxStatus stat = gPropSuite->propSetString(_propHandle, property, idx, value.c_str());
    OFX::Log::error(stat != kOfxStatOK, "Failed on setting string property %s[%d] to %s, host returned status %s;", 
      property, idx, value.c_str(), mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Set string property %s[%d] to be %s.",  property, idx, value.c_str());
  }

  /** @brief, Set a single dimension double property */
  void PropertySet::propSetDouble(const char* property, double value, int idx, bool throwOnFailure)
  {
    assert(_propHandle != 0);
    OfxStatus stat = gPropSuite->propSetDouble(_propHandle, property, idx, value);
    OFX::Log::error(stat != kOfxStatOK, "Failed on setting double property %s[%d] to %lf, host returned status %s;", 
      property, idx, value, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Set double property %s[%d] to be %lf.",  property, idx, value);
  }

  /** @brief, Set a single dimension int property */
  void PropertySet::propSetInt(const char* property, int value, int idx, bool throwOnFailure)
  {
    assert(_propHandle != 0);
    OfxStatus stat = gPropSuite->propSetInt(_propHandle, property, idx, value);
    OFX::Log::error(stat != kOfxStatOK, "Failed on setting int property %s[%d] to %d, host returned status %s (%d);", 
      property, idx, value, mapStatusToString(stat), stat);
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Set int property %s[%d] to be %d.",  property, idx, value);
  }

  /** @brief, Set a multiple dimension double property */
  void PropertySet::propSetDoubleN(const char* property, const double* values, int count, bool throwOnFailure)
  {
    assert(_propHandle != 0);
    OfxStatus stat = gPropSuite->propSetDoubleN(_propHandle, property, count, values);
    OFX::Log::error(stat != kOfxStatOK, "Failed on setting double property %s[0..%d], host returned status %s;",
      property, count-1, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Set double property %s[0..%d].",  property, count-1);
  }

  /** @brief Get single pointer property */
  void*  PropertySet::propGetPointer(const char* property, int idx, bool throwOnFailure) const
  {
    assert(_propHandle != 0);
    void *value = 0;
    OfxStatus stat = gPropSuite->propGetPointer(_propHandle, property, idx, &value);
    OFX::Log::error(stat != kOfxStatOK, "Failed on getting pointer property %s[%d], host returned status %s;", 
      property, idx, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Retrieved pointer property %s[%d], was given %p.",  property, idx, value);

    return value;
  }

  /** @brief Get single string property */
  std::string PropertySet::propGetString(const char* property, int idx, bool throwOnFailure) const
  {
    assert(_propHandle != 0);
    char *value = NULL;
    OfxStatus stat = gPropSuite->propGetString(_propHandle, property, idx, &value);
    OFX::Log::error(stat != kOfxStatOK, "Failed on getting string property %s[%d], host returned status %s;", 
      property, idx, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property);

    if(_gPropLogging > 0) Log::print("Retrieved string property %s[%d], was given %s.",  property, idx, value);
    return value != NULL ?  std::string(value) : std::string();
  }

  /** @brief Get single double property */
  double PropertySet::propGetDouble(const char* property, int idx, bool throwOnFailure) const
  {
    assert(_propHandle != 0);
    double value = 0;
    OfxStatus stat = gPropSuite->propGetDouble(_propHandle, property, idx, &value);
    OFX::Log::error(stat != kOfxStatOK, "Failed on getting double property %s[%d], host returned status %s;", 
      property, idx, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Retrieved double property %s[%d], was given %lf.",  property, idx, value);
    return value;
  }

  /** @brief Get single int property */
  int PropertySet::propGetInt(const char* property, int idx, bool throwOnFailure) const
  {
    assert(_propHandle != 0);
    int value = 0;
    OfxStatus stat = gPropSuite->propGetInt(_propHandle, property, idx, &value);
    OFX::Log::error(stat != kOfxStatOK, "Failed on getting int property %s[%d], host returned status %s;", 
      property, idx, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property); 

    if(_gPropLogging > 0) Log::print("Retrieved int property %s[%d], was given %d.",  property, idx, value);
    return value;
  }
    
  std::list<std::string> PropertySet::propGetNString(const char* property, bool throwOnFailure) const
  {
    assert(_propHandle != 0);
    std::list<std::string> ret;
    int dimension = propGetDimension(property,throwOnFailure);
    if (dimension <= 0) {
      return ret;
    }
    std::vector<char*> rawValue(dimension);
    OfxStatus stat = gPropSuite->propGetStringN(_propHandle, property, dimension, rawValue.data());
    OFX::Log::error(stat != kOfxStatOK, "Failed on getting string property %s, host returned status %s;",
                      property, mapStatusToString(stat));
    if(throwOnFailure)
      throwPropertyException(stat, property);
      
    if(_gPropLogging > 0) Log::print("Retrieved string property %s, was given %s.",  property,rawValue.data());
      
    for (int i = 0; i < dimension; ++i) {
      ret.push_back(std::string(rawValue[i]));
    }
    return ret;

  }

};
