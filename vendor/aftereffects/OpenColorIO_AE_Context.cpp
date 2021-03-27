// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpenColorIO_AE_Context.h"

#include <fstream>
#include <map>
#include <sstream>

#include <assert.h>

#include "ocioicc.h"

#include "OpenColorIO_AE_Dialogs.h"




static const char mac_delimiter = '/';
static const char win_delimiter = '\\';

#ifdef WIN_ENV
static const char delimiter = win_delimiter;
#else
static const char delimiter = mac_delimiter;
#endif

static const int LUT3D_EDGE_SIZE = 32;


Path::Path(const std::string &path, const std::string &dir) :
    _path(path),
    _dir(dir)
{

}


Path::Path(const Path &path)
{
    _path = path._path;
    _dir  = path._dir;
}


std::string Path::full_path() const
{
    if( !_path.empty() && is_relative(_path) && !_dir.empty() )
    {
        std::vector<std::string> path_vec = components( convert_delimiters(_path) );
        std::vector<std::string> dir_vec = components(_dir);
        
        int up_dirs = 0;
        int down_dirs = 0;
        
        while(down_dirs < path_vec.size() - 1 &&
                (path_vec[down_dirs] == ".." || path_vec[down_dirs] == ".") )
        {
            down_dirs++;
            
            if(path_vec[down_dirs] == "..")
                up_dirs++;
        }
        
        
        std::string path;
        
        if(path_type(_dir) == TYPE_MAC)
            path += mac_delimiter;
            
        for(int i=0; i < dir_vec.size() - up_dirs; i++)
        {
            path += dir_vec[i] + delimiter;
        }
        
        for(int i = down_dirs; i < path_vec.size() - 1; i++)
        {
            path += path_vec[i] + delimiter;
        }
        
        path += path_vec.back();
        
        return path;
    }
    else
    {
        return _path;
    }
}


std::string Path::relative_path(bool force) const
{
    if( _dir.empty() || _path.empty() || is_relative(_path) )
    {
        return _path;
    }
    else
    {
        std::vector<std::string> path_vec = components(_path);
        std::vector<std::string> dir_vec = components(_dir);
        
        int match_idx = 0;
        
        while(match_idx < path_vec.size() &&
                match_idx < dir_vec.size() &&
                path_vec[match_idx] == dir_vec[match_idx])
            match_idx++;
        
        if(match_idx == 0)
        {
            // can't do relative path
            if(force)
                return _path;
            else
                return "";
        }
        else
        {
            std::string rel_path;
            
            // is the file in a folder below or actually inside the dir?
            if(match_idx == dir_vec.size())
            {
                rel_path += std::string(".") + delimiter;
            }
            else
            {
                for(int i = match_idx; i < dir_vec.size(); i++)
                {
                    rel_path += std::string("..") + delimiter;
                }
            }
                
            for(int i = match_idx; i < path_vec.size() - 1; i++)
            {
                rel_path += path_vec[i] + delimiter;
            }
            
            rel_path += path_vec.back();
            
            return rel_path;
        }
    }
}


bool Path::exists() const
{
    std::string path = full_path();
    
    if(path.empty())
        return false;
    
    std::ifstream f( path.c_str() );
    
    return !!f;
}


Path::PathType Path::path_type(const std::string &path)
{
    if( path.empty() )
    {
        return TYPE_UNKNOWN;
    }
    if(path[0] == mac_delimiter)
    {
        return TYPE_MAC;
    }
    else if(path[1] == ':' && path[2] == win_delimiter)
    {
        return TYPE_WIN;
    }
    else if(path[0] == win_delimiter && path[1] == win_delimiter)
    {
        return TYPE_WIN;
    }
    else
    {
        size_t mac_pos = path.find(mac_delimiter);
        size_t win_pos = path.find(win_delimiter);
        
        if(mac_pos != std::string::npos && win_pos == std::string::npos)
        {
            return TYPE_MAC;
        }
        else if(mac_pos == std::string::npos && win_pos != std::string::npos)
        {
            return TYPE_WIN;
        }
        else if(mac_pos == std::string::npos && win_pos == std::string::npos)
        {
            return TYPE_UNKNOWN;
        }
        else // neither npos?
        {
            if(mac_pos < win_pos)
                return TYPE_MAC;
            else
                return TYPE_WIN;
        }
    }
}


