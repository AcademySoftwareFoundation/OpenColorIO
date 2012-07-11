//---------------------------------------------------------------------------------
//
//  Little Color Management System
//  Copyright (c) 1998-2010 Marti Maria Saguer
//
// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software 
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//---------------------------------------------------------------------------------
//

#ifdef _MSC_VER
#    define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "lcms2_internal.h"

// On Visual Studio, use debug CRT
#ifdef _MSC_VER
#     include "crtdbg.h"
#     include <io.h>
#endif

// A single check. Returns 1 if success, 0 if failed
typedef cmsInt32Number (*TestFn)(void);

// A parametric Tone curve test function
typedef cmsFloat32Number (* dblfnptr)(cmsFloat32Number x, const cmsFloat64Number Params[]);

// Some globals to keep track of error
#define TEXT_ERROR_BUFFER_SIZE  4096

static char ReasonToFailBuffer[TEXT_ERROR_BUFFER_SIZE];
static char SubTestBuffer[TEXT_ERROR_BUFFER_SIZE];
static cmsInt32Number TotalTests = 0, TotalFail = 0;
static cmsBool TrappedError;
static cmsInt32Number SimultaneousErrors;


#define cmsmin(a, b) (((a) < (b)) ? (a) : (b))

// Die, a fatal unexpected error is detected!
static
void Die(const char* Reason)
{
    printf("\n\nArrrgggg!!: %s!\n\n", Reason);
    fflush(stdout);
    exit(1);
}

// Memory management replacement -----------------------------------------------------------------------------


// This is just a simple plug-in for malloc, free and realloc to keep track of memory allocated,
// maximum requested as a single block and maximum allocated at a given time. Results are printed at the end
static cmsUInt32Number SingleHit, MaxAllocated=0, TotalMemory=0;

// I'm hidding the size before the block. This is a well-known technique and probably the blocks coming from
// malloc are built in a way similar to that, but I do on my own to be portable.
typedef struct { 
    cmsUInt32Number KeepSize;
    cmsUInt32Number Align8;
    cmsContext WhoAllocated; // Some systems do need pointers aligned to 8-byte boundaries.

} _cmsMemoryBlock;

#define SIZE_OF_MEM_HEADER (sizeof(_cmsMemoryBlock))

// This is a fake thread descriptor used to check thread integrity. 
// Basically it returns a different threadID each time it is called.
// Then the memory management replacement functions does check if each
// free() is being called with same ContextID used on malloc()
static
cmsContext DbgThread(void)
{
    static cmsUInt32Number n = 1;

    return (cmsContext) n++;
}

// The allocate routine
static
void* DebugMalloc(cmsContext ContextID, cmsUInt32Number size)
{
    _cmsMemoryBlock* blk;

    if (size <= 0) {
       Die("malloc requested with zero bytes");
    }

    TotalMemory += size;

    if (TotalMemory > MaxAllocated)
        MaxAllocated = TotalMemory;

    if (size > SingleHit) 
        SingleHit = size;

    blk = (_cmsMemoryBlock*) malloc(size + SIZE_OF_MEM_HEADER);
    if (blk == NULL) return NULL;

    blk ->KeepSize = size;
    blk ->WhoAllocated = ContextID;

    return (void*) ((cmsUInt8Number*) blk + SIZE_OF_MEM_HEADER);
}

// The free routine
static
void  DebugFree(cmsContext ContextID, void *Ptr)
{
    _cmsMemoryBlock* blk;
    
    if (Ptr == NULL) {
        Die("NULL free (which is a no-op in C, but may be an clue of something going wrong)");
    }

    blk = (_cmsMemoryBlock*) (((cmsUInt8Number*) Ptr) - SIZE_OF_MEM_HEADER);
    TotalMemory -= blk ->KeepSize;

    if (blk ->WhoAllocated != ContextID) {
        Die("Trying to free memory allocated by a different thread");
    }

    free(blk);
}

// Reallocate, just a malloc, a copy and a free in this case.
static
void * DebugRealloc(cmsContext ContextID, void* Ptr, cmsUInt32Number NewSize)
{
    _cmsMemoryBlock* blk;
    void*  NewPtr;
    cmsUInt32Number max_sz;

    NewPtr = DebugMalloc(ContextID, NewSize);
    if (Ptr == NULL) return NewPtr;

    blk = (_cmsMemoryBlock*) (((cmsUInt8Number*) Ptr) - SIZE_OF_MEM_HEADER);
    max_sz = blk -> KeepSize > NewSize ? NewSize : blk ->KeepSize;
    memmove(NewPtr, Ptr, max_sz);
    DebugFree(ContextID, Ptr);
    
    return NewPtr;
}

// Let's know the totals
static
void DebugMemPrintTotals(void)
{
    printf("[Memory statistics]\n");
    printf("Allocated = %d MaxAlloc = %d Single block hit = %d\n", TotalMemory, MaxAllocated, SingleHit);
}

// Here we go with the plug-in declaration
static cmsPluginMemHandler DebugMemHandler = {{ cmsPluginMagicNumber, 2000, cmsPluginMemHandlerSig, NULL },
                                               DebugMalloc, DebugFree, DebugRealloc, NULL, NULL, NULL };

// Utils  -------------------------------------------------------------------------------------

static
void FatalErrorQuit(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *Text)
{
    Die(Text); 
}

// Print a dot for gauging
static 
void Dot(void)
{
    fprintf(stdout, "."); fflush(stdout);
}

// Keep track of the reason to fail
static
void Fail(const char* frm, ...)
{
    va_list args;
    va_start(args, frm);
    vsprintf(ReasonToFailBuffer, frm, args);
    va_end(args);
}

// Keep track of subtest
static
void SubTest(const char* frm, ...)
{
    va_list args;

    Dot();
    va_start(args, frm);
    vsprintf(SubTestBuffer, frm, args);
    va_end(args);
}


// Memory string
static
const char* MemStr(cmsUInt32Number size)
{
    static char Buffer[1024];

    if (size > 1024*1024) {
        sprintf(Buffer, "%g Mb", (cmsFloat64Number) size / (1024.0*1024.0));
    }
    else
        if (size > 1024) {
            sprintf(Buffer, "%g Kb", (cmsFloat64Number) size / 1024.0);
        }
        else
            sprintf(Buffer, "%g bytes", (cmsFloat64Number) size);

    return Buffer;
}


// The check framework
static
void Check(const char* Title, TestFn Fn)
{
    printf("Checking %s ...", Title);
    fflush(stdout);

    ReasonToFailBuffer[0] = 0;
    SubTestBuffer[0] = 0;
    TrappedError = FALSE;
    SimultaneousErrors = 0;
    TotalTests++;

    if (Fn() && !TrappedError) {

        // It is a good place to check memory
        if (TotalMemory > 0)
            printf("Ok, but %s are left!\n", MemStr(TotalMemory));
        else
            printf("Ok.\n");
    }
    else {
        printf("FAIL!\n");

        if (SubTestBuffer[0]) 
            printf("%s: [%s]\n\t%s\n", Title, SubTestBuffer, ReasonToFailBuffer);
        else
            printf("%s:\n\t%s\n", Title, ReasonToFailBuffer);

        if (SimultaneousErrors > 1) 
               printf("\tMore than one (%d) errors were reported\n", SimultaneousErrors); 

        TotalFail++;
    }   
    fflush(stdout);
}

// Dump a tone curve, for easy diagnostic
void DumpToneCurve(cmsToneCurve* gamma, const char* FileName)
{
    cmsHANDLE hIT8;
    cmsUInt32Number i;

    hIT8 = cmsIT8Alloc(gamma ->InterpParams->ContextID);

    cmsIT8SetPropertyDbl(hIT8, "NUMBER_OF_FIELDS", 2);
    cmsIT8SetPropertyDbl(hIT8, "NUMBER_OF_SETS", gamma ->nEntries);

    cmsIT8SetDataFormat(hIT8, 0, "SAMPLE_ID");
    cmsIT8SetDataFormat(hIT8, 1, "VALUE");

    for (i=0; i < gamma ->nEntries; i++) {
        char Val[30];

        sprintf(Val, "%d", i);
        cmsIT8SetDataRowCol(hIT8, i, 0, Val);
        sprintf(Val, "0x%x", gamma ->Table16[i]);
        cmsIT8SetDataRowCol(hIT8, i, 1, Val);
    }

    cmsIT8SaveToFile(hIT8, FileName);
    cmsIT8Free(hIT8);
}

// -------------------------------------------------------------------------------------------------


// Used to perform several checks. 
// The space used is a clone of a well-known commercial 
// color space which I will name "Above RGB"
static
cmsHPROFILE Create_AboveRGB(void)
{
    cmsToneCurve* Curve[3];
    cmsHPROFILE hProfile;
    cmsCIExyY D65;
    cmsCIExyYTRIPLE Primaries = {{0.64, 0.33, 1 },
                                 {0.21, 0.71, 1 },
                                 {0.15, 0.06, 1 }};
    
    Curve[0] = Curve[1] = Curve[2] = cmsBuildGamma(DbgThread(), 2.19921875);

    cmsWhitePointFromTemp(&D65, 6504);
    hProfile = cmsCreateRGBProfileTHR(DbgThread(), &D65, &Primaries, Curve);
    cmsFreeToneCurve(Curve[0]);
    
    return hProfile;
}

// A gamma-2.2 gray space
static
cmsHPROFILE Create_Gray22(void)
{
    cmsHPROFILE hProfile;
    cmsToneCurve* Curve = cmsBuildGamma(DbgThread(), 2.2);
    if (Curve == NULL) return NULL;

    hProfile = cmsCreateGrayProfileTHR(DbgThread(), cmsD50_xyY(), Curve);
    cmsFreeToneCurve(Curve);

    return hProfile;
}


static
cmsHPROFILE Create_GrayLab(void)
{
    cmsHPROFILE hProfile;
    cmsToneCurve* Curve = cmsBuildGamma(DbgThread(), 1.0);
    if (Curve == NULL) return NULL;

    hProfile = cmsCreateGrayProfileTHR(DbgThread(), cmsD50_xyY(), Curve);
    cmsFreeToneCurve(Curve);

    cmsSetPCS(hProfile, cmsSigLabData);
    return hProfile;
}

// A CMYK devicelink that adds gamma 3.0 to each channel
static
cmsHPROFILE Create_CMYK_DeviceLink(void)
{
    cmsHPROFILE hProfile;
    cmsToneCurve* Tab[4];
    cmsToneCurve* Curve = cmsBuildGamma(DbgThread(), 3.0);
    if (Curve == NULL) return NULL;

    Tab[0] = Curve;
    Tab[1] = Curve;
    Tab[2] = Curve;
    Tab[3] = Curve;   

    hProfile = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigCmykData, Tab);
    if (hProfile == NULL) return NULL;

    cmsFreeToneCurve(Curve);

    return hProfile;
}


// Create a fake CMYK profile, without any other requeriment that being coarse CMYK. 
// DONT USE THIS PROFILE FOR ANYTHING, IT IS USELESS BUT FOR TESTING PURPOSES.
typedef struct {

    cmsHTRANSFORM hLab2sRGB;
    cmsHTRANSFORM sRGB2Lab;
    cmsHTRANSFORM hIlimit;

} FakeCMYKParams;

static
cmsFloat64Number Clip(cmsFloat64Number v)
{
    if (v < 0) return 0;
    if (v > 1) return 1;

    return v;
}

static
cmsInt32Number ForwardSampler(register const cmsUInt16Number In[], cmsUInt16Number Out[], void* Cargo)
{
    FakeCMYKParams* p = (FakeCMYKParams*) Cargo;
    cmsFloat64Number rgb[3], cmyk[4];   
    cmsFloat64Number c, m, y, k;

    cmsDoTransform(p ->hLab2sRGB, In, rgb, 1);
         
    c = 1 - rgb[0];
    m = 1 - rgb[1];
    y = 1 - rgb[2];

    k = (c < m ? cmsmin(c, y) : cmsmin(m, y));
   
    // NONSENSE WARNING!: I'm doing this just because this is a test 
    // profile that may have ink limit up to 400%. There is no UCR here
    // so the profile is basically useless for anything but testing.

    cmyk[0] = c;
    cmyk[1] = m;
    cmyk[2] = y;
    cmyk[3] = k;

    cmsDoTransform(p ->hIlimit, cmyk, Out, 1);

    return 1;
}


static
cmsInt32Number ReverseSampler(register const cmsUInt16Number In[], register cmsUInt16Number Out[], register void* Cargo)
{
    FakeCMYKParams* p = (FakeCMYKParams*) Cargo;
    cmsFloat64Number c, m, y, k, rgb[3];    

    c = In[0] / 65535.0;
    m = In[1] / 65535.0;
    y = In[2] / 65535.0;
    k = In[3] / 65535.0;

    if (k == 0) {

        rgb[0] = Clip(1 - c);
        rgb[1] = Clip(1 - m);
        rgb[2] = Clip(1 - y);
    } 
    else 
        if (k == 1) {

            rgb[0] = rgb[1] = rgb[2] = 0;
        } 
        else {

            rgb[0] = Clip((1 - c) * (1 - k));
            rgb[1] = Clip((1 - m) * (1 - k));
            rgb[2] = Clip((1 - y) * (1 - k));       
        }   

        cmsDoTransform(p ->sRGB2Lab, rgb, Out, 1);
        return 1;
}



static
cmsHPROFILE CreateFakeCMYK(cmsFloat64Number InkLimit, cmsBool lUseAboveRGB)
{
    cmsHPROFILE hICC;
    cmsPipeline* AToB0, *BToA0;
    cmsStage* CLUT;
    cmsContext ContextID;
    FakeCMYKParams p;
    cmsHPROFILE hLab, hsRGB, hLimit;
    cmsUInt32Number cmykfrm;


    if (lUseAboveRGB)
        hsRGB = Create_AboveRGB();
    else
       hsRGB  = cmsCreate_sRGBProfile();

    hLab   = cmsCreateLab4Profile(NULL);
    hLimit = cmsCreateInkLimitingDeviceLink(cmsSigCmykData, InkLimit);

    cmykfrm = FLOAT_SH(1) | BYTES_SH(0)|CHANNELS_SH(4);
    p.hLab2sRGB = cmsCreateTransform(hLab,  TYPE_Lab_16,  hsRGB, TYPE_RGB_DBL, INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
    p.sRGB2Lab  = cmsCreateTransform(hsRGB, TYPE_RGB_DBL, hLab,  TYPE_Lab_16,  INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
    p.hIlimit   = cmsCreateTransform(hLimit, cmykfrm, NULL, TYPE_CMYK_16, INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);

    cmsCloseProfile(hLab); cmsCloseProfile(hsRGB); cmsCloseProfile(hLimit);

    ContextID = DbgThread();
    hICC = cmsCreateProfilePlaceholder(ContextID);
    if (!hICC) return NULL;

    cmsSetProfileVersion(hICC, 4.2);

    cmsSetDeviceClass(hICC, cmsSigOutputClass);
    cmsSetColorSpace(hICC,  cmsSigCmykData);
    cmsSetPCS(hICC,         cmsSigLabData);

    BToA0 = cmsPipelineAlloc(ContextID, 3, 4);
    if (BToA0 == NULL) return 0;
    CLUT = cmsStageAllocCLut16bit(ContextID, 17, 3, 4, NULL);
    if (CLUT == NULL) return 0;
    if (!cmsStageSampleCLut16bit(CLUT, ForwardSampler, &p, 0)) return 0;
    
    cmsPipelineInsertStage(BToA0, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(ContextID, 3)); 
    cmsPipelineInsertStage(BToA0, cmsAT_END, CLUT);
    cmsPipelineInsertStage(BToA0, cmsAT_END, _cmsStageAllocIdentityCurves(ContextID, 4));
    
    if (!cmsWriteTag(hICC, cmsSigBToA0Tag, (void*) BToA0)) return 0;
    cmsPipelineFree(BToA0);
    
    AToB0 = cmsPipelineAlloc(ContextID, 4, 3);
    if (AToB0 == NULL) return 0;
    CLUT = cmsStageAllocCLut16bit(ContextID, 17, 4, 3, NULL);
    if (CLUT == NULL) return 0;
    if (!cmsStageSampleCLut16bit(CLUT, ReverseSampler, &p, 0)) return 0;

    cmsPipelineInsertStage(AToB0, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(ContextID, 4)); 
    cmsPipelineInsertStage(AToB0, cmsAT_END, CLUT);
    cmsPipelineInsertStage(AToB0, cmsAT_END, _cmsStageAllocIdentityCurves(ContextID, 3));
    
    if (!cmsWriteTag(hICC, cmsSigAToB0Tag, (void*) AToB0)) return 0;
    cmsPipelineFree(AToB0);

    cmsDeleteTransform(p.hLab2sRGB);
    cmsDeleteTransform(p.sRGB2Lab);
    cmsDeleteTransform(p.hIlimit);

    cmsLinkTag(hICC, cmsSigAToB1Tag, cmsSigAToB0Tag);
    cmsLinkTag(hICC, cmsSigAToB2Tag, cmsSigAToB0Tag);
    cmsLinkTag(hICC, cmsSigBToA1Tag, cmsSigBToA0Tag);
    cmsLinkTag(hICC, cmsSigBToA2Tag, cmsSigBToA0Tag);

    return hICC;    
}


// Does create several profiles for latter use------------------------------------------------------------------------------------------------

static
cmsInt32Number OneVirtual(cmsHPROFILE h, const char* SubTestTxt, const char* FileName)
{
    SubTest(SubTestTxt);
    if (h == NULL) return 0;

    if (!cmsSaveProfileToFile(h, FileName)) return 0;
    cmsCloseProfile(h);

    h = cmsOpenProfileFromFile(FileName, "r");
    if (h == NULL) return 0;

    // Do some teste....

    cmsCloseProfile(h);

    return 1;
}



// This test checks the ability of lcms2 to save its built-ins as valid profiles. 
// It does not check the functionality of such profiles
static
cmsInt32Number CreateTestProfiles(void)
{
    cmsHPROFILE h;

    h = cmsCreate_sRGBProfileTHR(DbgThread());
    if (!OneVirtual(h, "sRGB profile", "sRGBlcms2.icc")) return 0;

    // ----

    h = Create_AboveRGB();
    if (!OneVirtual(h, "aRGB profile", "aRGBlcms2.icc")) return 0;

    // ----
    
    h = Create_Gray22();
    if (!OneVirtual(h, "Gray profile", "graylcms2.icc")) return 0;

    // ----

    h = Create_GrayLab();
    if (!OneVirtual(h, "Gray Lab profile", "glablcms2.icc")) return 0;
    
    // ----

    h = Create_CMYK_DeviceLink();
    if (!OneVirtual(h, "Linearization profile", "linlcms2.icc")) return 0;

    // -------
    h = cmsCreateInkLimitingDeviceLinkTHR(DbgThread(), cmsSigCmykData, 150);
    if (h == NULL) return 0;
    if (!OneVirtual(h, "Ink-limiting profile", "limitlcms2.icc")) return 0;

    // ------

    h = cmsCreateLab2ProfileTHR(DbgThread(), NULL);
    if (!OneVirtual(h, "Lab 2 identity profile", "labv2lcms2.icc")) return 0;

    // ----

    h = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
    if (!OneVirtual(h, "Lab 4 identity profile", "labv4lcms2.icc")) return 0;

    // ----

    h = cmsCreateXYZProfileTHR(DbgThread());
    if (!OneVirtual(h, "XYZ identity profile", "xyzlcms2.icc")) return 0;

    // ----

    h = cmsCreateNULLProfileTHR(DbgThread());
    if (!OneVirtual(h, "NULL profile", "nullcms2.icc")) return 0;

    // ---

    h = cmsCreateBCHSWabstractProfileTHR(DbgThread(), 17, 0, 0, 0, 0, 5000, 6000);
    if (!OneVirtual(h, "BCHS profile", "bchslcms2.icc")) return 0;

    // ---

    h = CreateFakeCMYK(300, FALSE);
    if (!OneVirtual(h, "Fake CMYK profile", "lcms2cmyk.icc")) return 0;

    return 1;
}

static
void RemoveTestProfiles(void)
{
    remove("sRGBlcms2.icc");
    remove("aRGBlcms2.icc");
    remove("graylcms2.icc");
    remove("linlcms2.icc");
    remove("limitlcms2.icc");
    remove("labv2lcms2.icc");
    remove("labv4lcms2.icc");
    remove("xyzlcms2.icc");
    remove("nullcms2.icc");
    remove("bchslcms2.icc");
    remove("lcms2cmyk.icc");
    remove("glablcms2.icc");
}

// -------------------------------------------------------------------------------------------------

// Check the size of basic types. If this test fails, nothing is going to work anyway
static
cmsInt32Number CheckBaseTypes(void)
{
    if (sizeof(cmsUInt8Number) != 1) return 0;
    if (sizeof(cmsInt8Number) != 1) return 0;
    if (sizeof(cmsUInt16Number) != 2) return 0;
    if (sizeof(cmsInt16Number) != 2) return 0;
    if (sizeof(cmsUInt32Number) != 4) return 0;
    if (sizeof(cmsInt32Number) != 4) return 0;
    if (sizeof(cmsUInt64Number) != 8) return 0;
    if (sizeof(cmsInt64Number) != 8) return 0;
    if (sizeof(cmsFloat32Number) != 4) return 0;
    if (sizeof(cmsFloat64Number) != 8) return 0;
    if (sizeof(cmsSignature) != 4) return 0;
    if (sizeof(cmsU8Fixed8Number) != 2) return 0;
    if (sizeof(cmsS15Fixed16Number) != 4) return 0;
    if (sizeof(cmsU16Fixed16Number) != 4) return 0;
    
    return 1;
}

// -------------------------------------------------------------------------------------------------


// Are we little or big endian?  From Harbison&Steele.  
static
cmsInt32Number CheckEndianess(void)
{
    cmsInt32Number BigEndian, IsOk;   
    union {
        long l;
        char c[sizeof (long)];
    } u;

    u.l = 1;
    BigEndian = (u.c[sizeof (long) - 1] == 1);

#ifdef CMS_USE_BIG_ENDIAN
    IsOk = BigEndian;
#else
    IsOk = !BigEndian;
#endif

    if (!IsOk) {
        Fail("\nOOOPPSS! You have CMS_USE_BIG_ENDIAN toggle misconfigured!\n\n"
            "Please, edit lcms2.h and %s the CMS_USE_BIG_ENDIAN toggle.\n", BigEndian? "uncomment" : "comment");
        return 0;
    }

    return 1;
}

// Check quick floor
static
cmsInt32Number CheckQuickFloor(void)
{
    if ((_cmsQuickFloor(1.234) != 1) ||
        (_cmsQuickFloor(32767.234) != 32767) ||
        (_cmsQuickFloor(-1.234) != -2) ||
        (_cmsQuickFloor(-32767.1) != -32768)) {

            Fail("\nOOOPPSS! _cmsQuickFloor() does not work as expected in your machine!\n\n"
                "Please, edit lcms.h and uncomment the CMS_DONT_USE_FAST_FLOOR toggle.\n");
            return 0;

    }

    return 1;
}

// Quick floor restricted to word
static
cmsInt32Number CheckQuickFloorWord(void)
{
    cmsUInt32Number i;

    for (i=0; i < 65535; i++) {

        if (_cmsQuickFloorWord((cmsFloat64Number) i + 0.1234) != i) {

            Fail("\nOOOPPSS! _cmsQuickFloorWord() does not work as expected in your machine!\n\n"
                "Please, edit lcms.h and uncomment the CMS_DONT_USE_FAST_FLOOR toggle.\n");
            return 0;
        }
    }

    return 1;
}

// -------------------------------------------------------------------------------------------------

// Precision stuff. 

// On 15.16 fixed point, this is the maximum we can obtain. Remember ICC profiles have storage limits on this number 
#define FIXED_PRECISION_15_16 (1.0 / 65535.0)

// On 8.8 fixed point, that is the max we can obtain.
#define FIXED_PRECISION_8_8 (1.0 / 255.0)

// On cmsFloat32Number type, this is the precision we expect
#define FLOAT_PRECISSION      (0.00001)

static cmsFloat64Number MaxErr;
static cmsFloat64Number AllowedErr = FIXED_PRECISION_15_16;

static
cmsBool IsGoodVal(const char *title, cmsFloat64Number in, cmsFloat64Number out, cmsFloat64Number max)
{
    cmsFloat64Number Err = fabs(in - out);

    if (Err > MaxErr) MaxErr = Err;

        if ((Err > max )) {

              Fail("(%s): Must be %f, But is %f ", title, in, out);
              return FALSE;
              }

       return TRUE;
}

static
cmsBool  IsGoodFixed15_16(const char *title, cmsFloat64Number in, cmsFloat64Number out)
{   
    return IsGoodVal(title, in, out, FIXED_PRECISION_15_16);
}

static
cmsBool  IsGoodFixed8_8(const char *title, cmsFloat64Number in, cmsFloat64Number out)
{
    return IsGoodVal(title, in, out, FIXED_PRECISION_8_8);
}

static
cmsBool  IsGoodWord(const char *title, cmsUInt16Number in, cmsUInt16Number out)
{
    if ((abs(in - out) > 0 )) {

        Fail("(%s): Must be %x, But is %x ", title, in, out);
        return FALSE;
    }

    return TRUE;
}

static
cmsBool  IsGoodWordPrec(const char *title, cmsUInt16Number in, cmsUInt16Number out, cmsUInt16Number maxErr)
{
    if ((abs(in - out) > maxErr )) {

        Fail("(%s): Must be %x, But is %x ", title, in, out);
        return FALSE;
    }

    return TRUE;
}

// Fixed point ----------------------------------------------------------------------------------------------

static
cmsInt32Number TestSingleFixed15_16(cmsFloat64Number d)
{
    cmsS15Fixed16Number f = _cmsDoubleTo15Fixed16(d);
    cmsFloat64Number RoundTrip = _cms15Fixed16toDouble(f);
    cmsFloat64Number Error     = fabs(d - RoundTrip);

    return ( Error <= FIXED_PRECISION_15_16);
}

static
cmsInt32Number CheckFixedPoint15_16(void)
{
    if (!TestSingleFixed15_16(1.0)) return 0;
    if (!TestSingleFixed15_16(2.0)) return 0;
    if (!TestSingleFixed15_16(1.23456)) return 0;
    if (!TestSingleFixed15_16(0.99999)) return 0;
    if (!TestSingleFixed15_16(0.1234567890123456789099999)) return 0;
    if (!TestSingleFixed15_16(-1.0)) return 0;
    if (!TestSingleFixed15_16(-2.0)) return 0;
    if (!TestSingleFixed15_16(-1.23456)) return 0;
    if (!TestSingleFixed15_16(-1.1234567890123456789099999)) return 0;
    if (!TestSingleFixed15_16(+32767.1234567890123456789099999)) return 0;
    if (!TestSingleFixed15_16(-32767.1234567890123456789099999)) return 0;
    return 1;
}

static
cmsInt32Number TestSingleFixed8_8(cmsFloat64Number d)
{
    cmsS15Fixed16Number f = _cmsDoubleTo8Fixed8(d);
    cmsFloat64Number RoundTrip = _cms8Fixed8toDouble((cmsUInt16Number) f);
    cmsFloat64Number Error     = fabs(d - RoundTrip);

    return ( Error <= FIXED_PRECISION_8_8);
}

static
cmsInt32Number CheckFixedPoint8_8(void)
{
    if (!TestSingleFixed8_8(1.0)) return 0;
    if (!TestSingleFixed8_8(2.0)) return 0;
    if (!TestSingleFixed8_8(1.23456)) return 0;
    if (!TestSingleFixed8_8(0.99999)) return 0;
    if (!TestSingleFixed8_8(0.1234567890123456789099999)) return 0;
    if (!TestSingleFixed8_8(+255.1234567890123456789099999)) return 0;

    return 1;
}

// Linear interpolation -----------------------------------------------------------------------------------------------

// Since prime factors of 65535 (FFFF) are,
//
//            0xFFFF = 3 * 5 * 17 * 257
//
// I test tables of 2, 4, 6, and 18 points, that will be exact.

static
void BuildTable(cmsInt32Number n, cmsUInt16Number Tab[], cmsBool  Descending)
{
    cmsInt32Number i;

    for (i=0; i < n; i++) {
        cmsFloat64Number v = (cmsFloat64Number) ((cmsFloat64Number) 65535.0 * i ) / (n-1);

        Tab[Descending ? (n - i - 1) : i ] = (cmsUInt16Number) floor(v + 0.5);
    }
}

// A single function that does check 1D interpolation
// nNodesToCheck = number on nodes to check
// Down = Create decreasing tables
// Reverse = Check reverse interpolation
// max_err = max allowed error 

static
cmsInt32Number Check1D(cmsInt32Number nNodesToCheck, cmsBool  Down, cmsInt32Number max_err)
{
    cmsUInt32Number i;
    cmsUInt16Number in, out;
    cmsInterpParams* p;
    cmsUInt16Number* Tab;

    Tab = (cmsUInt16Number*) malloc(sizeof(cmsUInt16Number)* nNodesToCheck);
    if (Tab == NULL) return 0;

    p = _cmsComputeInterpParams(DbgThread(), nNodesToCheck, 1, 1, Tab, CMS_LERP_FLAGS_16BITS);
    if (p == NULL) return 0;

    BuildTable(nNodesToCheck, Tab, Down);

    for (i=0; i <= 0xffff; i++) {

        in = (cmsUInt16Number) i;
        out = 0;

        p ->Interpolation.Lerp16(&in, &out, p);

        if (Down) out = 0xffff - out;

        if (abs(out - in) > max_err) {

            Fail("(%dp): Must be %x, But is %x : ", nNodesToCheck, in, out);
            _cmsFreeInterpParams(p);
            free(Tab);
            return 0;
        }
    }

    _cmsFreeInterpParams(p);
    free(Tab);
    return 1;
}


static
cmsInt32Number Check1DLERP2(void)
{
    return Check1D(2, FALSE, 0);
}


static
cmsInt32Number Check1DLERP3(void)
{
    return Check1D(3, FALSE, 1);    
}


static
cmsInt32Number Check1DLERP4(void)
{
    return Check1D(4, FALSE, 0);
}

static
cmsInt32Number Check1DLERP6(void)
{
    return Check1D(6, FALSE, 0);
}

static
cmsInt32Number Check1DLERP18(void)
{
    return Check1D(18, FALSE, 0);
}


static
cmsInt32Number Check1DLERP2Down(void)
{
    return Check1D(2, TRUE, 0);
}


static
cmsInt32Number Check1DLERP3Down(void)
{
    return Check1D(3, TRUE, 1);
}

static
cmsInt32Number Check1DLERP6Down(void)
{
    return Check1D(6, TRUE, 0);
}

static
cmsInt32Number Check1DLERP18Down(void)
{
    return Check1D(18, TRUE, 0);
}

static
cmsInt32Number ExhaustiveCheck1DLERP(void)
{
    cmsUInt32Number j;
    
    printf("\n");
    for (j=10; j <= 4096; j++) {

        if ((j % 10) == 0) printf("%d    \r", j);

        if (!Check1D(j, FALSE, 1)) return 0;    
    }

    printf("\rResult is ");
    return 1;
}

static
cmsInt32Number ExhaustiveCheck1DLERPDown(void)
{
    cmsUInt32Number j;
    
    printf("\n");
    for (j=10; j <= 4096; j++) {

        if ((j % 10) == 0) printf("%d    \r", j);

        if (!Check1D(j, TRUE, 1)) return 0; 
    }


    printf("\rResult is ");
    return 1;
}



// 3D interpolation -------------------------------------------------------------------------------------------------

static
cmsInt32Number Check3DinterpolationFloatTetrahedral(void)
{
    cmsInterpParams* p;
    cmsInt32Number i;
    cmsFloat32Number In[3], Out[3];
    cmsFloat32Number FloatTable[] = { //R     G    B

        0,    0,   0,     // B=0,G=0,R=0
        0,    0,  .25,    // B=1,G=0,R=0

        0,   .5,    0,    // B=0,G=1,R=0
        0,   .5,  .25,    // B=1,G=1,R=0

        1,    0,    0,    // B=0,G=0,R=1
        1,    0,  .25,    // B=1,G=0,R=1

        1,    .5,   0,    // B=0,G=1,R=1
        1,    .5,  .25    // B=1,G=1,R=1

    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT);


    MaxErr = 0.0;
     for (i=0; i < 0xffff; i++) {

       In[0] = In[1] = In[2] = (cmsFloat32Number) ( (cmsFloat32Number) i / 65535.0F);

        p ->Interpolation.LerpFloat(In, Out, p);

       if (!IsGoodFixed15_16("Channel 1", Out[0], In[0])) goto Error;
       if (!IsGoodFixed15_16("Channel 2", Out[1], (cmsFloat32Number) In[1] / 2.F)) goto Error;
       if (!IsGoodFixed15_16("Channel 3", Out[2], (cmsFloat32Number) In[2] / 4.F)) goto Error;
     }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr);
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;
}

static
cmsInt32Number Check3DinterpolationFloatTrilinear(void)
{
    cmsInterpParams* p;
    cmsInt32Number i;
    cmsFloat32Number In[3], Out[3];
    cmsFloat32Number FloatTable[] = { //R     G    B

        0,    0,   0,     // B=0,G=0,R=0
        0,    0,  .25,    // B=1,G=0,R=0

        0,   .5,    0,    // B=0,G=1,R=0
        0,   .5,  .25,    // B=1,G=1,R=0

        1,    0,    0,    // B=0,G=0,R=1
        1,    0,  .25,    // B=1,G=0,R=1

        1,    .5,   0,    // B=0,G=1,R=1
        1,    .5,  .25    // B=1,G=1,R=1

    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT|CMS_LERP_FLAGS_TRILINEAR);

    MaxErr = 0.0;
     for (i=0; i < 0xffff; i++) {

       In[0] = In[1] = In[2] = (cmsFloat32Number) ( (cmsFloat32Number) i / 65535.0F);

        p ->Interpolation.LerpFloat(In, Out, p);

       if (!IsGoodFixed15_16("Channel 1", Out[0], In[0])) goto Error;
       if (!IsGoodFixed15_16("Channel 2", Out[1], (cmsFloat32Number) In[1] / 2.F)) goto Error;
       if (!IsGoodFixed15_16("Channel 3", Out[2], (cmsFloat32Number) In[2] / 4.F)) goto Error;
     }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr);
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;

}

