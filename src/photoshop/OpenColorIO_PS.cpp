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

#include "OpenColorIO_PS.h"

#include "OpenColorIO_PS_Dialog.h"
#include "OpenColorIO_PS_Terminology.h"

#include "OpenColorIO_PS_Context.h"

#ifdef __PIWin__
//#include <Windows.h>
#include <Shlobj.h>

// DLLInstance for Windows
HINSTANCE hDllInstance = NULL;
#endif


// some of the lame-ass supporting code needs this
SPBasicSuite * sSPBasic = NULL;
FilterRecord * gFilterRecord = NULL;


typedef struct {
    long        sig;
    
    OCIO_Source source;
    Str255      configName;
    Str255      configPath;
    OCIO_Action action;
    Boolean     invert;
    OCIO_Interp interpolation;
    Str255      inputSpace;
    Str255      outputSpace;
    Str255      transform;
    Str255      device;
} Param;


#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif  /* MAX */

static void myC2PString(Str255 &pstr, const char *cstr)
{
    const size_t len = MIN(strlen(cstr), 254);
    
    strncpy((char *)&pstr[1], cstr, len);
    
    pstr[0] = len;
    pstr[len + 1] = '\0';
}

static const char * myP2CString(Str255 &pstr)
{
    size_t len = pstr[0];
    
    pstr[len + 1] = '\0';
    
    return (const char *)&pstr[1];
}

static void myP2PString(Str255 &dest, const Str255 &src)
{
    memcpy(&dest[0], &src[0], 256);
}


static void ReportException(GPtr globals, const std::exception &e)
{
    Str255 p_str;
    
    myC2PString(p_str, e.what());
    
    PIReportError(p_str);
    
    gResult = errReportString;
}


static Boolean ReadScriptParams(GPtr globals)
{
    PIReadDescriptor            token = NULL;
    DescriptorKeyID             key = 0;
    DescriptorTypeID            type = 0;
    DescriptorKeyIDArray        array = { NULLID };
    int32                       flags = 0;
    OSErr                       stickyError = noErr;
    Boolean                     returnValue = true;
    
    if(DescriptorAvailable(NULL))
    {
        token = OpenReader(array);
        
        if(token)
        {
            while(PIGetKey(token, &key, &type, &flags))
            {
                if(key == ocioKeySource)
                {
                    DescriptorEnumID ostypeStoreValue;
                    PIGetEnum(token, &ostypeStoreValue);
                    globals->source = (ostypeStoreValue == sourceEnvironment ? OCIO_SOURCE_ENVIRONMENT :
                                        ostypeStoreValue == sourceCustom ? OCIO_SOURCE_CUSTOM :
                                        OCIO_SOURCE_STANDARD);
                }
                else if(key == ocioKeyConfigName)
                {
                    PIGetStr(token, &globals->configName);
                }
                else if(key == ocioKeyConfigFileHandle)
                {
                    PIGetAlias(token, (Handle *)&globals->configFileHandle);
                }
                else if(key == ocioKeyAction)
                {
                    DescriptorEnumID ostypeStoreValue;
                    PIGetEnum(token, &ostypeStoreValue);
                    globals->action = (ostypeStoreValue == actionLUT ? OCIO_ACTION_LUT :
                                        ostypeStoreValue == actionDisplay ? OCIO_ACTION_DISPLAY :
                                        OCIO_ACTION_CONVERT);
                }
                else if(key == ocioKeyInvert)
                {
                    PIGetBool(token, &globals->invert);
                }
                else if(key == ocioKeyInterpolation)
                {
                    DescriptorEnumID ostypeStoreValue;
                    PIGetEnum(token, &ostypeStoreValue);
                    globals->action = (ostypeStoreValue == interpNearest ? OCIO_INTERP_NEAREST :
                                        ostypeStoreValue == interpLinear ? OCIO_INTERP_LINEAR :
                                        ostypeStoreValue == interpTetrahedral ? OCIO_INTERP_TETRAHEDRAL :
                                        OCIO_INTERP_BEST);
                }
                else if(key == ocioKeyInputSpace)
                {
                    PIGetStr(token, &globals->inputSpace);
                }
                else if(key == ocioKeyOutputSpace)
                {
                    PIGetStr(token, &globals->outputSpace);
                }
                else if(key == ocioKeyDevice)
                {
                    PIGetStr(token, &globals->device);
                }
                else if(key == ocioKeyTransform)
                {
                    PIGetStr(token, &globals->transform);
                }
            }

            stickyError = CloseReader(&token); // closes & disposes.
        }
        
        returnValue = PlayDialog();
    }
    
    return returnValue;
}