bool Path::is_relative(const std::string &path)
{
    Path::PathType type = path_type(path);
    
    if(type == TYPE_MAC)
    {
        return (path[0] != mac_delimiter);
    }
    else if(type == TYPE_WIN)
    {
        return !( (path[1] == ':' && path[2] == win_delimiter) ||
                  (path[0] == win_delimiter && path[1] == win_delimiter) );
    }
    else
    {   // TYPE_UNKNOWN
    
        // just a filename perhaps?
        // should never have this: even a file in the same directory will be ./file.ocio
        // we'll assume it's relative, but raise a stink during debugging
        assert(type != TYPE_UNKNOWN);
        
        return true;
    }
}


std::string Path::convert_delimiters(const std::string &path)
{
#ifdef WIN_ENV
    const char search = mac_delimiter;
    const char replace = win_delimiter;
#else
    const char search = win_delimiter;
    const char replace = mac_delimiter;
#endif

    std::string path_copy = path;

    for(int i=0; i < path_copy.size(); i++)
    {
        if(path_copy[i] == search)
            path_copy[i] = replace;
    }
    
    return path_copy;
}


std::vector<std::string> Path::components(const std::string &path)
{
    std::vector<std::string> vec;
    
    size_t pos = 0;
    size_t len = path.size();
    
    size_t start, finish;
    
    while(path[pos] == delimiter && pos < len)
        pos++;
    
    while(pos < len)
    {
        start = pos;
        
        while(path[pos] != delimiter && pos < len)
            pos++;
        
        finish = ((pos == len - 1) ? pos : pos - 1);
        
        vec.push_back( path.substr(start, 1 + finish - start) );
        
        while(path[pos] == delimiter && pos < len)
            pos++;
    }
    
    return vec;
}