static
cmsInt32Number Check3DinterpolationTetrahedral16(void)
{
    cmsInterpParams* p;
    cmsInt32Number i;
    cmsUInt16Number In[3], Out[3];
    cmsUInt16Number Table[] = { 

        0,    0,   0,     
        0,    0,   0xffff,    

        0,    0xffff,    0,   
        0,    0xffff,    0xffff,  

        0xffff,    0,    0,    
        0xffff,    0,    0xffff,   

        0xffff,    0xffff,   0,    
        0xffff,    0xffff,   0xffff    
    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_16BITS);

    MaxErr = 0.0;
     for (i=0; i < 0xffff; i++) {

       In[0] = In[1] = In[2] = (cmsUInt16Number) i;

        p ->Interpolation.Lerp16(In, Out, p);

       if (!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
       if (!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
       if (!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
     }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr);
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;
}

static
cmsInt32Number Check3DinterpolationTrilinear16(void)
{
    cmsInterpParams* p;
    cmsInt32Number i;
    cmsUInt16Number In[3], Out[3];
    cmsUInt16Number Table[] = { 

        0,    0,   0,     
        0,    0,   0xffff,    

        0,    0xffff,    0,   
        0,    0xffff,    0xffff,  

        0xffff,    0,    0,    
        0xffff,    0,    0xffff,   

        0xffff,    0xffff,   0,    
        0xffff,    0xffff,   0xffff    
    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_TRILINEAR);

    MaxErr = 0.0;
     for (i=0; i < 0xffff; i++) {

       In[0] = In[1] = In[2] = (cmsUInt16Number) i;

        p ->Interpolation.Lerp16(In, Out, p);

       if (!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
       if (!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
       if (!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
     }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr);
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;
}


static
cmsInt32Number ExaustiveCheck3DinterpolationFloatTetrahedral(void)
{
    cmsInterpParams* p;
    cmsInt32Number r, g, b;
    cmsFloat32Number In[3], Out[3];
    cmsFloat32Number FloatTable[] = { //R     G    B

        0,    0,   0,     // B=0,G=0,R=0
        0,    0,  .25,    // B=1,G=0,R=0

        0,   .5,    0,    // B=0,G=1,R=0
        0,   .5,  .25,    // B=1,G=1,R=0

        1,    0,    0,    // B=0,G=0,R=1
        1,    0,  .25,    // B=1,G=0,R=1

        1,    .5,   0,    // B=0,G=1,R=1
        1,    .5,  .25    // B=1,G=1,R=1

    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT);

    MaxErr = 0.0;
    for (r=0; r < 0xff; r++) 
        for (g=0; g < 0xff; g++) 
            for (b=0; b < 0xff; b++) 
        {

            In[0] = (cmsFloat32Number) r / 255.0F;
            In[1] = (cmsFloat32Number) g / 255.0F;
            In[2] = (cmsFloat32Number) b / 255.0F;

       
        p ->Interpolation.LerpFloat(In, Out, p);

       if (!IsGoodFixed15_16("Channel 1", Out[0], In[0])) goto Error;
       if (!IsGoodFixed15_16("Channel 2", Out[1], (cmsFloat32Number) In[1] / 2.F)) goto Error;
       if (!IsGoodFixed15_16("Channel 3", Out[2], (cmsFloat32Number) In[2] / 4.F)) goto Error;
     }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr);
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;
}

static
cmsInt32Number ExaustiveCheck3DinterpolationFloatTrilinear(void)
{
    cmsInterpParams* p;
    cmsInt32Number r, g, b;
    cmsFloat32Number In[3], Out[3];
    cmsFloat32Number FloatTable[] = { //R     G    B

        0,    0,   0,     // B=0,G=0,R=0
        0,    0,  .25,    // B=1,G=0,R=0

        0,   .5,    0,    // B=0,G=1,R=0
        0,   .5,  .25,    // B=1,G=1,R=0

        1,    0,    0,    // B=0,G=0,R=1
        1,    0,  .25,    // B=1,G=0,R=1

        1,    .5,   0,    // B=0,G=1,R=1
        1,    .5,  .25    // B=1,G=1,R=1

    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT|CMS_LERP_FLAGS_TRILINEAR);

    MaxErr = 0.0;
    for (r=0; r < 0xff; r++) 
        for (g=0; g < 0xff; g++) 
            for (b=0; b < 0xff; b++) 
            {

                In[0] = (cmsFloat32Number) r / 255.0F;
                In[1] = (cmsFloat32Number) g / 255.0F;
                In[2] = (cmsFloat32Number) b / 255.0F;


                p ->Interpolation.LerpFloat(In, Out, p);

                if (!IsGoodFixed15_16("Channel 1", Out[0], In[0])) goto Error;
                if (!IsGoodFixed15_16("Channel 2", Out[1], (cmsFloat32Number) In[1] / 2.F)) goto Error;
                if (!IsGoodFixed15_16("Channel 3", Out[2], (cmsFloat32Number) In[2] / 4.F)) goto Error;
            }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr);
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;

}

static
cmsInt32Number ExhaustiveCheck3DinterpolationTetrahedral16(void)
{
    cmsInterpParams* p;
    cmsInt32Number r, g, b;
    cmsUInt16Number In[3], Out[3];
    cmsUInt16Number Table[] = { 

        0,    0,   0,     
        0,    0,   0xffff,    

        0,    0xffff,    0,   
        0,    0xffff,    0xffff,  

        0xffff,    0,    0,    
        0xffff,    0,    0xffff,   

        0xffff,    0xffff,   0,    
        0xffff,    0xffff,   0xffff    
    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_16BITS);

    for (r=0; r < 0xff; r++) 
        for (g=0; g < 0xff; g++) 
            for (b=0; b < 0xff; b++) 
        {
            In[0] = (cmsUInt16Number) r ;
            In[1] = (cmsUInt16Number) g ;
            In[2] = (cmsUInt16Number) b ;


        p ->Interpolation.Lerp16(In, Out, p);

       if (!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
       if (!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
       if (!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
     }
    
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;
}

static
cmsInt32Number ExhaustiveCheck3DinterpolationTrilinear16(void)
{
    cmsInterpParams* p;
    cmsInt32Number r, g, b;
    cmsUInt16Number In[3], Out[3];
    cmsUInt16Number Table[] = { 

        0,    0,   0,     
        0,    0,   0xffff,    

        0,    0xffff,    0,   
        0,    0xffff,    0xffff,  

        0xffff,    0,    0,    
        0xffff,    0,    0xffff,   

        0xffff,    0xffff,   0,    
        0xffff,    0xffff,   0xffff    
    };

    p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_TRILINEAR);

    for (r=0; r < 0xff; r++) 
        for (g=0; g < 0xff; g++) 
            for (b=0; b < 0xff; b++) 
        {
            In[0] = (cmsUInt16Number) r ;
            In[1] = (cmsUInt16Number)g ;
            In[2] = (cmsUInt16Number)b ;


        p ->Interpolation.Lerp16(In, Out, p);

       if (!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
       if (!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
       if (!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
     }

    
    _cmsFreeInterpParams(p);
    return 1;

Error:
    _cmsFreeInterpParams(p);
    return 0;
}

// Check reverse interpolation on LUTS. This is right now exclusively used by K preservation algorithm
static
cmsInt32Number CheckReverseInterpolation3x3(void)
{
 cmsPipeline* Lut;
 cmsStage* clut;
 cmsFloat32Number Target[3], Result[3], Hint[3];
 cmsFloat32Number err, max;
 cmsInt32Number i;
 cmsUInt16Number Table[] = { 

        0,    0,   0,                 // 0 0 0  
        0,    0,   0xffff,            // 0 0 1  

        0,    0xffff,    0,           // 0 1 0  
        0,    0xffff,    0xffff,      // 0 1 1  

        0xffff,    0,    0,           // 1 0 0  
        0xffff,    0,    0xffff,      // 1 0 1  

        0xffff,    0xffff,   0,       // 1 1 0  
        0xffff,    0xffff,   0xffff,  // 1 1 1  
    };

  

   Lut = cmsPipelineAlloc(DbgThread(), 3, 3);

   clut = cmsStageAllocCLut16bit(DbgThread(), 2, 3, 3, Table);
   cmsPipelineInsertStage(Lut, cmsAT_BEGIN, clut);
 
   Target[0] = 0; Target[1] = 0; Target[2] = 0; 
   Hint[0] = 0; Hint[1] = 0; Hint[2] = 0;
   cmsPipelineEvalReverseFloat(Target, Result, NULL, Lut);
   if (Result[0] != 0 || Result[1] != 0 || Result[2] != 0){

       Fail("Reverse interpolation didn't find zero");
       return 0;
   }

   // Transverse identity
   max = 0;
   for (i=0; i <= 100; i++) {

       cmsFloat32Number in = i / 100.0F;

       Target[0] = in; Target[1] = 0; Target[2] = 0; 
       cmsPipelineEvalReverseFloat(Target, Result, Hint, Lut);

       err = fabsf(in - Result[0]);
       if (err > max) max = err;

       memcpy(Hint, Result, sizeof(Hint));
   }

    cmsPipelineFree(Lut);
    return (max <= FLOAT_PRECISSION);
}


static
cmsInt32Number CheckReverseInterpolation4x3(void)
{
 cmsPipeline* Lut;
 cmsStage* clut;
 cmsFloat32Number Target[4], Result[4], Hint[4];
 cmsFloat32Number err, max;
 cmsInt32Number i;

 // 4 -> 3, output gets 3 first channels copied
 cmsUInt16Number Table[] = { 

        0,         0,         0,          //  0 0 0 0   = ( 0, 0, 0)
        0,         0,         0,          //  0 0 0 1   = ( 0, 0, 0)
                                        
        0,         0,         0xffff,     //  0 0 1 0   = ( 0, 0, 1)
        0,         0,         0xffff,     //  0 0 1 1   = ( 0, 0, 1)
                                            
        0,         0xffff,    0,          //  0 1 0 0   = ( 0, 1, 0)
        0,         0xffff,    0,          //  0 1 0 1   = ( 0, 1, 0)
                                            
        0,         0xffff,    0xffff,     //  0 1 1 0    = ( 0, 1, 1)
        0,         0xffff,    0xffff,     //  0 1 1 1    = ( 0, 1, 1)

        0xffff,    0,         0,          //  1 0 0 0    = ( 1, 0, 0)
        0xffff,    0,         0,          //  1 0 0 1    = ( 1, 0, 0)
                                            
        0xffff,    0,         0xffff,     //  1 0 1 0    = ( 1, 0, 1)
        0xffff,    0,         0xffff,     //  1 0 1 1    = ( 1, 0, 1)
                                            
        0xffff,    0xffff,    0,          //  1 1 0 0    = ( 1, 1, 0)
        0xffff,    0xffff,    0,          //  1 1 0 1    = ( 1, 1, 0)
                                            
        0xffff,    0xffff,    0xffff,     //  1 1 1 0    = ( 1, 1, 1)
        0xffff,    0xffff,    0xffff,     //  1 1 1 1    = ( 1, 1, 1)
    };

   
   Lut = cmsPipelineAlloc(DbgThread(), 4, 3);

   clut = cmsStageAllocCLut16bit(DbgThread(), 2, 4, 3, Table);
   cmsPipelineInsertStage(Lut, cmsAT_BEGIN, clut);
 
   // Check if the LUT is behaving as expected
   SubTest("4->3 feasibility");
   for (i=0; i <= 100; i++) {

       Target[0] = i / 100.0F;
       Target[1] = Target[0];
       Target[2] = 0;
       Target[3] = 12;

       cmsPipelineEvalFloat(Target, Result, Lut);

       if (!IsGoodFixed15_16("0", Target[0], Result[0])) return 0;
       if (!IsGoodFixed15_16("1", Target[1], Result[1])) return 0;
       if (!IsGoodFixed15_16("2", Target[2], Result[2])) return 0;
   }

   SubTest("4->3 zero");
   Target[0] = 0; 
   Target[1] = 0; 
   Target[2] = 0; 

   // This one holds the fixed K
   Target[3] = 0; 

   // This is our hint (which is a big lie in this case)
   Hint[0] = 0.1F; Hint[1] = 0.1F; Hint[2] = 0.1F;

   cmsPipelineEvalReverseFloat(Target, Result, Hint, Lut);

   if (Result[0] != 0 || Result[1] != 0 || Result[2] != 0 || Result[3] != 0){

       Fail("Reverse interpolation didn't find zero");
       return 0;
   }

   SubTest("4->3 find CMY");
   max = 0;
   for (i=0; i <= 100; i++) {

       cmsFloat32Number in = i / 100.0F;

       Target[0] = in; Target[1] = 0; Target[2] = 0; 
       cmsPipelineEvalReverseFloat(Target, Result, Hint, Lut);

       err = fabsf(in - Result[0]);
       if (err > max) max = err;

       memcpy(Hint, Result, sizeof(Hint));
   }

    cmsPipelineFree(Lut);
    return (max <= FLOAT_PRECISSION);
}



// Check all interpolation. 

static
cmsUInt16Number Fn8D1(cmsUInt16Number a1, cmsUInt16Number a2, cmsUInt16Number a3, cmsUInt16Number a4,
                      cmsUInt16Number a5, cmsUInt16Number a6, cmsUInt16Number a7, cmsUInt16Number a8,
                      cmsUInt32Number m)
{
    return (cmsUInt16Number) ((a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8) / m);
}


static
cmsUInt16Number Fn8D2(cmsUInt16Number a1, cmsUInt16Number a2, cmsUInt16Number a3, cmsUInt16Number a4,
                      cmsUInt16Number a5, cmsUInt16Number a6, cmsUInt16Number a7, cmsUInt16Number a8,
                      cmsUInt32Number m)
{
    return (cmsUInt16Number) ((a1 + 3* a2 + 3* a3 + a4 + a5 + a6 + a7 + a8 ) / (m + 4));
}


static
cmsUInt16Number Fn8D3(cmsUInt16Number a1, cmsUInt16Number a2, cmsUInt16Number a3, cmsUInt16Number a4,
                      cmsUInt16Number a5, cmsUInt16Number a6, cmsUInt16Number a7, cmsUInt16Number a8,
                      cmsUInt32Number m)
{
    return (cmsUInt16Number) ((3*a1 + 2*a2 + 3*a3 + a4 + a5 + a6 + a7 + a8) / (m + 5));
}




static
cmsInt32Number Sampler3D(register const cmsUInt16Number In[],
               register cmsUInt16Number Out[],
               register void * Cargo)
{

    Out[0] = Fn8D1(In[0], In[1], In[2], 0, 0, 0, 0, 0, 3);
    Out[1] = Fn8D2(In[0], In[1], In[2], 0, 0, 0, 0, 0, 3);
    Out[2] = Fn8D3(In[0], In[1], In[2], 0, 0, 0, 0, 0, 3);

    return 1;
}

static
cmsInt32Number Sampler4D(register const cmsUInt16Number In[],
               register cmsUInt16Number Out[],
               register void * Cargo)
{

    Out[0] = Fn8D1(In[0], In[1], In[2], In[3], 0, 0, 0, 0, 4);
    Out[1] = Fn8D2(In[0], In[1], In[2], In[3], 0, 0, 0, 0, 4);
    Out[2] = Fn8D3(In[0], In[1], In[2], In[3], 0, 0, 0, 0, 4);

    return 1;
}

static
cmsInt32Number Sampler5D(register const cmsUInt16Number In[],
               register cmsUInt16Number Out[],
               register void * Cargo)
{

    Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], 0, 0, 0, 5);
    Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], 0, 0, 0, 5);
    Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], 0, 0, 0, 5);

    return 1;
}

static
cmsInt32Number Sampler6D(register const cmsUInt16Number In[],
               register cmsUInt16Number Out[],
               register void * Cargo)
{

    Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], In[5], 0, 0, 6);
    Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], In[5], 0, 0, 6);
    Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], In[5], 0, 0, 6);

    return 1;
}

static
cmsInt32Number Sampler7D(register const cmsUInt16Number In[],
               register cmsUInt16Number Out[],
               register void * Cargo)
{

    Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], In[5], In[6], 0, 7);
    Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], In[5], In[6], 0, 7);
    Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], In[5], In[6], 0, 7);

    return 1;
}

static
cmsInt32Number Sampler8D(register const cmsUInt16Number In[],
               register cmsUInt16Number Out[],
               register void * Cargo)
{

    Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], In[5], In[6], In[7], 8);
    Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], In[5], In[6], In[7], 8);
    Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], In[5], In[6], In[7], 8);

    return 1;
}

static 
cmsBool CheckOne3D(cmsPipeline* lut, cmsUInt16Number a1, cmsUInt16Number a2, cmsUInt16Number a3)
{
    cmsUInt16Number In[3], Out1[3], Out2[3];

    In[0] = a1; In[1] = a2; In[2] = a3; 

    // This is the interpolated value
    cmsPipelineEval16(In, Out1, lut);

    // This is the real value
    Sampler3D(In, Out2, NULL);

    // Let's see the difference

    if (!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;

    return TRUE;
}

static 
cmsBool CheckOne4D(cmsPipeline* lut, cmsUInt16Number a1, cmsUInt16Number a2, cmsUInt16Number a3, cmsUInt16Number a4)
{
    cmsUInt16Number In[4], Out1[3], Out2[3];

    In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4;

    // This is the interpolated value
    cmsPipelineEval16(In, Out1, lut);

    // This is the real value
    Sampler4D(In, Out2, NULL);

    // Let's see the difference

    if (!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;

    return TRUE;
}

static 
cmsBool CheckOne5D(cmsPipeline* lut, cmsUInt16Number a1, cmsUInt16Number a2, 
                                     cmsUInt16Number a3, cmsUInt16Number a4, cmsUInt16Number a5)
{
    cmsUInt16Number In[5], Out1[3], Out2[3];

    In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4; In[4] = a5;

    // This is the interpolated value
    cmsPipelineEval16(In, Out1, lut);

    // This is the real value
    Sampler5D(In, Out2, NULL);

    // Let's see the difference

    if (!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;

    return TRUE;
}

static 
cmsBool CheckOne6D(cmsPipeline* lut, cmsUInt16Number a1, cmsUInt16Number a2, 
                                     cmsUInt16Number a3, cmsUInt16Number a4, 
                                     cmsUInt16Number a5, cmsUInt16Number a6)
{
    cmsUInt16Number In[6], Out1[3], Out2[3];

    In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4; In[4] = a5; In[5] = a6;

    // This is the interpolated value
    cmsPipelineEval16(In, Out1, lut);

    // This is the real value
    Sampler6D(In, Out2, NULL);

    // Let's see the difference

    if (!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;

    return TRUE;
}


static 
cmsBool CheckOne7D(cmsPipeline* lut, cmsUInt16Number a1, cmsUInt16Number a2, 
                                     cmsUInt16Number a3, cmsUInt16Number a4, 
                                     cmsUInt16Number a5, cmsUInt16Number a6,
                                     cmsUInt16Number a7)
{
    cmsUInt16Number In[7], Out1[3], Out2[3];

    In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4; In[4] = a5; In[5] = a6; In[6] = a7;

    // This is the interpolated value
    cmsPipelineEval16(In, Out1, lut);

    // This is the real value
    Sampler7D(In, Out2, NULL);

    // Let's see the difference

    if (!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;

    return TRUE;
}


static 
cmsBool CheckOne8D(cmsPipeline* lut, cmsUInt16Number a1, cmsUInt16Number a2, 
                                     cmsUInt16Number a3, cmsUInt16Number a4, 
                                     cmsUInt16Number a5, cmsUInt16Number a6,
                                     cmsUInt16Number a7, cmsUInt16Number a8)
{
    cmsUInt16Number In[8], Out1[3], Out2[3];

    In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4; In[4] = a5; In[5] = a6; In[6] = a7; In[7] = a8;

    // This is the interpolated value
    cmsPipelineEval16(In, Out1, lut);

    // This is the real value
    Sampler8D(In, Out2, NULL);

    // Let's see the difference

    if (!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
    if (!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;

    return TRUE;
}


static
cmsInt32Number Check3Dinterp(void)
{
    cmsPipeline* lut;
    cmsStage* mpe;

    lut = cmsPipelineAlloc(DbgThread(), 3, 3);
    mpe = cmsStageAllocCLut16bit(DbgThread(), 9, 3, 3, NULL);
    cmsStageSampleCLut16bit(mpe, Sampler3D, NULL, 0);
    cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);

    // Check accuracy

    if (!CheckOne3D(lut, 0, 0, 0)) return 0;
    if (!CheckOne3D(lut, 0xffff, 0xffff, 0xffff)) return 0; 

    if (!CheckOne3D(lut, 0x8080, 0x8080, 0x8080)) return 0;
    if (!CheckOne3D(lut, 0x0000, 0xFE00, 0x80FF)) return 0;
    if (!CheckOne3D(lut, 0x1111, 0x2222, 0x3333)) return 0;
    if (!CheckOne3D(lut, 0x0000, 0x0012, 0x0013)) return 0; 
    if (!CheckOne3D(lut, 0x3141, 0x1415, 0x1592)) return 0; 
    if (!CheckOne3D(lut, 0xFF00, 0xFF01, 0xFF12)) return 0; 

    cmsPipelineFree(lut);

    return 1;
}

static
cmsInt32Number Check3DinterpGranular(void)
{
    cmsPipeline* lut;
    cmsStage* mpe;
    cmsUInt32Number Dimensions[] = { 7, 8, 9 };

    lut = cmsPipelineAlloc(DbgThread(), 3, 3);
    mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 3, 3, NULL);
    cmsStageSampleCLut16bit(mpe, Sampler3D, NULL, 0);
    cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);

    // Check accuracy

    if (!CheckOne3D(lut, 0, 0, 0)) return 0;
    if (!CheckOne3D(lut, 0xffff, 0xffff, 0xffff)) return 0; 

    if (!CheckOne3D(lut, 0x8080, 0x8080, 0x8080)) return 0;
    if (!CheckOne3D(lut, 0x0000, 0xFE00, 0x80FF)) return 0;
    if (!CheckOne3D(lut, 0x1111, 0x2222, 0x3333)) return 0;
    if (!CheckOne3D(lut, 0x0000, 0x0012, 0x0013)) return 0; 
    if (!CheckOne3D(lut, 0x3141, 0x1415, 0x1592)) return 0; 
    if (!CheckOne3D(lut, 0xFF00, 0xFF01, 0xFF12)) return 0; 

    cmsPipelineFree(lut);

    return 1;
}


static
cmsInt32Number Check4Dinterp(void)
{
    cmsPipeline* lut;
    cmsStage* mpe;

    lut = cmsPipelineAlloc(DbgThread(), 4, 3);
    mpe = cmsStageAllocCLut16bit(DbgThread(), 9, 4, 3, NULL);
    cmsStageSampleCLut16bit(mpe, Sampler4D, NULL, 0);
    cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);

    // Check accuracy

    if (!CheckOne4D(lut, 0, 0, 0, 0)) return 0;
    if (!CheckOne4D(lut, 0xffff, 0xffff, 0xffff, 0xffff)) return 0; 

    if (!CheckOne4D(lut, 0x8080, 0x8080, 0x8080, 0x8080)) return 0;
    if (!CheckOne4D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888)) return 0;
    if (!CheckOne4D(lut, 0x1111, 0x2222, 0x3333, 0x4444)) return 0;
    if (!CheckOne4D(lut, 0x0000, 0x0012, 0x0013, 0x0014)) return 0; 
    if (!CheckOne4D(lut, 0x3141, 0x1415, 0x1592, 0x9261)) return 0; 
    if (!CheckOne4D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13)) return 0; 

    cmsPipelineFree(lut);

    return 1;
}



static
cmsInt32Number Check4DinterpGranular(void)
{
    cmsPipeline* lut;
    cmsStage* mpe;
    cmsUInt32Number Dimensions[] = { 9, 8, 7, 6 };

    lut = cmsPipelineAlloc(DbgThread(), 4, 3);
    mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 4, 3, NULL);
    cmsStageSampleCLut16bit(mpe, Sampler4D, NULL, 0);
    cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);

    // Check accuracy

    if (!CheckOne4D(lut, 0, 0, 0, 0)) return 0;
    if (!CheckOne4D(lut, 0xffff, 0xffff, 0xffff, 0xffff)) return 0; 

    if (!CheckOne4D(lut, 0x8080, 0x8080, 0x8080, 0x8080)) return 0;
    if (!CheckOne4D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888)) return 0;
    if (!CheckOne4D(lut, 0x1111, 0x2222, 0x3333, 0x4444)) return 0;
    if (!CheckOne4D(lut, 0x0000, 0x0012, 0x0013, 0x0014)) return 0; 
    if (!CheckOne4D(lut, 0x3141, 0x1415, 0x1592, 0x9261)) return 0; 
    if (!CheckOne4D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13)) return 0; 

    cmsPipelineFree(lut);

    return 1;
}


