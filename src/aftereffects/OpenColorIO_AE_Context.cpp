
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//


#include "OpenColorIO_AE_Context.h"

#include "lcms2.h"

#include <map>
#include <fstream>

using namespace OCIO;
using namespace std;


static const char mac_delimiter = '/';
static const char win_delimiter = '\\';

#ifdef WIN_ENV
static const char delimiter = win_delimiter;
#else
static const char delimiter = mac_delimiter;
#endif


Path::Path(const std::string path, const std::string dir) :
	_path(path),
	_dir(dir)
{

}


Path::Path(const Path & path)
{
	_path = path._path;
	_dir  = path._dir;
}


string
Path::full_path() const
{
	if( is_relative(_path) && !_dir.empty() )
	{
		vector<string> path_vec = components( convert_delimiters(_path) );
		vector<string> dir_vec = components(_dir);
		
		int up_dirs = 0;
		int down_dirs = 0;
		
		while(down_dirs < path_vec.size() - 1 && (path_vec[down_dirs] == ".." || path_vec[down_dirs] == "."))
		{
			down_dirs++;
			
			if(path_vec[down_dirs] == "..")
				up_dirs++;
		}
		
		
		string path;
		
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


string
Path::relative_path(bool force) const
{
	if( is_relative(_path) || _dir.empty() || _path.empty() )
	{
		return _path;
	}
	else
	{
		vector<string> path_vec = components(_path);
		vector<string> dir_vec = components(_dir);
		
		int match_idx = 0;
		
		while(match_idx < path_vec.size() && match_idx < dir_vec.size() && path_vec[match_idx] == dir_vec[match_idx])
			match_idx++;
		
		if(match_idx == 0)
		{
			// can't do relative path
			if(force)
				return _path;
			else
				return string("");
		}
		else
		{
			string rel_path;
			
			// is the file actually inside the dir?
			if(match_idx == path_vec.size() - 1)
			{
				rel_path += string(".") + delimiter;
			}
			else
			{
				for(int i = match_idx; i < dir_vec.size(); i++)
				{
					rel_path += string("..") + delimiter;
				}
				
				for(int i = match_idx; i < path_vec.size() - 1; i++)
				{
					rel_path += path_vec[i] + delimiter;
				}
			}
			
			rel_path += path_vec.back();
			
			return rel_path;
		}
	}
}


bool
Path::exists() const
{
	string path = full_path();
	
	if(path.empty())
		return false;
	
	ifstream f( path.c_str() );
	
	return !!f;
}


Path::PathType
Path::path_type(string path)
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
	else
	{
		size_t mac_pos = path.find(mac_delimiter);
		size_t win_pos = path.find(win_delimiter);
		
		if(mac_pos != string::npos && win_pos == string::npos)
		{
			return TYPE_MAC;
		}
		else if(mac_pos == string::npos && win_pos != string::npos)
		{
			return TYPE_WIN;
		}
		else if(mac_pos == string::npos && win_pos == string::npos)
		{
			return TYPE_UNKNOWN;
		}
		else // neighther npos
		{
			if(mac_pos < win_pos)
				return TYPE_MAC;
			else
				return TYPE_WIN;
		}
	}
}


bool
Path::is_relative(string path)
{
	Path::PathType type = path_type(path);
	
	if(type == TYPE_MAC)
	{
		return (path[0] != mac_delimiter);
	}
	else if(type == TYPE_WIN)
	{
		return !(path[1] == ':' && path[2] == win_delimiter);
	}
	else
		return false;
}


string
Path::convert_delimiters(string path)
{
#ifdef WIN_ENV
	char search = mac_delimiter;
	char replace = win_delimiter;
#else
	char search = win_delimiter;
	char replace = mac_delimiter;
#endif

	for(int i=0; i < path.size(); i++)
	{
		if(path[i] == search)
			path[i] = replace;
	}
	
	return path;
}


vector<string>
Path::components(string path)
{
	vector<string> vec;
	
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


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const ArbitraryData *arb_data, const string dir)
{
	_type = OCIO_TYPE_NONE;
	
	
	Path absolute_path(arb_data->path);
	Path relative_path(arb_data->relative_path, dir);
	
	if( absolute_path.exists() )
	{
		_path = absolute_path.full_path();
	}
	else
	{
		_path = relative_path.full_path();
	}
	

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


bool
OpenColorIO_AE_Context::Verify(const ArbitraryData *arb_data, const string dir)
{
	// comparing the paths, cheking relative path only if necessary
	if(_path != arb_data->path)
	{
		string rel_path(arb_data->relative_path);
		
		if( !dir.empty() && !rel_path.empty() )
		{
			Path relative_path(rel_path, dir);
			
			if( _path != relative_path.full_path() )
				return false;
		}
		else
			return false;
	}
	
	// we can switch between Convert and Display, but not LUT and non-LUT
	if((arb_data->type == OCIO_TYPE_NONE) ||
		(_type == OCIO_TYPE_LUT && arb_data->type != OCIO_TYPE_LUT) ||
		(_type != OCIO_TYPE_LUT && arb_data->type == OCIO_TYPE_LUT) )
	{
		return false;
	}
	
	bool force_reset = (_type != arb_data->type);	
	
	
	// If the type and path are compatible, we can patch up
	// differences here and return true.
	// Returning false means the context will be deleted and rebuilt.
	if(arb_data->type == OCIO_TYPE_LUT)
	{
		if(_invert != (bool)arb_data->invert || force_reset)
		{
			setupLUT(arb_data->invert);
		}
	}
	else if(arb_data->type == OCIO_TYPE_CONVERT)
	{
		if(_input != arb_data->input ||
			_output != arb_data->output ||
			force_reset)
		{
			setupConvert(arb_data->input, arb_data->output);
		}
	}
	else if(arb_data->type == OCIO_TYPE_DISPLAY)
	{
		if(_input != arb_data->input ||
			_transform != arb_data->transform ||
			_device != arb_data->device ||
			force_reset)
		{
			setupDisplay(arb_data->input, arb_data->transform, arb_data->device);
		}
	}
	else
		throw Exception("Bad OCIO type");
	
	
	return true;
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
	FileTransformRcPtr transform = FileTransform::Create();
	
	transform = FileTransform::Create();
	transform->setSrc( _path.c_str() );
	transform->setInterpolation(INTERP_LINEAR);
	transform->setDirection(invert ? TRANSFORM_DIR_INVERSE : TRANSFORM_DIR_FORWARD);
	
	_processor = _config->getProcessor(transform);
	
	_invert = invert;
	
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
		size_t filename_start = path.find_last_of(delimiter) + 1;
		size_t filename_end = path.find_last_of('.') - 1;
		
		string description = path.substr( path.find_last_of(delimiter) + 1, filename_end - filename_start);
		
		
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
