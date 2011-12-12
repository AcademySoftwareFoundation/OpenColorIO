
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
#include <sstream>

using namespace OCIO;
using namespace std;


static const char mac_delimiter = '/';
static const char win_delimiter = '\\';

#ifdef WIN_ENV
static const char delimiter = win_delimiter;
#else
static const char delimiter = mac_delimiter;
#endif

static const int LUT3D_EDGE_SIZE = 32;


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
			
			// is the file in a folder below or actually inside the dir?
			if(match_idx == dir_vec.size())
			{
				rel_path += string(".") + delimiter;
			}
			else
			{
				for(int i = match_idx; i < dir_vec.size(); i++)
				{
					rel_path += string("..") + delimiter;
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
	else if(path[0] == win_delimiter && path[1] == win_delimiter)
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
		else // neither npos?
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
		return !( (path[1] == ':' && path[2] == win_delimiter) ||
				  (path[0] == win_delimiter && path[1] == win_delimiter) );
	}
	else
	{	// TYPE_UNKNOWN
	
		// just a filename perhaps?
		// should never have this: even a file in the same directory will be ./file.ocio
		// we'll assume it's relative, but raise a stink during debugging
		assert(type != TYPE_UNKNOWN);
		
		return true;
	}
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


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const string path) :
	_gl_init(false)
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


OpenColorIO_AE_Context::OpenColorIO_AE_Context(const ArbitraryData *arb_data, const string dir) :
	_gl_init(false)
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


OpenColorIO_AE_Context::~OpenColorIO_AE_Context()
{
	if(_gl_init)
	{
		glDeleteShader(_fragShader);
		glDeleteProgram(_program);
		glDeleteTextures(1, &_imageTexID);
		
		if(_bufferWidth != 0 && _bufferHeight != 0)
			glDeleteRenderbuffersEXT(1, &_renderBuffer);
	}
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
	
	UpdateOCIOGLState();
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
	
	UpdateOCIOGLState();
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
	
	UpdateOCIOGLState();
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
		
		string description = path.substr( path.find_last_of(delimiter) + 1, 1 + filename_end - filename_start);
		
		
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
		//	`- cmsSigCurveSetElemType
		//	 `- cmsSigMatrixElemType
		//	  `- cmsSigCurveSetElemType
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
		AddIdentityMatrix(AToB0Tag);	// cmsSigMatrixElemType
		Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType

		// Add AToB0Tag
		cmsWriteTag(hProfile, cmsSigAToB0Tag, AToB0Tag);
		cmsPipelineFree(AToB0Tag);

		//
		// BToA0Tag - PCS to Device space (16-bit) intent of 0 (perceptual)
		//
		// cmsSigCurveSetElemType
		// `- cmsSigMatrixElemType
		//	`- cmsSigCurveSetElemType
		//	 `- cmsSigCLutElemType 
		//	  `- cmsSigCurveSetElemType
		//
		//std::cout << "[OpenColorIO INFO]: Adding BToA0Tag\n";
		cmsPipeline* BToA0Tag = cmsPipelineAlloc(NULL, 3, 3);

		Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType
		AddIdentityMatrix(BToA0Tag);	// cmsSigMatrixElemType
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


void
OpenColorIO_AE_Context::InitOCIOGL()
{
	if(!_gl_init)
	{
		SetPluginContext();
	
		glGenTextures(1, &_imageTexID);
		glGenTextures(1, &_lut3dTexID);
		
		int num3Dentries = 3*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE;
		_lut3d.resize(num3Dentries);
		memset(&_lut3d[0], 0, sizeof(float)*num3Dentries);
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, _lut3dTexID);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F_ARB,
						LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
							0, GL_RGB,GL_FLOAT, &_lut3d[0]);
							
		_fragShader = 0;
		_program = 0;
		
		_bufferWidth = _bufferHeight = 0;
		
		_gl_init = true;
		
		SetAEContext();
	}
}


static const char * g_fragShaderText = ""
"\n"
"uniform sampler2D tex1;\n"
"uniform sampler3D tex2;\n"
"\n"
"void main()\n"
"{\n"
"	 vec4 col = texture2D(tex1, gl_TexCoord[0].st);\n"
"	 gl_FragColor = OCIODisplay(col, tex2);\n"
"}\n";


