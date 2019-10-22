// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef _OPENCOLORIO_PS_H_
#define _OPENCOLORIO_PS_H_

//#include "PIDefines.h"
#include "PIFilter.h"
#include "PIUtilities.h"
//#include "FileUtilities.h"
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
    OCIO_INTERP_CUBIC = 4,
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
    Handle                  configFileHandle;       // Not using PIPlatformFileHandle anymore apparently
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
