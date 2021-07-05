// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OpenColorIO_PS_Context.h"

#include <assert.h>

#ifndef __APPLE__
#include <Windows.h>
#endif


int
FindSpace(const SpaceVec &spaceVec, const std::string &space)
{
    for(int i=0; i < spaceVec.size(); i++)
    {
        const std::string &s = spaceVec[i];
        
        if(s == space)
            return i;
    }
    
    return -1;
}

OpenColorIO_PS_Context::OpenColorIO_PS_Context(const std::string &path) :
    _path(path)
{
    const std::string extension = _path.substr( path.find_last_of('.') + 1 );
    
    if(extension == "ocio")
    {
        _config = OCIO::Config::CreateFromFile( _path.c_str() );
        
        _config->validate();
        
        _isLUT = false;
        
        for(int i=0; i < _config->getNumColorSpaces(); ++i)
        {
            const std::string colorSpaceName = _config->getColorSpaceNameByIndex(i);
            
            _colorSpaces.push_back(colorSpaceName);
        }
        
        OCIO::ConstColorSpaceRcPtr defaultInput = _config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
        
        _defaultColorSpace = (defaultInput ? defaultInput->getName() : OCIO::ROLE_SCENE_LINEAR);
        
        
        for(int i=0; i < _config->getNumDisplays(); ++i)
        {
            const std::string displayName = _config->getDisplay(i);
            
            _displays.push_back(displayName);
        }
        
        _defaultDisplay = _config->getDefaultDisplay();
    }
    else
    {
        _config = OCIO::Config::Create();
        
        
        OCIO::FileTransformRcPtr forwardTransform = OCIO::FileTransform::Create();
        
        forwardTransform->setSrc( _path.c_str() );
        forwardTransform->setInterpolation(OCIO::INTERP_LINEAR);
        forwardTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
        
        OCIO::ConstProcessorRcPtr forwardProcessor = _config->getProcessor(forwardTransform);
        
        _isLUT = true;
    }
}

OCIO::ConstCPUProcessorRcPtr
OpenColorIO_PS_Context::getConvertProcessor(const std::string &inputSpace, const std::string &outputSpace, bool invert) const
{
    assert( !isLUT() );

    OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
    
    transform->setSrc( inputSpace.c_str() );
    transform->setDst( outputSpace.c_str() );
    transform->setDirection(invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
    
    OCIO::ConstProcessorRcPtr processor = _config->getProcessor(transform);
    
    OCIO::ConstCPUProcessorRcPtr cpu_processor = processor->getDefaultCPUProcessor();
    
    return cpu_processor;
}


OCIO::ConstCPUProcessorRcPtr
OpenColorIO_PS_Context::getDisplayProcessor(const std::string &inputSpace, const std::string &display, const std::string &view, bool invert) const
{
    assert( !isLUT() );

    OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();

    transform->setSrc( inputSpace.c_str() );
    transform->setDisplay( display.c_str() );
    transform->setView( view.c_str() );
    transform->setDirection(invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::ConstProcessorRcPtr processor = _config->getProcessor(transform);
    
    OCIO::ConstCPUProcessorRcPtr cpu_processor = processor->getDefaultCPUProcessor();
    
    return cpu_processor;
}


OCIO::ConstCPUProcessorRcPtr
OpenColorIO_PS_Context::getLUTProcessor(OCIO::Interpolation interpolation, bool invert) const
{
    assert( isLUT() );

    OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
    
    transform->setSrc( _path.c_str() );
    transform->setInterpolation(interpolation);
    transform->setDirection(invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
    
    OCIO::ConstProcessorRcPtr processor = _config->getProcessor(transform);
    
    OCIO::ConstCPUProcessorRcPtr cpu_processor = processor->getDefaultCPUProcessor();
    
    return cpu_processor;
}


OCIO::BakerRcPtr
OpenColorIO_PS_Context::getConvertBaker(const std::string &inputSpace, const std::string &outputSpace, bool invert) const
{
    assert( !isLUT() );

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    baker->setConfig(_config);
    baker->setInputSpace(invert ? outputSpace.c_str() : inputSpace.c_str());
    baker->setTargetSpace(invert ? inputSpace.c_str() : outputSpace.c_str() );
	
    return baker;
}


OCIO::BakerRcPtr
OpenColorIO_PS_Context::getDisplayBaker(const std::string &inputSpace, const std::string &display, const std::string &view, bool invert) const
{
    assert( !isLUT() );

    OCIO::ConfigRcPtr editableConfig = _config->createEditableCopy();
    
    OCIO::ColorSpaceRcPtr inputColorSpace = OCIO::ColorSpace::Create();
    const std::string input_space = "RawInput";
    inputColorSpace->setName( input_space.c_str() );
    editableConfig->addColorSpace(inputColorSpace);
    
    
    OCIO::ColorSpaceRcPtr outputColorSpace = OCIO::ColorSpace::Create();
    const std::string output_space = "ProcessedOutput";
    outputColorSpace->setName( output_space.c_str() );
    
    OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
    
    transform->setSrc( inputSpace.c_str() );
    transform->setDisplay( display.c_str() );
    transform->setView( view.c_str() );
    transform->setDirection(invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
    
    outputColorSpace->setTransform(transform, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    
    editableConfig->addColorSpace(outputColorSpace);
    
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    baker->setConfig(editableConfig);
    baker->setInputSpace( input_space.c_str() );
    baker->setTargetSpace( output_space.c_str() );
    
    return baker;
}


OCIO::BakerRcPtr
OpenColorIO_PS_Context::getLUTBaker(OCIO::Interpolation interpolation, bool invert) const
{
    assert( isLUT() );

    OCIO::ConfigRcPtr editableConfig = OCIO::Config::Create();

    OCIO::ColorSpaceRcPtr inputColorSpace = OCIO::ColorSpace::Create();
    const std::string inputspace = "RawInput";
    inputColorSpace->setName(inputspace.c_str());
    editableConfig->addColorSpace(inputColorSpace);
    
    
    OCIO::ColorSpaceRcPtr outputColorSpace = OCIO::ColorSpace::Create();
    const std::string outputspace = "ProcessedOutput";
    outputColorSpace->setName(outputspace.c_str());
    
    OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
    
    transform = OCIO::FileTransform::Create();
    transform->setSrc(_path.c_str());
    transform->setInterpolation(interpolation);
    transform->setDirection(invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
    
    outputColorSpace->setTransform(transform, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    
    editableConfig->addColorSpace(outputColorSpace);
    
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    baker->setConfig(editableConfig);
    baker->setInputSpace(inputspace.c_str());
    baker->setTargetSpace(outputspace.c_str());
    
    return baker;
}


SpaceVec
OpenColorIO_PS_Context::getViews(const std::string &display) const
{
    SpaceVec transforms;
    
    for(int i=0; i < _config->getNumViews( display.c_str() ); ++i)
    {
        const std::string viewName = _config->getView(display.c_str(), i);
        
        transforms.push_back(viewName);
    }
    
    return transforms;
}


std::string
OpenColorIO_PS_Context::getDefaultView(const std::string &display) const
{
    return _config->getDefaultView( display.c_str() );
}


void
OpenColorIO_PS_Context::getenv(const char *name, std::string &value)
{
#ifdef __APPLE__
    char *env = std::getenv(name);

    value = (env != NULL ? env : "");
#else
    char env[1024] = { '\0' };

    const DWORD result = GetEnvironmentVariable(name, env, 1023);

    value = (result > 0 ? env : "");
#endif
}