static
cmsInt32Number Check5DinterpGranular(void)
{
    cmsPipeline* lut;
    cmsStage* mpe;
    cmsUInt32Number Dimensions[] = { 3, 2, 2, 2, 2 };

    lut = cmsPipelineAlloc(DbgThread(), 5, 3);
    mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 5, 3, NULL);
    cmsStageSampleCLut16bit(mpe, Sampler5D, NULL, 0);
    cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);

    // Check accuracy

    if (!CheckOne5D(lut, 0, 0, 0, 0, 0)) return 0;
    if (!CheckOne5D(lut, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff)) return 0; 

    if (!CheckOne5D(lut, 0x8080, 0x8080, 0x8080, 0x8080, 0x1234)) return 0;
    if (!CheckOne5D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888, 0x8078)) return 0;
    if (!CheckOne5D(lut, 0x1111, 0x2222, 0x3333, 0x4444, 0x1455)) return 0;
    if (!CheckOne5D(lut, 0x0000, 0x0012, 0x0013, 0x0014, 0x2333)) return 0; 
    if (!CheckOne5D(lut, 0x3141, 0x1415, 0x1592, 0x9261, 0x4567)) return 0; 
    if (!CheckOne5D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13, 0xF344)) return 0; 

    cmsPipelineFree(lut);

    return 1;
}

static
cmsInt32Number Check6DinterpGranular(void)
{
    cmsPipeline* lut;
    cmsStage* mpe;
    cmsUInt32Number Dimensions[] = { 4, 3, 3, 2, 2, 2 };

    lut = cmsPipelineAlloc(DbgThread(), 6, 3);
    mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 6, 3, NULL);
    cmsStageSampleCLut16bit(mpe, Sampler6D, NULL, 0);
    cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);

    // Check accuracy

    if (!CheckOne6D(lut, 0, 0, 0, 0, 0, 0)) return 0;
    if (!CheckOne6D(lut, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff)) return 0; 

    if (!CheckOne6D(lut, 0x8080, 0x8080, 0x8080, 0x8080, 0x1234, 0x1122)) return 0;
    if (!CheckOne6D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888, 0x8078, 0x2233)) return 0;
    if (!CheckOne6D(lut, 0x1111, 0x2222, 0x3333, 0x4444, 0x1455, 0x3344)) return 0;
    if (!CheckOne6D(lut, 0x0000, 0x0012, 0x0013, 0x0014, 0x2333, 0x4455)) return 0; 
    if (!CheckOne6D(lut, 0x3141, 0x1415, 0x1592, 0x9261, 0x4567, 0x5566)) return 0; 
    if (!CheckOne6D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13, 0xF344, 0x6677)) return 0; 

    cmsPipelineFree(lut);

    return 1;
}

static
cmsInt32Number Check7DinterpGranular(void)
{
    cmsPipeline* lut;
    cmsStage* mpe;
    cmsUInt32Number Dimensions[] = { 4, 3, 3, 2, 2, 2, 2 };

    lut = cmsPipelineAlloc(DbgThread(), 7, 3);
    mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 7, 3, NULL);
    cmsStageSampleCLut16bit(mpe, Sampler7D, NULL, 0);
    cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);

    // Check accuracy

    if (!CheckOne7D(lut, 0, 0, 0, 0, 0, 0, 0)) return 0;
    if (!CheckOne7D(lut, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff)) return 0; 

    if (!CheckOne7D(lut, 0x8080, 0x8080, 0x8080, 0x8080, 0x1234, 0x1122, 0x0056)) return 0;
    if (!CheckOne7D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888, 0x8078, 0x2233, 0x0088)) return 0;
    if (!CheckOne7D(lut, 0x1111, 0x2222, 0x3333, 0x4444, 0x1455, 0x3344, 0x1987)) return 0;
    if (!CheckOne7D(lut, 0x0000, 0x0012, 0x0013, 0x0014, 0x2333, 0x4455, 0x9988)) return 0; 
    if (!CheckOne7D(lut, 0x3141, 0x1415, 0x1592, 0x9261, 0x4567, 0x5566, 0xfe56)) return 0; 
    if (!CheckOne7D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13, 0xF344, 0x6677, 0xbabe)) return 0; 

    cmsPipelineFree(lut);

    return 1;
}
// Colorimetric conversions -------------------------------------------------------------------------------------------------

// Lab to LCh and back should be performed at 1E-12 accuracy at least
static
cmsInt32Number CheckLab2LCh(void)
{
    cmsInt32Number l, a, b;
    cmsFloat64Number dist, Max = 0;
    cmsCIELab Lab, Lab2;
    cmsCIELCh LCh;

    for (l=0; l <= 100; l += 10) {

        for (a=-128; a <= +128; a += 8) {
            
            for (b=-128; b <= 128; b += 8) {

                Lab.L = l;
                Lab.a = a;
                Lab.b = b;

                cmsLab2LCh(&LCh, &Lab);
                cmsLCh2Lab(&Lab2, &LCh);

                dist = cmsDeltaE(&Lab, &Lab2);
                if (dist > Max) Max = dist;                
            }
        }
    }

    return Max < 1E-12;
}

// Lab to LCh and back should be performed at 1E-12 accuracy at least
static
cmsInt32Number CheckLab2XYZ(void)
{
    cmsInt32Number l, a, b;
    cmsFloat64Number dist, Max = 0;
    cmsCIELab Lab, Lab2;
    cmsCIEXYZ XYZ;

    for (l=0; l <= 100; l += 10) {

        for (a=-128; a <= +128; a += 8) {

            for (b=-128; b <= 128; b += 8) {

                Lab.L = l;
                Lab.a = a;
                Lab.b = b;

                cmsLab2XYZ(NULL, &XYZ, &Lab);
                cmsXYZ2Lab(NULL, &Lab2, &XYZ);

                dist = cmsDeltaE(&Lab, &Lab2);
                if (dist > Max) Max = dist;

            }
        }
    }

    return Max < 1E-12;
}

// Lab to xyY and back should be performed at 1E-12 accuracy at least
static
cmsInt32Number CheckLab2xyY(void)
{
    cmsInt32Number l, a, b;
    cmsFloat64Number dist, Max = 0;
    cmsCIELab Lab, Lab2;
    cmsCIEXYZ XYZ;
    cmsCIExyY xyY;

    for (l=0; l <= 100; l += 10) {

        for (a=-128; a <= +128; a += 8) {

            for (b=-128; b <= 128; b += 8) {

                Lab.L = l;
                Lab.a = a;
                Lab.b = b;

                cmsLab2XYZ(NULL, &XYZ, &Lab);
                cmsXYZ2xyY(&xyY, &XYZ);
                cmsxyY2XYZ(&XYZ, &xyY);
                cmsXYZ2Lab(NULL, &Lab2, &XYZ);

                dist = cmsDeltaE(&Lab, &Lab2);
                if (dist > Max) Max = dist;

            }
        }
    }

    return Max < 1E-12;
}


static
cmsInt32Number CheckLabV2encoding(void)
{
    cmsInt32Number n2, i, j;
    cmsUInt16Number Inw[3], aw[3];
    cmsCIELab Lab;

    n2=0;
    
    for (j=0; j < 65535; j++) {

        Inw[0] = Inw[1] = Inw[2] = (cmsUInt16Number) j;

        cmsLabEncoded2FloatV2(&Lab, Inw);
        cmsFloat2LabEncodedV2(aw, &Lab);

        for (i=0; i < 3; i++) {

        if (aw[i] != j) {
            n2++;
        }
        }

    }

    return (n2 == 0);
}

static
cmsInt32Number CheckLabV4encoding(void)
{
    cmsInt32Number n2, i, j;
    cmsUInt16Number Inw[3], aw[3];
    cmsCIELab Lab;

    n2=0;
    
    for (j=0; j < 65535; j++) {

        Inw[0] = Inw[1] = Inw[2] = (cmsUInt16Number) j;

        cmsLabEncoded2Float(&Lab, Inw);
        cmsFloat2LabEncoded(aw, &Lab);
        
        for (i=0; i < 3; i++) {

        if (aw[i] != j) {
            n2++;
        }
        }

    }

    return (n2 == 0);
}


// BlackBody -----------------------------------------------------------------------------------------------------

static
cmsInt32Number CheckTemp2CHRM(void)
{
    cmsInt32Number j;
    cmsFloat64Number d, v, Max = 0;
    cmsCIExyY White;
    
    for (j=4000; j < 25000; j++) {

        cmsWhitePointFromTemp(&White, j);
        if (!cmsTempFromWhitePoint(&v, &White)) return 0;

        d = fabs(v - j);
        if (d > Max) Max = d;
    }

    // 100 degree is the actual resolution
    return (Max < 100);
}



// Tone curves -----------------------------------------------------------------------------------------------------

static
cmsInt32Number CheckGammaEstimation(cmsToneCurve* c, cmsFloat64Number g)
{
    cmsFloat64Number est = cmsEstimateGamma(c, 0.001);

    SubTest("Gamma estimation");
    if (fabs(est - g) > 0.001) return 0;
    return 1;
}

static
cmsInt32Number CheckGammaCreation16(void)
{
    cmsToneCurve* LinGamma = cmsBuildGamma(DbgThread(), 1.0);
    cmsInt32Number i;
    cmsUInt16Number in, out;

    for (i=0; i < 0xffff; i++) {

        in = (cmsUInt16Number) i;
        out = cmsEvalToneCurve16(LinGamma, in);
        if (in != out) {
            Fail("(lin gamma): Must be %x, But is %x : ", in, out);
            cmsFreeToneCurve(LinGamma);
            return 0;
        }
    }       

    if (!CheckGammaEstimation(LinGamma, 1.0)) return 0;

    cmsFreeToneCurve(LinGamma);
    return 1;

}

static
cmsInt32Number CheckGammaCreationFlt(void)
{
    cmsToneCurve* LinGamma = cmsBuildGamma(DbgThread(), 1.0);
    cmsInt32Number i;
    cmsFloat32Number in, out;

    for (i=0; i < 0xffff; i++) {

        in = (cmsFloat32Number) (i / 65535.0);
        out = cmsEvalToneCurveFloat(LinGamma, in);
        if (fabs(in - out) > (1/65535.0)) {
            Fail("(lin gamma): Must be %f, But is %f : ", in, out);
            cmsFreeToneCurve(LinGamma);
            return 0;
        }
    }       

    if (!CheckGammaEstimation(LinGamma, 1.0)) return 0;
    cmsFreeToneCurve(LinGamma);
    return 1;
}

// Curve curves using a single power function
// Error is given in 0..ffff counts
static
cmsInt32Number CheckGammaFloat(cmsFloat64Number g)
{
    cmsToneCurve* Curve = cmsBuildGamma(DbgThread(), g);
    cmsInt32Number i;
    cmsFloat32Number in, out;
    cmsFloat64Number val, Err;

    MaxErr = 0.0;
    for (i=0; i < 0xffff; i++) {

        in = (cmsFloat32Number) (i / 65535.0);
        out = cmsEvalToneCurveFloat(Curve, in);
        val = pow((cmsFloat64Number) in, g);

        Err = fabs( val - out);
        if (Err > MaxErr) MaxErr = Err;     
    }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr * 65535.0);

    if (!CheckGammaEstimation(Curve, g)) return 0;

    cmsFreeToneCurve(Curve);
    return 1;
}

static cmsInt32Number CheckGamma18(void) 
{
    return CheckGammaFloat(1.8);
}

static cmsInt32Number CheckGamma22(void) 
{
    return CheckGammaFloat(2.2);
}

static cmsInt32Number CheckGamma30(void) 
{
    return CheckGammaFloat(3.0);
}


// Check table-based gamma functions
static
cmsInt32Number CheckGammaFloatTable(cmsFloat64Number g)
{
    cmsFloat32Number Values[1025];
    cmsToneCurve* Curve; 
    cmsInt32Number i;
    cmsFloat32Number in, out;
    cmsFloat64Number val, Err;

    for (i=0; i <= 1024; i++) {

        in = (cmsFloat32Number) (i / 1024.0);
        Values[i] = powf(in, (float) g);
    }

    Curve = cmsBuildTabulatedToneCurveFloat(DbgThread(), 1025, Values);

    MaxErr = 0.0;
    for (i=0; i <= 0xffff; i++) {

        in = (cmsFloat32Number) (i / 65535.0);
        out = cmsEvalToneCurveFloat(Curve, in);
        val = pow(in, g);

        Err = fabs(val - out);
        if (Err > MaxErr) MaxErr = Err;     
    }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr * 65535.0);

    if (!CheckGammaEstimation(Curve, g)) return 0;

    cmsFreeToneCurve(Curve);
    return 1;
}


static cmsInt32Number CheckGamma18Table(void) 
{
    return CheckGammaFloatTable(1.8);
}

static cmsInt32Number CheckGamma22Table(void) 
{
    return CheckGammaFloatTable(2.2);
}

static cmsInt32Number CheckGamma30Table(void) 
{
    return CheckGammaFloatTable(3.0);
}

// Create a curve from a table (which is a pure gamma function) and check it against the pow function.
static
cmsInt32Number CheckGammaWordTable(cmsFloat64Number g)
{
    cmsUInt16Number Values[1025];
    cmsToneCurve* Curve; 
    cmsInt32Number i;
    cmsFloat32Number in, out;
    cmsFloat64Number val, Err;

    for (i=0; i <= 1024; i++) {

        in = (cmsFloat32Number) (i / 1024.0);
        Values[i] = (cmsUInt16Number) floor(pow(in, g) * 65535.0 + 0.5);
    }

    Curve = cmsBuildTabulatedToneCurve16(DbgThread(), 1025, Values);

    MaxErr = 0.0;
    for (i=0; i <= 0xffff; i++) {

        in = (cmsFloat32Number) (i / 65535.0);
        out = cmsEvalToneCurveFloat(Curve, in);
        val = pow(in, g);

        Err = fabs(val - out);
        if (Err > MaxErr) MaxErr = Err;     
    }

    if (MaxErr > 0) printf("|Err|<%lf ", MaxErr * 65535.0);

    if (!CheckGammaEstimation(Curve, g)) return 0;

    cmsFreeToneCurve(Curve);
    return 1;
}

static cmsInt32Number CheckGamma18TableWord(void) 
{
    return CheckGammaWordTable(1.8);
}

static cmsInt32Number CheckGamma22TableWord(void) 
{
    return CheckGammaWordTable(2.2);
}

static cmsInt32Number CheckGamma30TableWord(void) 
{
    return CheckGammaWordTable(3.0);
}


// Curve joining test. Joining two high-gamma of 3.0 curves should
// give something like linear
static
cmsInt32Number CheckJointCurves(void)
{
    cmsToneCurve *Forward, *Reverse, *Result;
    cmsBool  rc;

    Forward = cmsBuildGamma(DbgThread(), 3.0);
    Reverse = cmsBuildGamma(DbgThread(), 3.0);

    Result = cmsJoinToneCurve(DbgThread(), Forward, Reverse, 256);

    cmsFreeToneCurve(Forward); cmsFreeToneCurve(Reverse); 

    rc = cmsIsToneCurveLinear(Result);
    cmsFreeToneCurve(Result); 

    if (!rc)
        Fail("Joining same curve twice does not result in a linear ramp");

    return rc;
}


// Create a gamma curve by cheating the table
static
cmsToneCurve* GammaTableLinear(cmsInt32Number nEntries, cmsBool Dir)
{
    cmsInt32Number i;
    cmsToneCurve* g = cmsBuildTabulatedToneCurve16(DbgThread(), nEntries, NULL);

    for (i=0; i < nEntries; i++) {

        cmsInt32Number v = _cmsQuantizeVal(i, nEntries);

        if (Dir)
            g->Table16[i] = (cmsUInt16Number) v;
        else
            g->Table16[i] = (cmsUInt16Number) (0xFFFF - v);
    }

    return g;
}


static
cmsInt32Number CheckJointCurvesDescending(void)
{
    cmsToneCurve *Forward, *Reverse, *Result;
    cmsInt32Number i, rc;

     Forward = cmsBuildGamma(DbgThread(), 2.2); 

    // Fake the curve to be table-based

    for (i=0; i < 4096; i++)
        Forward ->Table16[i] = 0xffff - Forward->Table16[i];
    Forward ->Segments[0].Type = 0;

    Reverse = cmsReverseToneCurve(Forward); 
    
    Result = cmsJoinToneCurve(DbgThread(), Reverse, Reverse, 256);
    
    cmsFreeToneCurve(Forward); 
    cmsFreeToneCurve(Reverse); 

    rc = cmsIsToneCurveLinear(Result);
    cmsFreeToneCurve(Result); 

    return rc;
}


static
cmsInt32Number CheckFToneCurvePoint(cmsToneCurve* c, cmsUInt16Number Point, cmsInt32Number Value)
{
    cmsInt32Number Result;

    Result = cmsEvalToneCurve16(c, Point);
    
    return (abs(Value - Result) < 2);
}

static
cmsInt32Number CheckReverseDegenerated(void)
{
    cmsToneCurve* p, *g;
    cmsUInt16Number Tab[16];
    
    Tab[0] = 0;
    Tab[1] = 0;
    Tab[2] = 0;
    Tab[3] = 0;
    Tab[4] = 0;
    Tab[5] = 0x5555;
    Tab[6] = 0x6666;
    Tab[7] = 0x7777;
    Tab[8] = 0x8888;
    Tab[9] = 0x9999;
    Tab[10]= 0xffff;
    Tab[11]= 0xffff;
    Tab[12]= 0xffff;
    Tab[13]= 0xffff;
    Tab[14]= 0xffff;
    Tab[15]= 0xffff;

    p = cmsBuildTabulatedToneCurve16(DbgThread(), 16, Tab);
    g = cmsReverseToneCurve(p);
    
    // Now let's check some points
    if (!CheckFToneCurvePoint(g, 0x5555, 0x5555)) return 0;
    if (!CheckFToneCurvePoint(g, 0x7777, 0x7777)) return 0;

    // First point for zero
    if (!CheckFToneCurvePoint(g, 0x0000, 0x4444)) return 0;

    // Last point
    if (!CheckFToneCurvePoint(g, 0xFFFF, 0xFFFF)) return 0;

    cmsFreeToneCurve(p);
    cmsFreeToneCurve(g);

    return 1;
}


// Build a parametric sRGB-like curve
static
cmsToneCurve* Build_sRGBGamma(void)
{
    cmsFloat64Number Parameters[5];

    Parameters[0] = 2.4;
    Parameters[1] = 1. / 1.055;
    Parameters[2] = 0.055 / 1.055;
    Parameters[3] = 1. / 12.92;
    Parameters[4] = 0.04045;    // d

    return cmsBuildParametricToneCurve(DbgThread(), 4, Parameters);
}



// Join two gamma tables in floting point format. Result should be a straight line
static
cmsToneCurve* CombineGammaFloat(cmsToneCurve* g1, cmsToneCurve* g2)
{
    cmsUInt16Number Tab[256];
    cmsFloat32Number f;
    cmsInt32Number i;

    for (i=0; i < 256; i++) {

        f = (cmsFloat32Number) i / 255.0F;
        f = cmsEvalToneCurveFloat(g2, cmsEvalToneCurveFloat(g1, f));

        Tab[i] = (cmsUInt16Number) floor(f * 65535.0 + 0.5);
    }

    return  cmsBuildTabulatedToneCurve16(DbgThread(), 256, Tab);
}

// Same of anterior, but using quantized tables
static
cmsToneCurve* CombineGamma16(cmsToneCurve* g1, cmsToneCurve* g2)
{
    cmsUInt16Number Tab[256];

    cmsInt32Number i;

    for (i=0; i < 256; i++) {

        cmsUInt16Number wValIn;

        wValIn = _cmsQuantizeVal(i, 256);     
        Tab[i] = cmsEvalToneCurve16(g2, cmsEvalToneCurve16(g1, wValIn));
    }

    return  cmsBuildTabulatedToneCurve16(DbgThread(), 256, Tab);
}

static
cmsInt32Number CheckJointFloatCurves_sRGB(void)
{
    cmsToneCurve *Forward, *Reverse, *Result;
    cmsBool  rc;

    Forward = Build_sRGBGamma();
    Reverse = cmsReverseToneCurve(Forward);
    Result = CombineGammaFloat(Forward, Reverse);
    cmsFreeToneCurve(Forward); cmsFreeToneCurve(Reverse); 

    rc = cmsIsToneCurveLinear(Result);
    cmsFreeToneCurve(Result); 

    return rc;
}

static
cmsInt32Number CheckJoint16Curves_sRGB(void)
{
    cmsToneCurve *Forward, *Reverse, *Result;
    cmsBool  rc;

    Forward = Build_sRGBGamma();
    Reverse = cmsReverseToneCurve(Forward);
    Result = CombineGamma16(Forward, Reverse);
    cmsFreeToneCurve(Forward); cmsFreeToneCurve(Reverse); 

    rc = cmsIsToneCurveLinear(Result);
    cmsFreeToneCurve(Result); 

    return rc;
}

// sigmoidal curve f(x) = (1-x^g) ^(1/g)

static
cmsInt32Number CheckJointCurvesSShaped(void)
{
    cmsFloat64Number p = 3.2;
    cmsToneCurve *Forward, *Reverse, *Result;
    cmsInt32Number rc;

    Forward = cmsBuildParametricToneCurve(DbgThread(), 108, &p);    
    Reverse = cmsReverseToneCurve(Forward);
    Result = cmsJoinToneCurve(DbgThread(), Forward, Forward, 4096);

    cmsFreeToneCurve(Forward); 
    cmsFreeToneCurve(Reverse); 

    rc = cmsIsToneCurveLinear(Result);
    cmsFreeToneCurve(Result); 
    return rc;
}


// --------------------------------------------------------------------------------------------------------

// Implementation of some tone curve functions
static
cmsFloat32Number Gamma(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    return (cmsFloat32Number) pow(x, Params[0]);
}

static
cmsFloat32Number CIE122(cmsFloat32Number x, const cmsFloat64Number Params[])

{
    cmsFloat64Number e, Val;

    if (x >= -Params[2] / Params[1]) {

        e = Params[1]*x + Params[2];

        if (e > 0)
            Val = pow(e, Params[0]);
        else
            Val = 0;
    }
    else
        Val = 0;

    return (cmsFloat32Number) Val;
}

static
cmsFloat32Number IEC61966_3(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    cmsFloat64Number e, Val;

    if (x >= -Params[2] / Params[1]) {

        e = Params[1]*x + Params[2];                    

        if (e > 0)
            Val = pow(e, Params[0]) + Params[3];
        else
            Val = 0;
    }
    else
        Val = Params[3];

    return (cmsFloat32Number) Val;
}

static
cmsFloat32Number IEC61966_21(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    cmsFloat64Number e, Val;

    if (x >= Params[4]) {

        e = Params[1]*x + Params[2];

        if (e > 0)
            Val = pow(e, Params[0]);
        else
            Val = 0;
    }
    else
        Val = x * Params[3];

    return (cmsFloat32Number) Val;
}

static
cmsFloat32Number param_5(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    cmsFloat64Number e, Val;
    // Y = (aX + b)^Gamma + e | X >= d
    // Y = cX + f             | else
    if (x >= Params[4]) {

        e = Params[1]*x + Params[2];
        if (e > 0)
            Val = pow(e, Params[0]) + Params[5];
        else
            Val = 0;
    }        
    else
        Val = x*Params[3] + Params[6];

    return (cmsFloat32Number) Val;
}

static
cmsFloat32Number param_6(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    cmsFloat64Number e, Val;
    
    e = Params[1]*x + Params[2];
    if (e > 0) 
        Val = pow(e, Params[0]) + Params[3];
    else
        Val = 0;

    return (cmsFloat32Number) Val;
}

static
cmsFloat32Number param_7(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    cmsFloat64Number Val;


    Val = Params[1]*log10(Params[2] * pow(x, Params[0]) + Params[3]) + Params[4];

    return (cmsFloat32Number) Val;
}


static
cmsFloat32Number param_8(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    cmsFloat64Number Val;

    Val = (Params[0] * pow(Params[1], Params[2] * x + Params[3]) + Params[4]);

    return (cmsFloat32Number) Val;
}


static
cmsFloat32Number sigmoidal(cmsFloat32Number x, const cmsFloat64Number Params[])
{
    cmsFloat64Number Val;

    Val = pow(1.0 - pow(1 - x, 1/Params[0]), 1/Params[0]);

    return (cmsFloat32Number) Val;
}


static
cmsBool CheckSingleParametric(const char* Name, dblfnptr fn, cmsInt32Number Type, const cmsFloat64Number Params[])
{
    cmsInt32Number i;
    cmsToneCurve* tc;
    cmsToneCurve* tc_1;
    char InverseText[256];

    tc = cmsBuildParametricToneCurve(DbgThread(), Type, Params);
    tc_1 = cmsBuildParametricToneCurve(DbgThread(), -Type, Params);

    for (i=0; i <= 1000; i++) {

        cmsFloat32Number x = (cmsFloat32Number) i / 1000;
        cmsFloat32Number y_fn, y_param, x_param, y_param2;

        y_fn = fn(x, Params);
        y_param = cmsEvalToneCurveFloat(tc, x);
        x_param = cmsEvalToneCurveFloat(tc_1, y_param);

        y_param2 = fn(x_param, Params);

        if (!IsGoodVal(Name, y_fn, y_param, FIXED_PRECISION_15_16))
            goto Error;

        sprintf(InverseText, "Inverse %s", Name);
        if (!IsGoodVal(InverseText, y_fn, y_param2, FIXED_PRECISION_15_16)) 
            goto Error;       
    }

    cmsFreeToneCurve(tc);
    cmsFreeToneCurve(tc_1);
    return TRUE;

Error:
    cmsFreeToneCurve(tc);
    cmsFreeToneCurve(tc_1);
    return FALSE;
}

// Check against some known values
static
cmsInt32Number CheckParametricToneCurves(void)
{
    cmsFloat64Number Params[10];

     // 1) X = Y ^ Gamma
     
     Params[0] = 2.2;

     if (!CheckSingleParametric("Gamma", Gamma, 1, Params)) return 0;

     // 2) CIE 122-1966
     // Y = (aX + b)^Gamma  | X >= -b/a
     // Y = 0               | else

     Params[0] = 2.2;
     Params[1] = 1.5;
     Params[2] = -0.5;

     if (!CheckSingleParametric("CIE122-1966", CIE122, 2, Params)) return 0;

     // 3) IEC 61966-3
     // Y = (aX + b)^Gamma | X <= -b/a
     // Y = c              | else

     Params[0] = 2.2;
     Params[1] = 1.5;
     Params[2] = -0.5;
     Params[3] = 0.3;
  

     if (!CheckSingleParametric("IEC 61966-3", IEC61966_3, 3, Params)) return 0;

     // 4) IEC 61966-2.1 (sRGB)
     // Y = (aX + b)^Gamma | X >= d
     // Y = cX             | X < d

     Params[0] = 2.4;
     Params[1] = 1. / 1.055;
     Params[2] = 0.055 / 1.055;
     Params[3] = 1. / 12.92;
     Params[4] = 0.04045;    

     if (!CheckSingleParametric("IEC 61966-2.1", IEC61966_21, 4, Params)) return 0;

               
     // 5) Y = (aX + b)^Gamma + e | X >= d
     // Y = cX + f             | else

     Params[0] = 2.2;
     Params[1] = 0.7;
     Params[2] = 0.2;
     Params[3] = 0.3;
     Params[4] = 0.1;
     Params[5] = 0.5;
     Params[6] = 0.2;
     
     if (!CheckSingleParametric("param_5", param_5, 5, Params)) return 0;

     // 6) Y = (aX + b) ^ Gamma + c

     Params[0] = 2.2;
     Params[1] = 0.7;
     Params[2] = 0.2;
     Params[3] = 0.3;
     
     if (!CheckSingleParametric("param_6", param_6, 6, Params)) return 0;

     // 7) Y = a * log (b * X^Gamma + c) + d

     Params[0] = 2.2;
     Params[1] = 0.9;
     Params[2] = 0.9;
     Params[3] = 0.02;
     Params[4] = 0.1;

     if (!CheckSingleParametric("param_7", param_7, 7, Params)) return 0;

     // 8) Y = a * b ^ (c*X+d) + e          
     
     Params[0] = 0.9;
     Params[1] = 0.9;
     Params[2] = 1.02;
     Params[3] = 0.1;
     Params[4] = 0.2;

     if (!CheckSingleParametric("param_8", param_8, 8, Params)) return 0;

     // 108: S-Shaped: (1 - (1-x)^1/g)^1/g

     Params[0] = 1.9;
     if (!CheckSingleParametric("sigmoidal", sigmoidal, 108, Params)) return 0;

     // All OK

     return 1;
}

// LUT checks ------------------------------------------------------------------------------

static
cmsInt32Number CheckLUTcreation(void)
{
    cmsPipeline* lut;
    cmsPipeline* lut2;
    cmsInt32Number n1, n2;
    
    lut = cmsPipelineAlloc(DbgThread(), 1, 1);
    n1 = cmsPipelineStageCount(lut);
    lut2 = cmsPipelineDup(lut);
    n2 = cmsPipelineStageCount(lut2);

    cmsPipelineFree(lut);
    cmsPipelineFree(lut2);

    return (n1 == 0) && (n2 == 0);
}

// Create a MPE for a identity matrix
static
void AddIdentityMatrix(cmsPipeline* lut)
{
    const cmsFloat64Number Identity[] = { 1, 0, 0,
                          0, 1, 0, 
                          0, 0, 1, 
                          0, 0, 0 };

    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocMatrix(DbgThread(), 3, 3, Identity, NULL));
}

// Create a MPE for identity cmsFloat32Number CLUT
static
void AddIdentityCLUTfloat(cmsPipeline* lut)
{
    const cmsFloat32Number  Table[] = { 

        0,    0,    0,    
        0,    0,    1.0,

        0,    1.0,    0, 
        0,    1.0,    1.0,

        1.0,    0,    0,    
        1.0,    0,    1.0,

        1.0,    1.0,    0, 
        1.0,    1.0,    1.0
    };

    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocCLutFloat(DbgThread(), 2, 3, 3, Table));
}

