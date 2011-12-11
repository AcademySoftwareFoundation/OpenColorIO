
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//


#ifndef _OPENCOLORIO_AE_CONTEXT_H_
#define _OPENCOLORIO_AE_CONTEXT_H_

#include "OpenColorIO_AE.h"
#include "OpenColorIO_AE_GL.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


#include <string>
#include <vector>


// yeah, this probably could/should go in a seperate file
class Path
{
  public:
	Path(const std::string path, const std::string dir = "");
	Path(const Path & path);
	~Path() {}
	
	std::string full_path() const;
	std::string relative_path(bool force = false) const;
	
	bool exists() const;
	
  private:
	std::string _path;
	std::string _dir;
	
	typedef enum {
		TYPE_UNKNOWN = 0,
		TYPE_MAC,
		TYPE_WIN
	} PathType;
	
	static PathType path_type(std::string path);
	static bool is_relative(std::string path);
	static std::string convert_delimiters(std::string path);
	static std::vector<std::string> components(std::string path);
};


class OpenColorIO_AE_Context
{
  public:
	OpenColorIO_AE_Context(const std::string path);
	OpenColorIO_AE_Context(const ArbitraryData *arb_data, const std::string dir = "");
	~OpenColorIO_AE_Context();
	
	bool Verify(const ArbitraryData *arb_data, const std::string dir = "");
	
	void setupConvert(const char *input, const char *output);
	void setupDisplay(const char *input, const char *transform, const char *device);
	void setupLUT(bool invert = false);
  
	typedef std::vector<std::string> SpaceVec;

	OCIO_Type getType() const { return _type; }
	const std::string & getInput() const { return _input; }
	const std::string & getOutput() const { return _output; }
	const std::string & getTransform() const { return _transform; }
	const std::string & getDevice() const { return _device; }
	const SpaceVec & getInputs() const { return _inputs; }
	const SpaceVec & getTransforms() const { return _transforms; }
	const SpaceVec & getDevices() const { return _devices; }
	
	const OCIO::ConstProcessorRcPtr & processor() const { return _processor; }
	
	bool ExportLUT(const std::string path, const std::string display_icc_path = "");
	
	bool ProcessWorldGL(PF_EffectWorld *float_world);

  private:
	std::string _path;
  
	OCIO_Type _type;
	
	std::string _input;
	std::string _output;
	std::string _transform;
	std::string _device;
	SpaceVec _inputs;
	SpaceVec _transforms;
	SpaceVec _devices;
	
	bool _invert;
	
	
	OCIO::ConstConfigRcPtr		_config;
	OCIO::ConstProcessorRcPtr	_processor;
	
	
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