/*
Copyright (c) 2003-2017 Sony Pictures Imageworks Inc., et al.
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

#ifndef _OPENCOLORIO_PS_CONTEXT_H_
#define _OPENCOLORIO_PS_CONTEXT_H_

#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


typedef std::vector<std::string> SpaceVec;

int FindSpace(const SpaceVec &spaceVec, const std::string &space); // returns -1 if not found


class OpenColorIO_PS_Context
{
  public:
    OpenColorIO_PS_Context(const std::string &path);
    ~OpenColorIO_PS_Context() {}
    
    //const std::string getPath() const { return _path; }
    
    bool isLUT() const { return _isLUT; }
    bool canInvertLUT() const { return (isLUT() && _canInvertLUT); }
    
    OCIO::ConstConfigRcPtr getConfig() const { return _config; }
    
    OCIO::ConstProcessorRcPtr getConvertProcessor(const std::string &inputSpace, const std::string &outputSpace) const;
    OCIO::ConstProcessorRcPtr getDisplayProcessor(const std::string &inputSpace, const std::string &device, const std::string &transform) const;
    OCIO::ConstProcessorRcPtr getLUTProcessor(OCIO::Interpolation interpolation, OCIO::TransformDirection direction) const;
    
    OCIO::BakerRcPtr getConvertBaker(const std::string &inputSpace, const std::string &outputSpace) const;
    OCIO::BakerRcPtr getDisplayBaker(const std::string &inputSpace, const std::string &device, const std::string &transform) const;
    OCIO::BakerRcPtr getLUTBaker(OCIO::Interpolation interpolation, OCIO::TransformDirection direction) const;

    const SpaceVec & getColorSpaces(bool fullPath=false) const { return (fullPath ? _colorSpacesFullPaths : _colorSpaces); }
    const std::string & getDefaultColorSpace() const { return _defaultColorSpace; }
    
    const SpaceVec & getDevices() const { return _devices; };
    const std::string & getDefaultDevice() const { return _defaultDevice; }
    
    SpaceVec getTransforms(const std::string &device) const;
    std::string getDefaultTransform(const std::string &device) const;

  private:
    std::string _path;
    
    OCIO::ConstConfigRcPtr _config;
    
    SpaceVec _colorSpaces;
    SpaceVec _colorSpacesFullPaths;
    std::string _defaultColorSpace;
    SpaceVec _devices;
    std::string _defaultDevice;
    
    bool _isLUT;
    bool _canInvertLUT;
};

#endif // _OPENCOLORIO_PS_CONTEXT_H_