// Create a MPE for identity cmsFloat32Number CLUT
static
void AddIdentityCLUT16(cmsPipeline* lut)
{
    const cmsUInt16Number Table[] = { 

        0,    0,    0,    
        0,    0,    0xffff,

        0,    0xffff,    0, 
        0,    0xffff,    0xffff,

        0xffff,    0,    0,    
        0xffff,    0,    0xffff,

        0xffff,    0xffff,    0, 
        0xffff,    0xffff,    0xffff
    };
   

    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocCLut16bit(DbgThread(), 2, 3, 3, Table));
}


// Create a 3 fn identity curves

static 
void Add3GammaCurves(cmsPipeline* lut, cmsFloat64Number Curve)
{
    cmsToneCurve* id = cmsBuildGamma(DbgThread(), Curve);
    cmsToneCurve* id3[3];

    id3[0] = id;
    id3[1] = id;
    id3[2] = id;

    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocToneCurves(DbgThread(), 3, id3));
    
    cmsFreeToneCurve(id);
}


static
cmsInt32Number CheckFloatLUT(cmsPipeline* lut)
{
    cmsInt32Number n1, i, j;
    cmsFloat32Number Inf[3], Outf[3];

    n1=0;

    for (j=0; j < 65535; j++) {

        cmsInt32Number af[3];

        Inf[0] = Inf[1] = Inf[2] = (cmsFloat32Number) j / 65535.0F;
        cmsPipelineEvalFloat(Inf, Outf, lut);

        af[0] = (cmsInt32Number) floor(Outf[0]*65535.0 + 0.5);
        af[1] = (cmsInt32Number) floor(Outf[1]*65535.0 + 0.5);
        af[2] = (cmsInt32Number) floor(Outf[2]*65535.0 + 0.5);

        for (i=0; i < 3; i++) {

            if (af[i] != j) {
                n1++;
            }
        }

    }

    return (n1 == 0);
}


static
cmsInt32Number Check16LUT(cmsPipeline* lut)
{
    cmsInt32Number n2, i, j;
    cmsUInt16Number Inw[3], Outw[3];

    n2=0;
    
    for (j=0; j < 65535; j++) {

        cmsInt32Number aw[3];

        Inw[0] = Inw[1] = Inw[2] = (cmsUInt16Number) j;
        cmsPipelineEval16(Inw, Outw, lut);
        aw[0] = Outw[0];
        aw[1] = Outw[1];
        aw[2] = Outw[2];

        for (i=0; i < 3; i++) {

        if (aw[i] != j) {
            n2++;
        }
        }

    }

    return (n2 == 0);
}


// Check any LUT that is linear
static
cmsInt32Number CheckStagesLUT(cmsPipeline* lut, cmsInt32Number ExpectedStages)
{
    
    cmsInt32Number nInpChans, nOutpChans, nStages;

    nInpChans  = cmsPipelineInputChannels(lut);
    nOutpChans = cmsPipelineOutputChannels(lut);
    nStages    = cmsPipelineStageCount(lut);
    
    return (nInpChans == 3) && (nOutpChans == 3) && (nStages == ExpectedStages);
}


static
cmsInt32Number CheckFullLUT(cmsPipeline* lut, cmsInt32Number ExpectedStages)
{
    cmsInt32Number rc = CheckStagesLUT(lut, ExpectedStages) && Check16LUT(lut) && CheckFloatLUT(lut);

    cmsPipelineFree(lut);
    return rc;
}


static
cmsInt32Number Check1StageLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    return CheckFullLUT(lut, 1);
}



static
cmsInt32Number Check2StageLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUTfloat(lut);

    return CheckFullLUT(lut, 2);
}

static
cmsInt32Number Check2Stage16LUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUT16(lut);

    return CheckFullLUT(lut, 2);
}



static
cmsInt32Number Check3StageLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUTfloat(lut);
    Add3GammaCurves(lut, 1.0);

    return CheckFullLUT(lut, 3);
}

static
cmsInt32Number Check3Stage16LUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUT16(lut);
    Add3GammaCurves(lut, 1.0);

    return CheckFullLUT(lut, 3);
}



static
cmsInt32Number Check4StageLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUTfloat(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityMatrix(lut);

    return CheckFullLUT(lut, 4);
}

static
cmsInt32Number Check4Stage16LUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUT16(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityMatrix(lut);

    return CheckFullLUT(lut, 4);
}

static
cmsInt32Number Check5StageLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUTfloat(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityMatrix(lut);
    Add3GammaCurves(lut, 1.0);
    
    return CheckFullLUT(lut, 5);
}


static
cmsInt32Number Check5Stage16LUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    AddIdentityCLUT16(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityMatrix(lut);
    Add3GammaCurves(lut, 1.0);
    
    return CheckFullLUT(lut, 5);
}

static
cmsInt32Number Check6StageLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityCLUTfloat(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityMatrix(lut);
    Add3GammaCurves(lut, 1.0);
    
    return CheckFullLUT(lut, 6);
}

static
cmsInt32Number Check6Stage16LUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);

    AddIdentityMatrix(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityCLUT16(lut);
    Add3GammaCurves(lut, 1.0);
    AddIdentityMatrix(lut);
    Add3GammaCurves(lut, 1.0);
    
    return CheckFullLUT(lut, 6);
}


static
cmsInt32Number CheckLab2LabLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);
    cmsInt32Number rc;

    cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocLab2XYZ(DbgThread()));
    cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocXYZ2Lab(DbgThread()));

    rc = CheckFloatLUT(lut) && CheckStagesLUT(lut, 2);

    cmsPipelineFree(lut);

    return rc;
}


static
cmsInt32Number CheckXYZ2XYZLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);
    cmsInt32Number rc;

    cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocXYZ2Lab(DbgThread()));
    cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocLab2XYZ(DbgThread()));

    rc = CheckFloatLUT(lut) && CheckStagesLUT(lut, 2);

    cmsPipelineFree(lut);

    return rc;
}



static
cmsInt32Number CheckLab2LabMatLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);
    cmsInt32Number rc;

    cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocLab2XYZ(DbgThread()));
    AddIdentityMatrix(lut);
    cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocXYZ2Lab(DbgThread()));

    rc = CheckFloatLUT(lut) && CheckStagesLUT(lut, 3);

    cmsPipelineFree(lut);

    return rc;
}

static
cmsInt32Number CheckNamedColorLUT(void)
{
    cmsPipeline* lut = cmsPipelineAlloc(DbgThread(), 3, 3);
    cmsNAMEDCOLORLIST* nc;
    cmsInt32Number i,j, rc = 1, n2;
    cmsUInt16Number PCS[3];
    cmsUInt16Number Colorant[cmsMAXCHANNELS];
    char Name[255];
    cmsUInt16Number Inw[3], Outw[3];


    nc = cmsAllocNamedColorList(DbgThread(), 256, 3, "pre", "post");
    if (nc == NULL) return 0;

    for (i=0; i < 256; i++) {

        PCS[0] = PCS[1] = PCS[2] = (cmsUInt16Number) i;
        Colorant[0] = Colorant[1] = Colorant[2] = Colorant[3] = (cmsUInt16Number) i;

        sprintf(Name, "#%d", i);
        if (!cmsAppendNamedColor(nc, Name, PCS, Colorant)) { rc = 0; break; }
    }

    cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocNamedColor(nc));

    cmsFreeNamedColorList(nc);
    if (rc == 0) return 0;

    n2=0;

    for (j=0; j < 256; j++) {

        Inw[0] = (cmsUInt16Number) j;

        cmsPipelineEval16(Inw, Outw, lut);
        for (i=0; i < 3; i++) {

            if (Outw[i] != j) {
                n2++;
            }
        }

    }

    cmsPipelineFree(lut);
    return (n2 == 0);
}



// --------------------------------------------------------------------------------------------

// A lightweight test of multilocalized unicode structures. 

static
cmsInt32Number CheckMLU(void)
{
    cmsMLU* mlu, *mlu2, *mlu3;
    char Buffer[256], Buffer2[256];
    cmsInt32Number rc = 1;
    cmsInt32Number i;
    cmsHPROFILE h= NULL;

    // Allocate a MLU structure, no preferred size
    mlu = cmsMLUalloc(DbgThread(), 0);    

    // Add some localizations
    cmsMLUsetWide(mlu, "en", "US", L"Hello, world");
    cmsMLUsetWide(mlu, "es", "ES", L"Hola, mundo");
    cmsMLUsetWide(mlu, "fr", "FR", L"Bonjour, le monde");
    cmsMLUsetWide(mlu, "ca", "CA", L"Hola, mon");


    // Check the returned string for each language

    cmsMLUgetASCII(mlu, "en", "US", Buffer, 256);
    if (strcmp(Buffer, "Hello, world") != 0) rc = 0;


    cmsMLUgetASCII(mlu, "es", "ES", Buffer, 256);
    if (strcmp(Buffer, "Hola, mundo") != 0) rc = 0;

  
    cmsMLUgetASCII(mlu, "fr", "FR", Buffer, 256);
    if (strcmp(Buffer, "Bonjour, le monde") != 0) rc = 0;

   
    cmsMLUgetASCII(mlu, "ca", "CA", Buffer, 256);
    if (strcmp(Buffer, "Hola, mon") != 0) rc = 0;

    if (rc == 0)
        Fail("Unexpected string '%s'", Buffer);

    // So far, so good.
    cmsMLUfree(mlu);

    // Now for performance, allocate an empty struct
    mlu = cmsMLUalloc(DbgThread(), 0);    

    // Fill it with several thousands of different lenguages
    for (i=0; i < 4096; i++) {

        char Lang[3];
        
        Lang[0] = (char) (i % 255);
        Lang[1] = (char) (i / 255);
        Lang[2] = 0;

        sprintf(Buffer, "String #%i", i);
        cmsMLUsetASCII(mlu, Lang, Lang, Buffer);
    }

    // Duplicate it
    mlu2 = cmsMLUdup(mlu);

    // Get rid of original
    cmsMLUfree(mlu);

    // Check all is still in place
    for (i=0; i < 4096; i++) {

        char Lang[3];
        
        Lang[0] = (char)(i % 255);
        Lang[1] = (char)(i / 255);
        Lang[2] = 0;

        cmsMLUgetASCII(mlu2, Lang, Lang, Buffer2, 256);
        sprintf(Buffer, "String #%i", i);

        if (strcmp(Buffer, Buffer2) != 0) { rc = 0; break; }
    }

    if (rc == 0) 
        Fail("Unexpected string '%s'", Buffer2);

    // Check profile IO

    h = cmsOpenProfileFromFileTHR(DbgThread(), "mlucheck.icc", "w");
        
    cmsSetProfileVersion(h, 4.2);

    cmsWriteTag(h, cmsSigProfileDescriptionTag, mlu2);
    cmsCloseProfile(h);
    cmsMLUfree(mlu2);

    
    h = cmsOpenProfileFromFileTHR(DbgThread(), "mlucheck.icc", "r");

    mlu3 = cmsReadTag(h, cmsSigProfileDescriptionTag);
    if (mlu3 == NULL) { Fail("Profile didn't get the MLU\n"); rc = 0; goto Error; }

    // Check all is still in place
    for (i=0; i < 4096; i++) {

        char Lang[3];
        
        Lang[0] = (char) (i % 255);
        Lang[1] = (char) (i / 255);
        Lang[2] = 0;

        cmsMLUgetASCII(mlu3, Lang, Lang, Buffer2, 256);
        sprintf(Buffer, "String #%i", i);

        if (strcmp(Buffer, Buffer2) != 0) { rc = 0; break; }
    }

    if (rc == 0) Fail("Unexpected string '%s'", Buffer2);

Error:

    if (h != NULL) cmsCloseProfile(h);
    remove("mlucheck.icc");

    return rc;
}


// A lightweight test of named color structures.
static
cmsInt32Number CheckNamedColorList(void)
{
    cmsNAMEDCOLORLIST* nc = NULL, *nc2;
    cmsInt32Number i, j, rc=1;
    char Name[255];
    cmsUInt16Number PCS[3];
    cmsUInt16Number Colorant[cmsMAXCHANNELS];
    char CheckName[255];
    cmsUInt16Number CheckPCS[3];
    cmsUInt16Number CheckColorant[cmsMAXCHANNELS];
    cmsHPROFILE h;

    nc = cmsAllocNamedColorList(DbgThread(), 0, 4, "prefix", "suffix");
    if (nc == NULL) return 0;

    for (i=0; i < 4096; i++) {


        PCS[0] = PCS[1] = PCS[2] = (cmsUInt16Number) i;
        Colorant[0] = Colorant[1] = Colorant[2] = Colorant[3] = (cmsUInt16Number) (4096 - i);

        sprintf(Name, "#%d", i);
        if (!cmsAppendNamedColor(nc, Name, PCS, Colorant)) { rc = 0; break; }
    }

    for (i=0; i < 4096; i++) {

        CheckPCS[0] = CheckPCS[1] = CheckPCS[2] = (cmsUInt16Number) i;
        CheckColorant[0] = CheckColorant[1] = CheckColorant[2] = CheckColorant[3] = (cmsUInt16Number) (4096 - i);

        sprintf(CheckName, "#%d", i);
        if (!cmsNamedColorInfo(nc, i, Name, NULL, NULL, PCS, Colorant)) { rc = 0; goto Error; }


        for (j=0; j < 3; j++) {
            if (CheckPCS[j] != PCS[j]) { rc = 0; Fail("Invalid PCS"); goto Error; }
        }

        for (j=0; j < 4; j++) {
            if (CheckColorant[j] != Colorant[j]) { rc = 0; Fail("Invalid Colorant"); goto Error; };
        }

        if (strcmp(Name, CheckName) != 0) {rc = 0; Fail("Invalid Name"); goto Error; };
    }

    h = cmsOpenProfileFromFileTHR(DbgThread(), "namedcol.icc", "w");
    if (h == NULL) return 0;
    if (!cmsWriteTag(h, cmsSigNamedColor2Tag, nc)) return 0;
    cmsCloseProfile(h);
    cmsFreeNamedColorList(nc);
    nc = NULL;

    h = cmsOpenProfileFromFileTHR(DbgThread(), "namedcol.icc", "r");
    nc2 = cmsReadTag(h, cmsSigNamedColor2Tag);

    if (cmsNamedColorCount(nc2) != 4096) { rc = 0; Fail("Invalid count"); goto Error; }

    i = cmsNamedColorIndex(nc2, "#123");
    if (i != 123) { rc = 0; Fail("Invalid index"); goto Error; }


    for (i=0; i < 4096; i++) {

        CheckPCS[0] = CheckPCS[1] = CheckPCS[2] = (cmsUInt16Number) i;
        CheckColorant[0] = CheckColorant[1] = CheckColorant[2] = CheckColorant[3] = (cmsUInt16Number) (4096 - i);

        sprintf(CheckName, "#%d", i);
        if (!cmsNamedColorInfo(nc2, i, Name, NULL, NULL, PCS, Colorant)) { rc = 0; goto Error; }


        for (j=0; j < 3; j++) {
            if (CheckPCS[j] != PCS[j]) { rc = 0; Fail("Invalid PCS"); goto Error; }
        }

        for (j=0; j < 4; j++) {
            if (CheckColorant[j] != Colorant[j]) { rc = 0; Fail("Invalid Colorant"); goto Error; };
        }

        if (strcmp(Name, CheckName) != 0) {rc = 0; Fail("Invalid Name"); goto Error; };
    }

    cmsCloseProfile(h);
    remove("namedcol.icc");

Error:
    if (nc != NULL) cmsFreeNamedColorList(nc);
    return rc;
}



// ----------------------------------------------------------------------------------------------------------

// Formatters

static cmsBool  FormatterFailed;

static
void CheckSingleFormatter16(cmsUInt32Number Type, const char* Text)
{
    cmsUInt16Number Values[cmsMAXCHANNELS];
    cmsUInt8Number Buffer[1024];
    cmsFormatter f, b;
    cmsInt32Number i, j, nChannels, bytes;
    _cmsTRANSFORM info;

    // Already failed?
    if (FormatterFailed) return;

    memset(&info, 0, sizeof(info));
    info.OutputFormat = info.InputFormat = Type;

    // Go forth and back
    f = _cmsGetFormatter(Type,  cmsFormatterInput, 0);
    b = _cmsGetFormatter(Type,  cmsFormatterOutput, 0);

    if (f.Fmt16 == NULL || b.Fmt16 == NULL) {
        Fail("no formatter for %s", Text);
        FormatterFailed = TRUE;

        // Useful for debug
        f = _cmsGetFormatter(Type,  cmsFormatterInput, 0);
        b = _cmsGetFormatter(Type,  cmsFormatterOutput, 0);
        return;
    }

    nChannels = T_CHANNELS(Type);
    bytes     = T_BYTES(Type);

    for (j=0; j < 5; j++) {

        for (i=0; i < nChannels; i++) { 
            Values[i] = (cmsUInt16Number) (i+j);
            // For 8-bit
            if (bytes == 1) 
                Values[i] <<= 8;
        }

    b.Fmt16(&info, Values, Buffer, 1);
    memset(Values, 0, sizeof(Values));
    f.Fmt16(&info, Values, Buffer, 1);

    for (i=0; i < nChannels; i++) {
        if (bytes == 1) 
            Values[i] >>= 8;

        if (Values[i] != i+j) {

            Fail("%s failed", Text);        
            FormatterFailed = TRUE;

            // Useful for debug
            for (i=0; i < nChannels; i++) { 
                Values[i] = (cmsUInt16Number) (i+j);
                // For 8-bit
                if (bytes == 1) 
                    Values[i] <<= 8;
            }

            b.Fmt16(&info, Values, Buffer, 1);
            f.Fmt16(&info, Values, Buffer, 1);
            return;
        }
    }
    }
}

#define C(a) CheckSingleFormatter16(a, #a)


// Check all formatters
static
cmsInt32Number CheckFormatters16(void)
{
    FormatterFailed = FALSE;

   C( TYPE_GRAY_8            );
   C( TYPE_GRAY_8_REV        );
   C( TYPE_GRAY_16           );
   C( TYPE_GRAY_16_REV       );
   C( TYPE_GRAY_16_SE        );
   C( TYPE_GRAYA_8           );
   C( TYPE_GRAYA_16          );
   C( TYPE_GRAYA_16_SE       );
   C( TYPE_GRAYA_8_PLANAR    );
   C( TYPE_GRAYA_16_PLANAR   );
   C( TYPE_RGB_8             );
   C( TYPE_RGB_8_PLANAR      );
   C( TYPE_BGR_8             );
   C( TYPE_BGR_8_PLANAR      );
   C( TYPE_RGB_16            );
   C( TYPE_RGB_16_PLANAR     );
   C( TYPE_RGB_16_SE         );
   C( TYPE_BGR_16            );
   C( TYPE_BGR_16_PLANAR     );
   C( TYPE_BGR_16_SE         );
   C( TYPE_RGBA_8            );
   C( TYPE_RGBA_8_PLANAR     );
   C( TYPE_RGBA_16           );
   C( TYPE_RGBA_16_PLANAR    );
   C( TYPE_RGBA_16_SE        );
   C( TYPE_ARGB_8            );
   C( TYPE_ARGB_16           );
   C( TYPE_ABGR_8            );
   C( TYPE_ABGR_16           );
   C( TYPE_ABGR_16_PLANAR    );
   C( TYPE_ABGR_16_SE        );
   C( TYPE_BGRA_8            );
   C( TYPE_BGRA_16           );
   C( TYPE_BGRA_16_SE        );
   C( TYPE_CMY_8             );
   C( TYPE_CMY_8_PLANAR      );
   C( TYPE_CMY_16            );
   C( TYPE_CMY_16_PLANAR     );
   C( TYPE_CMY_16_SE         );
   C( TYPE_CMYK_8            );
   C( TYPE_CMYKA_8           );
   C( TYPE_CMYK_8_REV        );
   C( TYPE_YUVK_8            );
   C( TYPE_CMYK_8_PLANAR     );
   C( TYPE_CMYK_16           );
   C( TYPE_CMYK_16_REV       );
   C( TYPE_YUVK_16           );
   C( TYPE_CMYK_16_PLANAR    );
   C( TYPE_CMYK_16_SE        );
   C( TYPE_KYMC_8            );
   C( TYPE_KYMC_16           );
   C( TYPE_KYMC_16_SE        );
   C( TYPE_KCMY_8            );
   C( TYPE_KCMY_8_REV        );
   C( TYPE_KCMY_16           );
   C( TYPE_KCMY_16_REV       );
   C( TYPE_KCMY_16_SE        );
   C( TYPE_CMYK5_8           );
   C( TYPE_CMYK5_16          );
   C( TYPE_CMYK5_16_SE       );
   C( TYPE_KYMC5_8           );
   C( TYPE_KYMC5_16          );
   C( TYPE_KYMC5_16_SE       );
   C( TYPE_CMYK6_8          );
   C( TYPE_CMYK6_8_PLANAR   );
   C( TYPE_CMYK6_16         );
   C( TYPE_CMYK6_16_PLANAR  );
   C( TYPE_CMYK6_16_SE      );
   C( TYPE_CMYK7_8           );
   C( TYPE_CMYK7_16          );
   C( TYPE_CMYK7_16_SE       );
   C( TYPE_KYMC7_8           );
   C( TYPE_KYMC7_16          );
   C( TYPE_KYMC7_16_SE       );
   C( TYPE_CMYK8_8           );
   C( TYPE_CMYK8_16          );
   C( TYPE_CMYK8_16_SE       );
   C( TYPE_KYMC8_8           );
   C( TYPE_KYMC8_16          );
   C( TYPE_KYMC8_16_SE       );
   C( TYPE_CMYK9_8           );
   C( TYPE_CMYK9_16          );
   C( TYPE_CMYK9_16_SE       );
   C( TYPE_KYMC9_8           );
   C( TYPE_KYMC9_16          );
   C( TYPE_KYMC9_16_SE       );
   C( TYPE_CMYK10_8          );
   C( TYPE_CMYK10_16         );
   C( TYPE_CMYK10_16_SE      );
   C( TYPE_KYMC10_8          );
   C( TYPE_KYMC10_16         );
   C( TYPE_KYMC10_16_SE      );
   C( TYPE_CMYK11_8          );
   C( TYPE_CMYK11_16         );
   C( TYPE_CMYK11_16_SE      );
   C( TYPE_KYMC11_8          );
   C( TYPE_KYMC11_16         );
   C( TYPE_KYMC11_16_SE      );
   C( TYPE_CMYK12_8          );
   C( TYPE_CMYK12_16         );
   C( TYPE_CMYK12_16_SE      );
   C( TYPE_KYMC12_8          );
   C( TYPE_KYMC12_16         );
   C( TYPE_KYMC12_16_SE      );
   C( TYPE_XYZ_16            );
   C( TYPE_Lab_8             );
   C( TYPE_ALab_8            );
   C( TYPE_Lab_16            );
   C( TYPE_Yxy_16            );
   C( TYPE_YCbCr_8           );
   C( TYPE_YCbCr_8_PLANAR    );
   C( TYPE_YCbCr_16          );
   C( TYPE_YCbCr_16_PLANAR   );
   C( TYPE_YCbCr_16_SE       );
   C( TYPE_YUV_8             );
   C( TYPE_YUV_8_PLANAR      );
   C( TYPE_YUV_16            );
   C( TYPE_YUV_16_PLANAR     );
   C( TYPE_YUV_16_SE         );
   C( TYPE_HLS_8             );
   C( TYPE_HLS_8_PLANAR      );
   C( TYPE_HLS_16            );
   C( TYPE_HLS_16_PLANAR     );
   C( TYPE_HLS_16_SE         );
   C( TYPE_HSV_8             );
   C( TYPE_HSV_8_PLANAR      );
   C( TYPE_HSV_16            );
   C( TYPE_HSV_16_PLANAR     );
   C( TYPE_HSV_16_SE         );
        
   C( TYPE_XYZ_FLT  );
   C( TYPE_Lab_FLT  );
   C( TYPE_GRAY_FLT );
   C( TYPE_RGB_FLT  );
   C( TYPE_CMYK_FLT );
   C( TYPE_XYZA_FLT );
   C( TYPE_LabA_FLT );
   C( TYPE_RGBA_FLT );

   C( TYPE_XYZ_DBL  );
   C( TYPE_Lab_DBL  );
   C( TYPE_GRAY_DBL );
   C( TYPE_RGB_DBL  );
   C( TYPE_CMYK_DBL );

   C( TYPE_LabV2_8  );
   C( TYPE_ALabV2_8 );
   C( TYPE_LabV2_16 );

   return FormatterFailed == 0 ? 1 : 0;
}
#undef C

static
void CheckSingleFormatterFloat(cmsUInt32Number Type, const char* Text)
{
    cmsFloat32Number Values[cmsMAXCHANNELS];
    cmsUInt8Number Buffer[1024];
    cmsFormatter f, b;
    cmsInt32Number i, j, nChannels;
    _cmsTRANSFORM info;

    // Already failed?
    if (FormatterFailed) return;

    memset(&info, 0, sizeof(info));
    info.OutputFormat = info.InputFormat = Type;

    // Go forth and back
    f = _cmsGetFormatter(Type,  cmsFormatterInput, CMS_PACK_FLAGS_FLOAT);
    b = _cmsGetFormatter(Type,  cmsFormatterOutput, CMS_PACK_FLAGS_FLOAT);

    if (f.FmtFloat == NULL || b.FmtFloat == NULL) {
        Fail("no formatter for %s", Text);
        FormatterFailed = TRUE;

        // Useful for debug
        f = _cmsGetFormatter(Type,  cmsFormatterInput, CMS_PACK_FLAGS_FLOAT);
        b = _cmsGetFormatter(Type,  cmsFormatterOutput, CMS_PACK_FLAGS_FLOAT);
        return;
    }

    nChannels = T_CHANNELS(Type);

    for (j=0; j < 5; j++) {

        for (i=0; i < nChannels; i++) { 
            Values[i] = (cmsFloat32Number) (i+j);
        }

        b.FmtFloat(&info, Values, Buffer, 1);
        memset(Values, 0, sizeof(Values));
        f.FmtFloat(&info, Values, Buffer, 1);

        for (i=0; i < nChannels; i++) {

            cmsFloat64Number delta = fabs(Values[i] - ( i+j));

            if (delta > 0.000000001) {

                Fail("%s failed", Text);        
                FormatterFailed = TRUE;

                // Useful for debug
                for (i=0; i < nChannels; i++) { 
                    Values[i] = (cmsFloat32Number) (i+j);
                }

                b.FmtFloat(&info, Values, Buffer, 1);
                f.FmtFloat(&info, Values, Buffer, 1);
                return;
            }
        }
    }
}

#define C(a) CheckSingleFormatterFloat(a, #a)

static
cmsInt32Number CheckFormattersFloat(void)
{
    FormatterFailed = FALSE;

    C( TYPE_XYZ_FLT  ); 
    C( TYPE_Lab_FLT  );
    C( TYPE_GRAY_FLT );
    C( TYPE_RGB_FLT  );
    C( TYPE_CMYK_FLT );

    // User
    C( TYPE_XYZA_FLT );
    C( TYPE_LabA_FLT );
    C( TYPE_RGBA_FLT );

    C( TYPE_XYZ_DBL  );
    C( TYPE_Lab_DBL  );
    C( TYPE_GRAY_DBL );
    C( TYPE_RGB_DBL  );
    C( TYPE_CMYK_DBL );

    return FormatterFailed == 0 ? 1 : 0;
}
#undef C


static
cmsInt32Number CheckOneRGB(cmsHTRANSFORM xform, cmsUInt32Number R, cmsUInt32Number G, cmsUInt32Number B, cmsUInt32Number Ro, cmsUInt32Number Go, cmsUInt32Number Bo)
{
    cmsUInt16Number RGB[3];
    cmsUInt16Number Out[3];

    RGB[0] = R;
    RGB[1] = G;
    RGB[2] = B;

    cmsDoTransform(xform, RGB, Out, 1);

    return IsGoodWord("R", Ro , Out[0]) &&
           IsGoodWord("G", Go , Out[1]) &&
           IsGoodWord("B", Bo , Out[2]);
}

// Check known values going from sRGB to XYZ
static
cmsInt32Number CheckOneRGB_double(cmsHTRANSFORM xform, cmsFloat64Number R, cmsFloat64Number G, cmsFloat64Number B, cmsFloat64Number Ro, cmsFloat64Number Go, cmsFloat64Number Bo)
{
    cmsFloat64Number RGB[3];
    cmsFloat64Number Out[3];

    RGB[0] = R;
    RGB[1] = G;
    RGB[2] = B;

    cmsDoTransform(xform, RGB, Out, 1);

    return IsGoodVal("R", Ro , Out[0], 0.01) &&
           IsGoodVal("G", Go , Out[1], 0.01) &&
           IsGoodVal("B", Bo , Out[2], 0.01);
}


