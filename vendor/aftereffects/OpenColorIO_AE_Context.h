// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef _OPENCOLORIO_AE_CONTEXT_H_
#define _OPENCOLORIO_AE_CONTEXT_H_

#include <string>
#include <vector>

#include "OpenColorIO_AE.h"
#include "OpenColorIO_AE_GL.h"

#include "glsl.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;



// yeah, this probably could/should go in a separate file
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
    void setupDisplay(const char *input, const char *display, const char *view);
    void setupLUT(OCIO_Invert invert, OCIO_Interp interpolation);
  
    typedef std::vector<std::string> SpaceVec;

    OCIO_Action getAction() const { return _action; }
    const std::string & getInput() const { return _input; }
    const std::string & getOutput() const { return _output; }
    const std::string & getDisplay() const { return _display; }
    const std::string & getView() const { return _view; }
    const SpaceVec & getInputs(bool fullPath=false) const { return fullPath ? _inputsFullPath : _inputs; }
    const SpaceVec & getDisplays() const { return _displays; }
    const SpaceVec & getViews() const { return _views; }
    
    OCIO::ConstConfigRcPtr config() const { return _config; }
    OCIO::ConstProcessorRcPtr processor() const { return _processor; }
    OCIO::ConstCPUProcessorRcPtr cpu_processor() const { return _cpu_processor; }
    OCIO::ConstGPUProcessorRcPtr gpu_processor() const { return _gpu_processor; }
    
    bool ExportLUT(const std::string &path, const std::string &display_icc_path);
    
    bool ProcessWorldGL(PF_EffectWorld *float_world);

    static void getenv(const char *name, std::string &value);

    static void getenvOCIO(std::string &value) { getenv("OCIO", value); }

  private:
    std::string _path;
    OCIO_Source _source;
    std::string _config_name;
  
    OCIO_Action _action;
    
    std::string _input;
    std::string _output;
    std::string _display;
    std::string _view;
    SpaceVec _inputs;
    SpaceVec _inputsFullPath;
    SpaceVec _displays;
    SpaceVec _views;
    
    OCIO_Invert _invert;
    OCIO_Interp _interpolation;
    
    
    OCIO::ConstConfigRcPtr _config;
    OCIO::ConstProcessorRcPtr _processor;
    OCIO::ConstCPUProcessorRcPtr _cpu_processor;
    OCIO::ConstGPUProcessorRcPtr _gpu_processor;
    
    
    bool _gl_init;
    
    OCIO::OpenGLBuilderRcPtr _oglBuilder;

    GLuint _imageTexID;

    GLuint _renderBuffer;
    int _bufferWidth;
    int _bufferHeight;
    
    void InitOCIOGL();
    void UpdateOCIOGLState();
};


#endif // _OPENCOLORIO_AE_CONTEXT_H_