static GLuint
CompileShaderText(GLenum shaderType, const char *text)
{
	GLuint shader;
	GLint stat;
	
	shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, (const GLchar **) &text, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
	
	if (!stat)
	{
		GLchar log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		//fprintf(stderr, "Error: problem compiling shader: %s\n", log);
		return 0;
	}
	
	return shader;
}


static GLuint
LinkShaders(GLuint fragShader)
{
	if (!fragShader) return 0;
	
	GLuint program = glCreateProgram();
	
	if (fragShader)
		glAttachShader(program, fragShader);
	
	glLinkProgram(program);
	
	// check link
	{
		GLint stat;
		glGetProgramiv(program, GL_LINK_STATUS, &stat);
		if (!stat) {
			GLchar log[1000];
			GLsizei len;
			glGetProgramInfoLog(program, 1000, &len, log);
			//fprintf(stderr, "Shader link error:\n%s\n", log);
			return 0;
		}
	}
	
	return program;
}


void
OpenColorIO_AE_Context::UpdateOCIOGLState()
{
	if(_gl_init)
	{
		SetPluginContext();
		
		// Step 1: Create a GPU Shader Description
		OCIO::GpuShaderDesc shaderDesc;
		shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
		shaderDesc.setFunctionName("OCIODisplay");
		shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);
		
		// Step 2: Compute the 3D LUT
		std::string lut3dCacheID = _processor->getGpuLut3DCacheID(shaderDesc);
		if(lut3dCacheID != _lut3dcacheid)
		{
			//std::cerr << "Computing 3DLut " << g_lut3dcacheid << std::endl;
			
			_lut3dcacheid = lut3dCacheID;
			_processor->getGpuLut3D(&_lut3d[0], shaderDesc);
		}
		
		// Step 3: Compute the Shader
		std::string shaderCacheID = _processor->getGpuShaderTextCacheID(shaderDesc);
		if(_program == 0 || shaderCacheID != _shadercacheid)
		{
			//std::cerr << "Computing Shader " << g_shadercacheid << std::endl;
			
			_shadercacheid = shaderCacheID;
			
			std::ostringstream os;
			os << _processor->getGpuShaderText(shaderDesc) << "\n";
			os << g_fragShaderText;
			//std::cerr << os.str() << std::endl;
			
			if(_fragShader) glDeleteShader(_fragShader);
			_fragShader = CompileShaderText(GL_FRAGMENT_SHADER, os.str().c_str());
			if(_program) glDeleteProgram(_program);
			_program = LinkShaders(_fragShader);
		}
		
		SetAEContext();
	}
}


bool
OpenColorIO_AE_Context::ProcessWorldGL(PF_EffectWorld *float_world)
{
	if(!_gl_init)
	{
		InitOCIOGL();
		UpdateOCIOGLState();
	}
	
	
	if(_program == 0 || _fragShader == 0)
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
	
	
	glBindTexture(GL_TEXTURE_3D, _lut3dTexID);
	glTexSubImage3D(GL_TEXTURE_3D, 0,
					0, 0, 0, 
					LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
					GL_RGB, GL_FLOAT, &_lut3d[0]);
	
	
	glUseProgram(_program);
	glUniform1i(glGetUniformLocation(_program, "tex1"), 0);
	glUniform1i(glGetUniformLocation(_program, "tex2"), 1);
	

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
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, _bufferWidth, _bufferHeight);
		
		// attach renderbuffer to framebuffer
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, _renderBuffer);
	}
	
	if(GL_FRAMEBUFFER_COMPLETE_EXT != glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT))
	{
		SetAEContext();
		return false;
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	

	
	
	glViewport(0, 0, float_world->width, float_world->height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, float_world->width, 0.0, float_world->height, -100.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	glEnable(GL_TEXTURE_2D);
	glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
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
	glReadPixels(0, 0, float_world->width, float_world->height, GL_RGBA, GL_FLOAT, rgba_origin);


	glFinish();
	

	SetAEContext();
	
	return true;
}