static
cmsInt32Number CheckChangeBufferFormat(void)
{
    cmsHPROFILE hsRGB = cmsCreate_sRGBProfile();
    cmsHTRANSFORM xform;


    xform = cmsCreateTransform(hsRGB, TYPE_RGB_16, hsRGB, TYPE_RGB_16, INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(hsRGB);
    if (xform == NULL) return 0;

    
    if (!CheckOneRGB(xform, 0, 0, 0, 0, 0, 0)) return 0;
    if (!CheckOneRGB(xform, 120, 0, 0, 120, 0, 0)) return 0;
    if (!CheckOneRGB(xform, 0, 222, 255, 0, 222, 255)) return 0;


    if (!cmsChangeBuffersFormat(xform, TYPE_BGR_16, TYPE_RGB_16)) return 0;

    if (!CheckOneRGB(xform, 0, 0, 123, 123, 0, 0)) return 0;
    if (!CheckOneRGB(xform, 154, 234, 0, 0, 234, 154)) return 0;

    if (!cmsChangeBuffersFormat(xform, TYPE_RGB_DBL, TYPE_RGB_DBL)) return 0;

    if (!CheckOneRGB_double(xform, 0.20, 0, 0, 0.20, 0, 0)) return 0;
    if (!CheckOneRGB_double(xform, 0, 0.9, 1, 0, 0.9, 1)) return 0;

    cmsDeleteTransform(xform);
    
return 1;
}


// Write tag testbed ----------------------------------------------------------------------------------------

static
cmsInt32Number CheckXYZ(cmsInt32Number Pass, cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsCIEXYZ XYZ, *Pt;

    
    switch (Pass) {

        case 1:

            XYZ.X = 1.0; XYZ.Y = 1.1; XYZ.Z = 1.2;
            return cmsWriteTag(hProfile, tag, &XYZ);
    
        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;
            return IsGoodFixed15_16("X", 1.0, Pt ->X) &&
                   IsGoodFixed15_16("Y", 1.1, Pt->Y) &&
                   IsGoodFixed15_16("Z", 1.2, Pt -> Z);

        default:
            return 0;
    }
}


static
cmsInt32Number CheckGamma(cmsInt32Number Pass, cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsToneCurve *g, *Pt;
    cmsInt32Number rc;

    switch (Pass) {

        case 1:

            g = cmsBuildGamma(DbgThread(), 1.0);
            rc = cmsWriteTag(hProfile, tag, g);
            cmsFreeToneCurve(g);
            return rc;
    
        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;
            return cmsIsToneCurveLinear(Pt);
            
        default:
            return 0;
    }
}

static
cmsInt32Number CheckText(cmsInt32Number Pass, cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsMLU *m, *Pt;
    cmsInt32Number rc;
    char Buffer[256];

    
    switch (Pass) {

        case 1:
            m = cmsMLUalloc(DbgThread(), 0);
            cmsMLUsetASCII(m, cmsNoLanguage, cmsNoCountry, "Test test");
            rc = cmsWriteTag(hProfile, tag, m);
            cmsMLUfree(m);
            return rc;
    
        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;
            cmsMLUgetASCII(Pt, cmsNoLanguage, cmsNoCountry, Buffer, 256);
            return strcmp(Buffer, "Test test") == 0;
            
        default:
            return 0;
    }
}

static
cmsInt32Number CheckData(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsICCData *Pt;
    cmsICCData d = { 1, 0, { '?' }};
    cmsInt32Number rc;

    
    switch (Pass) {

        case 1:
            rc = cmsWriteTag(hProfile, tag, &d);
            return rc;
    
        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;
            return (Pt ->data[0] == '?') && (Pt ->flag == 0) && (Pt ->len == 1);

        default:
            return 0;
    }
}


static
cmsInt32Number CheckSignature(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsTagSignature *Pt, Holder;
    
    switch (Pass) {

        case 1:
            Holder = cmsSigPerceptualReferenceMediumGamut;
            return cmsWriteTag(hProfile, tag, &Holder);
    
        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;
            return *Pt == cmsSigPerceptualReferenceMediumGamut;

        default:
            return 0;
    }
}


static
cmsInt32Number CheckDateTime(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    struct tm *Pt, Holder;

    switch (Pass) {

        case 1:
    
            Holder.tm_hour = 1;
            Holder.tm_min = 2;
            Holder.tm_sec = 3;
            Holder.tm_mday = 4;
            Holder.tm_mon = 5;
            Holder.tm_year = 2009 - 1900;
            return cmsWriteTag(hProfile, tag, &Holder);
    
        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            return (Pt ->tm_hour == 1 && 
                Pt ->tm_min == 2 && 
                Pt ->tm_sec == 3 &&
                Pt ->tm_mday == 4 &&
                Pt ->tm_mon == 5 &&
                Pt ->tm_year == 2009 - 1900);

        default:
            return 0;
    }

}


static
cmsInt32Number CheckNamedColor(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag, cmsInt32Number max_check, cmsBool  colorant_check)
{
    cmsNAMEDCOLORLIST* nc;
    cmsInt32Number i, j, rc;
    char Name[255];
    cmsUInt16Number PCS[3];
    cmsUInt16Number Colorant[cmsMAXCHANNELS];
    char CheckName[255];
    cmsUInt16Number CheckPCS[3];
    cmsUInt16Number CheckColorant[cmsMAXCHANNELS];

    switch (Pass) {

    case 1:

        nc = cmsAllocNamedColorList(DbgThread(), 0, 4, "prefix", "suffix");
        if (nc == NULL) return 0;

        for (i=0; i < max_check; i++) {

            PCS[0] = PCS[1] = PCS[2] = (cmsUInt16Number) i;
            Colorant[0] = Colorant[1] = Colorant[2] = Colorant[3] = (cmsUInt16Number) (max_check - i);

            sprintf(Name, "#%d", i);
            if (!cmsAppendNamedColor(nc, Name, PCS, Colorant)) { Fail("Couldn't append named color"); return 0; }
        }

        rc = cmsWriteTag(hProfile, tag, nc);
        cmsFreeNamedColorList(nc);
        return rc;

    case 2:

        nc = cmsReadTag(hProfile, tag);
        if (nc == NULL) return 0;

        for (i=0; i < max_check; i++) {

            CheckPCS[0] = CheckPCS[1] = CheckPCS[2] = (cmsUInt16Number) i;
            CheckColorant[0] = CheckColorant[1] = CheckColorant[2] = CheckColorant[3] = (cmsUInt16Number) (max_check - i);

            sprintf(CheckName, "#%d", i);
            if (!cmsNamedColorInfo(nc, i, Name, NULL, NULL, PCS, Colorant)) { Fail("Invalid string"); return 0; }


            for (j=0; j < 3; j++) {
                if (CheckPCS[j] != PCS[j]) {  Fail("Invalid PCS"); return 0; }
            }

            // This is only used on named color list
            if (colorant_check) {

            for (j=0; j < 4; j++) {
                if (CheckColorant[j] != Colorant[j]) { Fail("Invalid Colorant"); return 0; };
            }
            }

            if (strcmp(Name, CheckName) != 0) { Fail("Invalid Name");  return 0; };
        }
        return 1;


    default: return 0;
    }
}


static
cmsInt32Number CheckLUT(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsPipeline* Lut, *Pt;
    cmsInt32Number rc;


    switch (Pass) {

        case 1:

            Lut = cmsPipelineAlloc(DbgThread(), 3, 3);
            if (Lut == NULL) return 0;

            // Create an identity LUT
            cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(DbgThread(), 3));
            cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocIdentityCLut(DbgThread(), 3));
            cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocIdentityCurves(DbgThread(), 3));
        
            rc =  cmsWriteTag(hProfile, tag, Lut);
            cmsPipelineFree(Lut);
            return rc;

        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            // Transform values, check for identity
            return Check16LUT(Pt);

        default:
            return 0;
    }
}

static
cmsInt32Number CheckCHAD(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsFloat64Number *Pt;
    cmsFloat64Number CHAD[] = { 0, .1, .2, .3, .4, .5, .6, .7, .8 };
    cmsInt32Number i;

    switch (Pass) {

        case 1:         
            return cmsWriteTag(hProfile, tag, CHAD);
            

        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            for (i=0; i < 9; i++) {
                if (!IsGoodFixed15_16("CHAD", Pt[i], CHAD[i])) return 0;
            }

            return 1;

        default:
            return 0;
    }
}

static
cmsInt32Number CheckChromaticity(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsCIExyYTRIPLE *Pt, c = { {0, .1, 1 }, { .3, .4, 1 }, { .6, .7, 1 }};

    switch (Pass) {

        case 1:         
            return cmsWriteTag(hProfile, tag, &c);
            

        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            if (!IsGoodFixed15_16("xyY", Pt ->Red.x, c.Red.x)) return 0;
            if (!IsGoodFixed15_16("xyY", Pt ->Red.y, c.Red.y)) return 0;
            if (!IsGoodFixed15_16("xyY", Pt ->Green.x, c.Green.x)) return 0;
            if (!IsGoodFixed15_16("xyY", Pt ->Green.y, c.Green.y)) return 0;
            if (!IsGoodFixed15_16("xyY", Pt ->Blue.x, c.Blue.x)) return 0;
            if (!IsGoodFixed15_16("xyY", Pt ->Blue.y, c.Blue.y)) return 0;
            return 1;

        default:
            return 0;
    }
}


static
cmsInt32Number CheckColorantOrder(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsUInt8Number *Pt, c[cmsMAXCHANNELS];
    cmsInt32Number i;

    switch (Pass) {

        case 1:         
            for (i=0; i < cmsMAXCHANNELS; i++) c[i] = (cmsUInt8Number) (cmsMAXCHANNELS - i - 1);
            return cmsWriteTag(hProfile, tag, c);
            

        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            for (i=0; i < cmsMAXCHANNELS; i++) {
                if (Pt[i] != ( cmsMAXCHANNELS - i - 1 )) return 0;
            }
            return 1;

        default:
            return 0;
    }
}

static
cmsInt32Number CheckMeasurement(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsICCMeasurementConditions *Pt, m;

    switch (Pass) {

        case 1:         
            m.Backing.X = 0.1;
            m.Backing.Y = 0.2;
            m.Backing.Z = 0.3;
            m.Flare = 1.0;
            m.Geometry = 1;
            m.IlluminantType = cmsILLUMINANT_TYPE_D50;
            m.Observer = 1;
            return cmsWriteTag(hProfile, tag, &m);
            

        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            if (!IsGoodFixed15_16("Backing", Pt ->Backing.X, 0.1)) return 0;
            if (!IsGoodFixed15_16("Backing", Pt ->Backing.Y, 0.2)) return 0;
            if (!IsGoodFixed15_16("Backing", Pt ->Backing.Z, 0.3)) return 0;
            if (!IsGoodFixed15_16("Flare",   Pt ->Flare, 1.0)) return 0;

            if (Pt ->Geometry != 1) return 0;
            if (Pt ->IlluminantType != cmsILLUMINANT_TYPE_D50) return 0;
            if (Pt ->Observer != 1) return 0;
            return 1;

        default:
            return 0;
    }
}


static
cmsInt32Number CheckUcrBg(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsUcrBg *Pt, m;
    cmsInt32Number rc;
    char Buffer[256];

    switch (Pass) {

        case 1:         
            m.Ucr = cmsBuildGamma(DbgThread(), 2.4);
            m.Bg  = cmsBuildGamma(DbgThread(), -2.2);
            m.Desc = cmsMLUalloc(DbgThread(), 1);
            cmsMLUsetASCII(m.Desc,  cmsNoLanguage, cmsNoCountry, "test UCR/BG");         
            rc = cmsWriteTag(hProfile, tag, &m);
            cmsMLUfree(m.Desc);
            cmsFreeToneCurve(m.Bg);
            cmsFreeToneCurve(m.Ucr);
            return rc;
            

        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            cmsMLUgetASCII(Pt ->Desc, cmsNoLanguage, cmsNoCountry, Buffer, 256);
            if (strcmp(Buffer, "test UCR/BG") != 0) return 0;       
            return 1;

        default:
            return 0;
    }
}


static
cmsInt32Number CheckCRDinfo(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsMLU *mlu;
    char Buffer[256];
    cmsInt32Number rc;

    switch (Pass) {

        case 1: 
            mlu = cmsMLUalloc(DbgThread(), 5);

            cmsMLUsetWide(mlu,  "PS", "nm", L"test postscript");
            cmsMLUsetWide(mlu,  "PS", "#0", L"perceptual");
            cmsMLUsetWide(mlu,  "PS", "#1", L"relative_colorimetric");
            cmsMLUsetWide(mlu,  "PS", "#2", L"saturation");
            cmsMLUsetWide(mlu,  "PS", "#3", L"absolute_colorimetric");  
            rc = cmsWriteTag(hProfile, tag, mlu);
            cmsMLUfree(mlu);
            return rc;
            

        case 2:
            mlu = (cmsMLU*) cmsReadTag(hProfile, tag);
            if (mlu == NULL) return 0;


             
             cmsMLUgetASCII(mlu, "PS", "nm", Buffer, 256);
             if (strcmp(Buffer, "test postscript") != 0) return 0;

           
             cmsMLUgetASCII(mlu, "PS", "#0", Buffer, 256);
             if (strcmp(Buffer, "perceptual") != 0) return 0;

            
             cmsMLUgetASCII(mlu, "PS", "#1", Buffer, 256);
             if (strcmp(Buffer, "relative_colorimetric") != 0) return 0;

             
             cmsMLUgetASCII(mlu, "PS", "#2", Buffer, 256);
             if (strcmp(Buffer, "saturation") != 0) return 0;

            
             cmsMLUgetASCII(mlu, "PS", "#3", Buffer, 256);
             if (strcmp(Buffer, "absolute_colorimetric") != 0) return 0;
             return 1;

        default:
            return 0;
    }
}


static
cmsToneCurve *CreateSegmentedCurve(void)
{
    cmsCurveSegment Seg[3];
    cmsFloat32Number Sampled[2] = { 0, 1};

    Seg[0].Type = 6;
    Seg[0].Params[0] = 1;
    Seg[0].Params[1] = 0;
    Seg[0].Params[2] = 0;
    Seg[0].Params[3] = 0;
    Seg[0].x0 = -1E22F;
    Seg[0].x1 = 0;

    Seg[1].Type = 0;
    Seg[1].nGridPoints = 2;
    Seg[1].SampledPoints = Sampled;
    Seg[1].x0 = 0;
    Seg[1].x1 = 1;

    Seg[2].Type = 6;
    Seg[2].Params[0] = 1;
    Seg[2].Params[1] = 0;
    Seg[2].Params[2] = 0;
    Seg[2].Params[3] = 0;               
    Seg[2].x0 = 1;
    Seg[2].x1 = 1E22F;

    return cmsBuildSegmentedToneCurve(DbgThread(), 3, Seg);
}


static
cmsInt32Number CheckMPE(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsPipeline* Lut, *Pt;
    cmsToneCurve* G[3];
    cmsInt32Number rc;

    switch (Pass) {

        case 1: 

            Lut = cmsPipelineAlloc(DbgThread(), 3, 3);
            
            cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocLabV2ToV4(DbgThread()));
            cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocLabV4ToV2(DbgThread()));
            AddIdentityCLUTfloat(Lut);

            G[0] = G[1] = G[2] = CreateSegmentedCurve();
            cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocToneCurves(DbgThread(), 3, G));
            cmsFreeToneCurve(G[0]);
            
            rc = cmsWriteTag(hProfile, tag, Lut);
            cmsPipelineFree(Lut);
            return rc;
            
        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;           
            return CheckFloatLUT(Pt);

        default:
            return 0;
    }
}


static
cmsInt32Number CheckScreening(cmsInt32Number Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
    cmsScreening *Pt, sc;
    cmsInt32Number rc;
    
    switch (Pass) {

        case 1:         
            
            sc.Flag = 0;
            sc.nChannels = 1;
            sc.Channels[0].Frequency = 2.0;
            sc.Channels[0].ScreenAngle = 3.0;
            sc.Channels[0].SpotShape = cmsSPOT_ELLIPSE;

            rc = cmsWriteTag(hProfile, tag, &sc);
            return rc;
            

        case 2:
            Pt = cmsReadTag(hProfile, tag);
            if (Pt == NULL) return 0;

            if (Pt ->nChannels != 1) return 0;
            if (Pt ->Flag      != 0) return 0;
            if (!IsGoodFixed15_16("Freq", Pt ->Channels[0].Frequency, 2.0)) return 0;
            if (!IsGoodFixed15_16("Angle", Pt ->Channels[0].ScreenAngle, 3.0)) return 0;
            if (Pt ->Channels[0].SpotShape != cmsSPOT_ELLIPSE) return 0;
            return 1;

        default:
            return 0;
    }
}


static
cmsBool CheckOneStr(cmsMLU* mlu, cmsInt32Number n)
{
    char Buffer[256], Buffer2[256];

    
    cmsMLUgetASCII(mlu, "en", "US", Buffer, 255);
    sprintf(Buffer2, "Hello, world %d", n);
    if (strcmp(Buffer, Buffer2) != 0) return FALSE;

  
    cmsMLUgetASCII(mlu, "es", "ES", Buffer, 255);
    sprintf(Buffer2, "Hola, mundo %d", n);
    if (strcmp(Buffer, Buffer2) != 0) return FALSE;

    return TRUE;
}


static
void SetOneStr(cmsMLU** mlu, wchar_t* s1, wchar_t* s2)
{
    *mlu = cmsMLUalloc(DbgThread(), 0);
    cmsMLUsetWide(*mlu, "en", "US", s1);
    cmsMLUsetWide(*mlu, "es", "ES", s2);
}


static
cmsInt32Number CheckProfileSequenceTag(cmsInt32Number Pass,  cmsHPROFILE hProfile)
{
    cmsSEQ* s;
    cmsInt32Number i;

    switch (Pass) {

    case 1:

        s = cmsAllocProfileSequenceDescription(DbgThread(), 3);
        if (s == NULL) return 0;

        SetOneStr(&s -> seq[0].Manufacturer, L"Hello, world 0", L"Hola, mundo 0");
        SetOneStr(&s -> seq[0].Model, L"Hello, world 0", L"Hola, mundo 0");
        SetOneStr(&s -> seq[1].Manufacturer, L"Hello, world 1", L"Hola, mundo 1");
        SetOneStr(&s -> seq[1].Model, L"Hello, world 1", L"Hola, mundo 1");
        SetOneStr(&s -> seq[2].Manufacturer, L"Hello, world 2", L"Hola, mundo 2");
        SetOneStr(&s -> seq[2].Model, L"Hello, world 2", L"Hola, mundo 2");


#ifdef CMS_DONT_USE_INT64
        s ->seq[0].attributes[0] = cmsTransparency|cmsMatte;
        s ->seq[0].attributes[1] = 0;
#else
        s ->seq[0].attributes = cmsTransparency|cmsMatte;
#endif

#ifdef CMS_DONT_USE_INT64
        s ->seq[1].attributes[0] = cmsReflective|cmsMatte;
        s ->seq[1].attributes[1] = 0;
#else
        s ->seq[1].attributes = cmsReflective|cmsMatte;
#endif

#ifdef CMS_DONT_USE_INT64
        s ->seq[2].attributes[0] = cmsTransparency|cmsGlossy;
        s ->seq[2].attributes[1] = 0;
#else
        s ->seq[2].attributes = cmsTransparency|cmsGlossy;
#endif

        if (!cmsWriteTag(hProfile, cmsSigProfileSequenceDescTag, s)) return 0;    
        cmsFreeProfileSequenceDescription(s);
        return 1;

    case 2:

        s = cmsReadTag(hProfile, cmsSigProfileSequenceDescTag);
        if (s == NULL) return 0;

        if (s ->n != 3) return 0;

#ifdef CMS_DONT_USE_INT64
        if (s ->seq[0].attributes[0] != (cmsTransparency|cmsMatte)) return 0;
        if (s ->seq[0].attributes[1] != 0) return 0;
#else
        if (s ->seq[0].attributes != (cmsTransparency|cmsMatte)) return 0;
#endif

#ifdef CMS_DONT_USE_INT64
        if (s ->seq[1].attributes[0] != (cmsReflective|cmsMatte)) return 0;
        if (s ->seq[1].attributes[1] != 0) return 0;
#else
        if (s ->seq[1].attributes != (cmsReflective|cmsMatte)) return 0;
#endif

#ifdef CMS_DONT_USE_INT64
        if (s ->seq[2].attributes[0] != (cmsTransparency|cmsGlossy)) return 0;
        if (s ->seq[2].attributes[1] != 0) return 0;
#else
        if (s ->seq[2].attributes != (cmsTransparency|cmsGlossy)) return 0;
#endif

        // Check MLU
        for (i=0; i < 3; i++) {

            if (!CheckOneStr(s -> seq[i].Manufacturer, i)) return 0;
            if (!CheckOneStr(s -> seq[i].Model, i)) return 0;
        }
        return 1;

    default:
        return 0;
    }
}


static
cmsInt32Number CheckProfileSequenceIDTag(cmsInt32Number Pass,  cmsHPROFILE hProfile)
{
    cmsSEQ* s;
    cmsInt32Number i;

    switch (Pass) {

    case 1:

        s = cmsAllocProfileSequenceDescription(DbgThread(), 3);
        if (s == NULL) return 0;

        memcpy(s ->seq[0].ProfileID.ID8, "0123456789ABCDEF", 16);
        memcpy(s ->seq[1].ProfileID.ID8, "1111111111111111", 16);
        memcpy(s ->seq[2].ProfileID.ID8, "2222222222222222", 16);


        SetOneStr(&s -> seq[0].Description, L"Hello, world 0", L"Hola, mundo 0");
        SetOneStr(&s -> seq[1].Description, L"Hello, world 1", L"Hola, mundo 1");
        SetOneStr(&s -> seq[2].Description, L"Hello, world 2", L"Hola, mundo 2");

        if (!cmsWriteTag(hProfile, cmsSigProfileSequenceIdTag, s)) return 0;    
        cmsFreeProfileSequenceDescription(s);
        return 1;

    case 2:

        s = cmsReadTag(hProfile, cmsSigProfileSequenceIdTag);
        if (s == NULL) return 0;

        if (s ->n != 3) return 0;

        if (memcmp(s ->seq[0].ProfileID.ID8, "0123456789ABCDEF", 16) != 0) return 0;
        if (memcmp(s ->seq[1].ProfileID.ID8, "1111111111111111", 16) != 0) return 0;
        if (memcmp(s ->seq[2].ProfileID.ID8, "2222222222222222", 16) != 0) return 0;

        for (i=0; i < 3; i++) {

            if (!CheckOneStr(s -> seq[i].Description, i)) return 0;
        }

        return 1;

    default:
        return 0;
    }
}


static
cmsInt32Number CheckICCViewingConditions(cmsInt32Number Pass,  cmsHPROFILE hProfile)
{
    cmsICCViewingConditions* v;
    cmsICCViewingConditions  s;

    switch (Pass) {

        case 1:         
            s.IlluminantType = 1;
            s.IlluminantXYZ.X = 0.1;
            s.IlluminantXYZ.Y = 0.2;
            s.IlluminantXYZ.Z = 0.3;
            s.SurroundXYZ.X = 0.4;
            s.SurroundXYZ.Y = 0.5;
            s.SurroundXYZ.Z = 0.6;

            if (!cmsWriteTag(hProfile, cmsSigViewingConditionsTag, &s)) return 0;    
            return 1;

        case 2:
            v = cmsReadTag(hProfile, cmsSigViewingConditionsTag);
            if (v == NULL) return 0;

            if (v ->IlluminantType != 1) return 0;
            if (!IsGoodVal("IlluminantXYZ.X", v ->IlluminantXYZ.X, 0.1, 0.001)) return 0;
            if (!IsGoodVal("IlluminantXYZ.Y", v ->IlluminantXYZ.Y, 0.2, 0.001)) return 0;
            if (!IsGoodVal("IlluminantXYZ.Z", v ->IlluminantXYZ.Z, 0.3, 0.001)) return 0;

            if (!IsGoodVal("SurroundXYZ.X", v ->SurroundXYZ.X, 0.4, 0.001)) return 0;
            if (!IsGoodVal("SurroundXYZ.Y", v ->SurroundXYZ.Y, 0.5, 0.001)) return 0;
            if (!IsGoodVal("SurroundXYZ.Z", v ->SurroundXYZ.Z, 0.6, 0.001)) return 0;

            return 1;

        default:
            return 0;
    }

}


static
cmsInt32Number CheckVCGT(cmsInt32Number Pass,  cmsHPROFILE hProfile)
{
    cmsToneCurve* Curves[3];
    cmsToneCurve** PtrCurve;
    
     switch (Pass) {

        case 1:     
            Curves[0] = cmsBuildGamma(DbgThread(), 1.1);
            Curves[1] = cmsBuildGamma(DbgThread(), 2.2);
            Curves[2] = cmsBuildGamma(DbgThread(), 3.4);

            if (!cmsWriteTag(hProfile, cmsSigVcgtTag, Curves)) return 0;    

            cmsFreeToneCurveTriple(Curves);
            return 1;


        case 2:

             PtrCurve = cmsReadTag(hProfile, cmsSigVcgtTag);
             if (PtrCurve == NULL) return 0;
             if (!IsGoodVal("VCGT R", cmsEstimateGamma(PtrCurve[0], 0.01), 1.1, 0.001)) return 0;
             if (!IsGoodVal("VCGT G", cmsEstimateGamma(PtrCurve[1], 0.01), 2.2, 0.001)) return 0;
             if (!IsGoodVal("VCGT B", cmsEstimateGamma(PtrCurve[2], 0.01), 3.4, 0.001)) return 0;
             return 1;

        default:;
    }

    return 0;
}


static
cmsInt32Number CheckRAWtags(cmsInt32Number Pass,  cmsHPROFILE hProfile)
{
    char Buffer[7];

    switch (Pass) {

        case 1:         
            return cmsWriteRawTag(hProfile, 0x31323334, "data123", 7);

        case 2:
            if (!cmsReadRawTag(hProfile, 0x31323334, Buffer, 7)) return 0;

            if (strncmp(Buffer, "data123", 7) != 0) return 0;
            return 1;

        default:
            return 0;
    }
}


// This is a very big test that checks every single tag
static
cmsInt32Number CheckProfileCreation(void)
{
    cmsHPROFILE h;
    cmsInt32Number Pass;

    h = cmsCreateProfilePlaceholder(DbgThread());
    if (h == NULL) return 0;

    cmsSetProfileVersion(h, 4.2);
    if (cmsGetTagCount(h) != 0) { Fail("Empty profile with nonzero number of tags"); return 0; }
    if (cmsIsTag(h, cmsSigAToB0Tag)) { Fail("Found a tag in an empty profile"); return 0; }

    cmsSetColorSpace(h, cmsSigRgbData);
    if (cmsGetColorSpace(h) !=  cmsSigRgbData) { Fail("Unable to set colorspace"); return 0; }

    cmsSetPCS(h, cmsSigLabData);
    if (cmsGetPCS(h) !=  cmsSigLabData) { Fail("Unable to set colorspace"); return 0; }

    cmsSetDeviceClass(h, cmsSigDisplayClass);
    if (cmsGetDeviceClass(h) != cmsSigDisplayClass) { Fail("Unable to set deviceclass"); return 0; }

    cmsSetHeaderRenderingIntent(h, INTENT_SATURATION);
    if (cmsGetHeaderRenderingIntent(h) != INTENT_SATURATION) { Fail("Unable to set rendering intent"); return 0; }

    for (Pass = 1; Pass <= 2; Pass++) {

        SubTest("Tags holding XYZ");

        if (!CheckXYZ(Pass, h, cmsSigBlueColorantTag)) return 0;
        if (!CheckXYZ(Pass, h, cmsSigGreenColorantTag)) return 0;
        if (!CheckXYZ(Pass, h, cmsSigRedColorantTag)) return 0;
        if (!CheckXYZ(Pass, h, cmsSigMediaBlackPointTag)) return 0;
        if (!CheckXYZ(Pass, h, cmsSigMediaWhitePointTag)) return 0;
        if (!CheckXYZ(Pass, h, cmsSigLuminanceTag)) return 0;

        SubTest("Tags holding curves");
        
        if (!CheckGamma(Pass, h, cmsSigBlueTRCTag)) return 0;
        if (!CheckGamma(Pass, h, cmsSigGrayTRCTag)) return 0;
        if (!CheckGamma(Pass, h, cmsSigGreenTRCTag)) return 0;
        if (!CheckGamma(Pass, h, cmsSigRedTRCTag)) return 0;
        
        SubTest("Tags holding text");

        if (!CheckText(Pass, h, cmsSigCharTargetTag)) return 0;
        if (!CheckText(Pass, h, cmsSigCopyrightTag)) return 0;
        if (!CheckText(Pass, h, cmsSigProfileDescriptionTag)) return 0;
        if (!CheckText(Pass, h, cmsSigDeviceMfgDescTag)) return 0;
        if (!CheckText(Pass, h, cmsSigDeviceModelDescTag)) return 0;
        if (!CheckText(Pass, h, cmsSigViewingCondDescTag)) return 0;
        if (!CheckText(Pass, h, cmsSigScreeningDescTag)) return 0;

        SubTest("Tags holding cmsICCData");

        if (!CheckData(Pass, h, cmsSigPs2CRD0Tag)) return 0;
        if (!CheckData(Pass, h, cmsSigPs2CRD1Tag)) return 0;
        if (!CheckData(Pass, h, cmsSigPs2CRD2Tag)) return 0;
        if (!CheckData(Pass, h, cmsSigPs2CRD3Tag)) return 0;
        if (!CheckData(Pass, h, cmsSigPs2CSATag)) return 0;
        if (!CheckData(Pass, h, cmsSigPs2RenderingIntentTag)) return 0;

        SubTest("Tags holding signatures");

        if (!CheckSignature(Pass, h, cmsSigColorimetricIntentImageStateTag)) return 0;
        if (!CheckSignature(Pass, h, cmsSigPerceptualRenderingIntentGamutTag)) return 0;
        if (!CheckSignature(Pass, h, cmsSigSaturationRenderingIntentGamutTag)) return 0;
        if (!CheckSignature(Pass, h, cmsSigTechnologyTag)) return 0;

        SubTest("Tags holding date_time");

        if (!CheckDateTime(Pass, h, cmsSigCalibrationDateTimeTag)) return 0;
        if (!CheckDateTime(Pass, h, cmsSigDateTimeTag)) return 0;

        SubTest("Tags holding named color lists");

        if (!CheckNamedColor(Pass, h, cmsSigColorantTableTag, 15, FALSE)) return 0;
        if (!CheckNamedColor(Pass, h, cmsSigColorantTableOutTag, 15, FALSE)) return 0;
        if (!CheckNamedColor(Pass, h, cmsSigNamedColor2Tag, 4096, TRUE)) return 0;

        SubTest("Tags holding LUTs");

        if (!CheckLUT(Pass, h, cmsSigAToB0Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigAToB1Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigAToB2Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigBToA0Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigBToA1Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigBToA2Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigPreview0Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigPreview1Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigPreview2Tag)) return 0;
        if (!CheckLUT(Pass, h, cmsSigGamutTag)) return 0;
        
        SubTest("Tags holding CHAD");
        if (!CheckCHAD(Pass, h, cmsSigChromaticAdaptationTag)) return 0;

        SubTest("Tags holding Chromaticity");
        if (!CheckChromaticity(Pass, h, cmsSigChromaticityTag)) return 0;

        SubTest("Tags holding colorant order");
        if (!CheckColorantOrder(Pass, h, cmsSigColorantOrderTag)) return 0;

        SubTest("Tags holding measurement");
        if (!CheckMeasurement(Pass, h, cmsSigMeasurementTag)) return 0;

        SubTest("Tags holding CRD info");
        if (!CheckCRDinfo(Pass, h, cmsSigCrdInfoTag)) return 0;

        SubTest("Tags holding UCR/BG");
        if (!CheckUcrBg(Pass, h, cmsSigUcrBgTag)) return 0;

        SubTest("Tags holding MPE");
        if (!CheckMPE(Pass, h, cmsSigDToB0Tag)) return 0;
        if (!CheckMPE(Pass, h, cmsSigDToB1Tag)) return 0;
        if (!CheckMPE(Pass, h, cmsSigDToB2Tag)) return 0;
        if (!CheckMPE(Pass, h, cmsSigDToB3Tag)) return 0;
        if (!CheckMPE(Pass, h, cmsSigBToD0Tag)) return 0;
        if (!CheckMPE(Pass, h, cmsSigBToD1Tag)) return 0;
        if (!CheckMPE(Pass, h, cmsSigBToD2Tag)) return 0;
        if (!CheckMPE(Pass, h, cmsSigBToD3Tag)) return 0;
        
        SubTest("Tags using screening");
        if (!CheckScreening(Pass, h, cmsSigScreeningTag)) return 0;

        SubTest("Tags holding profile sequence description");
        if (!CheckProfileSequenceTag(Pass, h)) return 0;
        if (!CheckProfileSequenceIDTag(Pass, h)) return 0;

        SubTest("Tags holding ICC viewing conditions");
        if (!CheckICCViewingConditions(Pass, h)) return 0;


        SubTest("VCGT tags");
        if (!CheckVCGT(Pass, h)) return 0;

        SubTest("RAW tags");
        if (!CheckRAWtags(Pass, h)) return 0;


        if (Pass == 1) {
            cmsSaveProfileToFile(h, "alltags.icc");
            cmsCloseProfile(h);
            h = cmsOpenProfileFromFileTHR(DbgThread(), "alltags.icc", "r");
        }

    }

    /*    
    Not implemented (by design):

    cmsSigDataTag                           = 0x64617461,  // 'data'  -- Unused   
    cmsSigDeviceSettingsTag                 = 0x64657673,  // 'devs'  -- Unused
    cmsSigNamedColorTag                     = 0x6E636f6C,  // 'ncol'  -- Don't use this one, deprecated by ICC
    cmsSigOutputResponseTag                 = 0x72657370,  // 'resp'  -- Possible patent on this 
    */
    
    cmsCloseProfile(h);
    remove("alltags.icc");
    return 1;
}


