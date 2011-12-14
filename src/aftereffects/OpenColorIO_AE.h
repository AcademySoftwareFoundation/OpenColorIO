
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//

#pragma once

#ifndef _OPENCOLORIO_AE_H_
#define _OPENCOLORIO_AE_H_


//#define PF_DEEP_COLOR_AWARE 1  // do we really still need this?

#include "AEConfig.h"
#include "entry.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include "AE_EffectCB.h"


#ifdef MSWindows
	#include <Windows.h>
#else 
	#ifndef __MACH__
		#include "string.h"
	#endif
#endif	


// Versioning information 
#define	MAJOR_VERSION		1
#define	MINOR_VERSION		0
#define	BUG_VERSION			0
#define	STAGE_VERSION		PF_Stage_RELEASE
#define	BUILD_VERSION		0

// Paramater constants
enum {
	OCIO_INPUT = 0,
	OCIO_DATA,
	OCIO_GPU,
	
	OCIO_NUM_PARAMS
};

enum {
	OCIO_DATA_ID = 1,
	OCIO_GPU_ID
};


// Our Arbitrary Data struct

#define CURRENT_ARB_VERSION 1
#define ARB_PATH_LEN 255
#define ARB_SPACE_LEN	63

enum {
	OCIO_TYPE_NONE = 0,
	OCIO_TYPE_LUT,
	OCIO_TYPE_CONVERT,
	OCIO_TYPE_DISPLAY
};
typedef A_u_char OCIO_Type;

enum {
	OCIO_STORAGE_NONE = 0,
	OCIO_STORAGE_ZIP_FILE
};
typedef A_u_char OCIO_Storage;

typedef struct {
	A_u_char		version; // version of this data structure
	OCIO_Type		type;
	A_Boolean		invert; // only used for LUTs
	OCIO_Storage	storage; // storage not used...yet
	A_u_long		storage_size;
	A_u_char		reserved[56]; // 64 pre-path bytes
	char			path[ARB_PATH_LEN+1];
	char			relative_path[ARB_PATH_LEN+1];
	char			input[ARB_SPACE_LEN+1];
	char			output[ARB_SPACE_LEN+1];
	char			transform[ARB_SPACE_LEN+1];
	char			device[ARB_SPACE_LEN+1];
	A_u_char		storage_buf[1];
} ArbitraryData;


#ifdef __cplusplus

class OpenColorIO_AE_Context;

enum {
	STATUS_UNKNOWN = 0,
	STATUS_NO_FILE,
	STATUS_USING_ABSOLUTE,
	STATUS_USING_RELATIVE,
	STATUS_FILE_MISSING,
	STATUS_OCIO_ERROR
};
typedef A_u_char FileStatus;

enum {
	GPU_ERR_NONE = 0,
	GPU_ERR_INSUFFICIENT,
	GPU_ERR_RENDER_ERR
};
typedef A_u_char GPUErr;

typedef struct {
	FileStatus				status;
	GPUErr					gpu_err;
	A_u_char				reserved[2];
	OpenColorIO_AE_Context	*context;
	char					path[ARB_PATH_LEN+1];
	char					relative_path[ARB_PATH_LEN+1];
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