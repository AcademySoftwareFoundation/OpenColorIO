/*
 *  OpenColorIO_AE_Context.cpp
 *  OpenColorIO_AE
 *
 *  Created by Brendan Bolles on 11/22/11.
 *  Copyright 2011 fnord. All rights reserved.
 *
 */

#include "OpenColorIO_AE_Context.h"

#include "lcms2.h"
#include "lcms2_plugin.h"

#include <map>
#include <fstream>

using namespace OCIO;
using namespace std;


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const string path)
{
	_type = OCIO_TYPE_NONE;
	
	_path = path;
	
	if(!_path.empty())
	{
		string the_extension = _path.substr( _path.find_last_of('.') + 1 );
		
		if(the_extension == "ocio")
		{
			_config = Config::CreateFromFile( _path.c_str() );
			
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
			
			
			setupConvert(ROLE_SCENE_LINEAR, ROLE_SCENE_LINEAR);
			
			_transform = defaultTransform;
			_device = defaultDisplay;
		}
		else
		{
			_config = Config::Create();
			
			setupLUT();
		}
	}
	else
		throw Exception("Got nothin");
}


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const ArbitraryData *arb_data)
{
	_type = OCIO_TYPE_NONE;
	
	_path = arb_data->path;

	if(!_path.empty())
	{
		string the_extension = _path.substr( _path.find_last_of('.') + 1 );
		
		if(the_extension == "ocio")
		{
			_config = Config::CreateFromFile( _path.c_str() );
			
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
			
			
			if(arb_data->type == OCIO_TYPE_CONVERT)
			{
				setupConvert(arb_data->input, arb_data->output);
				
				_transform = arb_data->transform;
				_device = arb_data->device;
			}
			else
			{
				setupDisplay(arb_data->input, arb_data->transform, arb_data->device);
				
				_output = arb_data->output;
			}
		}
		else
		{
			_config = Config::Create();
			
			setupLUT(arb_data->invert);
		}
	}
	else
		throw Exception("Got nothin");
}

void
OpenColorIO_AE_Context::setupConvert(const char *input, const char *output)
{
	ColorSpaceTransformRcPtr transform = ColorSpaceTransform::Create();
	
	transform->setSrc(input);
	transform->setDst(output);
	transform->setDirection(TRANSFORM_DIR_FORWARD);
	
	_input = input;
	_output = output;
	
	_processor = _config->getProcessor(transform);
	
	_type = OCIO_TYPE_CONVERT;
}


void
OpenColorIO_AE_Context::setupDisplay(const char *input, const char *xform, const char *device)
{
	DisplayTransformRcPtr transform = DisplayTransform::Create();
	
	transform->setInputColorSpaceName(input);
	transform->setView(xform);
	transform->setDisplay(device);
	
	_input = input;
	_transform = xform;
	_device = device;
	

	_processor = _config->getProcessor(transform);
	
	_type = OCIO_TYPE_DISPLAY;
}


void
OpenColorIO_AE_Context::setupLUT(bool invert)
{
	_invert = invert;
	
	FileTransformRcPtr transform = FileTransform::Create();
	
	transform = FileTransform::Create();
	transform->setSrc( _path.c_str() );
	transform->setInterpolation(INTERP_LINEAR);
	transform->setDirection(_invert ? TRANSFORM_DIR_INVERSE : TRANSFORM_DIR_FORWARD);
	
	_processor = _config->getProcessor(transform);
	
	_type = OCIO_TYPE_LUT;
}


// these functions ripped out of ocio2icc
static void
ErrorHandler(cmsContext /*ContextID*/, cmsUInt32Number /*ErrorCode*/, const char *Text)
{
	throw Exception("lcms error");
}

typedef struct
{
    cmsHTRANSFORM to_PCS16;
    cmsHTRANSFORM from_PCS16;
    //OCIO::ConstProcessorRcPtr shaper_processor;
    OCIO::ConstProcessorRcPtr processor;
} SamplerData;

static void Add3GammaCurves(cmsPipeline* lut, cmsFloat64Number Curve)
{
    cmsToneCurve* id = cmsBuildGamma(NULL, Curve);
    cmsToneCurve* id3[3];
    id3[0] = id;
    id3[1] = id;
    id3[2] = id;
    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocToneCurves(NULL, 3, id3));
    cmsFreeToneCurve(id);
}

static void AddIdentityMatrix(cmsPipeline* lut)
{
    const cmsFloat64Number Identity[] = {
        1, 0, 0,
        0, 1, 0, 
        0, 0, 1, 
        0, 0, 0 };
    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocMatrix(NULL, 3, 3, Identity, NULL));
}

static cmsInt32Number Display2PCS_Sampler16(const cmsUInt16Number in[], cmsUInt16Number out[], void* userdata)
{
    //std::cout << "r" << in[0] << " g" << in[1] << " b" << in[2] << "\n";
    SamplerData* data = (SamplerData*) userdata;
    cmsFloat32Number pix[3] = { static_cast<float>(in[0])/65535.f,
                                static_cast<float>(in[1])/65535.f,
                                static_cast<float>(in[2])/65535.f};
    data->processor->applyRGB(pix);
    out[0] = (cmsUInt16Number)std::max(std::min(pix[0] * 65535.f, 65535.f), 0.f);
    out[1] = (cmsUInt16Number)std::max(std::min(pix[1] * 65535.f, 65535.f), 0.f);
    out[2] = (cmsUInt16Number)std::max(std::min(pix[2] * 65535.f, 65535.f), 0.f);
    cmsDoTransform(data->to_PCS16, out, out, 1);
    return 1;
}