// Error reporting  -------------------------------------------------------------------------------------------------------


static
void ErrorReportingFunction(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *Text)
{
    TrappedError = TRUE;
    SimultaneousErrors++;
    strncpy(ReasonToFailBuffer, Text, TEXT_ERROR_BUFFER_SIZE-1);    
}


static
cmsInt32Number CheckBadProfiles(void)
{
    cmsHPROFILE h;

    h = cmsOpenProfileFromFileTHR(DbgThread(), "IDoNotExist.icc", "r");
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }

    h = cmsOpenProfileFromFileTHR(DbgThread(), "IAmIllFormed*.icc", "r");
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }

    // No profile name given
    h = cmsOpenProfileFromFileTHR(DbgThread(), "", "r");
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }
    
    h = cmsOpenProfileFromFileTHR(DbgThread(), "..", "r");
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }
    
    h = cmsOpenProfileFromFileTHR(DbgThread(), "IHaveBadAccessMode.icc", "@");
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }

    h = cmsOpenProfileFromFileTHR(DbgThread(), "bad.icc", "r");
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }

     h = cmsOpenProfileFromFileTHR(DbgThread(), "toosmall.icc", "r");
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }

    h = cmsOpenProfileFromMemTHR(DbgThread(), NULL, 3);
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }

    h = cmsOpenProfileFromMemTHR(DbgThread(), "123", 3);
    if (h != NULL) {
        cmsCloseProfile(h);
        return 0;
    }

    if (SimultaneousErrors != 9) return 0;      
    
    return 1;
}


static
cmsInt32Number CheckErrReportingOnBadProfiles(void)
{
    cmsInt32Number rc;

    cmsSetLogErrorHandler(ErrorReportingFunction);
    rc = CheckBadProfiles();
    cmsSetLogErrorHandler(FatalErrorQuit);

    // Reset the error state
    TrappedError = FALSE;
    return rc;
}


static
cmsInt32Number CheckBadTransforms(void)
{
    cmsHPROFILE h1 = cmsCreate_sRGBProfile();
    cmsHTRANSFORM x1;

    x1 = cmsCreateTransform(NULL, 0, NULL, 0, 0, 0);
    if (x1 != NULL) {
        cmsDeleteTransform(x1);
        return 0;
    }

    

    x1 = cmsCreateTransform(h1, TYPE_RGB_8, h1, TYPE_RGB_8, 12345, 0);
    if (x1 != NULL) {
        cmsDeleteTransform(x1);
        return 0;
    }

    x1 = cmsCreateTransform(h1, TYPE_CMYK_8, h1, TYPE_RGB_8, 0, 0);
    if (x1 != NULL) {
        cmsDeleteTransform(x1);
        return 0;
    }

    x1 = cmsCreateTransform(h1, TYPE_RGB_8, h1, TYPE_CMYK_8, 1, 0);
    if (x1 != NULL) {
        cmsDeleteTransform(x1);
        return 0;
    }

    // sRGB does its output as XYZ!
    x1 = cmsCreateTransform(h1, TYPE_RGB_8, NULL, TYPE_Lab_8, 1, 0);
    if (x1 != NULL) {
        cmsDeleteTransform(x1);
        return 0;
    }

    cmsCloseProfile(h1);


    {

    cmsHPROFILE h1 = cmsOpenProfileFromFile("USWebCoatedSWOP.icc", "r");
    cmsHPROFILE h2 = cmsCreate_sRGBProfile();

    x1 = cmsCreateTransform(h1, TYPE_BGR_8, h2, TYPE_BGR_8, INTENT_PERCEPTUAL, 0);

    cmsCloseProfile(h1); cmsCloseProfile(h2);
    if (x1 != NULL) {
        cmsDeleteTransform(x1);
        return 0;
    }
    }

    return 1;

}

static
cmsInt32Number CheckErrReportingOnBadTransforms(void)
{
    cmsInt32Number rc;

    cmsSetLogErrorHandler(ErrorReportingFunction);
    rc = CheckBadTransforms();
    cmsSetLogErrorHandler(FatalErrorQuit);

    // Reset the error state
    TrappedError = FALSE;
    return rc;
}




// ---------------------------------------------------------------------------------------------------------

// Check a linear xform
static
cmsInt32Number Check8linearXFORM(cmsHTRANSFORM xform, cmsInt32Number nChan)
{
    cmsInt32Number n2, i, j;
    cmsUInt8Number Inw[cmsMAXCHANNELS], Outw[cmsMAXCHANNELS];

    n2=0;
    
    for (j=0; j < 0xFF; j++) {
        
        memset(Inw, j, sizeof(Inw));
        cmsDoTransform(xform, Inw, Outw, 1);
        
        for (i=0; i < nChan; i++) {

           cmsInt32Number dif = abs(Outw[i] - j);
           if (dif > n2) n2 = dif;

        }
    }

   // We allow 2 contone of difference on 8 bits 
    if (n2 > 2) {

        Fail("Differences too big (%x)", n2);
        return 0;
    }
    
    return 1;
}

static
cmsInt32Number Compare8bitXFORM(cmsHTRANSFORM xform1, cmsHTRANSFORM xform2, cmsInt32Number nChan)
{
    cmsInt32Number n2, i, j;
    cmsUInt8Number Inw[cmsMAXCHANNELS], Outw1[cmsMAXCHANNELS], Outw2[cmsMAXCHANNELS];;

    n2=0;
    
    for (j=0; j < 0xFF; j++) {
        
        memset(Inw, j, sizeof(Inw));
        cmsDoTransform(xform1, Inw, Outw1, 1);
        cmsDoTransform(xform2, Inw, Outw2, 1);

        for (i=0; i < nChan; i++) {

           cmsInt32Number dif = abs(Outw2[i] - Outw1[i]);
           if (dif > n2) n2 = dif;

        }
    }

   // We allow 2 contone of difference on 8 bits 
    if (n2 > 2) {

        Fail("Differences too big (%x)", n2);
        return 0;
    }

    
    return 1;
}


// Check a linear xform
static
cmsInt32Number Check16linearXFORM(cmsHTRANSFORM xform, cmsInt32Number nChan)
{
    cmsInt32Number n2, i, j;
    cmsUInt16Number Inw[cmsMAXCHANNELS], Outw[cmsMAXCHANNELS];

    n2=0;    
    for (j=0; j < 0xFFFF; j++) {

        for (i=0; i < nChan; i++) Inw[i] = (cmsUInt16Number) j;

        cmsDoTransform(xform, Inw, Outw, 1);
        
        for (i=0; i < nChan; i++) {

           cmsInt32Number dif = abs(Outw[i] - j);
           if (dif > n2) n2 = dif;

        }
    

   // We allow 2 contone of difference on 16 bits
    if (n2 > 0x200) {

        Fail("Differences too big (%x)", n2);
        return 0;
    }
    }
    
    return 1;
}

static
cmsInt32Number Compare16bitXFORM(cmsHTRANSFORM xform1, cmsHTRANSFORM xform2, cmsInt32Number nChan)
{
    cmsInt32Number n2, i, j;
    cmsUInt16Number Inw[cmsMAXCHANNELS], Outw1[cmsMAXCHANNELS], Outw2[cmsMAXCHANNELS];;

    n2=0;
    
    for (j=0; j < 0xFFFF; j++) {
        
        for (i=0; i < nChan; i++) Inw[i] = (cmsUInt16Number) j;

        cmsDoTransform(xform1, Inw, Outw1, 1);
        cmsDoTransform(xform2, Inw, Outw2, 1);

        for (i=0; i < nChan; i++) {

           cmsInt32Number dif = abs(Outw2[i] - Outw1[i]);
           if (dif > n2) n2 = dif;

        }
    }

   // We allow 2 contone of difference on 16 bits 
    if (n2 > 0x200) {

        Fail("Differences too big (%x)", n2);
        return 0;
    }

    
    return 1;
}


// Check a linear xform
static
cmsInt32Number CheckFloatlinearXFORM(cmsHTRANSFORM xform, cmsInt32Number nChan)
{
    cmsInt32Number n2, i, j;
    cmsFloat32Number In[cmsMAXCHANNELS], Out[cmsMAXCHANNELS];

    n2=0;
    
    for (j=0; j < 0xFFFF; j++) {

        for (i=0; i < nChan; i++) In[i] = (cmsFloat32Number) (j / 65535.0);;

        cmsDoTransform(xform, In, Out, 1);
        
        for (i=0; i < nChan; i++) {

           // We allow no difference in floating point
            if (!IsGoodFixed15_16("linear xform cmsFloat32Number", Out[i], (cmsFloat32Number) (j / 65535.0))) 
                return 0;
        }        
    }
    
    return 1;
}


// Check a linear xform
static
cmsInt32Number CompareFloatXFORM(cmsHTRANSFORM xform1, cmsHTRANSFORM xform2, cmsInt32Number nChan)
{
    cmsInt32Number n2, i, j;
    cmsFloat32Number In[cmsMAXCHANNELS], Out1[cmsMAXCHANNELS], Out2[cmsMAXCHANNELS];

    n2=0;
    
    for (j=0; j < 0xFFFF; j++) {

        for (i=0; i < nChan; i++) In[i] = (cmsFloat32Number) (j / 65535.0);;

        cmsDoTransform(xform1, In, Out1, 1);
        cmsDoTransform(xform2, In, Out2, 1);

        for (i=0; i < nChan; i++) {

           // We allow no difference in floating point
            if (!IsGoodFixed15_16("linear xform cmsFloat32Number", Out1[i], Out2[i])) 
                return 0;
        }
        
    }
    
    return 1;
}


// Curves only transforms ----------------------------------------------------------------------------------------

static
cmsInt32Number CheckCurvesOnlyTransforms(void)
{

    cmsHTRANSFORM xform1, xform2;
    cmsHPROFILE h1, h2, h3;
    cmsToneCurve* c1, *c2, *c3;
    cmsInt32Number rc = 1;


    c1 = cmsBuildGamma(DbgThread(), 2.2);
    c2 = cmsBuildGamma(DbgThread(), 1/2.2);
    c3 = cmsBuildGamma(DbgThread(), 4.84);    

    h1 = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigGrayData, &c1);
    h2 = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigGrayData, &c2);
    h3 = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigGrayData, &c3);

    SubTest("Gray float optimizeable transform");
    xform1 = cmsCreateTransform(h1, TYPE_GRAY_FLT, h2, TYPE_GRAY_FLT, INTENT_PERCEPTUAL, 0);    
    rc &= CheckFloatlinearXFORM(xform1, 1);
    cmsDeleteTransform(xform1);
    if (rc == 0) goto Error;
    
    SubTest("Gray 8 optimizeable transform");
    xform1 = cmsCreateTransform(h1, TYPE_GRAY_8, h2, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);    
    rc &= Check8linearXFORM(xform1, 1);
    cmsDeleteTransform(xform1);
    if (rc == 0) goto Error;
    
    SubTest("Gray 16 optimizeable transform");
    xform1 = cmsCreateTransform(h1, TYPE_GRAY_16, h2, TYPE_GRAY_16, INTENT_PERCEPTUAL, 0);  
    rc &= Check16linearXFORM(xform1, 1);
    cmsDeleteTransform(xform1);
    if (rc == 0) goto Error;

    SubTest("Gray float non-optimizeable transform");
    xform1 = cmsCreateTransform(h1, TYPE_GRAY_FLT, h1, TYPE_GRAY_FLT, INTENT_PERCEPTUAL, 0);    
    xform2 = cmsCreateTransform(h3, TYPE_GRAY_FLT, NULL, TYPE_GRAY_FLT, INTENT_PERCEPTUAL, 0);  

    rc &= CompareFloatXFORM(xform1, xform2, 1);
    cmsDeleteTransform(xform1);
    cmsDeleteTransform(xform2);
    if (rc == 0) goto Error;
    
    SubTest("Gray 8 non-optimizeable transform");
    xform1 = cmsCreateTransform(h1, TYPE_GRAY_8, h1, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);    
    xform2 = cmsCreateTransform(h3, TYPE_GRAY_8, NULL, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);  

    rc &= Compare8bitXFORM(xform1, xform2, 1);
    cmsDeleteTransform(xform1);
    cmsDeleteTransform(xform2);
    if (rc == 0) goto Error;


    SubTest("Gray 16 non-optimizeable transform");
    xform1 = cmsCreateTransform(h1, TYPE_GRAY_16, h1, TYPE_GRAY_16, INTENT_PERCEPTUAL, 0);  
    xform2 = cmsCreateTransform(h3, TYPE_GRAY_16, NULL, TYPE_GRAY_16, INTENT_PERCEPTUAL, 0);    

    rc &= Compare16bitXFORM(xform1, xform2, 1);
    cmsDeleteTransform(xform1);
    cmsDeleteTransform(xform2);
    if (rc == 0) goto Error;

Error:

    cmsCloseProfile(h1); cmsCloseProfile(h2); cmsCloseProfile(h3);
    cmsFreeToneCurve(c1); cmsFreeToneCurve(c2); cmsFreeToneCurve(c3);
    
    return rc;
}
    


// Lab to Lab trivial transforms ----------------------------------------------------------------------------------------

static cmsFloat64Number MaxDE;

static
cmsInt32Number CheckOneLab(cmsHTRANSFORM xform, cmsFloat64Number L, cmsFloat64Number a, cmsFloat64Number b)
{
    cmsCIELab In, Out;
    cmsFloat64Number dE;

    In.L = L; In.a = a; In.b = b;
    cmsDoTransform(xform, &In, &Out, 1);

    dE = cmsDeltaE(&In, &Out);
    
    if (dE > MaxDE) MaxDE = dE;

    if (MaxDE >  0.003) {
        Fail("dE=%f Lab1=(%f, %f, %f)\n\tLab2=(%f %f %f)", MaxDE, In.L, In.a, In.b, Out.L, Out.a, Out.b);
        cmsDoTransform(xform, &In, &Out, 1);
        return 0;
    }

    return 1;
}

// Check several Lab, slicing at non-exact values. Precision should be 16 bits. 50x50x50 checks aprox.
static
cmsInt32Number CheckSeveralLab(cmsHTRANSFORM xform)
{
    cmsInt32Number L, a, b;

    MaxDE = 0;
    for (L=0; L < 65536; L += 1311) {
    
        for (a = 0; a < 65536; a += 1232) {

            for (b = 0; b < 65536; b += 1111) {

                if (!CheckOneLab(xform, (L * 100.0) / 65535.0, 
                                        (a  / 257.0) - 128, (b / 257.0) - 128)) 
                    return 0;
            }

        }

    }
    return 1;
}


static
cmsInt32Number OneTrivialLab(cmsHPROFILE hLab1, cmsHPROFILE hLab2, const char* txt)
{   
    cmsHTRANSFORM xform;
    cmsInt32Number rc;

    SubTest(txt);
    xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_DBL, hLab2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hLab1); cmsCloseProfile(hLab2);

    rc = CheckSeveralLab(xform);
    cmsDeleteTransform(xform);
    return rc;
}


static
cmsInt32Number CheckFloatLabTransforms(void)
{
    return OneTrivialLab(cmsCreateLab4ProfileTHR(DbgThread(), NULL), cmsCreateLab4ProfileTHR(DbgThread(), NULL),  "Lab4/Lab4") &&
           OneTrivialLab(cmsCreateLab2ProfileTHR(DbgThread(), NULL), cmsCreateLab2ProfileTHR(DbgThread(), NULL),  "Lab2/Lab2") &&
           OneTrivialLab(cmsCreateLab4ProfileTHR(DbgThread(), NULL), cmsCreateLab2ProfileTHR(DbgThread(), NULL),  "Lab4/Lab2") &&
           OneTrivialLab(cmsCreateLab2ProfileTHR(DbgThread(), NULL), cmsCreateLab4ProfileTHR(DbgThread(), NULL),  "Lab2/Lab4");
}


static
cmsInt32Number CheckEncodedLabTransforms(void)
{
    cmsHTRANSFORM xform;
    cmsUInt16Number In[3];
    cmsCIELab Lab;
    cmsCIELab White = { 100, 0, 0 };
    cmsHPROFILE hLab1 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
    cmsHPROFILE hLab2 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
  

    xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_16, hLab2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hLab1); cmsCloseProfile(hLab2);

    In[0] = 0xFFFF;
    In[1] = 0x8080;
    In[2] = 0x8080;

    cmsDoTransform(xform, In, &Lab, 1);

    if (cmsDeltaE(&Lab, &White) > 0.0001) return 0;
    cmsDeleteTransform(xform);

    hLab1 = cmsCreateLab2ProfileTHR(DbgThread(), NULL);
    hLab2 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

    xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_LabV2_16, hLab2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hLab1); cmsCloseProfile(hLab2);


    In[0] = 0xFF00;
    In[1] = 0x8000;
    In[2] = 0x8000;

    cmsDoTransform(xform, In, &Lab, 1);

    if (cmsDeltaE(&Lab, &White) > 0.0001) return 0;

    cmsDeleteTransform(xform);

    hLab2 = cmsCreateLab2ProfileTHR(DbgThread(), NULL);
    hLab1 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

    xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_DBL, hLab2, TYPE_LabV2_16, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hLab1); cmsCloseProfile(hLab2);

    Lab.L = 100;
    Lab.a = 0;
    Lab.b = 0;

    cmsDoTransform(xform, &Lab, In, 1);
    if (In[0] != 0xFF00 ||
        In[1] != 0x8000 ||
        In[2] != 0x8000) return 0;

    cmsDeleteTransform(xform);

    hLab1 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
    hLab2 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

    xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_DBL, hLab2, TYPE_Lab_16, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hLab1); cmsCloseProfile(hLab2);

    Lab.L = 100;
    Lab.a = 0;
    Lab.b = 0;

    cmsDoTransform(xform, &Lab, In, 1);

    if (In[0] != 0xFFFF ||
        In[1] != 0x8080 ||
        In[2] != 0x8080) return 0;

    cmsDeleteTransform(xform);
   
    return 1;
}