#pragma mark-


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const std::string &path, OCIO_Source source) :
    _gl_init(false)
{
    _action = OCIO_ACTION_NONE;
    
    
    _source = source;
    
    if(_source == OCIO_SOURCE_ENVIRONMENT)
    {
        std::string env;
        getenvOCIO(env);
        
        if(!env.empty())
        {
            _path = env;
        }
        else
            throw OCIO::Exception("No $OCIO environment variable.");
    }
    else if(_source == OCIO_SOURCE_STANDARD)
    {
        _config_name = path;
        
        _path = GetStdConfigPath(_config_name);
        
        if( _path.empty() )
            throw OCIO::Exception("Error getting config.");
    }
    else
    {
        _path = path;
    }
    
    
    if(!_path.empty())
    {
        std::string the_extension = _path.substr( _path.find_last_of('.') + 1 );
        
        if(the_extension == "ocio")
        {
            _config = OCIO::Config::CreateFromFile( _path.c_str() );
            
            _config->validate();
            
            for(int i=0; i < _config->getNumColorSpaces(); ++i)
            {
                const char *colorSpaceName = _config->getColorSpaceNameByIndex(i);
                
                OCIO::ConstColorSpaceRcPtr colorSpace = _config->getColorSpace(colorSpaceName);
                
                const char *family = colorSpace->getFamily();
                
                _inputs.push_back(colorSpaceName);
                
                const std::string fullPath = (family == NULL ? colorSpaceName : std::string(family) + "/" + colorSpaceName);
                
                _inputsFullPath.push_back(fullPath);
            }
            
            
            for(int i=0; i < _config->getNumDisplays(); ++i)
            {
                _displays.push_back( _config->getDisplay(i) );
            }
            
            
            OCIO::ConstColorSpaceRcPtr defaultInput = _config->getColorSpace(OCIO::ROLE_DEFAULT);
            
            const char *defaultInputName = (defaultInput ? defaultInput->getName() : OCIO::ROLE_DEFAULT);
            
            
            setupConvert(defaultInputName, defaultInputName);
            
            
            const char *defaultDisplay = _config->getDefaultDisplay();
            const char *defaultView = _config->getDefaultView(defaultDisplay);
            
            _display = defaultDisplay;
            _view = defaultView;
        }
        else
        {
            _config = OCIO::Config::Create();
            
            setupLUT(false, OCIO_INTERP_LINEAR);
        }
    }
    else
        throw OCIO::Exception("Got nothin");
}


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const ArbitraryData *arb_data, const std::string &dir) :
    _gl_init(false)
{
    _action = OCIO_ACTION_NONE;
    
    
    _source = arb_data->source;
    
    if(_source == OCIO_SOURCE_ENVIRONMENT)
    {
        std::string env;
        getenvOCIO(env);
        
        if(!env.empty())
        {
            _path = env;
        }
        else
            throw OCIO::Exception("No $OCIO environment variable.");
    }
    else if(_source == OCIO_SOURCE_STANDARD)
    {
        _config_name = arb_data->path;
        
        _path = GetStdConfigPath(_config_name);
        
        if( _path.empty() )
            throw OCIO::Exception("Error getting config.");
    }
    else
    {
        Path absolute_path(arb_data->path, dir);
        Path relative_path(arb_data->relative_path, dir);
        
        if( absolute_path.exists() )
        {
            _path = absolute_path.full_path();
        }
        else
        {
            _path = relative_path.full_path();
        }
    }
    

    if(!_path.empty())
    {
        std::string the_extension = _path.substr( _path.find_last_of('.') + 1 );
        
        if(the_extension == "ocio")
        {
            _config = OCIO::Config::CreateFromFile( _path.c_str() );
            
            _config->validate();
            
            for(int i=0; i < _config->getNumColorSpaces(); ++i)
            {
                const char *colorSpaceName = _config->getColorSpaceNameByIndex(i);
                
                OCIO::ConstColorSpaceRcPtr colorSpace = _config->getColorSpace(colorSpaceName);
                
                const char *family = colorSpace->getFamily();
                
                _inputs.push_back(colorSpaceName);
                
                const std::string fullPath = (family == NULL ? colorSpaceName : std::string(family) + "/" + colorSpaceName);
                
                _inputsFullPath.push_back(fullPath);
            }
            
            
            for(int i=0; i < _config->getNumDisplays(); ++i)
            {
                _displays.push_back( _config->getDisplay(i) );
            }
            
            if(arb_data->action == OCIO_ACTION_CONVERT)
            {
                setupConvert(arb_data->input, arb_data->output);
                
                _display = arb_data->display;
                _view = arb_data->view;
            }
            else
            {
                setupDisplay(arb_data->input, arb_data->display, arb_data->view);
                
                _output = arb_data->output;
            }
        }
        else
        {
            _config = OCIO::Config::Create();
            
            setupLUT(arb_data->invert, arb_data->interpolation);
        }
    }
    else
        throw OCIO::Exception("Got nothin");
}


OpenColorIO_AE_Context::~OpenColorIO_AE_Context()
{
    if(_gl_init)
    {
        glDeleteTextures(1, &_imageTexID);
        
        if(_bufferWidth != 0 && _bufferHeight != 0)
            glDeleteRenderbuffersEXT(1, &_renderBuffer);
    }
}