static cmsInt32Number PCS2Display_Sampler16(const cmsUInt16Number in[], cmsUInt16Number out[], void* userdata)
{
    //std::cout << "r" << in[0] << " g" << in[1] << " b" << in[2] << "\n";
    SamplerData* data = (SamplerData*) userdata;
    cmsDoTransform(data->from_PCS16, in, out, 1);
    // we don't have a reverse Lab -> Display transform
    return 1;
}

bool
OpenColorIO_AE_Context::ExportLUT(const string path, const string display_icc_path)
{
	string the_extension = path.substr( path.find_last_of('.') + 1 );
	
	try{
	
	if(the_extension == "icc")
	{
		int cubesize = 32;
		int whitepointtemp = 6505;
		string copyright = "OpenColorIO, Sony Imageworks";
		
		// create a description tag from the filename
#ifdef WIN32
#define PATH_DELIMITER	'\\'
#else
#define PATH_DELIMITER	'/'
#endif
		size_t filename_start = path.find_last_of(PATH_DELIMITER) + 1;
		size_t filename_end = path.find_last_of('.') - 1;
		
		string description = path.substr( path.find_last_of(PATH_DELIMITER) + 1, filename_end - filename_start);
		
		
		// Create the ICC Profile

		// Setup the Error Handler
		cmsSetLogErrorHandler(ErrorHandler);

		// D65 white point
		cmsCIExyY whitePoint;
		cmsWhitePointFromTemp(&whitePoint, whitepointtemp);

		// LAB PCS
		cmsHPROFILE labProfile = cmsCreateLab4ProfileTHR(NULL, &whitePoint);

		// Display (OCIO sRGB cube -> LAB)
		cmsHPROFILE DisplayProfile;
		if(display_icc_path != "") DisplayProfile = cmsOpenProfileFromFile(display_icc_path.c_str(), "r");
		else DisplayProfile = cmsCreate_sRGBProfileTHR(NULL);

		// Create an empty RGB Profile
		cmsHPROFILE hProfile = cmsCreateRGBProfileTHR(NULL, &whitePoint, NULL, NULL);

		//std::cout << "[OpenColorIO INFO]: Setting up Profile: " << outputfile << "\n";

		// Added Header fields
		cmsSetProfileVersion(hProfile, 4.2);
		cmsSetDeviceClass(hProfile, cmsSigDisplayClass);
		cmsSetColorSpace(hProfile, cmsSigRgbData);
		cmsSetPCS(hProfile, cmsSigLabData);
		cmsSetHeaderRenderingIntent(hProfile, INTENT_PERCEPTUAL);

		//
		cmsMLU* DescriptionMLU = cmsMLUalloc(NULL, 1);
		cmsMLU* CopyrightMLU = cmsMLUalloc(NULL, 1);
		cmsMLUsetASCII(DescriptionMLU, "en", "US", description.c_str());
		cmsMLUsetASCII(CopyrightMLU, "en", "US", copyright.c_str());
		cmsWriteTag(hProfile, cmsSigProfileDescriptionTag, DescriptionMLU);
		cmsWriteTag(hProfile, cmsSigCopyrightTag, CopyrightMLU);

		//
		SamplerData data;
		data.processor = _processor;

		// 16Bit
		data.to_PCS16 = cmsCreateTransform(DisplayProfile, TYPE_RGB_16, labProfile, TYPE_LabV2_16,
										   INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
		data.from_PCS16 = cmsCreateTransform(labProfile, TYPE_LabV2_16, DisplayProfile, TYPE_RGB_16,
											 INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);

		//
		// AToB0Tag - Device to PCS (16-bit) intent of 0 (perceptual)
		//
		// cmsSigCurveSetElemType
		// `- cmsSigCLutElemType
		//  `- cmsSigCurveSetElemType
		//   `- cmsSigMatrixElemType
		//    `- cmsSigCurveSetElemType
		//
		//std::cout << "[OpenColorIO INFO]: Adding AToB0Tag\n";
		cmsPipeline* AToB0Tag = cmsPipelineAlloc(NULL, 3, 3);

		Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType

		// cmsSigCLutElemType
		cmsStage* AToB0Clut = cmsStageAllocCLut16bit(NULL, cubesize, 3, 3, NULL);
		//std::cout << "[OpenColorIO INFO]: Sampling AToB0 CLUT from Display to Lab\n";
		cmsStageSampleCLut16bit(AToB0Clut, Display2PCS_Sampler16, &data, 0);
		cmsPipelineInsertStage(AToB0Tag, cmsAT_END, AToB0Clut);

		Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType
		AddIdentityMatrix(AToB0Tag);    // cmsSigMatrixElemType
		Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType

		// Add AToB0Tag
		cmsWriteTag(hProfile, cmsSigAToB0Tag, AToB0Tag);
		cmsPipelineFree(AToB0Tag);

		//
		// BToA0Tag - PCS to Device space (16-bit) intent of 0 (perceptual)
		//
		// cmsSigCurveSetElemType
		// `- cmsSigMatrixElemType
		//  `- cmsSigCurveSetElemType
		//   `- cmsSigCLutElemType 
		//    `- cmsSigCurveSetElemType
		//
		//std::cout << "[OpenColorIO INFO]: Adding BToA0Tag\n";
		cmsPipeline* BToA0Tag = cmsPipelineAlloc(NULL, 3, 3);

		Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType
		AddIdentityMatrix(BToA0Tag);    // cmsSigMatrixElemType
		Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType

		// cmsSigCLutElemType
		cmsStage* BToA0Clut = cmsStageAllocCLut16bit(NULL, cubesize, 3, 3, NULL);
		//std::cout << "[OpenColorIO INFO]: Sampling BToA0 CLUT from Lab to Display\n";
		cmsStageSampleCLut16bit(BToA0Clut, PCS2Display_Sampler16, &data, 0);
		cmsPipelineInsertStage(BToA0Tag, cmsAT_END, BToA0Clut);

		Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType

		// Add BToA0Tag
		cmsWriteTag(hProfile, cmsSigBToA0Tag, BToA0Tag);
		cmsPipelineFree(BToA0Tag);

		//
		// D2Bx - Device to PCS (float) (Not Yet Impl)
		//

		//
		// B2Dx - PCS to Device (float) (Not Yet Impl)
		//

		//
		// Write
		//
		//std::cout << "[OpenColorIO INFO]: Writing " << outputfile << std::endl;
		cmsSaveProfileToFile(hProfile, path.c_str());
		cmsCloseProfile(hProfile);
	}
	else
	{
		// this code lovingly pulled from ociobakelut
		
		// need an extension->format map (yes, just did this one call up)
		map<string, string> extensions;
		
		for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
		{
			const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
			const char *format = OCIO::Baker::getFormatNameByIndex(i);
			
			extensions[ extension ] = format;
		}
		
		string format = extensions[ the_extension ];
		
		
		OCIO::BakerRcPtr baker = OCIO::Baker::Create();
		
		baker->setFormat(format.c_str());
		
		if(_type == OCIO_TYPE_CONVERT)
		{
			baker->setConfig(_config);
			baker->setInputSpace(_input.c_str());
			baker->setTargetSpace(_output.c_str());
		
			ofstream f(path.c_str());
			baker->bake(f);
		}
		else if(_type == OCIO_TYPE_DISPLAY)
		{
			OCIO::ConfigRcPtr editableConfig = _config->createEditableCopy();
			
			OCIO::ColorSpaceRcPtr inputColorSpace = OCIO::ColorSpace::Create();
			string inputspace = "RawInput";
			inputColorSpace->setName(inputspace.c_str());
			editableConfig->addColorSpace(inputColorSpace);
			
			
			OCIO::ColorSpaceRcPtr outputColorSpace = OCIO::ColorSpace::Create();
			string outputspace = "ProcessedOutput";
			outputColorSpace->setName(outputspace.c_str());
			
			DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
			
			transform->setInputColorSpaceName(_input.c_str());
			transform->setView(_transform.c_str());
			transform->setDisplay(_device.c_str());
			
			outputColorSpace->setTransform(transform, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
			
			editableConfig->addColorSpace(outputColorSpace);
			
			
			baker->setConfig(editableConfig);
			baker->setInputSpace(inputspace.c_str());
			baker->setTargetSpace(outputspace.c_str());
			
			ofstream f(path.c_str());
			baker->bake(f);
		}
		else if(_type == OCIO_TYPE_LUT)
		{
			OCIO::ConfigRcPtr editableConfig = OCIO::Config::Create();

			OCIO::ColorSpaceRcPtr inputColorSpace = OCIO::ColorSpace::Create();
			string inputspace = "RawInput";
			inputColorSpace->setName(inputspace.c_str());
			editableConfig->addColorSpace(inputColorSpace);
			
			
			OCIO::ColorSpaceRcPtr outputColorSpace = OCIO::ColorSpace::Create();
			string outputspace = "ProcessedOutput";
			outputColorSpace->setName(outputspace.c_str());
			
			OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
			
			transform = OCIO::FileTransform::Create();
			transform->setSrc(_path.c_str());
			transform->setInterpolation(INTERP_LINEAR);
			transform->setDirection(_invert ? TRANSFORM_DIR_INVERSE : TRANSFORM_DIR_FORWARD);
			
			outputColorSpace->setTransform(transform, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
			
			editableConfig->addColorSpace(outputColorSpace);
			
			
			baker->setConfig(editableConfig);
			baker->setInputSpace(inputspace.c_str());
			baker->setTargetSpace(outputspace.c_str());
			
			ofstream f(path.c_str());
			baker->bake(f);
		}
	}
	
	}catch(...) { return false; }
	
	return true;
}
