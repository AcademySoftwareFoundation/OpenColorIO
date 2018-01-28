/*
Copyright (c) 2003-2017 Sony Pictures Imageworks Inc., et al.
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

#ifndef _OPENCOLORIO_PS_H_
#define _OPENCOLORIO_PS_H_

//#include "PIDefines.h"
#include "PIFilter.h"
#include "PIUtilities.h"
#include "FilterBigDocument.h"


enum {
    OCIO_SOURCE_NONE = 0,
    OCIO_SOURCE_ENVIRONMENT,
    OCIO_SOURCE_STANDARD,
    OCIO_SOURCE_CUSTOM
};
typedef uint8 OCIO_Source;

enum {
    OCIO_ACTION_NONE = 0,
    OCIO_ACTION_LUT,
    OCIO_ACTION_CONVERT,
    OCIO_ACTION_DISPLAY
};
typedef uint8 OCIO_Action;

enum {
    OCIO_INTERP_UNKNOWN = 0,
    OCIO_INTERP_NEAREST = 1,
    OCIO_INTERP_LINEAR = 2,
    OCIO_INTERP_TETRAHEDRAL = 3,
    OCIO_INTERP_BEST = 255
};
typedef uint8 OCIO_Interp;


// this stuff would usually be in a header file
typedef struct Globals
{
    short                   *result;                // Must always be first in Globals.
    FilterRecord            *filterParamBlock;      // Must always be second in Globals.

    Boolean                 do_dialog;
    
    OCIO_Source             source;
    Str255                  configName;
    PIPlatformFileHandle    configFileHandle;
    OCIO_Action             action;
    Boolean                 invert;
    OCIO_Interp             interpolation;
    Str255                  inputSpace;
    Str255                  outputSpace;
    Str255                  device;
    Str255                  transform;
} Globals, *GPtr, **GHdl;


// some of the lame-ass supporting code needs this
extern SPBasicSuite *sSPBasic;
extern FilterRecord *gFilterRecord;

#define gResult             (*(globals->result))
#define gStuff              (globals->filterParamBlock)

#ifdef PS_CS4_SDK
typedef intptr_t entryData;
typedef void * allocateGlobalsPointer;
#else
typedef long entryData;
typedef uint32 allocateGlobalsPointer;
#endif

// our entry function
DLLExport SPAPI void PluginMain(const int16 selector,
                                FilterRecord * filterRecord,
                                entryData * data,
                                int16 * result);


#endif // _OPENCOLORIO_PS_H_