bool OpenColorIO_AE_Context::Verify(const ArbitraryData *arb_data, const std::string &dir)
{
    if(_source != arb_data->source)
        return false;
    
    if(_source == OCIO_SOURCE_STANDARD)
    {
        if(_config_name != arb_data->path)
            return false;
    }
    else if(_source == OCIO_SOURCE_CUSTOM)
    {
        // comparing the paths, cheking relative path only if necessary
        if(_path != arb_data->path)
        {
            std::string rel_path(arb_data->relative_path);
            
            if( !dir.empty() && !rel_path.empty() )
            {
                Path relative_path(rel_path, dir);
                
                if( _path != relative_path.full_path() )
                    return false;
            }
            else
                return false;
        }
    }
    
    // we can switch between Convert and Display, but not LUT and non-LUT
    if((arb_data->action == OCIO_ACTION_NONE) ||
        (_action == OCIO_ACTION_LUT && arb_data->action != OCIO_ACTION_LUT) ||
        (_action != OCIO_ACTION_LUT && arb_data->action == OCIO_ACTION_LUT) )
    {
        return false;
    }
    
    bool force_reset = (_action != arb_data->action);   
    
    
    // If the type and path are compatible, we can patch up
    // differences here and return true.
    // Returning false means the context will be deleted and rebuilt.
    if(arb_data->action == OCIO_ACTION_LUT)
    {
        if(_invert != arb_data->invert ||
            _interpolation != arb_data->interpolation ||
            force_reset)
        {
            setupLUT(arb_data->invert, arb_data->interpolation);
        }
    }
    else if(arb_data->action == OCIO_ACTION_CONVERT)
    {
        if(_input != arb_data->input ||
            _output != arb_data->output ||
            force_reset)
        {
            setupConvert(arb_data->input, arb_data->output);
        }
    }
    else if(arb_data->action == OCIO_ACTION_DISPLAY)
    {
        if(_input != arb_data->input ||
            _display != arb_data->display ||
            _view != arb_data->view ||
            force_reset)
        {
            setupDisplay(arb_data->input, arb_data->display, arb_data->view);
        }
    }
    else
        throw OCIO::Exception("Bad OCIO type");
    
    
    return true;
}


void OpenColorIO_AE_Context::setupConvert(const char *input, const char *output)
{
    OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
    
    transform->setSrc(input);
    transform->setDst(output);
    transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    
    _input = input;
    _output = output;
    
    _processor = _config->getProcessor(transform);
    
    _cpu_processor = _processor->getDefaultCPUProcessor();
    _gpu_processor = _processor->getDefaultGPUProcessor();
    
    _action = OCIO_ACTION_CONVERT;
    
    UpdateOCIOGLState();
}


void OpenColorIO_AE_Context::setupDisplay(const char *input, const char *display, const char *view)
{
    _views.clear();
    
    bool viewValid = false;
    
    for(int i=0; i < _config->getNumViews(display); i++)
    {
        const std::string viewName = _config->getView(display, i);
        
        if(viewName == view)
            viewValid = true;
        
        _views.push_back(viewName);
    }
    
    if(!viewValid)
        view = _config->getDefaultView(display);
    

    OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
    
    transform->setSrc(input);
    transform->setDisplay(display);
    transform->setView(view);
    
    _input = input;
    _display = display;
    _view = view;
    

    _processor = _config->getProcessor(transform);
    
    _cpu_processor = _processor->getDefaultCPUProcessor();
    _gpu_processor = _processor->getDefaultGPUProcessor();
    
    _action = OCIO_ACTION_DISPLAY;
    
    UpdateOCIOGLState();
}


void OpenColorIO_AE_Context::setupLUT(OCIO_Invert invert, OCIO_Interp interpolation)
{
    OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
    
    if(interpolation != OCIO_INTERP_NEAREST && interpolation != OCIO_INTERP_LINEAR &&
        interpolation != OCIO_INTERP_TETRAHEDRAL && interpolation != OCIO_INTERP_CUBIC &&
        interpolation != OCIO_INTERP_BEST)
    {
        interpolation = OCIO_INTERP_LINEAR;
    }
    
    transform->setSrc( _path.c_str() );
    transform->setInterpolation(static_cast<OCIO::Interpolation>(interpolation));
    transform->setDirection(invert > OCIO_INVERT_OFF ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
    
    _processor = _config->getProcessor(transform);
    
    if(invert == OCIO_INVERT_EXACT)
    {
        _cpu_processor = _processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_LOSSLESS);
        _gpu_processor = _processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_LOSSLESS);
    }
    else
    {
        _cpu_processor = _processor->getDefaultCPUProcessor();
        _gpu_processor = _processor->getDefaultGPUProcessor();
    }

    _invert = invert;
    _interpolation = interpolation;
    
    _action = OCIO_ACTION_LUT;
    
    UpdateOCIOGLState();
}


