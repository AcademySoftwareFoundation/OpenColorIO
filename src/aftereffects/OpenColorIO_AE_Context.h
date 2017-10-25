/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
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


#ifndef _OPENCOLORIO_AE_CONTEXT_H_
#define _OPENCOLORIO_AE_CONTEXT_H_

#include <string>
#include <vector>

#include "OpenColorIO_AE.h"
#include "OpenColorIO_AE_GL.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;



// yeah, this probably could/should go in a seperate file
class Path
{
  public:
    Path(const std::string &path, const std::string &dir);
    Path(const Path &path);
    ~Path() {}
    
    std::string full_path() const;
    std::string relative_path(bool force) const;
    
    bool exists() const;
    
  private:
    std::string _path;
    std::string _dir;
    
    typedef enum {
        TYPE_UNKNOWN = 0,
        TYPE_MAC,
        TYPE_WIN
    } PathType;
    
    static PathType path_type(const std::string &path);
    static bool is_relative(const std::string &path);
    static std::string convert_delimiters(const std::string &path);
    static std::vector<std::string> components(const std::string &path);
};


class OpenColorIO_AE_Context
{
  public:
    OpenColorIO_AE_Context(const std::string &path, OCIO_Source source);
    OpenColorIO_AE_Context(const ArbitraryData *arb_data, const std::string &dir);
    ~OpenColorIO_AE_Context();
    
    bool Verify(const ArbitraryData *arb_data, const std::string &dir);
    
    void setupConvert(const char *input, const char *output);
    void setupDisplay(const char *input, const char *device, const char *transform);
    void setupLUT(bool invert, OCIO_Interp interpolation);
  
    typedef std::vector<std::string> SpaceVec;

    OCIO_Action getAction() const { return _action; }
    const std::string & getInput() const { return _input; }
    const std::string & getOutput() const { return _output; }
    const std::string & getDevice() const { return _device; }
    const std::string & getTransform() const { return _transform; }
    const SpaceVec & getInputs(bool fullPath=false) const { return fullPath ? _inputsFullPath : _inputs; }
    const SpaceVec & getDevices() const { return _devices; }
    const SpaceVec & getTransforms() const { return _transforms; }
    
    OCIO::ConstConfigRcPtr config() const { return _config; }
    OCIO::ConstProcessorRcPtr processor() const { return _processor; }
    
    bool ExportLUT(const std::string &path, const std::string &display_icc_path);
    
    bool ProcessWorldGL(PF_EffectWorld *float_world);

  private:
    std::string _path;
    OCIO_Source _source;
    std::string _config_name;
  
    OCIO_Action _action;
    
    std::string _input;
    std::string _output;
    std::string _device;
    std::string _transform;
    SpaceVec _inputs;
    SpaceVec _inputsFullPath;
    SpaceVec _devices;
    SpaceVec _transforms;
    
    bool _invert;
    OCIO_Interp _interpolation;
    
    
    OCIO::ConstConfigRcPtr      _config;
    OCIO::ConstProcessorRcPtr   _processor;
    
    
    bool _gl_init;
    
    GLuint _fragShader;
    GLuint _program;

    GLuint _imageTexID;

    GLuint _lut3dTexID;
    std::vector<float> _lut3d;
    std::string _lut3dcacheid;
    std::string _shadercacheid;

    std::string _inputColorSpace;
    std::string _display;
    std::string _transformName;
    
    
    GLuint _renderBuffer;
    int _bufferWidth;
    int _bufferHeight;
    
    void InitOCIOGL();
    void UpdateOCIOGLState();
};


#endif // _OPENCOLORIO_AE_CONTEXT_H_