static OSErr WriteScriptParams(GPtr globals)
{
    PIWriteDescriptor           token = nil;
    OSErr                       gotErr = noErr;
            
    if(DescriptorAvailable(NULL))
    {
        token = OpenWriter();
        
        if(token)
        {
            PIPutEnum(token, ocioKeySource, typeSource, (globals->source == OCIO_SOURCE_ENVIRONMENT ? sourceEnvironment :
                                                        globals->source == OCIO_SOURCE_CUSTOM ? sourceCustom :
                                                        sourceStandard));
        
            if(globals->source == OCIO_SOURCE_STANDARD)
            {
                PIPutStr(token, ocioKeyConfigName, globals->configName);
            }
            else if(globals->source == OCIO_SOURCE_CUSTOM)
            {
                PIPutAlias(token, ocioKeyConfigFileHandle, (Handle)globals->configFileHandle);
            }
            
            
            PIPutEnum(token, ocioKeyAction, typeAction, (globals->action == OCIO_ACTION_LUT ? actionLUT :
                                                        globals->action == OCIO_ACTION_DISPLAY ? actionDisplay :
                                                        actionConvert));
            
            if(globals->action == OCIO_ACTION_LUT)
            {
                PIPutBool(token, ocioKeyInvert, globals->invert);
                PIPutEnum(token, ocioKeyInterpolation, typeInterpolation, (globals->interpolation == OCIO_INTERP_NEAREST ? interpNearest :
                                                                        globals->interpolation == OCIO_INTERP_LINEAR ? interpLinear :
                                                                        globals->interpolation == OCIO_INTERP_TETRAHEDRAL ? interpTetrahedral :
                                                                        interpBest));
                                                                        
            }
            else if(globals->action == OCIO_ACTION_DISPLAY)
            {
                PIPutStr(token, ocioKeyInputSpace, globals->inputSpace);
                PIPutStr(token, ocioKeyDevice, globals->device);
                PIPutStr(token, ocioKeyTransform, globals->transform);
            }
            else
            {
                assert(globals->action == OCIO_ACTION_CONVERT);
                
                PIPutStr(token, ocioKeyInputSpace, globals->inputSpace);
                PIPutStr(token, ocioKeyOutputSpace, globals->outputSpace);
            }
            
            
            gotErr = CloseWriter(&token); // closes and sets dialog optional
        }
    }
    
    return gotErr;
}

#pragma mark-

static void DoAbout(AboutRecordPtr aboutRecord)
{
#ifdef __PIMac__
    const char *plugHndl = "org.OpenColorIO.Photoshop";
    const void *hwnd = NULL;
#else
    // get platform handles
    const void *plugHndl = hDllInstance;
    HWND hwnd = (HWND)HostGetPlatformWindowPtr(aboutRecord);
#endif

    OpenColorIO_PS_About(plugHndl, hwnd);
}


void ValidateParameters(GPtr globals)
{
    if(gStuff->parameters == NULL)
    {
        gStuff->parameters = PINewHandle(sizeof(Param));

        if(gStuff->parameters != NULL)
        {
            Param *param = (Param *)PILockHandle(gStuff->parameters, FALSE);

            if(param)
            {
                param->sig = OpenColorIOSignature;
                
                param->source           = globals->source;
                myP2PString(param->configName, globals->configName);
                myC2PString(param->configPath, "dummyPath");
                param->action           = globals->action;
                param->invert           = globals->invert;
                param->interpolation    = globals->interpolation;
                myP2PString(param->inputSpace, globals->inputSpace);
                myP2PString(param->outputSpace, globals->outputSpace);
                myP2PString(param->device, globals->device);
                myP2PString(param->transform, globals->transform);

                PIUnlockHandle(gStuff->parameters);
            }
        }
        else
        {
            gResult = memFullErr;
            return;
        }
    }   
}


