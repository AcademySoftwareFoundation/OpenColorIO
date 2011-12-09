

#ifndef _OPENCOLORIO_AE_GL_H_
#define _OPENCOLORIO_AE_GL_H_

#include "AEConfig.h"
#include "entry.h"
#include "AE_Macros.h"
#include "AE_Effect.h"


PF_Err
GlobalSetup_GL ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output);

PF_Err
GlobalSetdown_GL ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output);


#endif // _OPENCOLORIO_AE_GL_H_