/*
 *  OpenColorIO_AE_Context.h
 *  OpenColorIO_AE
 *
 *  Created by Brendan Bolles on 11/22/11.
 *  Copyright 2011 fnord. All rights reserved.
 *
 */

#ifndef _OPENCOLORIO_AE_CONTEXT_H_
#define _OPENCOLORIO_AE_CONTEXT_H_

#include "OpenColorIO_AE.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


#include <string>
#include <vector>

class OpenColorIO_AE_Context
{
  public:
	OpenColorIO_AE_Context(const std::string path);
	OpenColorIO_AE_Context(const ArbitraryData *arb_data);
	~OpenColorIO_AE_Context() {}
	
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
	
	OCIO::ConstProcessorRcPtr & processor() { return _processor; }
	
	bool ExportLUT(const std::string path, const std::string display_icc_path = "");

  private:
	std::string _path;
  
	OCIO::ConstConfigRcPtr		_config;
	OCIO::ConstProcessorRcPtr	_processor;
	
	OCIO_Type _type;
	
	std::string _input;
	std::string _output;
	std::string _transform;
	std::string _device;
	SpaceVec _inputs;
	SpaceVec _transforms;
	SpaceVec _devices;
	
	bool _invert;
};


#endif // _OPENCOLORIO_AE_CONTEXT_H_