static void InitGlobals(Ptr globalPtr)
{   
    GPtr globals = (GPtr)globalPtr;

    globals->do_dialog  = FALSE;
    
    globals->source             = OCIO_SOURCE_ENVIRONMENT;
    myC2PString(globals->configName, "");
    globals->configFileHandle   = NULL;
    globals->action             = OCIO_ACTION_NONE;
    globals->invert             = FALSE;
    globals->interpolation      = OCIO_INTERP_LINEAR;
    myC2PString(globals->inputSpace, "");
    myC2PString(globals->outputSpace, "");
    myC2PString(globals->transform, "");
    myC2PString(globals->device, "");
    
    
    // set default with environment variable if it's set
    char *file = std::getenv("OCIO");
    
    if(file)
    {
        std::string path = file;
        
        if( !path.empty() )
        {
            try
            {
                OpenColorIO_PS_Context context(path);
                
                if( context.isLUT() )
                {
                    globals->source = OCIO_SOURCE_ENVIRONMENT;
                    globals->action = OCIO_ACTION_LUT;
                }
                else
                {
                    const std::string &defaultInputName = context.getDefaultColorSpace();
                    const std::string &defaultOutputName = defaultInputName;
                    
                    const std::string &defaultDevice = context.getDefaultDevice();
                    const std::string defaultTransform = context.getDefaultTransform(defaultDevice);
                    
                    
                    globals->source = OCIO_SOURCE_ENVIRONMENT;
                    globals->action = OCIO_ACTION_CONVERT;
                    myC2PString(globals->inputSpace, defaultInputName.c_str());
                    myC2PString(globals->outputSpace, defaultOutputName.c_str());
                    myC2PString(globals->device, defaultDevice.c_str());
                    myC2PString(globals->transform, defaultTransform.c_str());
                }
            }
            catch(const std::exception &e)
            {
                ReportException(globals, e);
            }
            catch(...)
            {
                gResult = formatCannotRead;
            }
        }
    }
    
    
    ValidateParameters(globals);
}


static void DoParameters(GPtr globals)
{
    Boolean do_dialog = ReadScriptParams(globals);
    
    if(do_dialog)
    {
        // in the modern era, we always do dialogs in the render function
        globals->do_dialog = TRUE;
    }
}

static void DoPrepare(GPtr globals)
{
    gStuff->bufferSpace = 0;
    gStuff->maxSpace = 0;
}


static inline float Clamp(const float &f)
{
    return (f < 0.f ? 0.f : f > 1.f ? 1.f : f);
}

template <typename T, int max, bool round, bool clamp>
static void ConvertRow(T *row, int len, OCIO::ConstProcessorRcPtr processor)
{
    float *floatRow = NULL;
    
    if(max == 1)
    {
        floatRow = (float *)row;
    }
    else
    {
        floatRow = (float *)malloc(sizeof(float) * len * 3);
        
        if(floatRow == NULL)
            return;
        
        const T *in = row;
        float *out = floatRow;
        
        for(int x=0; x < len; x++)
        {
            *out++ = ((float)*in++ / (float)max);
            *out++ = ((float)*in++ / (float)max);
            *out++ = ((float)*in++ / (float)max);
        }
    }
    
    
    OCIO::PackedImageDesc img(floatRow, len, 1, 3);
    
    processor->apply(img);
    
    
    if(max != 1)
    {
        const float *in = floatRow;
        T *out = row;
        
        assert(round && clamp);
        
        for(int x=0; x < len; x++)
        {
            *out++ = (Clamp(*in++) * (float)max) + 0.5f;
            *out++ = (Clamp(*in++) * (float)max) + 0.5f;
            *out++ = (Clamp(*in++) * (float)max) + 0.5f;
        }
        
        free(floatRow);
    }
}


static void ProcessTile(GPtr globals, void *tileData, VRect &tileRect, int32 rowBytes, OCIO::ConstProcessorRcPtr processor)
{

    const uint32 rectHeight = tileRect.bottom - tileRect.top;
    const uint32 rectWidth = tileRect.right - tileRect.left;
    
    unsigned char *row = (unsigned char *)tileData;

    for(uint32 pixelY = 0; pixelY < rectHeight; pixelY++)
    {
        if(gStuff->depth == 32)
            ConvertRow<float, 1, false, false>((float *)row, rectWidth, processor);
        else if(gStuff->depth == 16)
            ConvertRow<uint16, 0x8000, true, true>((uint16 *)row, rectWidth, processor);
        else if(gStuff->depth == 8)
            ConvertRow<uint8, 255, true, true>((uint8 *)row, rectWidth, processor);

        row += rowBytes;
    }
}


