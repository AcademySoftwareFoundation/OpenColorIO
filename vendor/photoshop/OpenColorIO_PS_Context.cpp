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
#include "OpenColorIO_PS_Context.h"

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
        
        _config->sanityCheck();
        
        _isLUT = false;
        
        for(int i=0; i < _config->getNumColorSpaces(); ++i)
        {
            const std::string colorSpaceName = _config->getColorSpaceNameByIndex(i);
            
            OCIO::ConstColorSpaceRcPtr colorSpace = _config->getColorSpace( colorSpaceName.c_str() );
            
            const std::string colorSpaceFamily = colorSpace->getFamily();
            
            const std::string colorSpacePath = (colorSpaceFamily.empty() ? colorSpaceName :
                                                (colorSpaceFamily + "/" + colorSpaceName));
                                                
            _colorSpaces.push_back(colorSpaceName);
            
            _colorSpacesFullPaths.push_back(colorSpacePath);
        }
        
        OCIO::ConstColorSpaceRcPtr defaultInput = _config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
        
        _defaultColorSpace = (defaultInput ? defaultInput->getName() : OCIO::ROLE_SCENE_LINEAR);
        
        
        for(int i=0; i < _config->getNumDisplays(); ++i)
        {
            const std::string deviceName = _config->getDisplay(i);
            
            _devices.push_back(deviceName);
        }
        
        _defaultDevice = _config->getDefaultDisplay();
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
        
        _canInvertLUT = true;
        
        try
        {
            OCIO::FileTransformRcPtr inverseTransform = OCIO::FileTransform::Create();
            
            inverseTransform->setSrc( _path.c_str() );
            inverseTransform->setInterpolation(OCIO::INTERP_LINEAR);
            inverseTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
            
            OCIO::ConstProcessorRcPtr inverseProcessor = _config->getProcessor(inverseTransform);
        }
        catch(...)
        {
            _canInvertLUT = false;
        }
    }
}

OCIO::ConstProcessorRcPtr
OpenColorIO_PS_Context::getConvertProcessor(const std::string &inputSpace, const std::string &outputSpace) const
{
    assert( !isLUT() );

    OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
    
    transform->setSrc( inputSpace.c_str() );
    transform->setDst( outputSpace.c_str() );
    transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    
    OCIO::ConstProcessorRcPtr processor = _config->getProcessor(transform);
    
    return processor;
}


OCIO::ConstProcessorRcPtr
OpenColorIO_PS_Context::getDisplayProcessor(const std::string &inputSpace, const std::string &device, const std::string &transform) const
{
    assert( !isLUT() );

    OCIO::DisplayTransformRcPtr ocio_transform = OCIO::DisplayTransform::Create();

    ocio_transform->setInputColorSpaceName( inputSpace.c_str() );
    ocio_transform->setDisplay( device.c_str() );
    ocio_transform->setView( transform.c_str() );

    OCIO::ConstProcessorRcPtr processor = _config->getProcessor(ocio_transform);
    
    return processor;
}


OCIO::ConstProcessorRcPtr
OpenColorIO_PS_Context::getLUTProcessor(OCIO::Interpolation interpolation, OCIO::TransformDirection direction) const
{
    assert( isLUT() );

    OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
    
    transform->setSrc( _path.c_str() );
    transform->setInterpolation(interpolation);
    transform->setDirection(direction);
    
    OCIO::ConstProcessorRcPtr processor = _config->getProcessor(transform);
    
    return processor;
}


OCIO::BakerRcPtr
OpenColorIO_PS_Context::getConvertBaker(const std::string &inputSpace, const std::string &outputSpace) const
{
    assert( !isLUT() );

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    baker->setConfig(_config);
    baker->setInputSpace( inputSpace.c_str() );
    baker->setTargetSpace( outputSpace.c_str() );
    
    return baker;
}


OCIO::BakerRcPtr
OpenColorIO_PS_Context::getDisplayBaker(const std::string &inputSpace, const std::string &device, const std::string &transform) const
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
    
    OCIO::DisplayTransformRcPtr transformPtr = OCIO::DisplayTransform::Create();
    
    transformPtr->setInputColorSpaceName( inputSpace.c_str() );
    transformPtr->setDisplay( device.c_str() );
    transformPtr->setView( transform.c_str() );
    
    outputColorSpace->setTransform(transformPtr, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    
    editableConfig->addColorSpace(outputColorSpace);
    
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    baker->setConfig(editableConfig);
    baker->setInputSpace( input_space.c_str() );
    baker->setTargetSpace( output_space.c_str() );
    
    return baker;
}


OCIO::BakerRcPtr
OpenColorIO_PS_Context::getLUTBaker(OCIO::Interpolation interpolation, OCIO::TransformDirection direction) const
{
    assert( isLUT() );

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
    transform->setInterpolation(interpolation);
    transform->setDirection(direction);
    
    outputColorSpace->setTransform(transform, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
    
    editableConfig->addColorSpace(outputColorSpace);
    
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    baker->setConfig(editableConfig);
    baker->setInputSpace(inputspace.c_str());
    baker->setTargetSpace(outputspace.c_str());
    
    return baker;
}


SpaceVec
OpenColorIO_PS_Context::getTransforms(const std::string &device) const
{
    SpaceVec transforms;
    
    for(int i=0; i < _config->getNumViews( device.c_str() ); ++i)
    {
        const std::string transformName = _config->getView(device.c_str(), i);
        
        transforms.push_back(transformName);
    }
    
    return transforms;
}


std::string
OpenColorIO_PS_Context::getDefaultTransform(const std::string &device) const
{
    return _config->getDefaultView( device.c_str() );
}

