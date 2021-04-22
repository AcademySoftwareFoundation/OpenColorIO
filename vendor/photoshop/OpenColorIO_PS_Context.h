// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
    
    OCIO::ConstCPUProcessorRcPtr getConvertProcessor(const std::string &inputSpace, const std::string &outputSpace) const;
    OCIO::ConstCPUProcessorRcPtr getDisplayProcessor(const std::string &inputSpace, const std::string &display, const std::string &view) const;
    OCIO::ConstCPUProcessorRcPtr getLUTProcessor(OCIO::Interpolation interpolation, OCIO::TransformDirection direction) const;
    
    OCIO::BakerRcPtr getConvertBaker(const std::string &inputSpace, const std::string &outputSpace) const;
    OCIO::BakerRcPtr getDisplayBaker(const std::string &inputSpace, const std::string &display, const std::string &view) const;
    OCIO::BakerRcPtr getLUTBaker(OCIO::Interpolation interpolation, OCIO::TransformDirection direction) const;

    const SpaceVec & getColorSpaces(bool fullPath=false) const { return (fullPath ? _colorSpacesFullPaths : _colorSpaces); }
    const std::string & getDefaultColorSpace() const { return _defaultColorSpace; }
    
    const SpaceVec & getDisplays() const { return _displays; };
    const std::string & getDefaultDisplay() const { return _defaultDisplay; }
    
    SpaceVec getViews(const std::string &display) const;
    std::string getDefaultView(const std::string &display) const;

    static void getenv(const char *name, std::string &value);

    static void getenvOCIO(std::string &value) { getenv("OCIO", value); }

  private:
    std::string _path;
    
    OCIO::ConstConfigRcPtr _config;
    
    SpaceVec _colorSpaces;
    SpaceVec _colorSpacesFullPaths;
    std::string _defaultColorSpace;
    SpaceVec _displays;
    std::string _defaultDisplay;
    
    bool _isLUT;
    bool _canInvertLUT;
};

#endif // _OPENCOLORIO_PS_CONTEXT_H_