static
cmsInt32Number CheckStoredIdentities(void)
{
    cmsHPROFILE hLab, hLink, h4, h2;
    cmsHTRANSFORM xform;
    cmsInt32Number rc = 1;

    hLab  = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
    xform = cmsCreateTransformTHR(DbgThread(), hLab, TYPE_Lab_8, hLab, TYPE_Lab_8, 0, 0);
    
    hLink = cmsTransform2DeviceLink(xform, 3.4, 0);
    cmsSaveProfileToFile(hLink, "abstractv2.icc");
    cmsCloseProfile(hLink);

    hLink = cmsTransform2DeviceLink(xform, 4.2, 0);
    cmsSaveProfileToFile(hLink, "abstractv4.icc");
    cmsCloseProfile(hLink);

    cmsDeleteTransform(xform);
    cmsCloseProfile(hLab);

    h4 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv4.icc", "r");

    xform = cmsCreateTransformTHR(DbgThread(), h4, TYPE_Lab_DBL, h4, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);

    SubTest("V4");
    rc &= CheckSeveralLab(xform);
    
    cmsDeleteTransform(xform);
    cmsCloseProfile(h4);
    if (!rc) goto Error;

    
    SubTest("V2");
    h2 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv2.icc", "r");

    xform = cmsCreateTransformTHR(DbgThread(), h2, TYPE_Lab_DBL, h2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
    rc &= CheckSeveralLab(xform);
    cmsDeleteTransform(xform);
    cmsCloseProfile(h2);
    if (!rc) goto Error;


    SubTest("V2 -> V4");
    h2 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv2.icc", "r");
    h4 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv4.icc", "r");

    xform = cmsCreateTransformTHR(DbgThread(), h4, TYPE_Lab_DBL, h2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
    rc &= CheckSeveralLab(xform);
    cmsDeleteTransform(xform);
    cmsCloseProfile(h2);
    cmsCloseProfile(h4);

    SubTest("V4 -> V2");
    h2 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv2.icc", "r");
    h4 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv4.icc", "r");

    xform = cmsCreateTransformTHR(DbgThread(), h2, TYPE_Lab_DBL, h4, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
    rc &= CheckSeveralLab(xform);
    cmsDeleteTransform(xform);
    cmsCloseProfile(h2);
    cmsCloseProfile(h4);

Error:
    remove("abstractv2.icc");
    remove("abstractv4.icc");
    return rc;

}



// Check a simple xform from a matrix profile to itself. Test floating point accuracy.
static
cmsInt32Number CheckMatrixShaperXFORMFloat(void)
{
    cmsHPROFILE hAbove, hSRGB;
    cmsHTRANSFORM xform;
    cmsInt32Number rc1, rc2;

    hAbove = Create_AboveRGB();
    xform = cmsCreateTransformTHR(DbgThread(), hAbove, TYPE_RGB_FLT, hAbove, TYPE_RGB_FLT,  INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hAbove);
    rc1 = CheckFloatlinearXFORM(xform, 3);
    cmsDeleteTransform(xform);

    hSRGB = cmsCreate_sRGBProfileTHR(DbgThread());
    xform = cmsCreateTransformTHR(DbgThread(), hSRGB, TYPE_RGB_FLT, hSRGB, TYPE_RGB_FLT,  INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hSRGB);
    rc2 = CheckFloatlinearXFORM(xform, 3);
    cmsDeleteTransform(xform);


    return rc1 && rc2;  
}

// Check a simple xform from a matrix profile to itself. Test 16 bits accuracy.
static
cmsInt32Number CheckMatrixShaperXFORM16(void)
{   
    cmsHPROFILE hAbove, hSRGB;
    cmsHTRANSFORM xform;
    cmsInt32Number rc1, rc2;

    hAbove = Create_AboveRGB();
    xform = cmsCreateTransformTHR(DbgThread(), hAbove, TYPE_RGB_16, hAbove, TYPE_RGB_16,  INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hAbove);
    rc1 = Check16linearXFORM(xform, 3);
    cmsDeleteTransform(xform);

    hSRGB = cmsCreate_sRGBProfileTHR(DbgThread());
    xform = cmsCreateTransformTHR(DbgThread(), hSRGB, TYPE_RGB_16, hSRGB, TYPE_RGB_16,  INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hSRGB);
    rc2 = Check16linearXFORM(xform, 3);
    cmsDeleteTransform(xform);

    return rc1 && rc2;  

}


// Check a simple xform from a matrix profile to itself. Test 8 bits accuracy.
static
cmsInt32Number CheckMatrixShaperXFORM8(void)
{   
    cmsHPROFILE hAbove, hSRGB;
    cmsHTRANSFORM xform;
    cmsInt32Number rc1, rc2;

    hAbove = Create_AboveRGB();
    xform = cmsCreateTransformTHR(DbgThread(), hAbove, TYPE_RGB_8, hAbove, TYPE_RGB_8,  INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hAbove);
    rc1 = Check8linearXFORM(xform, 3);
    cmsDeleteTransform(xform);

    hSRGB = cmsCreate_sRGBProfileTHR(DbgThread());
    xform = cmsCreateTransformTHR(DbgThread(), hSRGB, TYPE_RGB_8, hSRGB, TYPE_RGB_8,  INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hSRGB);
    rc2 = Check8linearXFORM(xform, 3);
    cmsDeleteTransform(xform);


    return rc1 && rc2;  
}


// TODO: Check LUT based to LUT based transforms for CMYK






// -----------------------------------------------------------------------------------------------------------------


// Check known values going from sRGB to XYZ
static
cmsInt32Number CheckOneRGB_f(cmsHTRANSFORM xform, cmsInt32Number R, cmsInt32Number G, cmsInt32Number B, cmsFloat64Number X, cmsFloat64Number Y, cmsFloat64Number Z, cmsFloat64Number err)
{
    cmsFloat32Number RGB[3];
    cmsFloat64Number Out[3];

    RGB[0] = (cmsFloat32Number) (R / 255.0);
    RGB[1] = (cmsFloat32Number) (G / 255.0);
    RGB[2] = (cmsFloat32Number) (B / 255.0);

    cmsDoTransform(xform, RGB, Out, 1);

    return IsGoodVal("X", X , Out[0], err) &&
           IsGoodVal("Y", Y , Out[1], err) &&
           IsGoodVal("Z", Z , Out[2], err);
}

static
cmsInt32Number Chack_sRGB_Float(void)
{
    cmsHPROFILE hsRGB, hXYZ, hLab;
    cmsHTRANSFORM xform1, xform2;
    cmsInt32Number rc;


    hsRGB = cmsCreate_sRGBProfileTHR(DbgThread());
    hXYZ  = cmsCreateXYZProfileTHR(DbgThread());
    hLab  = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

    xform1 =  cmsCreateTransformTHR(DbgThread(), hsRGB, TYPE_RGB_FLT, hXYZ, TYPE_XYZ_DBL,
                                INTENT_RELATIVE_COLORIMETRIC, 0);

    xform2 =  cmsCreateTransformTHR(DbgThread(), hsRGB, TYPE_RGB_FLT, hLab, TYPE_Lab_DBL,
                                INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hsRGB);
    cmsCloseProfile(hXYZ);
    cmsCloseProfile(hLab);

    MaxErr = 0;

    // Xform 1 goes from 8 bits to XYZ,  
    rc  = CheckOneRGB_f(xform1, 1, 1, 1,        0.0002926, 0.00030352, 0.00025037, 0.0001);
    rc  &= CheckOneRGB_f(xform1, 127, 127, 127, 0.2046329, 0.212230,   0.175069,   0.0001);
    rc  &= CheckOneRGB_f(xform1, 12, 13, 15,    0.0038364, 0.0039928,  0.00385212, 0.0001);
    rc  &= CheckOneRGB_f(xform1, 128, 0, 0,     0.0940846, 0.0480030,  0.00300543, 0.0001);
    rc  &= CheckOneRGB_f(xform1, 190, 25, 210,  0.3203491, 0.1605240,  0.46817115, 0.0001);
    
    // Xform 2 goes from 8 bits to Lab, we allow 0.01 error max
    rc  &= CheckOneRGB_f(xform2, 1, 1, 1,       0.2741748, 0, 0,                  0.01);
    rc  &= CheckOneRGB_f(xform2, 127, 127, 127, 53.192776, 0, 0,                  0.01);
    rc  &= CheckOneRGB_f(xform2, 190, 25, 210,  47.043171, 74.564576, -56.89373,  0.01);
    rc  &= CheckOneRGB_f(xform2, 128, 0, 0,     26.158100, 48.474477, 39.425916,  0.01);

    cmsDeleteTransform(xform1);
    cmsDeleteTransform(xform2);
    return rc;  
}


// ---------------------------------------------------

static
cmsBool GetProfileRGBPrimaries(cmsHPROFILE hProfile,
                                cmsCIEXYZTRIPLE *result,
                                cmsUInt32Number intent)
{
    cmsHPROFILE hXYZ;
    cmsHTRANSFORM hTransform;
    cmsFloat64Number rgb[3][3] = {{1., 0., 0.},
    {0., 1., 0.},
    {0., 0., 1.}};

    hXYZ = cmsCreateXYZProfile();
    if (hXYZ == NULL) return FALSE;

    hTransform = cmsCreateTransform(hProfile, TYPE_RGB_DBL, hXYZ, TYPE_XYZ_DBL,
        intent, cmsFLAGS_NOCACHE | cmsFLAGS_NOOPTIMIZE);
    cmsCloseProfile(hXYZ);
    if (hTransform == NULL) return FALSE;

    cmsDoTransform(hTransform, rgb, result, 3);
    cmsDeleteTransform(hTransform);
    return TRUE;
}


static
int CheckRGBPrimaries(void)
{
    cmsHPROFILE hsRGB;
    cmsCIEXYZTRIPLE tripXYZ;
    cmsCIExyYTRIPLE tripxyY;
    cmsBool result;

    hsRGB = cmsCreate_sRGBProfileTHR(DbgThread());
    if (!hsRGB) return 0;

    result = GetProfileRGBPrimaries(hsRGB, &tripXYZ,
        INTENT_ABSOLUTE_COLORIMETRIC);

    cmsCloseProfile(hsRGB);
    if (!result) return 0;

    cmsXYZ2xyY(&tripxyY.Red, &tripXYZ.Red);
    cmsXYZ2xyY(&tripxyY.Green, &tripXYZ.Green);
    cmsXYZ2xyY(&tripxyY.Blue, &tripXYZ.Blue);

    /* valus were taken from
    http://en.wikipedia.org/wiki/RGB_color_spaces#Specifications */

    if (!IsGoodFixed15_16("xRed", tripxyY.Red.x, 0.64) ||
        !IsGoodFixed15_16("yRed", tripxyY.Red.y, 0.33) ||
        !IsGoodFixed15_16("xGreen", tripxyY.Green.x, 0.30) || 
        !IsGoodFixed15_16("yGreen", tripxyY.Green.y, 0.60) ||
        !IsGoodFixed15_16("xBlue", tripxyY.Blue.x, 0.15) || 
        !IsGoodFixed15_16("yBlue", tripxyY.Blue.y, 0.06)) {
            Fail("One or more primaries are wrong.");
            return FALSE;
    }

    return TRUE;
}


// -----------------------------------------------------------------------------------------------------------------

// This function will check CMYK -> CMYK transforms. It uses FOGRA29 and SWOP ICC profiles

static
cmsInt32Number CheckCMYK(cmsInt32Number Intent, const char *Profile1, const char* Profile2)
{
    cmsHPROFILE hSWOP  = cmsOpenProfileFromFileTHR(DbgThread(), Profile1, "r");
    cmsHPROFILE hFOGRA = cmsOpenProfileFromFileTHR(DbgThread(), Profile2, "r");
    cmsHTRANSFORM xform, swop_lab, fogra_lab;
    cmsFloat32Number CMYK1[4], CMYK2[4];
    cmsCIELab Lab1, Lab2;
    cmsHPROFILE hLab;
    cmsFloat64Number DeltaL, Max;
    cmsInt32Number i;

    hLab = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

    xform = cmsCreateTransformTHR(DbgThread(), hSWOP, TYPE_CMYK_FLT, hFOGRA, TYPE_CMYK_FLT, Intent, 0);

    swop_lab = cmsCreateTransformTHR(DbgThread(), hSWOP,   TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, Intent, 0);
    fogra_lab = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, Intent, 0);

    Max = 0;
    for (i=0; i <= 100; i++) {

        CMYK1[0] = 10;
        CMYK1[1] = 20;
        CMYK1[2] = 30;
        CMYK1[3] = (cmsFloat32Number) i;

        cmsDoTransform(swop_lab, CMYK1, &Lab1, 1);
        cmsDoTransform(xform, CMYK1, CMYK2, 1);
        cmsDoTransform(fogra_lab, CMYK2, &Lab2, 1);

        DeltaL = fabs(Lab1.L - Lab2.L);

        if (DeltaL > Max) Max = DeltaL;
    }


    cmsDeleteTransform(xform);

    if (Max > 3.0) return 0;

    xform = cmsCreateTransformTHR(DbgThread(),  hFOGRA, TYPE_CMYK_FLT, hSWOP, TYPE_CMYK_FLT, Intent, 0);

    Max = 0;

    for (i=0; i <= 100; i++) {
        CMYK1[0] = 10;
        CMYK1[1] = 20;
        CMYK1[2] = 30;
        CMYK1[3] = (cmsFloat32Number) i;

        cmsDoTransform(fogra_lab, CMYK1, &Lab1, 1);
        cmsDoTransform(xform, CMYK1, CMYK2, 1);
        cmsDoTransform(swop_lab, CMYK2, &Lab2, 1);

        DeltaL = fabs(Lab1.L - Lab2.L);

        if (DeltaL > Max) Max = DeltaL;
    }


    cmsCloseProfile(hSWOP);
    cmsCloseProfile(hFOGRA);
    cmsCloseProfile(hLab);

    cmsDeleteTransform(xform);
    cmsDeleteTransform(swop_lab);
    cmsDeleteTransform(fogra_lab);

    return Max < 3.0;
}

static
cmsInt32Number CheckCMYKRoundtrip(void)
{
    return CheckCMYK(INTENT_RELATIVE_COLORIMETRIC, "USWebCoatedSWOP.icc", "USWebCoatedSWOP.icc");
}


static
cmsInt32Number CheckCMYKPerceptual(void)
{
    return CheckCMYK(INTENT_PERCEPTUAL, "USWebCoatedSWOP.icc", "UncoatedFOGRA29.icc");
}



static
cmsInt32Number CheckCMYKRelCol(void)
{
    return CheckCMYK(INTENT_RELATIVE_COLORIMETRIC, "USWebCoatedSWOP.icc", "UncoatedFOGRA29.icc");
}



static
cmsInt32Number CheckKOnlyBlackPreserving(void)
{
    cmsHPROFILE hSWOP  = cmsOpenProfileFromFileTHR(DbgThread(), "USWebCoatedSWOP.icc", "r");
    cmsHPROFILE hFOGRA = cmsOpenProfileFromFileTHR(DbgThread(), "UncoatedFOGRA29.icc", "r");
    cmsHTRANSFORM xform, swop_lab, fogra_lab;
    cmsFloat32Number CMYK1[4], CMYK2[4];
    cmsCIELab Lab1, Lab2;
    cmsHPROFILE hLab;
    cmsFloat64Number DeltaL, Max;
    cmsInt32Number i;

    hLab = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

    xform = cmsCreateTransformTHR(DbgThread(), hSWOP, TYPE_CMYK_FLT, hFOGRA, TYPE_CMYK_FLT, INTENT_PRESERVE_K_ONLY_PERCEPTUAL, 0);

    swop_lab = cmsCreateTransformTHR(DbgThread(), hSWOP,   TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);
    fogra_lab = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);

    Max = 0;

    for (i=0; i <= 100; i++) {
        CMYK1[0] = 0;
        CMYK1[1] = 0;
        CMYK1[2] = 0;
        CMYK1[3] = (cmsFloat32Number) i;

        // SWOP CMYK to Lab1
        cmsDoTransform(swop_lab, CMYK1, &Lab1, 1);

        // SWOP To FOGRA using black preservation
        cmsDoTransform(xform, CMYK1, CMYK2, 1);

        // Obtained FOGRA CMYK to Lab2
        cmsDoTransform(fogra_lab, CMYK2, &Lab2, 1);

        // We care only on L*
        DeltaL = fabs(Lab1.L - Lab2.L);

        if (DeltaL > Max) Max = DeltaL;
    }


    cmsDeleteTransform(xform);

    // dL should be below 3.0
    if (Max > 3.0) return 0;


    // Same, but FOGRA to SWOP
    xform = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hSWOP, TYPE_CMYK_FLT, INTENT_PRESERVE_K_ONLY_PERCEPTUAL, 0);

    Max = 0;

    for (i=0; i <= 100; i++) {
        CMYK1[0] = 0;
        CMYK1[1] = 0;
        CMYK1[2] = 0;
        CMYK1[3] = (cmsFloat32Number) i;

        cmsDoTransform(fogra_lab, CMYK1, &Lab1, 1);
        cmsDoTransform(xform, CMYK1, CMYK2, 1);
        cmsDoTransform(swop_lab, CMYK2, &Lab2, 1);

        DeltaL = fabs(Lab1.L - Lab2.L);

        if (DeltaL > Max) Max = DeltaL;
    }


    cmsCloseProfile(hSWOP);
    cmsCloseProfile(hFOGRA);
    cmsCloseProfile(hLab);

    cmsDeleteTransform(xform);
    cmsDeleteTransform(swop_lab);
    cmsDeleteTransform(fogra_lab);

    return Max < 3.0;
}

static
cmsInt32Number CheckKPlaneBlackPreserving(void)
{
    cmsHPROFILE hSWOP  = cmsOpenProfileFromFileTHR(DbgThread(), "USWebCoatedSWOP.icc", "r");
    cmsHPROFILE hFOGRA = cmsOpenProfileFromFileTHR(DbgThread(), "UncoatedFOGRA29.icc", "r");
    cmsHTRANSFORM xform, swop_lab, fogra_lab;
    cmsFloat32Number CMYK1[4], CMYK2[4];
    cmsCIELab Lab1, Lab2;
    cmsHPROFILE hLab;
    cmsFloat64Number DeltaE, Max;
    cmsInt32Number i;

    hLab = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

    xform = cmsCreateTransformTHR(DbgThread(), hSWOP, TYPE_CMYK_FLT, hFOGRA, TYPE_CMYK_FLT, INTENT_PERCEPTUAL, 0);

    swop_lab = cmsCreateTransformTHR(DbgThread(), hSWOP,  TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);
    fogra_lab = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);

    Max = 0;

    for (i=0; i <= 100; i++) {
        CMYK1[0] = 0;
        CMYK1[1] = 0;
        CMYK1[2] = 0;
        CMYK1[3] = (cmsFloat32Number) i;

        cmsDoTransform(swop_lab, CMYK1, &Lab1, 1);
        cmsDoTransform(xform, CMYK1, CMYK2, 1);
        cmsDoTransform(fogra_lab, CMYK2, &Lab2, 1);

        DeltaE = cmsDeltaE(&Lab1, &Lab2);

        if (DeltaE > Max) Max = DeltaE;
    }


    cmsDeleteTransform(xform);
    
    xform = cmsCreateTransformTHR(DbgThread(),  hFOGRA, TYPE_CMYK_FLT, hSWOP, TYPE_CMYK_FLT, INTENT_PRESERVE_K_PLANE_PERCEPTUAL, 0);

    for (i=0; i <= 100; i++) {
        CMYK1[0] = 30;
        CMYK1[1] = 20;
        CMYK1[2] = 10;
        CMYK1[3] = (cmsFloat32Number) i;

        cmsDoTransform(fogra_lab, CMYK1, &Lab1, 1);
        cmsDoTransform(xform, CMYK1, CMYK2, 1);
        cmsDoTransform(swop_lab, CMYK2, &Lab2, 1);

        DeltaE = cmsDeltaE(&Lab1, &Lab2);

        if (DeltaE > Max) Max = DeltaE;
    }
    
    cmsDeleteTransform(xform);
    
    

    cmsCloseProfile(hSWOP);
    cmsCloseProfile(hFOGRA);
    cmsCloseProfile(hLab);

    
    cmsDeleteTransform(swop_lab);
    cmsDeleteTransform(fogra_lab);

    return Max < 30.0;
}


// ------------------------------------------------------------------------------------------------------


static
cmsInt32Number CheckProofingXFORMFloat(void)
{
    cmsHPROFILE hAbove;
    cmsHTRANSFORM xform;
    cmsInt32Number rc;

    hAbove = Create_AboveRGB();
    xform =  cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_FLT, hAbove, TYPE_RGB_FLT, hAbove, 
                                INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_SOFTPROOFING);
    cmsCloseProfile(hAbove);
    rc = CheckFloatlinearXFORM(xform, 3);
    cmsDeleteTransform(xform);
    return rc;  
}

static
cmsInt32Number CheckProofingXFORM16(void)
{
    cmsHPROFILE hAbove;
    cmsHTRANSFORM xform;
    cmsInt32Number rc;

    hAbove = Create_AboveRGB();
    xform =  cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_16, hAbove, TYPE_RGB_16, hAbove, 
                                INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_SOFTPROOFING|cmsFLAGS_NOCACHE);
    cmsCloseProfile(hAbove);
    rc = Check16linearXFORM(xform, 3);
    cmsDeleteTransform(xform);
    return rc;  
}


static
cmsInt32Number CheckGamutCheck(void)
{
        cmsHPROFILE hSRGB, hAbove;
        cmsHTRANSFORM xform;
        cmsInt32Number rc;
        cmsUInt16Number Alarm[3] = { 0xDEAD, 0xBABE, 0xFACE };

        // Set alarm codes to fancy values so we could check the out of gamut condition
        cmsSetAlarmCodes(Alarm);

        // Create the profiles
        hSRGB  = cmsCreate_sRGBProfileTHR(DbgThread());
        hAbove = Create_AboveRGB();

        if (hSRGB == NULL || hAbove == NULL) return 0;  // Failed

        SubTest("Gamut check on floating point");

        // Create a gamut checker in the same space. No value should be out of gamut
        xform = cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_FLT, hAbove, TYPE_RGB_FLT, hAbove, 
                                INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_GAMUTCHECK);


        if (!CheckFloatlinearXFORM(xform, 3)) {
            cmsCloseProfile(hSRGB);
            cmsCloseProfile(hAbove);
            cmsDeleteTransform(xform);
            Fail("Gamut check on same profile failed");
            return 0;
        }

        cmsDeleteTransform(xform);

        SubTest("Gamut check on 16 bits");

        xform = cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_16, hAbove, TYPE_RGB_16, hAbove, 
                                INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_GAMUTCHECK);

        cmsCloseProfile(hSRGB);
        cmsCloseProfile(hAbove);

        rc = Check16linearXFORM(xform, 3);

        cmsDeleteTransform(xform);

        return rc;
}



// -------------------------------------------------------------------------------------------------------------------

static
cmsInt32Number CheckBlackPoint(void)
{
    cmsHPROFILE hProfile;
    cmsCIEXYZ Black;
    cmsCIELab Lab;

    hProfile  = cmsOpenProfileFromFileTHR(DbgThread(), "sRGB_Color_Space_Profile.icm", "r");  
    cmsDetectBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hProfile);


    hProfile = cmsOpenProfileFromFileTHR(DbgThread(), "USWebCoatedSWOP.icc", "r");
    cmsDetectBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsXYZ2Lab(NULL, &Lab, &Black);
    cmsCloseProfile(hProfile);

    hProfile = cmsOpenProfileFromFileTHR(DbgThread(), "lcms2cmyk.icc", "r");
    cmsDetectBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsXYZ2Lab(NULL, &Lab, &Black);
    cmsCloseProfile(hProfile);

    hProfile = cmsOpenProfileFromFileTHR(DbgThread(), "UncoatedFOGRA29.icc", "r");
    cmsDetectBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsXYZ2Lab(NULL, &Lab, &Black);
    cmsCloseProfile(hProfile);

    hProfile = cmsOpenProfileFromFileTHR(DbgThread(), "USWebCoatedSWOP.icc", "r");
    cmsDetectBlackPoint(&Black, hProfile, INTENT_PERCEPTUAL, 0);
    cmsXYZ2Lab(NULL, &Lab, &Black);
    cmsCloseProfile(hProfile);

    return 1;
}


static
cmsInt32Number CheckOneTAC(cmsFloat64Number InkLimit)
{
    cmsHPROFILE h;
    cmsFloat64Number d;

    h =CreateFakeCMYK(InkLimit, TRUE);
    cmsSaveProfileToFile(h, "lcmstac.icc");
    cmsCloseProfile(h);

    h = cmsOpenProfileFromFile("lcmstac.icc", "r");
    d = cmsDetectTAC(h);
    cmsCloseProfile(h);

    remove("lcmstac.icc");

    if (fabs(d - InkLimit) > 5) return 0;

    return 1;
}


static
cmsInt32Number CheckTAC(void)
{
    if (!CheckOneTAC(180)) return 0;
    if (!CheckOneTAC(220)) return 0;
    if (!CheckOneTAC(286)) return 0;
    if (!CheckOneTAC(310)) return 0;
    if (!CheckOneTAC(330)) return 0;

    return 1;
}

// -------------------------------------------------------------------------------------------------------


#define NPOINTS_IT8 10  // (17*17*17*17)

static
cmsInt32Number CheckCGATS(void)
{
    cmsHANDLE  it8;
    cmsInt32Number i;
    

    it8 = cmsIT8Alloc(DbgThread());
    if (it8 == NULL) return 0;

    cmsIT8SetSheetType(it8, "LCMS/TESTING");
    cmsIT8SetPropertyStr(it8, "ORIGINATOR",   "1 2 3 4");
    cmsIT8SetPropertyUncooked(it8, "DESCRIPTOR",   "1234");
    cmsIT8SetPropertyStr(it8, "MANUFACTURER", "3");
    cmsIT8SetPropertyDbl(it8, "CREATED",      4);
    cmsIT8SetPropertyDbl(it8, "SERIAL",       5);
    cmsIT8SetPropertyHex(it8, "MATERIAL",     0x123);

    cmsIT8SetPropertyDbl(it8, "NUMBER_OF_SETS", NPOINTS_IT8);
    cmsIT8SetPropertyDbl(it8, "NUMBER_OF_FIELDS", 4);

    cmsIT8SetDataFormat(it8, 0, "SAMPLE_ID");
    cmsIT8SetDataFormat(it8, 1, "RGB_R");
    cmsIT8SetDataFormat(it8, 2, "RGB_G");
    cmsIT8SetDataFormat(it8, 3, "RGB_B");

    for (i=0; i < NPOINTS_IT8; i++) {

          char Patch[20];

          sprintf(Patch, "P%d", i);

          cmsIT8SetDataRowCol(it8, i, 0, Patch);
          cmsIT8SetDataRowColDbl(it8, i, 1, i);
          cmsIT8SetDataRowColDbl(it8, i, 2, i);
          cmsIT8SetDataRowColDbl(it8, i, 3, i);
    }

    cmsIT8SaveToFile(it8, "TEST.IT8");
    cmsIT8Free(it8);


    it8 = cmsIT8LoadFromFile(DbgThread(), "TEST.IT8");
    cmsIT8SaveToFile(it8, "TEST.IT8");
    cmsIT8Free(it8);



    it8 = cmsIT8LoadFromFile(DbgThread(), "TEST.IT8");

    if (cmsIT8GetPropertyDbl(it8, "DESCRIPTOR") != 1234) {
   
        return 0;
    }


    cmsIT8SetPropertyDbl(it8, "DESCRIPTOR", 5678);

    if (cmsIT8GetPropertyDbl(it8, "DESCRIPTOR") != 5678) {
         
        return 0;
    }

    if (cmsIT8GetDataDbl(it8, "P3", "RGB_G") != 3) {
   
        return 0;
    }

    cmsIT8Free(it8);

    remove("TEST.IT8");
    return 1;

}


// Create CSA/CRD

static
void GenerateCSA(const char* cInProf, const char* FileName)
{
    cmsHPROFILE hProfile;   
    cmsUInt32Number n;
    char* Buffer;
    cmsContext BuffThread = DbgThread();
    FILE* o;


    if (cInProf == NULL) 
        hProfile = cmsCreateLab4Profile(NULL);
    else 
        hProfile = cmsOpenProfileFromFile(cInProf, "r");

    n = cmsGetPostScriptCSA(DbgThread(), hProfile, 0, 0, NULL, 0);
    if (n == 0) return;

    Buffer = (char*) _cmsMalloc(BuffThread, n + 1);
    cmsGetPostScriptCSA(DbgThread(), hProfile, 0, 0, Buffer, n);
    Buffer[n] = 0;

    if (FileName != NULL) {
        o = fopen(FileName, "wb");
        fwrite(Buffer, n, 1, o);
        fclose(o);
    }

    _cmsFree(BuffThread, Buffer);
    cmsCloseProfile(hProfile);
    remove(FileName);
}


static
void GenerateCRD(const char* cOutProf, const char* FileName)
{
    cmsHPROFILE hProfile;
    cmsUInt32Number n;
    char* Buffer;
    cmsUInt32Number dwFlags = 0;
    cmsContext BuffThread = DbgThread();


    if (cOutProf == NULL) 
        hProfile = cmsCreateLab4Profile(NULL);
    else 
        hProfile = cmsOpenProfileFromFile(cOutProf, "r");

    n = cmsGetPostScriptCRD(DbgThread(), hProfile, 0, dwFlags, NULL, 0);
    if (n == 0) return;

    Buffer = (char*) _cmsMalloc(BuffThread, n + 1);
    cmsGetPostScriptCRD(DbgThread(), hProfile, 0, dwFlags, Buffer, n);
    Buffer[n] = 0;

    if (FileName != NULL) {
        FILE* o = fopen(FileName, "wb");
        fwrite(Buffer, n, 1, o);
        fclose(o);
    }

    _cmsFree(BuffThread, Buffer);
    cmsCloseProfile(hProfile);
    remove(FileName);
}

static 
cmsInt32Number CheckPostScript(void)
{
    GenerateCSA("sRGB_Color_Space_Profile.icm", "sRGB_CSA.ps");
    GenerateCSA("aRGBlcms2.icc", "aRGB_CSA.ps");
    GenerateCSA("sRGB_v4_ICC_preference.icc", "sRGBV4_CSA.ps");
    GenerateCSA("USWebCoatedSWOP.icc", "SWOP_CSA.ps");
    GenerateCSA(NULL, "Lab_CSA.ps");
    GenerateCSA("graylcms2.icc", "gray_CSA.ps");
    
    GenerateCRD("sRGB_Color_Space_Profile.icm", "sRGB_CRD.ps");
    GenerateCRD("aRGBlcms2.icc", "aRGB_CRD.ps");
    GenerateCRD(NULL, "Lab_CRD.ps");
    GenerateCRD("USWebCoatedSWOP.icc", "SWOP_CRD.ps");
    GenerateCRD("sRGB_v4_ICC_preference.icc", "sRGBV4_CRD.ps");
    GenerateCRD("graylcms2.icc", "gray_CRD.ps");

    return 1;
}


static
cmsInt32Number CheckGray(cmsHTRANSFORM xform, cmsUInt8Number g, double L)
{
    cmsCIELab Lab;

    cmsDoTransform(xform, &g, &Lab, 1);

    if (!IsGoodVal("a axis on gray", 0, Lab.a, 0.001)) return 0;
    if (!IsGoodVal("b axis on gray", 0, Lab.b, 0.001)) return 0;

    return IsGoodVal("Gray value", L, Lab.L, 0.01);
}

static
cmsInt32Number CheckInputGray(void)
{
    cmsHPROFILE hGray = Create_Gray22();
    cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM xform;

    if (hGray == NULL || hLab == NULL) return 0;

    xform = cmsCreateTransform(hGray, TYPE_GRAY_8, hLab, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0); 
    cmsCloseProfile(hGray); cmsCloseProfile(hLab);

    if (!CheckGray(xform, 0, 0)) return 0;
    if (!CheckGray(xform, 125, 52.768)) return 0;
    if (!CheckGray(xform, 200, 81.069)) return 0;
    if (!CheckGray(xform, 255, 100.0)) return 0;

    cmsDeleteTransform(xform);
    return 1;
}

static
cmsInt32Number CheckLabInputGray(void)
{
    cmsHPROFILE hGray = Create_GrayLab();
    cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM xform;

    if (hGray == NULL || hLab == NULL) return 0;

    xform = cmsCreateTransform(hGray, TYPE_GRAY_8, hLab, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hGray); cmsCloseProfile(hLab);

    if (!CheckGray(xform, 0, 0)) return 0;
    if (!CheckGray(xform, 125, 49.019)) return 0;
    if (!CheckGray(xform, 200, 78.431)) return 0;
    if (!CheckGray(xform, 255, 100.0)) return 0;

    cmsDeleteTransform(xform);
    return 1;
}


static
cmsInt32Number CheckOutGray(cmsHTRANSFORM xform, double L, cmsUInt8Number g)
{
    cmsCIELab Lab;
    cmsUInt8Number g_out;

    Lab.L = L;
    Lab.a = 0;
    Lab.b = 0;

    cmsDoTransform(xform, &Lab, &g_out, 1);

    return IsGoodVal("Gray value", g, (double) g_out, 0.01);
}

static
cmsInt32Number CheckOutputGray(void)
{
    cmsHPROFILE hGray = Create_Gray22();
    cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM xform;

    if (hGray == NULL || hLab == NULL) return 0;

    xform = cmsCreateTransform( hLab, TYPE_Lab_DBL, hGray, TYPE_GRAY_8, INTENT_RELATIVE_COLORIMETRIC, 0); 
    cmsCloseProfile(hGray); cmsCloseProfile(hLab);

    if (!CheckOutGray(xform, 0, 0)) return 0;
    if (!CheckOutGray(xform, 100, 255)) return 0;
    
    if (!CheckOutGray(xform, 20, 52)) return 0;
    if (!CheckOutGray(xform, 50, 118)) return 0;
    

    cmsDeleteTransform(xform);
    return 1;
}


static
cmsInt32Number CheckLabOutputGray(void)
{
    cmsHPROFILE hGray = Create_GrayLab();
    cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
    cmsHTRANSFORM xform;
    cmsInt32Number i;

    if (hGray == NULL || hLab == NULL) return 0;

    xform = cmsCreateTransform( hLab, TYPE_Lab_DBL, hGray, TYPE_GRAY_8, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsCloseProfile(hGray); cmsCloseProfile(hLab);

    if (!CheckOutGray(xform, 0, 0)) return 0;
    if (!CheckOutGray(xform, 100, 255)) return 0;

    for (i=0; i < 100; i++) {

        cmsUInt8Number g;

        g = (cmsUInt8Number) floor(i * 255.0 / 100.0 + 0.5);

        if (!CheckOutGray(xform, i, g)) return 0;
    }
    

    cmsDeleteTransform(xform);
    return 1;
}


static
cmsInt32Number CheckV4gamma(void)
{
    cmsHPROFILE h;
    cmsUInt16Number Lin[] = {0, 0xffff};
    cmsToneCurve*g = cmsBuildTabulatedToneCurve16(DbgThread(), 2, Lin);

    h = cmsOpenProfileFromFileTHR(DbgThread(), "v4gamma.icc", "w");
    if (h == NULL) return 0;

    
    cmsSetProfileVersion(h, 4.2);

    if (!cmsWriteTag(h, cmsSigGrayTRCTag, g)) return 0;
    cmsCloseProfile(h);

    cmsFreeToneCurve(g);
    remove("v4gamma.icc");
    return 1;
}

// cmsBool cmsGBDdumpVRML(cmsHANDLE hGBD, const char* fname);

// Gamut descriptor routines
static
cmsInt32Number CheckGBD(void)
{
    cmsCIELab Lab;
    cmsHANDLE  h;
    cmsInt32Number L, a, b;
    cmsUInt32Number r1, g1, b1;
    cmsHPROFILE hLab, hsRGB;
    cmsHTRANSFORM xform;

    h = cmsGBDAlloc(DbgThread());
    if (h == NULL) return 0;

    // Fill all Lab gamut as valid
    SubTest("Filling RAW gamut");

    for (L=0; L <= 100; L += 10)
        for (a = -128; a <= 128; a += 5)
            for (b = -128; b <= 128; b += 5) {

                Lab.L = L;
                Lab.a = a;
                Lab.b = b;
                if (!cmsGDBAddPoint(h, &Lab)) return 0;
            }

    // Complete boundaries
    SubTest("computing Lab gamut");
    if (!cmsGDBCompute(h, 0)) return 0;


    // All points should be inside gamut
    SubTest("checking Lab gamut");
    for (L=10; L <= 90; L += 25)
        for (a = -120; a <= 120; a += 25)
            for (b = -120; b <= 120; b += 25) {

                Lab.L = L;
                Lab.a = a;
                Lab.b = b;
                if (!cmsGDBCheckPoint(h, &Lab)) {
                    return 0;
                }
            }
    cmsGBDFree(h);


    // Now for sRGB
    SubTest("checking sRGB gamut");
    h = cmsGBDAlloc(DbgThread());
    hsRGB = cmsCreate_sRGBProfile();
    hLab  = cmsCreateLab4Profile(NULL);

    xform = cmsCreateTransform(hsRGB, TYPE_RGB_8, hLab, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_NOCACHE);
    cmsCloseProfile(hsRGB); cmsCloseProfile(hLab);


    for (r1=0; r1 < 256; r1 += 5) {
        for (g1=0; g1 < 256; g1 += 5)
            for (b1=0; b1 < 256; b1 += 5) {

                
                cmsUInt8Number rgb[3];  

                rgb[0] = (cmsUInt8Number) r1;
                rgb[1] = (cmsUInt8Number) g1; 
                rgb[2] = (cmsUInt8Number) b1;

                cmsDoTransform(xform, rgb, &Lab, 1);

                // if (fabs(Lab.b) < 20 && Lab.a > 0) continue;

                if (!cmsGDBAddPoint(h, &Lab)) {
                    cmsGBDFree(h);
                    return 0;
                }
                

            }
    }
    

    if (!cmsGDBCompute(h, 0)) return 0;
    // cmsGBDdumpVRML(h, "c:\\colormaps\\lab.wrl");

    for (r1=10; r1 < 200; r1 += 10) {
        for (g1=10; g1 < 200; g1 += 10)
            for (b1=10; b1 < 200; b1 += 10) {

                
                cmsUInt8Number rgb[3];  

                rgb[0] = (cmsUInt8Number) r1;
                rgb[1] = (cmsUInt8Number) g1; 
                rgb[2] = (cmsUInt8Number) b1;

                cmsDoTransform(xform, rgb, &Lab, 1);
                if (!cmsGDBCheckPoint(h, &Lab)) {

                    cmsDeleteTransform(xform);
                    cmsGBDFree(h);
                    return 0;
                }
            }
    }


    cmsDeleteTransform(xform);
    cmsGBDFree(h);

    SubTest("checking LCh chroma ring");
    h = cmsGBDAlloc(DbgThread());

    
    for (r1=0; r1 < 360; r1++) {

        cmsCIELCh LCh;

        LCh.L = 70;
        LCh.C = 60;
        LCh.h = r1;

        cmsLCh2Lab(&Lab, &LCh);
        if (!cmsGDBAddPoint(h, &Lab)) {
                    cmsGBDFree(h);
                    return 0;
                }
    }
    

    if (!cmsGDBCompute(h, 0)) return 0;

    cmsGBDFree(h);

    return 1;
}


// --------------------------------------------------------------------------------------------------
// P E R F O R M A N C E   C H E C K S
// --------------------------------------------------------------------------------------------------


typedef struct {cmsUInt8Number r, g, b, a;}   Scanline_rgb1;
typedef struct {cmsUInt16Number r, g, b, a;}  Scanline_rgb2;
typedef struct {cmsUInt8Number r, g, b;}      Scanline_rgb8;
typedef struct {cmsUInt16Number r, g, b;}     Scanline_rgb0;


static 
void TitlePerformance(const char* Txt)
{
    printf("%-45s: ", Txt); fflush(stdout);
}

static
void PrintPerformance(cmsUInt32Number Bytes, cmsUInt32Number SizeOfPixel, cmsFloat64Number diff)
{
    cmsFloat64Number seconds  = (cmsFloat64Number) diff / CLOCKS_PER_SEC;
    cmsFloat64Number mpix_sec = Bytes / (1024.0*1024.0*seconds*SizeOfPixel);

    printf("%g MPixel/sec.\n", mpix_sec);
    fflush(stdout);
}


static
void SpeedTest16bits(const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, cmsInt32Number Intent)
{

    cmsInt32Number r, g, b, j;
    clock_t atime;
    cmsFloat64Number diff;
    cmsHTRANSFORM hlcmsxform;
    Scanline_rgb0 *In;
    cmsUInt32Number Mb;

    if (hlcmsProfileIn == NULL || hlcmsProfileOut == NULL) 
        Die("Unable to open profiles");

    hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_RGB_16, 
                               hlcmsProfileOut, TYPE_RGB_16, Intent, cmsFLAGS_NOCACHE);
    cmsCloseProfile(hlcmsProfileIn);
    cmsCloseProfile(hlcmsProfileOut);

    Mb = 256*256*256*sizeof(Scanline_rgb0);
    In = (Scanline_rgb0*) malloc(Mb);

    j = 0;
    for (r=0; r < 256; r++)
        for (g=0; g < 256; g++)
            for (b=0; b < 256; b++) {

        In[j].r = (cmsUInt16Number) ((r << 8) | r);
        In[j].g = (cmsUInt16Number) ((g << 8) | g);
        In[j].b = (cmsUInt16Number) ((b << 8) | b);

        j++;
    }


    TitlePerformance(Title);

    atime = clock();

    cmsDoTransform(hlcmsxform, In, In, 256*256*256);

    diff = clock() - atime;
    free(In);
        
    PrintPerformance(Mb, sizeof(Scanline_rgb0), diff);
    cmsDeleteTransform(hlcmsxform);
    
}