static void DoStart(GPtr globals)
{
    // legacy paramaters part
    if(gStuff->parameters)
    {
        Param *param = (Param *)PILockHandle(gStuff->parameters, FALSE);

        if(param && param->sig == OpenColorIOSignature)
        {
            // copy params (or not - seems to monkey with my dialog when I edit an action)

            PIUnlockHandle(gStuff->parameters);
        }
    }
    
    // modern scripting part
    Boolean do_dialog = ReadScriptParams(globals);
    
    if(do_dialog || globals->do_dialog)
    {
        DialogParams dialogParams;
        
        dialogParams.source = (globals->source == OCIO_SOURCE_ENVIRONMENT ? SOURCE_ENVIRONMENT :
                                globals->source == OCIO_SOURCE_CUSTOM ? SOURCE_CUSTOM :
                                SOURCE_STANDARD);
        
        if(globals->source == OCIO_SOURCE_CUSTOM)
        {
        #ifdef __PIMac__
            assert(globals->configFileHandle != NULL);
        
            FSRef fsref;
            Boolean wasChanged;
        
            if(noErr == FSResolveAlias(NULL, globals->configFileHandle, &fsref, &wasChanged))
            {
                char file_path[256];
                file_path[0] = '\0';

                FSRefMakePath(&fsref, (UInt8 *)file_path, 255);
                
                dialogParams.config = file_path;
            }
            else
            {
                dialogParams.source = SOURCE_ENVIRONMENT;
            }
        #else
            assert(globals->configFileHandle != NULL);

            dialogParams.config = PILockHandle(globals->configFileHandle, true);

            PIUnlockHandle(globals->configFileHandle);
        #endif
        }
        else if(globals->source == OCIO_SOURCE_STANDARD)
        {
            dialogParams.config = myP2CString(globals->configName);
        }
        
        dialogParams.action = (globals->action == OCIO_ACTION_LUT ? ACTION_LUT :
                                globals->action == OCIO_ACTION_DISPLAY ? ACTION_DISPLAY :
                                ACTION_CONVERT);
                                
        dialogParams.invert = globals->invert;
        
        dialogParams.interpolation = (globals->interpolation == OCIO_INTERP_NEAREST ? INTERPO_NEAREST :
                                        globals->interpolation == OCIO_INTERP_LINEAR ? INTERPO_LINEAR :
                                        globals->interpolation == OCIO_INTERP_TETRAHEDRAL ? INTERPO_TETRAHEDRAL :
                                        INTERPO_BEST);
                                        
        dialogParams.inputSpace = myP2CString(globals->inputSpace);
        dialogParams.outputSpace = myP2CString(globals->outputSpace);
        dialogParams.device = myP2CString(globals->device);
        dialogParams.transform = myP2CString(globals->transform);
        
    
    #ifdef __PIMac__
        const char *plugHndl = "org.OpenColorIO.Photoshop";
        const void *hwnd = NULL;
    #else
        // get platform handles
        const void *plugHndl = hDllInstance;
        HWND hwnd = (HWND)((PlatformData *)gStuff->platformData)->hwnd;
    #endif
        
        const DialogResult dialogResult = OpenColorIO_PS_Dialog(dialogParams, plugHndl, hwnd);
        
        
        if(dialogResult == RESULT_OK || dialogResult == RESULT_EXPORT)
        {
            globals->source = (dialogParams.source == SOURCE_ENVIRONMENT ? OCIO_SOURCE_ENVIRONMENT :
                                dialogParams.source == SOURCE_CUSTOM ? OCIO_SOURCE_CUSTOM :
                                OCIO_SOURCE_STANDARD);
            
            if(dialogParams.source == SOURCE_CUSTOM)
            {
            #ifdef __PIMac__
                FSRef temp_fsref;
                
                if(noErr == FSPathMakeRef((UInt8 *)dialogParams.config.c_str(), &temp_fsref, NULL) )
                {
                    FSNewAlias(NULL, &temp_fsref, &globals->configFileHandle);
                }
                else
                {
                    globals->source = OCIO_SOURCE_NONE;
                }
            #else
                if(globals->configFileHandle)
                    PISetHandleSize(globals->configFileHandle, (dialogParams.config.length() + 1) * sizeof(char));
                else    
                    globals->configFileHandle = PINewHandle((dialogParams.config.length() + 1) * sizeof(char));
                    
                char *path_buf = PILockHandle(globals->configFileHandle, true);

                strcpy(path_buf, dialogParams.config.c_str());

                PIUnlockHandle(globals->configFileHandle);
            #endif
            }
            else if(dialogParams.source == SOURCE_STANDARD)
            {
                myC2PString(globals->configName, dialogParams.config.c_str());
            }
            
            globals->action = (dialogParams.action == ACTION_LUT ? OCIO_ACTION_LUT :
                                dialogParams.action == ACTION_DISPLAY ? OCIO_ACTION_DISPLAY :
                                OCIO_ACTION_CONVERT);
            
            globals->invert = dialogParams.invert;
            
            globals->interpolation = (dialogParams.interpolation == INTERPO_NEAREST ? OCIO_INTERP_NEAREST :
                                        dialogParams.interpolation == INTERPO_LINEAR ? OCIO_INTERP_LINEAR :
                                        dialogParams.interpolation == INTERPO_TETRAHEDRAL ? OCIO_INTERP_TETRAHEDRAL :
                                        OCIO_INTERP_BEST);
                                        
            myC2PString(globals->inputSpace, dialogParams.inputSpace.c_str());
            myC2PString(globals->outputSpace, dialogParams.outputSpace.c_str());
            myC2PString(globals->device, dialogParams.device.c_str());
            myC2PString(globals->transform, dialogParams.transform.c_str());
        }
        else
            gResult = userCanceledErr;
        

        globals->do_dialog = FALSE;

        if(gResult == noErr)
        {
            // this will copy values to parameters
            ValidateParameters(globals);
        }
    }

    
    std::string path;
    
    if(gResult == noErr)
    {
        if(globals->source == OCIO_SOURCE_ENVIRONMENT)
        {
            char *envFile = std::getenv("OCIO");
            
            if(envFile != NULL && strlen(envFile) > 0)
            {
                path = envFile;
            }
        }
        else if(globals->source == OCIO_SOURCE_CUSTOM)
        {
        #ifdef __PIMac__
            assert(globals->configFileHandle != NULL);
        
            FSRef fsref;
            Boolean wasChanged;
        
            if(noErr == FSResolveAlias(NULL, globals->configFileHandle, &fsref, &wasChanged))
            {
                char file_path[256];
                file_path[0] = '\0';

                FSRefMakePath(&fsref, (UInt8 *)file_path, 255);
                
                path = file_path;
            }
        #else
            assert(globals->configFileHandle != NULL);

            path = PILockHandle(globals->configFileHandle, true);

            PIUnlockHandle(globals->configFileHandle);
        #endif
        }
        else
        {
            assert(globals->source == OCIO_SOURCE_STANDARD);
            
        #ifdef __PIMac__
            const char *standardDirectory = "/Library/Application Support/OpenColorIO";
            const std::string pathSeperator = "/";
        #else
            const std::string pathSeperator = "\\";

            char appdata_path[MAX_PATH];
            HRESULT result = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL,
                                                SHGFP_TYPE_CURRENT, appdata_path);

            const std::string standardDirectory = std::string(appdata_path) + pathSeperator + "OpenColorIO";
        #endif
        
            path = standardDirectory;
            
            path += pathSeperator;
            
            path += myP2CString(globals->configName);
            
            path += pathSeperator + "config.ocio";
        }
        
        if( path.empty() )
            gResult = filterBadParameters;
    }
    
    if(gResult == noErr)
    {
        try
        {
            OpenColorIO_PS_Context context(path);
            
            OCIO::ConstProcessorRcPtr processor;
            
            if( context.isLUT() )
            {
                assert(globals->action == OCIO_ACTION_LUT);
                
                const OCIO::Interpolation interpolation = (globals->interpolation == OCIO_INTERP_NEAREST ? OCIO::INTERP_NEAREST :
                                                            globals->interpolation == OCIO_INTERP_LINEAR ? OCIO::INTERP_LINEAR :
                                                            globals->interpolation == OCIO_INTERP_TETRAHEDRAL ? OCIO::INTERP_TETRAHEDRAL :
                                                            OCIO::INTERP_BEST);
                                                            
                const OCIO::TransformDirection direction = (globals->invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
                
                 processor = context.getLUTProcessor(interpolation, direction);
            }
            else
            {
                if(globals->action == OCIO_ACTION_DISPLAY)
                {
                    processor = context.getDisplayProcessor(myP2CString(globals->inputSpace), myP2CString(globals->device), myP2CString(globals->transform));
                }
                else
                {
                    assert(globals->action == OCIO_ACTION_CONVERT);
                    
                    processor = context.getConvertProcessor(myP2CString(globals->inputSpace), myP2CString(globals->outputSpace));
                }
            }
            
            
            
            // now the Photoshop part
            int16 tileHeight = gStuff->outTileHeight;
            int16 tileWidth = gStuff->outTileWidth;

            if(tileWidth == 0 || tileHeight == 0 || gStuff->advanceState == NULL)
            {
                gResult = filterBadParameters;
            }
            
            VRect outRect = GetOutRect();
            VRect filterRect = GetFilterRect();

            int32 imageVert = filterRect.bottom - filterRect.top;
            int32 imageHor = filterRect.right - filterRect.left;

            uint32 tilesVert = (tileHeight - 1 + imageVert) / tileHeight;
            uint32 tilesHoriz = (tileWidth - 1 + imageHor) / tileWidth;

            int32 progress_total = tilesVert;
            int32 progress_complete = 0;

            gStuff->outLoPlane = 0;
            gStuff->outHiPlane = 2;

            for(uint16 vertTile = 0; vertTile < tilesVert && gResult == noErr; vertTile++)
            {
                for(uint16 horizTile = 0; horizTile < tilesHoriz && gResult == noErr; horizTile++)
                {
                    outRect.top = filterRect.top + ( vertTile * tileHeight );
                    outRect.left = filterRect.left + ( horizTile * tileWidth );
                    outRect.bottom = outRect.top + tileHeight;
                    outRect.right = outRect.left + tileWidth;

                    if (outRect.bottom > filterRect.bottom)
                        outRect.bottom = filterRect.bottom;
                    if (outRect.right > filterRect.right)
                        outRect.right = filterRect.right;

                    SetOutRect(outRect);

                    gResult = AdvanceState();
                    
                    if(gResult == kNoErr)
                    {
                        outRect = GetOutRect();
                        
                        ProcessTile(globals,
                                    gStuff->outData, 
                                    outRect, 
                                    gStuff->outRowBytes,
                                    processor);
                    }
                }

                PIUpdateProgress(++progress_complete, progress_total);

                if( TestAbort() ) 
                {
                    gResult = userCanceledErr;
                }
            }
        }
        catch(const std::exception &e)
        {
            ReportException(globals, e);
        }
        catch(...)
        {
            gResult = filterBadParameters;
        }
    }

    VRect nullRect = {0,0,0,0};
    SetOutRect(nullRect);
    
    if(gResult == noErr)
        WriteScriptParams(globals);
}

