

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
#define ARB_SPACE_LEN	63

enum {
	ARB_TYPE_NONE = 0,
	ARB_TYPE_LUT,
	ARB_TYPE_OCIO
};
typedef A_u_char ArbType;

typedef struct {
	A_u_char	version; // version of this data structure
	ArbType		type;
	A_u_char	reserved[62]; // 64 pre-path bytes
	char		path[ARB_PATH_LEN+1];
	char		input[ARB_SPACE_LEN+1];
	char		transform[ARB_SPACE_LEN+1];
	char		device[ARB_SPACE_LEN+1];
} ArbitraryData;


#ifdef __cplusplus

class OpenColorIO_AE_Context;


typedef struct {
	OpenColorIO_AE_Context	*context;
} SequenceData;

#endif



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