static
void SpeedTest16bitsCMYK(const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut)
{
    cmsInt32Number r, g, b, j;
    clock_t atime;
    cmsFloat64Number diff;
    cmsHTRANSFORM hlcmsxform;
    Scanline_rgb2 *In;
    cmsUInt32Number Mb;
     
    if (hlcmsProfileOut == NULL || hlcmsProfileOut == NULL) 
        Die("Unable to open profiles");

    hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_CMYK_16, 
                hlcmsProfileOut, TYPE_CMYK_16, INTENT_PERCEPTUAL,  cmsFLAGS_NOCACHE);
    cmsCloseProfile(hlcmsProfileIn);
    cmsCloseProfile(hlcmsProfileOut);

    Mb = 256*256*256*sizeof(Scanline_rgb2);

    In = (Scanline_rgb2*) malloc(Mb);

    j = 0;
    for (r=0; r < 256; r++)
        for (g=0; g < 256; g++)
            for (b=0; b < 256; b++) {

        In[j].r = (cmsUInt16Number) ((r << 8) | r);
        In[j].g = (cmsUInt16Number) ((g << 8) | g);
        In[j].b = (cmsUInt16Number) ((b << 8) | b);
        In[j].a = 0;

        j++;
    }


    TitlePerformance(Title);

    atime = clock();

    cmsDoTransform(hlcmsxform, In, In, 256*256*256);

    diff = clock() - atime;

    free(In);
    
    PrintPerformance(Mb, sizeof(Scanline_rgb2), diff);

    cmsDeleteTransform(hlcmsxform);
 
}


static
void SpeedTest8bits(const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, cmsInt32Number Intent)
{
    cmsInt32Number r, g, b, j;
    clock_t atime;
    cmsFloat64Number diff;
    cmsHTRANSFORM hlcmsxform;
    Scanline_rgb8 *In;
    cmsUInt32Number Mb;
   
    if (hlcmsProfileIn == NULL || hlcmsProfileOut == NULL) 
        Die("Unable to open profiles");

    hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_RGB_8, 
                            hlcmsProfileOut, TYPE_RGB_8, Intent, cmsFLAGS_NOCACHE);
    cmsCloseProfile(hlcmsProfileIn);
    cmsCloseProfile(hlcmsProfileOut);

    Mb = 256*256*256*sizeof(Scanline_rgb8);

    In = (Scanline_rgb8*) malloc(Mb);

    j = 0;
    for (r=0; r < 256; r++)
        for (g=0; g < 256; g++)
            for (b=0; b < 256; b++) {

        In[j].r = (cmsUInt8Number) r;
        In[j].g = (cmsUInt8Number) g;
        In[j].b = (cmsUInt8Number) b;

        j++;
    }

    TitlePerformance(Title);

    atime = clock();

    cmsDoTransform(hlcmsxform, In, In, 256*256*256);

    diff = clock() - atime;

    free(In);
    
    PrintPerformance(Mb, sizeof(Scanline_rgb8), diff);
    
    cmsDeleteTransform(hlcmsxform);

}


static
void SpeedTest8bitsCMYK(const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut)
{
    cmsInt32Number r, g, b, j;
    clock_t atime;
    cmsFloat64Number diff;
    cmsHTRANSFORM hlcmsxform;
    Scanline_rgb2 *In;
    cmsUInt32Number Mb;
    
    if (hlcmsProfileIn == NULL || hlcmsProfileOut == NULL) 
        Die("Unable to open profiles");

    hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_CMYK_8, 
                        hlcmsProfileOut, TYPE_CMYK_8, INTENT_PERCEPTUAL, cmsFLAGS_NOCACHE);
    cmsCloseProfile(hlcmsProfileIn);
    cmsCloseProfile(hlcmsProfileOut);

    Mb = 256*256*256*sizeof(Scanline_rgb2);

    In = (Scanline_rgb2*) malloc(Mb);

    j = 0;
    for (r=0; r < 256; r++)
        for (g=0; g < 256; g++)
            for (b=0; b < 256; b++) {

        In[j].r = (cmsUInt8Number) r;
        In[j].g = (cmsUInt8Number) g;
        In[j].b = (cmsUInt8Number) b;
        In[j].a = (cmsUInt8Number) 0;

        j++;
    }

    TitlePerformance(Title);

    atime = clock();

    cmsDoTransform(hlcmsxform, In, In, 256*256*256);

    diff = clock() - atime;

    free(In);
    
    PrintPerformance(Mb, sizeof(Scanline_rgb2), diff);


    cmsDeleteTransform(hlcmsxform);

}


static
void SpeedTest8bitsGray(const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, cmsInt32Number Intent)
{
    cmsInt32Number r, g, b, j;
    clock_t atime;
    cmsFloat64Number diff;
    cmsHTRANSFORM hlcmsxform;
    cmsUInt8Number *In;
    cmsUInt32Number Mb;
   
   
    if (hlcmsProfileIn == NULL || hlcmsProfileOut == NULL) 
        Die("Unable to open profiles");

    hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, 
                        TYPE_GRAY_8, hlcmsProfileOut, TYPE_GRAY_8, Intent, cmsFLAGS_NOCACHE);
    cmsCloseProfile(hlcmsProfileIn);
    cmsCloseProfile(hlcmsProfileOut);
    Mb = 256*256*256;

    In = (cmsUInt8Number*) malloc(Mb);

    j = 0;
    for (r=0; r < 256; r++)
        for (g=0; g < 256; g++)
            for (b=0; b < 256; b++) {

        In[j] = (cmsUInt8Number) r;
        
        j++;
    }

    TitlePerformance(Title);

    atime = clock();

    cmsDoTransform(hlcmsxform, In, In, 256*256*256);

    diff = clock() - atime;
    free(In);
        
    PrintPerformance(Mb, sizeof(cmsUInt8Number), diff);
    cmsDeleteTransform(hlcmsxform);
}


static
cmsHPROFILE CreateCurves(void)
{
    cmsToneCurve* Gamma = cmsBuildGamma(DbgThread(), 1.1);
    cmsToneCurve* Transfer[3];
    cmsHPROFILE h;
    
    Transfer[0] = Transfer[1] = Transfer[2] = Gamma;
    h = cmsCreateLinearizationDeviceLink(cmsSigRgbData, Transfer);

    cmsFreeToneCurve(Gamma);

    return h;
}
    

static
void SpeedTest(void)
{

    printf("\n\nP E R F O R M A N C E   T E S T S\n");
    printf(    "=================================\n\n");
    fflush(stdout);

    SpeedTest16bits("16 bits on CLUT profiles", 
        cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"),
        cmsOpenProfileFromFile("sRGBSpac.icm", "r"), INTENT_PERCEPTUAL);

    SpeedTest8bits("8 bits on CLUT profiles", 
        cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"),
        cmsOpenProfileFromFile("sRGBSpac.icm", "r"),
        INTENT_PERCEPTUAL);
    
    SpeedTest8bits("8 bits on Matrix-Shaper profiles", 
        cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"), 
        cmsOpenProfileFromFile("aRGBlcms2.icc", "r"),
        INTENT_PERCEPTUAL);

    SpeedTest8bits("8 bits on SAME Matrix-Shaper profiles",
        cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"),
        cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"),
        INTENT_PERCEPTUAL);

    SpeedTest8bits("8 bits on Matrix-Shaper profiles (AbsCol)", 
       cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"),
       cmsOpenProfileFromFile("aRGBlcms2.icc", "r"),
        INTENT_ABSOLUTE_COLORIMETRIC);  

    SpeedTest16bits("16 bits on Matrix-Shaper profiles", 
       cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"),
        cmsOpenProfileFromFile("aRGBlcms2.icc", "r"),
        INTENT_PERCEPTUAL);

    SpeedTest16bits("16 bits on SAME Matrix-Shaper profiles", 
        cmsOpenProfileFromFile("aRGBlcms2.icc", "r"),
        cmsOpenProfileFromFile("aRGBlcms2.icc", "r"),
        INTENT_PERCEPTUAL);

    SpeedTest16bits("16 bits on Matrix-Shaper profiles (AbsCol)", 
       cmsOpenProfileFromFile("sRGB_Color_Space_Profile.icm", "r"),
       cmsOpenProfileFromFile("aRGBlcms2.icc", "r"),
        INTENT_ABSOLUTE_COLORIMETRIC);

    SpeedTest8bits("8 bits on curves", 
        CreateCurves(), 
        CreateCurves(),
        INTENT_PERCEPTUAL);

    SpeedTest16bits("16 bits on curves", 
        CreateCurves(), 
        CreateCurves(),
        INTENT_PERCEPTUAL);

    SpeedTest8bitsCMYK("8 bits on CMYK profiles", 
        cmsOpenProfileFromFile("USWebCoatedSWOP.icc", "r"),
        cmsOpenProfileFromFile("UncoatedFOGRA29.icc", "r"));

    SpeedTest16bitsCMYK("16 bits on CMYK profiles", 
        cmsOpenProfileFromFile("USWebCoatedSWOP.icc", "r"),
        cmsOpenProfileFromFile("UncoatedFOGRA29.icc", "r"));

    SpeedTest8bitsGray("8 bits on gray-to-gray",
        cmsOpenProfileFromFile("graylcms2.icc", "r"), 
        cmsOpenProfileFromFile("glablcms2.icc", "r"), INTENT_RELATIVE_COLORIMETRIC);

    SpeedTest8bitsGray("8 bits on SAME gray-to-gray",
        cmsOpenProfileFromFile("graylcms2.icc", "r"), 
        cmsOpenProfileFromFile("graylcms2.icc", "r"), INTENT_PERCEPTUAL);
}


// -----------------------------------------------------------------------------------------------------


// Print the supported intents
static
void PrintSupportedIntents(void)
{
    cmsUInt32Number n, i;
    cmsUInt32Number Codes[200];
    char* Descriptions[200];

    n = cmsGetSupportedIntents(200, Codes, Descriptions);

    printf("Supported intents:\n");
    for (i=0; i < n; i++) {
        printf("\t%d - %s\n", Codes[i], Descriptions[i]);
    }
    printf("\n");
}

// ZOO checks ------------------------------------------------------------------------------------------------------------


#ifdef CMS_IS_WINDOWS_

static char ZOOfolder[cmsMAX_PATH] = "c:\\colormaps\\";
static char ZOOwrite[cmsMAX_PATH]  = "c:\\colormaps\\write\\";
static char ZOORawWrite[cmsMAX_PATH]  = "c:\\colormaps\\rawwrite\\";


// Read all tags on a profile given by its handle
static
void ReadAllTags(cmsHPROFILE h)
{
    cmsInt32Number i, n;
    cmsTagSignature sig;

    n = cmsGetTagCount(h);
    for (i=0; i < n; i++) {

        sig = cmsGetTagSignature(h, i);
        if (cmsReadTag(h, sig) == NULL) return;
    }
}


// Read all tags on a profile given by its handle
static
void ReadAllRAWTags(cmsHPROFILE h)
{
    cmsInt32Number i, n;
    cmsTagSignature sig;
    cmsInt32Number len;

    n = cmsGetTagCount(h);
    for (i=0; i < n; i++) {

        sig = cmsGetTagSignature(h, i);
        len = cmsReadRawTag(h, sig, NULL, 0);
    }
}


static
void PrintInfo(cmsHPROFILE h, cmsInfoType Info)
{
    wchar_t* text;
    cmsInt32Number len;
    cmsContext id = DbgThread();

    len = cmsGetProfileInfo(h, Info, "en", "US", NULL, 0);
    if (len == 0) return;

    text = _cmsMalloc(id, len);
    cmsGetProfileInfo(h, Info, "en", "US", text, len);

    wprintf(L"%s\n", text);
    _cmsFree(id, text);
}


static
void PrintAllInfos(cmsHPROFILE h)
{
     PrintInfo(h, cmsInfoDescription);
     PrintInfo(h, cmsInfoManufacturer);
     PrintInfo(h, cmsInfoModel);       
     PrintInfo(h, cmsInfoCopyright);   
     printf("\n\n");
}

static
void ReadAllLUTS(cmsHPROFILE h)
{
    cmsPipeline* a;
    cmsCIEXYZ Black;
  
    a = _cmsReadInputLUT(h, INTENT_PERCEPTUAL);
    if (a) cmsPipelineFree(a);

    a = _cmsReadInputLUT(h, INTENT_RELATIVE_COLORIMETRIC);
    if (a) cmsPipelineFree(a);

    a = _cmsReadInputLUT(h, INTENT_SATURATION);
    if (a) cmsPipelineFree(a);

    a = _cmsReadInputLUT(h, INTENT_ABSOLUTE_COLORIMETRIC);
    if (a) cmsPipelineFree(a);


    a = _cmsReadOutputLUT(h, INTENT_PERCEPTUAL);
    if (a) cmsPipelineFree(a);

    a = _cmsReadOutputLUT(h, INTENT_RELATIVE_COLORIMETRIC);
    if (a) cmsPipelineFree(a);

    a = _cmsReadOutputLUT(h, INTENT_SATURATION);
    if (a) cmsPipelineFree(a);

    a = _cmsReadOutputLUT(h, INTENT_ABSOLUTE_COLORIMETRIC);
    if (a) cmsPipelineFree(a);


    a = _cmsReadDevicelinkLUT(h, INTENT_PERCEPTUAL);
    if (a) cmsPipelineFree(a);

    a = _cmsReadDevicelinkLUT(h, INTENT_RELATIVE_COLORIMETRIC);
    if (a) cmsPipelineFree(a);

    a = _cmsReadDevicelinkLUT(h, INTENT_SATURATION);
    if (a) cmsPipelineFree(a);

    a = _cmsReadDevicelinkLUT(h, INTENT_ABSOLUTE_COLORIMETRIC);
    if (a) cmsPipelineFree(a);


    cmsDetectBlackPoint(&Black, h, INTENT_PERCEPTUAL, 0);
    cmsDetectBlackPoint(&Black, h, INTENT_RELATIVE_COLORIMETRIC, 0);
    cmsDetectBlackPoint(&Black, h, INTENT_SATURATION, 0);
    cmsDetectBlackPoint(&Black, h, INTENT_ABSOLUTE_COLORIMETRIC, 0);
    cmsDetectTAC(h);
}

// Check one specimen in the ZOO

static
cmsInt32Number CheckSingleSpecimen(const char* Profile)
{
    char BuffSrc[256];
    char BuffDst[256];
    cmsHPROFILE h;

    sprintf(BuffSrc, "%s%s", ZOOfolder, Profile);
    sprintf(BuffDst, "%s%s", ZOOwrite,  Profile);

    h = cmsOpenProfileFromFile(BuffSrc, "r");
    if (h == NULL) return 0;
    
    printf("%s\n", Profile);
    PrintAllInfos(h);   
    ReadAllTags(h);  
    // ReadAllRAWTags(h);   
    ReadAllLUTS(h);

    cmsSaveProfileToFile(h, BuffDst);
    cmsCloseProfile(h);

    h = cmsOpenProfileFromFile(BuffDst, "r");
    if (h == NULL) return 0;
    ReadAllTags(h); 
    

    cmsCloseProfile(h);

    return 1;
}

static
cmsInt32Number CheckRAWSpecimen(const char* Profile)
{
    char BuffSrc[256];
    char BuffDst[256];
    cmsHPROFILE h;

    sprintf(BuffSrc, "%s%s", ZOOfolder, Profile);
    sprintf(BuffDst, "%s%s", ZOORawWrite,  Profile);

    h = cmsOpenProfileFromFile(BuffSrc, "r");
    if (h == NULL) return 0;
    
    ReadAllTags(h);
    ReadAllRAWTags(h);  
    cmsSaveProfileToFile(h, BuffDst);
    cmsCloseProfile(h);

    h = cmsOpenProfileFromFile(BuffDst, "r");
    if (h == NULL) return 0;
    ReadAllTags(h); 
    cmsCloseProfile(h);

    return 1;
}


static
void CheckProfileZOO(void)
{

    struct _finddata_t c_file;
    intptr_t hFile;

    cmsSetLogErrorHandler(NULL);

    if ( (hFile = _findfirst("c:\\colormaps\\*.*", &c_file)) == -1L )
        printf("No files in current directory");
    else
    {
        do
        {

            printf("%s\n", c_file.name);
            if (strcmp(c_file.name, ".") != 0 &&
                strcmp(c_file.name, "..") != 0) {

                    CheckSingleSpecimen( c_file.name);
                    CheckRAWSpecimen( c_file.name);

                    if (TotalMemory > 0)
                        printf("Ok, but %s are left!\n", MemStr(TotalMemory));
                    else
                        printf("Ok.\n");

            }

        } while ( _findnext(hFile, &c_file) == 0 );

        _findclose(hFile);
    }

    cmsSetLogErrorHandler(FatalErrorQuit);
}

#endif

// ---------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    cmsInt32Number Exhaustive = 0;
    cmsInt32Number DoSpeedTests = 1;

#ifdef _MSC_VER
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
#endif

    printf("LittleCMS %2.2f test bed %s %s\n\n", LCMS_VERSION / 1000.0, __DATE__, __TIME__);

    if ((argc == 2) && strcmp(argv[1], "--exhaustive") == 0) {

        Exhaustive = 1;
        printf("Running exhaustive tests (will take a while...)\n\n");
    }

    
    printf("Installing debug memory plug-in ... ");
    cmsPlugin(&DebugMemHandler);
    printf("done.\n");

    printf("Installing error logger ... ");
    cmsSetLogErrorHandler(FatalErrorQuit);
    printf("done.\n");

#ifdef CMS_IS_WINDOWS_     
     // CheckProfileZOO();
#endif

    PrintSupportedIntents();

    // Create utility profiles
    Check("Creation of test profiles", CreateTestProfiles);    

    Check("Base types", CheckBaseTypes);
    Check("endianess", CheckEndianess);
    Check("quick floor", CheckQuickFloor);
    Check("quick floor word", CheckQuickFloorWord);
    Check("Fixed point 15.16 representation", CheckFixedPoint15_16);
    Check("Fixed point 8.8 representation", CheckFixedPoint8_8);
    
    // Forward 1D interpolation
    Check("1D interpolation in 2pt tables", Check1DLERP2);
    Check("1D interpolation in 3pt tables", Check1DLERP3);
    Check("1D interpolation in 4pt tables", Check1DLERP4);
    Check("1D interpolation in 6pt tables", Check1DLERP6);
    Check("1D interpolation in 18pt tables", Check1DLERP18);
    Check("1D interpolation in descending 2pt tables", Check1DLERP2Down);
    Check("1D interpolation in descending 3pt tables", Check1DLERP3Down);
    Check("1D interpolation in descending 6pt tables", Check1DLERP6Down);
    Check("1D interpolation in descending 18pt tables", Check1DLERP18Down);
    
    if (Exhaustive) {

        Check("1D interpolation in n tables", ExhaustiveCheck1DLERP);
        Check("1D interpolation in descending tables", ExhaustiveCheck1DLERPDown);
    }
    
    // Forward 3D interpolation
    Check("3D interpolation Tetrahedral (float) ", Check3DinterpolationFloatTetrahedral);
    Check("3D interpolation Trilinear (float) ", Check3DinterpolationFloatTrilinear);
    Check("3D interpolation Tetrahedral (16) ", Check3DinterpolationTetrahedral16);
    Check("3D interpolation Trilinear (16) ", Check3DinterpolationTrilinear16);
    
    if (Exhaustive) {

        Check("Exhaustive 3D interpolation Tetrahedral (float) ", ExaustiveCheck3DinterpolationFloatTetrahedral);
        Check("Exhaustive 3D interpolation Trilinear  (float) ", ExaustiveCheck3DinterpolationFloatTrilinear);
        Check("Exhaustive 3D interpolation Tetrahedral (16) ", ExhaustiveCheck3DinterpolationTetrahedral16);
        Check("Exhaustive 3D interpolation Trilinear (16) ", ExhaustiveCheck3DinterpolationTrilinear16);
    }

    Check("Reverse interpolation 3 -> 3", CheckReverseInterpolation3x3);
    Check("Reverse interpolation 4 -> 3", CheckReverseInterpolation4x3);
  

    // High dimensionality interpolation

    Check("3D interpolation", Check3Dinterp);
    Check("3D interpolation with granularity", Check3DinterpGranular);
    Check("4D interpolation", Check4Dinterp);
    Check("4D interpolation with granularity", Check4DinterpGranular);
    Check("5D interpolation with granularity", Check5DinterpGranular);
    Check("6D interpolation with granularity", Check6DinterpGranular);
    Check("7D interpolation with granularity", Check7DinterpGranular);


    // Encoding of colorspaces
    Check("Lab to LCh and back (float only) ", CheckLab2LCh);
    Check("Lab to XYZ and back (float only) ", CheckLab2XYZ);
    Check("Lab to xyY and back (float only) ", CheckLab2xyY);
    Check("Lab V2 encoding", CheckLabV2encoding);
    Check("Lab V4 encoding", CheckLabV4encoding);

    // BlackBody
    Check("Blackbody radiator", CheckTemp2CHRM);
    
    // Tone curves
    Check("Linear gamma curves (16 bits)", CheckGammaCreation16);
    Check("Linear gamma curves (float)", CheckGammaCreationFlt);

    Check("Curve 1.8 (float)", CheckGamma18);
    Check("Curve 2.2 (float)", CheckGamma22);
    Check("Curve 3.0 (float)", CheckGamma30);

    Check("Curve 1.8 (table)", CheckGamma18Table);
    Check("Curve 2.2 (table)", CheckGamma22Table);
    Check("Curve 3.0 (table)", CheckGamma30Table);

    Check("Curve 1.8 (word table)", CheckGamma18TableWord);
    Check("Curve 2.2 (word table)", CheckGamma22TableWord);
    Check("Curve 3.0 (word table)", CheckGamma30TableWord);

    Check("Parametric curves", CheckParametricToneCurves);
    
    Check("Join curves", CheckJointCurves);
    Check("Join curves descending", CheckJointCurvesDescending);
    Check("Join curves degenerated", CheckReverseDegenerated);  
    Check("Join curves sRGB (Float)", CheckJointFloatCurves_sRGB);
    Check("Join curves sRGB (16 bits)", CheckJoint16Curves_sRGB);
    Check("Join curves sigmoidal", CheckJointCurvesSShaped);
        
    // LUT basics
    Check("LUT creation & dup", CheckLUTcreation);
    Check("1 Stage LUT ", Check1StageLUT);
    Check("2 Stage LUT ", Check2StageLUT);
    Check("2 Stage LUT (16 bits)", Check2Stage16LUT);
    Check("3 Stage LUT ", Check3StageLUT);
    Check("3 Stage LUT (16 bits)", Check3Stage16LUT);
    Check("4 Stage LUT ", Check4StageLUT);
    Check("4 Stage LUT (16 bits)", Check4Stage16LUT);
    Check("5 Stage LUT ", Check5StageLUT);
    Check("5 Stage LUT (16 bits) ", Check5Stage16LUT);
    Check("6 Stage LUT ", Check6StageLUT);
    Check("6 Stage LUT (16 bits) ", Check6Stage16LUT);
    
    // LUT operation
    Check("Lab to Lab LUT (float only) ", CheckLab2LabLUT);
    Check("XYZ to XYZ LUT (float only) ", CheckXYZ2XYZLUT);
    Check("Lab to Lab MAT LUT (float only) ", CheckLab2LabMatLUT);
    Check("Named Color LUT", CheckNamedColorLUT);
    Check("Usual formatters", CheckFormatters16);
    Check("Floating point formatters", CheckFormattersFloat);

    // ChangeBuffersFormat
    Check("ChangeBuffersFormat", CheckChangeBufferFormat);
    
    // MLU    
    Check("Multilocalized Unicode", CheckMLU);
    
    // Named color
    Check("Named color lists", CheckNamedColorList);
    
    // Profile I/O (this one is huge!)
    Check("Profile creation", CheckProfileCreation);

    
    // Error reporting
    Check("Error reporting on bad profiles", CheckErrReportingOnBadProfiles);
    Check("Error reporting on bad transforms", CheckErrReportingOnBadTransforms);
    
    // Transforms
    Check("Curves only transforms", CheckCurvesOnlyTransforms);
    Check("Float Lab->Lab transforms", CheckFloatLabTransforms);
    Check("Encoded Lab->Lab transforms", CheckEncodedLabTransforms);    
    Check("Stored identities", CheckStoredIdentities);

    Check("Matrix-shaper transform (float)",   CheckMatrixShaperXFORMFloat);
    Check("Matrix-shaper transform (16 bits)", CheckMatrixShaperXFORM16);   
    Check("Matrix-shaper transform (8 bits)",  CheckMatrixShaperXFORM8);

    Check("Primaries of sRGB", CheckRGBPrimaries);

    // Known values
    Check("Known values across matrix-shaper", Chack_sRGB_Float);
    Check("Gray input profile", CheckInputGray);
    Check("Gray Lab input profile", CheckLabInputGray);
    Check("Gray output profile", CheckOutputGray);
    Check("Gray Lab output profile", CheckLabOutputGray);

    Check("Matrix-shaper proofing transform (float)",   CheckProofingXFORMFloat);
    Check("Matrix-shaper proofing transform (16 bits)",  CheckProofingXFORM16);
    
    Check("Gamut check", CheckGamutCheck);
        
    Check("CMYK roundtrip on perceptual transform",   CheckCMYKRoundtrip);
    
    Check("CMYK perceptual transform",   CheckCMYKPerceptual);
    // Check("CMYK rel.col. transform",   CheckCMYKRelCol);
    
    Check("Black ink only preservation", CheckKOnlyBlackPreserving);
    Check("Black plane preservation", CheckKPlaneBlackPreserving);

 
    Check("Deciding curve types", CheckV4gamma);

    Check("Black point detection", CheckBlackPoint);
    Check("TAC detection", CheckTAC);    

    Check("CGATS parser", CheckCGATS);
    Check("PostScript generator", CheckPostScript);
    Check("Segment maxima GBD", CheckGBD);


    if (DoSpeedTests)
        SpeedTest();
    
    DebugMemPrintTotals();

    cmsUnregisterPlugins();

    // Cleanup
    RemoveTestProfiles();
    
    return TotalFail;
}
 


