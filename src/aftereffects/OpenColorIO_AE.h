

#pragma once

#ifndef _OPENCOLORIO_AE_H_
#define _OPENCOLORIO_AE_H_


#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_ChannelSuites.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEGP_SuiteHandler.h"

#ifdef MSWindows
	#include <Windows.h>
#else 
	#ifndef __MACH__
		#include "string.h"
	#endif
#endif	


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


// Versioning information 

#define NAME				"OpenColorIO"
#define DESCRIPTION			"color space operations"
#define RELEASE_DATE		__DATE__
#define AUTHOR				"Brendan Bolles"
#define COPYRIGHT			"\xA9 2007-2011 fnord"
#define WEBSITE				"www.fnordware.com"
#define	MAJOR_VERSION		1
#define	MINOR_VERSION		0
#define	BUG_VERSION			0
#define	STAGE_VERSION		PF_Stage_RELEASE
#define	BUILD_VERSION		0


enum {
	OCIO_INPUT = 0,
	OCIO_DATA,
	
	OCIO_NUM_PARAMS
};

enum {
	OCIO_DATA_ID = 1
};


#define CURRENT_ARB_VERSION 1

#define ARB_PATH_LEN 255


#ifdef __cplusplus

typedef struct {
	A_u_char		version; // version of this data structure
	A_u_char		path[ARB_PATH_LEN+1];
} ArbitraryData;


class OCIO_Context
{
  public:
	OCIO_Context(ArbitraryData *arb_data);
	~OCIO_Context() {}
	
	OCIO::ConstProcessorRcPtr & processor() { return _processor; }

  private:
	OCIO::ConstProcessorRcPtr	_processor;
};


typedef struct {
	OCIO_Context	*context;
} SequenceData;

#endif

// UI drawing constants


#define UI_CONTROL_HEIGHT	200
#define UI_CONTROL_WIDTH	0

#ifdef __cplusplus
	extern "C" {
#endif


// Prototypes

DllExport	PF_Err 
PluginMain (	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra) ;


PF_Err
HandleEvent ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*extra );


PF_Err
ArbNewDefault( // needed by ParamSetup()
	PF_InData			*in_data,
	PF_OutData			*out_data,
	void				*refconPV,
	PF_ArbitraryH		*arbPH);

PF_Err 
HandleArbitrary(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_ParamDef			*params[],
	PF_LayerDef			*output,
	PF_ArbParamsExtra	*extra);


#ifdef __cplusplus
	}
#endif



#endif // _OPENCOLORIO_AE_H_