static void DoContinue(GPtr globals)
{
    VRect outRect = { 0, 0, 0, 0};
    SetOutRect(outRect);
}

static void DoFinish(GPtr globals)
{
}


DLLExport SPAPI void PluginMain(const int16 selector,
                                FilterRecord * filterRecord,
                                entryData * data,
                                int16 * result)
{
    if (selector == filterSelectorAbout)
    {
        sSPBasic = ((AboutRecordPtr)filterRecord)->sSPBasic;

    #ifdef __PIWin__
        if(hDllInstance == NULL)
            hDllInstance = GetDLLInstance((SPPluginRef)((AboutRecordPtr)filterRecord)->plugInRef);
    #endif

        DoAbout((AboutRecordPtr)filterRecord);
    } 
    else 
    {
        gFilterRecord = filterRecord;
        sSPBasic = filterRecord->sSPBasic;

    #ifdef __PIWin__
        if(hDllInstance == NULL)
            hDllInstance = GetDLLInstance((SPPluginRef)filterRecord->plugInRef);
    #endif

        Ptr globalPtr = NULL;       // Pointer for global structure
        GPtr globals = NULL;        // actual globals


        globalPtr = AllocateGlobals ((allocateGlobalsPointer)result,
                                        (allocateGlobalsPointer)filterRecord,
                                        filterRecord->handleProcs,
                                        sizeof(Globals),
                                        data,
                                        InitGlobals);

        if(globalPtr == NULL)
        {       
            *result = memFullErr;
            return;
        }

        globals = (GPtr)globalPtr;


        if (gStuff->bigDocumentData != NULL)
            gStuff->bigDocumentData->PluginUsing32BitCoordinates = true;


        switch (selector)
        {
            case filterSelectorParameters:
                DoParameters(globals);
                break;
            case filterSelectorPrepare:
                DoPrepare(globals);
                break;
            case filterSelectorStart:
                DoStart(globals);
                break;
            case filterSelectorContinue:
                DoContinue(globals);
                break;
            case filterSelectorFinish:
                DoFinish(globals);
                break;
            default:
                gResult = filterBadParameters;
                break;
        }

        if ((Handle)*data != NULL)
            PIUnlockHandle((Handle)*data);
    }
}

