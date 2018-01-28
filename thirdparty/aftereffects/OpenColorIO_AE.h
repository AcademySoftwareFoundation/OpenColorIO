/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#ifndef _OPENCOLORIO_AE_H_
#define _OPENCOLORIO_AE_H_


//#define PF_DEEP_COLOR_AWARE 1  // do we really still need this?

#include "AEConfig.h"
#include "entry.h"
#include "SPTypes.h"
#include "PrSDKAESupport.h"
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
#define MAJOR_VERSION       1
#define MINOR_VERSION       0
#define BUG_VERSION         0
#define STAGE_VERSION       PF_Stage_RELEASE
#define BUILD_VERSION       0

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
#define ARB_SPACE_LEN   63

enum {
    OCIO_ACTION_NONE = 0,
    OCIO_ACTION_LUT,
    OCIO_ACTION_CONVERT,
    OCIO_ACTION_DISPLAY
};
typedef A_u_char OCIO_Action;

enum {
    OCIO_STORAGE_NONE = 0,
    OCIO_STORAGE_ZIP_FILE
};
typedef A_u_char OCIO_Storage;

enum {
    OCIO_SOURCE_NONE = 0,
    OCIO_SOURCE_ENVIRONMENT,
    OCIO_SOURCE_STANDARD,
    OCIO_SOURCE_CUSTOM
};
typedef A_u_char OCIO_Source;

enum {
    OCIO_INTERP_UNKNOWN = 0,
    OCIO_INTERP_NEAREST = 1,
    OCIO_INTERP_LINEAR = 2,
    OCIO_INTERP_TETRAHEDRAL = 3,
    OCIO_INTERP_BEST = 255
};
typedef A_u_char OCIO_Interp;

typedef struct {
    A_u_char        version; // version of this data structure
    OCIO_Action     action;
    A_Boolean       invert; // only used for LUTs
    OCIO_Storage    storage; // storage not used...yet
    A_u_long        storage_size;
    OCIO_Source     source;
    OCIO_Interp     interpolation;
    A_u_char        reserved[54]; // 64 pre-path bytes
    char            path[ARB_PATH_LEN+1];
    char            relative_path[ARB_PATH_LEN+1];
    char            input[ARB_SPACE_LEN+1];
    char            output[ARB_SPACE_LEN+1];
    char            transform[ARB_SPACE_LEN+1];
    char            device[ARB_SPACE_LEN+1];
    char            look[ARB_SPACE_LEN+1]; // not used currently
    A_u_char        storage_buf[1];
} ArbitraryData;


#ifdef __cplusplus

class OpenColorIO_AE_Context;

enum {
    STATUS_UNKNOWN = 0,
    STATUS_OK,
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

enum {
    PREMIERE_UNKNOWN = 0,
    PREMIERE_LINEAR,
    PREMIERE_NON_LINEAR
};
typedef A_u_char PremiereStatus;

typedef struct {
    FileStatus              status;
    GPUErr                  gpu_err;
    PremiereStatus          prem_status;
    OCIO_Source             source;
    OpenColorIO_AE_Context  *context;
    char                    path[ARB_PATH_LEN+1];
    char                    relative_path[ARB_PATH_LEN+1];
} SequenceData;

#endif



#define UI_CONTROL_HEIGHT   200
#define UI_CONTROL_WIDTH    500


#ifdef __cplusplus
    extern "C" {
#endif


// Prototypes

DllExport PF_Err PluginMain(
    PF_Cmd          cmd,
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    void            *extra) ;


PF_Err HandleEvent(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *extra );


PF_Err ArbNewDefault( // needed by ParamSetup()
    PF_InData           *in_data,
    PF_OutData          *out_data,
    void                *refconPV,
    PF_ArbitraryH       *arbPH);

PF_Err HandleArbitrary(
    PF_InData           *in_data,
    PF_OutData          *out_data,
    PF_ParamDef         *params[],
    PF_LayerDef         *output,
    PF_ArbParamsExtra   *extra);


#ifdef __cplusplus
    }
#endif



#endif // _OPENCOLORIO_AE_H_