bool OpenColorIO_AE_Context::ExportLUT(const std::string &path, const std::string &display_icc_path)
{
    std::string the_extension = path.substr( path.find_last_of('.') + 1 );
    
    try{
    
    if(the_extension == "icc")
    {
        int cubesize = 32;
        int whitepointtemp = 6505;
        std::string copyright = "";
        
        // create a description tag from the filename
        size_t filename_start = path.find_last_of(delimiter) + 1;
        size_t filename_end = path.find_last_of('.') - 1;
        
        std::string description = path.substr(path.find_last_of(delimiter) + 1,
                                            1 + filename_end - filename_start);
        
        SaveICCProfileToFile(path, _cpu_processor, cubesize, whitepointtemp,
                                display_icc_path, description, copyright, false);
    }
    else
    {
        // this code lovingly pulled from ociobakelut
        
        // need an extension->format map (yes, just did this one call up)
        std::map<std::string, std::string> extensions;
        
        for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
        {
            const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
            const char *format = OCIO::Baker::getFormatNameByIndex(i);
            
            extensions[ extension ] = format;
        }
        
        std::string format = extensions[ the_extension ];
        
        
        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        
        baker->setFormat(format.c_str());
        
        if(_action == OCIO_ACTION_CONVERT)
        {
            baker->setConfig(_config);
            baker->setInputSpace(_input.c_str());
            baker->setTargetSpace(_output.c_str());
        
            std::ofstream f(path.c_str());
            baker->bake(f);
        }
        else if(_action == OCIO_ACTION_DISPLAY)
        {
            OCIO::ConfigRcPtr editableConfig = _config->createEditableCopy();
            
            OCIO::ColorSpaceRcPtr inputColorSpace = OCIO::ColorSpace::Create();
            std::string inputspace = "RawInput";
            inputColorSpace->setName(inputspace.c_str());
            editableConfig->addColorSpace(inputColorSpace);
            
            
            OCIO::ColorSpaceRcPtr outputColorSpace = OCIO::ColorSpace::Create();
            std::string outputspace = "ProcessedOutput";
            outputColorSpace->setName(outputspace.c_str());
            
            OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
            
            transform->setSrc(_input.c_str());
            transform->setDisplay(_display.c_str());
            transform->setView(_view.c_str());

            outputColorSpace->setTransform(transform, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
            
            editableConfig->addColorSpace(outputColorSpace);
            
            
            baker->setConfig(editableConfig);
            baker->setInputSpace(inputspace.c_str());
            baker->setTargetSpace(outputspace.c_str());
            
            std::ofstream f(path.c_str());
            baker->bake(f);
        }
        else if(_action == OCIO_ACTION_LUT)
        {
            OCIO::ConfigRcPtr editableConfig = OCIO::Config::Create();

            OCIO::ColorSpaceRcPtr inputColorSpace = OCIO::ColorSpace::Create();
            std::string inputspace = "RawInput";
            inputColorSpace->setName(inputspace.c_str());
            editableConfig->addColorSpace(inputColorSpace);
            
            
            OCIO::ColorSpaceRcPtr outputColorSpace = OCIO::ColorSpace::Create();
            std::string outputspace = "ProcessedOutput";
            outputColorSpace->setName(outputspace.c_str());
            
            OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
            
            transform = OCIO::FileTransform::Create();
            transform->setSrc(_path.c_str());
            transform->setInterpolation(static_cast<OCIO::Interpolation>(_interpolation));
            transform->setDirection(_invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
            
            outputColorSpace->setTransform(transform, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
            
            editableConfig->addColorSpace(outputColorSpace);
            
            
            baker->setConfig(editableConfig);
            baker->setInputSpace(inputspace.c_str());
            baker->setTargetSpace(outputspace.c_str());
            
            std::ofstream f(path.c_str());
            baker->bake(f);
        }
    }
    
    }catch(...) { return false; }
    
    return true;
}


void OpenColorIO_AE_Context::InitOCIOGL()
{
    if(!_gl_init)
    {
        SetPluginContext();
    
        glGenTextures(1, &_imageTexID);
                            
        _bufferWidth = _bufferHeight = 0;
        
        _gl_init = true;
        
        SetAEContext();
    }
}


static const char * g_fragShaderText = ""
"\n"
"uniform sampler2D img;\n"
"\n"
"void main()\n"
"{\n"
"    vec4 col = texture2D(img, gl_TexCoord[0].st);\n"
"    gl_FragColor = OCIOMain(col);\n"
"}\n";


void OpenColorIO_AE_Context::UpdateOCIOGLState()
{
    if(_gl_init)
    {
        SetPluginContext();
        
        try
        {
            // Step 1: Create a GPU Shader Description
            OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
            shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_2);
            shaderDesc->setFunctionName("OCIOMain");
            shaderDesc->setResourcePrefix("ocio_");
            
            // Step 2: Collect the shader program information for a specific processor
            _gpu_processor->extractGpuShaderInfo(shaderDesc);
            
            // Step 3: Use the helper OpenGL builder
            _oglBuilder = OCIO::OpenGLBuilder::Create(shaderDesc);
            //_oglBuilder->setVerbose(true);
            
            // Step 4: Allocate & upload all the LUTs
            //
            // NB: The start index for the texture indices is 1 as one texture
            //     was already created for the input image.
            //
            _oglBuilder->allocateAllTextures(1);
            
            // Step 5: Build the fragment shader program
            _oglBuilder->buildProgram(g_fragShaderText);
        }
        catch(...)
        {
            _oglBuilder.reset();
        }
        
        SetAEContext();
    }
}


bool OpenColorIO_AE_Context::ProcessWorldGL(PF_EffectWorld *float_world)
{
    if(!_gl_init)
    {
        InitOCIOGL();
        UpdateOCIOGLState();
    }
    
    
    if(!_oglBuilder)
        return false;
    
    
    SetPluginContext();
    
    
    GLint max;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
    
    if(max < float_world->width || max < float_world->height || GL_NO_ERROR != glGetError())
    {
        SetAEContext();
        return false;
    }
    
    
    PF_PixelFloat *pix = (PF_PixelFloat *)float_world->data;
    float *rgba_origin = &pix->red;
    
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _imageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, float_world->width, float_world->height, 0,
        GL_RGBA, GL_FLOAT, rgba_origin);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    
    // Step 6: Enable the fragment shader program, and all needed textures
    _oglBuilder->useProgram();
    glUniform1i(glGetUniformLocation(_oglBuilder->getProgramHandle(), "img"), 0);
    _oglBuilder->useAllTextures();
    _oglBuilder->useAllUniforms();
    
    
    if(GL_NO_ERROR != glGetError())
    {
        SetAEContext();
        return false;
    }
    
    
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, GetFrameBuffer());
    
    if(_bufferWidth != float_world->width || _bufferHeight != float_world->height)
    {
        if(_bufferWidth != 0 && _bufferHeight != 0)
            glDeleteRenderbuffersEXT(1, &_renderBuffer);
        
        _bufferWidth = float_world->width;
        _bufferHeight = float_world->height;
        
        glGenRenderbuffersEXT(1, &_renderBuffer);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _renderBuffer);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA32F_ARB, _bufferWidth, _bufferHeight);
        
        // attach renderbuffer to framebuffer
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                        GL_RENDERBUFFER_EXT, _renderBuffer);
    }
    
    if(GL_FRAMEBUFFER_COMPLETE_EXT != glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT))
    {
        SetAEContext();
        return false;
    }
    
    
    glViewport(0, 0, float_world->width, float_world->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, float_world->width, 0.0, float_world->height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1, 1, 1);
    
    glPushMatrix();
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0.0f, float_world->height);
    
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(float_world->width, 0.0f);
    
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(float_world->width, float_world->height);
    
    glEnd();
    glPopMatrix();
    
    glDisable(GL_TEXTURE_2D);
    
    
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glReadPixels(0, 0, float_world->width, float_world->height,
                    GL_RGBA, GL_FLOAT, rgba_origin);
    
    glFinish();
    
    const bool result = (GL_NO_ERROR == glGetError());
    
    SetAEContext();
    
    return true;
}


void OpenColorIO_AE_Context::getenv(const char *name, std::string &value)
{
#ifdef WIN_ENV
    char env[1024] = { '\0' };

    const DWORD result = GetEnvironmentVariable(name, env, 1023);

    value = (result > 0 ? env : "");
#else
    char *env = std::getenv(name);

    value = (env != NULL ? env : "");
#endif
}
