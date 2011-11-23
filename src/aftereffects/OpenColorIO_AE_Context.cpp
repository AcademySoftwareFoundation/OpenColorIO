/*
 *  OpenColorIO_AE_Context.cpp
 *  OpenColorIO_AE
 *
 *  Created by Brendan Bolles on 11/22/11.
 *  Copyright 2011 fnord. All rights reserved.
 *
 */

#include "OpenColorIO_AE_Context.h"

using namespace OCIO;
using namespace std;


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const string path)
{
	if(!path.empty())
	{
		string the_extension = path.substr( path.find_last_of('.') + 1 );
		
		if(the_extension == "ocio")
		{
			_config = Config::CreateFromFile( path.c_str() );
			
			_config->sanityCheck();
			
			for(int i=0; i < _config->getNumColorSpaces(); ++i)
			{
				_inputs.push_back( _config->getColorSpaceNameByIndex(i) );
			}
			
			
			for(int i=0; i < _config->getNumDisplays(); ++i)
			{
				_devices.push_back( _config->getDisplay(i) );
			}
			
			const char * defaultDisplay = _config->getDefaultDisplay();
			const char * defaultTransform = _config->getDefaultView(defaultDisplay);
			
			for(int i=0; i < _config->getNumViews(defaultDisplay); ++i)
			{
				_transforms.push_back( _config->getView(defaultDisplay, i) );
			}
			
			
			setupOCIO(ROLE_SCENE_LINEAR, defaultTransform, defaultDisplay);
			
			_ocio = true;
		}
		else
		{
			_config = Config::Create();
			
			_config->sanityCheck();
			
			setupLUT( path.c_str() );
			
			_ocio = false;
		}
	}
	else
		throw Exception("Got nothin");
}


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const ArbitraryData *arb_data)
{
	string path(arb_data->path);

	if(!path.empty())
	{
		string the_extension = path.substr( path.find_last_of('.') + 1 );
		
		if(the_extension == "ocio")
		{
			if(arb_data->type == ARB_TYPE_OCIO)
			{
				_config = Config::CreateFromFile( path.c_str() );
				
				_config->sanityCheck();
				
				for(int i=0; i < _config->getNumColorSpaces(); ++i)
				{
					_inputs.push_back( _config->getColorSpaceNameByIndex(i) );
				}
				
				
				for(int i=0; i < _config->getNumDisplays(); ++i)
				{
					_devices.push_back( _config->getDisplay(i) );
				}
				
				const char * defaultDisplay = _config->getDefaultDisplay();
				const char * defaultTransform = _config->getDefaultView(defaultDisplay);
				
				for(int i=0; i < _config->getNumViews(defaultDisplay); ++i)
				{
					_transforms.push_back( _config->getView(defaultDisplay, i) );
				}
				
				
				setupOCIO(arb_data->input, arb_data->transform, arb_data->device);
				
				_ocio = true;
			}
		}
		else
		{
			_config = Config::Create();
			
			_config->sanityCheck();
			
			setupLUT( path.c_str() );
			
			_ocio = false;
		}
	}
	else
		throw Exception("Got nothin");
}


void
OpenColorIO_AE_Context::setupOCIO(const char *input, const char *xform, const char *device)
{
	DisplayTransformRcPtr transform = DisplayTransform::Create();
	
	transform->setInputColorSpaceName(input);
	transform->setView(xform);
	transform->setDisplay(device);
	
	_input = input;
	_transform = xform;
	_device = device;
	
/*
    {
        float gain = 1.f;
        const float slope4f[] = { gain, gain, gain, gain };
        float m44[16];
        float offset4[4];
        OCIO::MatrixTransform::Scale(m44, offset4, slope4f);
        OCIO::MatrixTransformRcPtr mtx =  OCIO::MatrixTransform::Create();
        mtx->setValue(m44, offset4);
        transform->setLinearCC(mtx);
    }
    
    // Channel swizzling
    {
		int channelHot[4] = { 1, 1, 1, 1 };  // show rgb
        float lumacoef[3];
        _config->getDefaultLumaCoefs(lumacoef);
        float m44[16];
        float offset[4];
        OCIO::MatrixTransform::View(m44, offset, channelHot, lumacoef);
        OCIO::MatrixTransformRcPtr swizzle = OCIO::MatrixTransform::Create();
        swizzle->setValue(m44, offset);
        transform->setChannelView(swizzle);
    }
    
    // Post-display transform gamma
    {
        float exponent = 1.0f/2.2f;
        const float exponent4f[] = { exponent, exponent, exponent, exponent };
        OCIO::ExponentTransformRcPtr expTransform =  OCIO::ExponentTransform::Create();
        expTransform->setValue(exponent4f);
        transform->setDisplayCC(expTransform);
    }
*/	

	_processor = _config->getProcessor(transform);
}


void
OpenColorIO_AE_Context::setupLUT(const char *path)
{
	FileTransformRcPtr transform = FileTransform::Create();
	
	transform = FileTransform::Create();
	transform->setSrc( path );
	transform->setInterpolation(INTERP_LINEAR);
	transform->setDirection(TRANSFORM_DIR_FORWARD);
	
	_processor = _config->getProcessor(transform);
}
