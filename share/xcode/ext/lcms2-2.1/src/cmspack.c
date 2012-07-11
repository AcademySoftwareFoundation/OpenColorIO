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

#include "lcms2_internal.h"

// This module handles all formats supported by lcms. There are two flavors, 16 bits and 
// floating point. Floating point is supported only in a subset, those formats holding
// cmsFloat32Number (4 bytes per component) and double (marked as 0 bytes per component as special 
// case)

// ---------------------------------------------------------------------------


// This macro return words stored as big endian
#define CHANGE_ENDIAN(w)    (cmsUInt16Number) ((cmsUInt16Number) ((w)<<8)|((w)>>8))

// These macros handles reversing (negative)
#define REVERSE_FLAVOR_8(x)     ((cmsUInt8Number) (0xff-(x)))
#define REVERSE_FLAVOR_16(x)    ((cmsUInt16Number)(0xffff-(x)))

// * 0xffff / 0xff00 = (255 * 257) / (255 * 256) = 257 / 256
cmsINLINE cmsUInt16Number FomLabV2ToLabV4(cmsUInt16Number x) 
{
    int a;

    a = (x << 8 | x) >> 8;  // * 257 / 256
    if ( a > 0xffff) return 0xffff;
    return (cmsUInt16Number) a;
}

// * 0xf00 / 0xffff = * 256 / 257
cmsINLINE cmsUInt16Number FomLabV4ToLabV2(cmsUInt16Number x) 
{
    return (cmsUInt16Number) (((x << 8) + 0x80) / 257);
}


typedef struct {
    cmsUInt32Number Type;
    cmsUInt32Number Mask;
    cmsFormatter16 Frm;

} cmsFormatters16;

typedef struct {
    cmsUInt32Number    Type;
    cmsUInt32Number    Mask;
    cmsFormatterFloat Frm;

} cmsFormattersFloat;

#define ANYSPACE        COLORSPACE_SH(31)
#define ANYCHANNELS     CHANNELS_SH(15)
#define ANYEXTRA        EXTRA_SH(7)
#define ANYPLANAR       PLANAR_SH(1)
#define ANYENDIAN       ENDIAN16_SH(1)
#define ANYSWAP         DOSWAP_SH(1)
#define ANYSWAPFIRST    SWAPFIRST_SH(1) 
#define ANYFLAVOR       FLAVOR_SH(1)


// Supress waning about info never being used

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

// Unpacking routines (16 bits) ---------------------------------------------------------------------------------------- 

// Does almost everything but is slow
static
cmsUInt8Number* UnrollChunkyBytes(register _cmsTRANSFORM* info, 
                                  register cmsUInt16Number wIn[], 
                                  register cmsUInt8Number* accum,
                                  register cmsUInt32Number Stride)
{
    int nChan      = T_CHANNELS(info -> InputFormat);
    int DoSwap     = T_DOSWAP(info ->InputFormat);
    int Reverse    = T_FLAVOR(info ->InputFormat);
    int SwapFirst  = T_SWAPFIRST(info -> InputFormat);
    int Extra      = T_EXTRA(info -> InputFormat);
    int ExtraFirst = DoSwap && !SwapFirst;
    cmsUInt16Number v;
    int i;

    if (ExtraFirst) {
        accum += Extra;
    }

    for (i=0; i < nChan; i++) {
        int index = DoSwap ? (nChan - i - 1) : i;

        v = FROM_8_TO_16(*accum); 
        v = Reverse ? REVERSE_FLAVOR_16(v) : v;
        wIn[index] = v;
        accum++;              
    }

    if (!ExtraFirst) {
        accum += Extra;
    }

    if (Extra == 0 && SwapFirst) {
        cmsUInt16Number tmp = wIn[0];

        memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(cmsUInt16Number));
        wIn[nChan-1] = tmp;
    }

    return accum;
}

// Extra channels are just ignored because come in the next planes
static
cmsUInt8Number* UnrollPlanarBytes(register _cmsTRANSFORM* info, 
                                  register cmsUInt16Number wIn[], 
                                  register cmsUInt8Number* accum,
                                  register cmsUInt32Number Stride)
{
    int nChan = T_CHANNELS(info -> InputFormat);
    int DoSwap= T_DOSWAP(info ->InputFormat);
    int Reverse= T_FLAVOR(info ->InputFormat);
    int i;
    cmsUInt8Number* Init = accum;

    if (DoSwap) {
        accum += T_EXTRA(info -> InputFormat) * Stride;
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;
        cmsUInt16Number v = FROM_8_TO_16(*accum); 

        wIn[index] = Reverse ? REVERSE_FLAVOR_16(v) : v;
        accum += Stride;
    }

    return (Init + 1);
}

// Special cases, provided for performance
static
cmsUInt8Number* Unroll4Bytes(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wIn[], 
                             register cmsUInt8Number* accum,
                             register cmsUInt32Number Stride)
{
    wIn[0] = FROM_8_TO_16(*accum); accum++; // C
    wIn[1] = FROM_8_TO_16(*accum); accum++; // M
    wIn[2] = FROM_8_TO_16(*accum); accum++; // Y
    wIn[3] = FROM_8_TO_16(*accum); accum++; // K

    return accum;
}

static
cmsUInt8Number* Unroll4BytesReverse(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wIn[], 
                                    register cmsUInt8Number* accum,
                                    register cmsUInt32Number Stride)
{
    wIn[0] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // C
    wIn[1] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // M
    wIn[2] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // Y
    wIn[3] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // K

    return accum;
}

static
cmsUInt8Number* Unroll4BytesSwapFirst(register _cmsTRANSFORM* info, 
                                      register cmsUInt16Number wIn[], 
                                      register cmsUInt8Number* accum,
                                      register cmsUInt32Number Stride)
{
    wIn[3] = FROM_8_TO_16(*accum); accum++; // K
    wIn[0] = FROM_8_TO_16(*accum); accum++; // C
    wIn[1] = FROM_8_TO_16(*accum); accum++; // M
    wIn[2] = FROM_8_TO_16(*accum); accum++; // Y

    return accum;
}

// KYMC
static
cmsUInt8Number* Unroll4BytesSwap(register _cmsTRANSFORM* info, 
                                 register cmsUInt16Number wIn[], 
                                 register cmsUInt8Number* accum,
                                 register cmsUInt32Number Stride)
{
    wIn[3] = FROM_8_TO_16(*accum); accum++;  // K
    wIn[2] = FROM_8_TO_16(*accum); accum++;  // Y
    wIn[1] = FROM_8_TO_16(*accum); accum++;  // M
    wIn[0] = FROM_8_TO_16(*accum); accum++;  // C

    return accum;
}

static
cmsUInt8Number* Unroll4BytesSwapSwapFirst(register _cmsTRANSFORM* info, 
                                          register cmsUInt16Number wIn[], 
                                          register cmsUInt8Number* accum,
                                          register cmsUInt32Number Stride)
{
    wIn[2] = FROM_8_TO_16(*accum); accum++;  // K
    wIn[1] = FROM_8_TO_16(*accum); accum++;  // Y
    wIn[0] = FROM_8_TO_16(*accum); accum++;  // M
    wIn[3] = FROM_8_TO_16(*accum); accum++;  // C

    return accum;
}

static
cmsUInt8Number* Unroll3Bytes(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wIn[], 
                             register cmsUInt8Number* accum,
                             register cmsUInt32Number Stride)
{
    wIn[0] = FROM_8_TO_16(*accum); accum++;     // R
    wIn[1] = FROM_8_TO_16(*accum); accum++;     // G
    wIn[2] = FROM_8_TO_16(*accum); accum++;     // B

    return accum;
}

static
cmsUInt8Number* Unroll3BytesSkip1Swap(register _cmsTRANSFORM* info, 
                                      register cmsUInt16Number wIn[], 
                                      register cmsUInt8Number* accum,
                                      register cmsUInt32Number Stride)
{
    accum++; // A
    wIn[2] = FROM_8_TO_16(*accum); accum++; // B
    wIn[1] = FROM_8_TO_16(*accum); accum++; // G
    wIn[0] = FROM_8_TO_16(*accum); accum++; // R

    return accum;
}

static
cmsUInt8Number* Unroll3BytesSkip1SwapFirst(register _cmsTRANSFORM* info, 
                                           register cmsUInt16Number wIn[], 
                                           register cmsUInt8Number* accum,
                                           register cmsUInt32Number Stride)
{
    accum++; // A
    wIn[0] = FROM_8_TO_16(*accum); accum++; // R
    wIn[1] = FROM_8_TO_16(*accum); accum++; // G
    wIn[2] = FROM_8_TO_16(*accum); accum++; // B

    return accum;
}


// BRG
static
cmsUInt8Number* Unroll3BytesSwap(register _cmsTRANSFORM* info, 
                                 register cmsUInt16Number wIn[], 
                                 register cmsUInt8Number* accum,
                                 register cmsUInt32Number Stride)
{
    wIn[2] = FROM_8_TO_16(*accum); accum++;     // B
    wIn[1] = FROM_8_TO_16(*accum); accum++;     // G
    wIn[0] = FROM_8_TO_16(*accum); accum++;     // R

    return accum;
}

static
cmsUInt8Number* UnrollLabV2_8(register _cmsTRANSFORM* info, 
                              register cmsUInt16Number wIn[], 
                              register cmsUInt8Number* accum,
                              register cmsUInt32Number Stride)
{
    wIn[0] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++;     // L
    wIn[1] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++;     // a
    wIn[2] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++;     // b

    return accum;
}

static
cmsUInt8Number* UnrollALabV2_8(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wIn[], 
                               register cmsUInt8Number* accum,
                               register cmsUInt32Number Stride)
{
    accum++;  // A
    wIn[0] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++;     // L
    wIn[1] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++;     // a
    wIn[2] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++;     // b

    return accum;
}

static
cmsUInt8Number* UnrollLabV2_16(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wIn[], 
                               register cmsUInt8Number* accum,
                               register cmsUInt32Number Stride)
{
    wIn[0] = FomLabV2ToLabV4(*(cmsUInt16Number*) accum); accum += 2;     // L
    wIn[1] = FomLabV2ToLabV4(*(cmsUInt16Number*) accum); accum += 2;     // a
    wIn[2] = FomLabV2ToLabV4(*(cmsUInt16Number*) accum); accum += 2;     // b

    return accum;
}

// for duplex
static
cmsUInt8Number* Unroll2Bytes(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wIn[], 
                             register cmsUInt8Number* accum,
                             register cmsUInt32Number Stride)
{
    wIn[0] = FROM_8_TO_16(*accum); accum++;     // ch1
    wIn[1] = FROM_8_TO_16(*accum); accum++;     // ch2
    return accum;
}




// Monochrome duplicates L into RGB for null-transforms
static
cmsUInt8Number* Unroll1Byte(register _cmsTRANSFORM* info, 
                                     register cmsUInt16Number wIn[], 
                                     register cmsUInt8Number* accum,
                                     register cmsUInt32Number Stride)
{
    wIn[0] = wIn[1] = wIn[2] = FROM_8_TO_16(*accum); accum++;     // L       
    return accum;
}


static
cmsUInt8Number* Unroll1ByteSkip1(register _cmsTRANSFORM* info, 
                            register cmsUInt16Number wIn[], 
                            register cmsUInt8Number* accum,
                            register cmsUInt32Number Stride)
{
    wIn[0] = wIn[1] = wIn[2] = FROM_8_TO_16(*accum); accum++;     // L
    accum += 1;
    return accum;
}

static
cmsUInt8Number* Unroll1ByteSkip2(register _cmsTRANSFORM* info, 
                                 register cmsUInt16Number wIn[], 
                                 register cmsUInt8Number* accum,
                                 register cmsUInt32Number Stride)
{
    wIn[0] = wIn[1] = wIn[2] = FROM_8_TO_16(*accum); accum++;     // L
    accum += 2;
    return accum;
}

static
cmsUInt8Number* Unroll1ByteReversed(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wIn[], 
                                    register cmsUInt8Number* accum,
                                    register cmsUInt32Number Stride)
{
    wIn[0] = wIn[1] = wIn[2] = REVERSE_FLAVOR_16(FROM_8_TO_16(*accum)); accum++;     // L
    return accum;
}


static
cmsUInt8Number* UnrollAnyWords(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wIn[], 
                               register cmsUInt8Number* accum,
                               register cmsUInt32Number Stride)
{
    int nChan       = T_CHANNELS(info -> InputFormat);
    int SwapEndian  = T_ENDIAN16(info -> InputFormat);
    int DoSwap      = T_DOSWAP(info ->InputFormat);
    int Reverse     = T_FLAVOR(info ->InputFormat);
    int SwapFirst   = T_SWAPFIRST(info -> InputFormat);
    int Extra       = T_EXTRA(info -> InputFormat);
    int ExtraFirst  = DoSwap && !SwapFirst;
    int i;

    if (ExtraFirst) {
        accum += Extra * sizeof(cmsUInt16Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;
        cmsUInt16Number v = *(cmsUInt16Number*) accum; 

        if (SwapEndian)
            v = CHANGE_ENDIAN(v);

        wIn[index] = Reverse ? REVERSE_FLAVOR_16(v) : v;

        accum += sizeof(cmsUInt16Number);              
    }

    if (!ExtraFirst) {
        accum += Extra * sizeof(cmsUInt16Number);
    }

    if (Extra == 0 && SwapFirst) {

        cmsUInt16Number tmp = wIn[0];

        memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(cmsUInt16Number));
        wIn[nChan-1] = tmp;
    }

    return accum;
}

static
cmsUInt8Number* UnrollPlanarWords(register _cmsTRANSFORM* info, 
                                  register cmsUInt16Number wIn[], 
                                  register cmsUInt8Number* accum,
                                  register cmsUInt32Number Stride)
{
    int nChan = T_CHANNELS(info -> InputFormat);
    int DoSwap= T_DOSWAP(info ->InputFormat);
    int Reverse= T_FLAVOR(info ->InputFormat);
    int SwapEndian = T_ENDIAN16(info -> InputFormat);
    int i;
    cmsUInt8Number* Init = accum;

    if (DoSwap) {
        accum += T_EXTRA(info -> InputFormat) * Stride * sizeof(cmsUInt16Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;
        cmsUInt16Number v = *(cmsUInt16Number*) accum; 

        if (SwapEndian)
            v = CHANGE_ENDIAN(v);

        wIn[index] = Reverse ? REVERSE_FLAVOR_16(v) : v;

        accum +=  Stride * sizeof(cmsUInt16Number);           
    }

    return (Init + sizeof(cmsUInt16Number));
}


static
cmsUInt8Number* Unroll4Words(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wIn[], 
                             register cmsUInt8Number* accum,
                             register cmsUInt32Number Stride)
{
    wIn[0] = *(cmsUInt16Number*) accum; accum+= 2; // C
    wIn[1] = *(cmsUInt16Number*) accum; accum+= 2; // M
    wIn[2] = *(cmsUInt16Number*) accum; accum+= 2; // Y
    wIn[3] = *(cmsUInt16Number*) accum; accum+= 2; // K

    return accum;
}

static
cmsUInt8Number* Unroll4WordsReverse(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wIn[], 
                                    register cmsUInt8Number* accum,
                                    register cmsUInt32Number Stride)
{
    wIn[0] = REVERSE_FLAVOR_16(*(cmsUInt16Number*) accum); accum+= 2; // C
    wIn[1] = REVERSE_FLAVOR_16(*(cmsUInt16Number*) accum); accum+= 2; // M
    wIn[2] = REVERSE_FLAVOR_16(*(cmsUInt16Number*) accum); accum+= 2; // Y
    wIn[3] = REVERSE_FLAVOR_16(*(cmsUInt16Number*) accum); accum+= 2; // K

    return accum;
}

static
cmsUInt8Number* Unroll4WordsSwapFirst(register _cmsTRANSFORM* info, 
                                      register cmsUInt16Number wIn[], 
                                      register cmsUInt8Number* accum,
                                      register cmsUInt32Number Stride)
{
    wIn[3] = *(cmsUInt16Number*) accum; accum+= 2; // K
    wIn[0] = *(cmsUInt16Number*) accum; accum+= 2; // C
    wIn[1] = *(cmsUInt16Number*) accum; accum+= 2; // M
    wIn[2] = *(cmsUInt16Number*) accum; accum+= 2; // Y

    return accum;
}

// KYMC
static
cmsUInt8Number* Unroll4WordsSwap(register _cmsTRANSFORM* info, 
                                 register cmsUInt16Number wIn[], 
                                 register cmsUInt8Number* accum,
                                 register cmsUInt32Number Stride)
{
    wIn[3] = *(cmsUInt16Number*) accum; accum+= 2; // K
    wIn[2] = *(cmsUInt16Number*) accum; accum+= 2; // Y
    wIn[1] = *(cmsUInt16Number*) accum; accum+= 2; // M
    wIn[0] = *(cmsUInt16Number*) accum; accum+= 2; // C

    return accum;
}

static
cmsUInt8Number* Unroll4WordsSwapSwapFirst(register _cmsTRANSFORM* info, 
                                          register cmsUInt16Number wIn[], 
                                          register cmsUInt8Number* accum,
                                          register cmsUInt32Number Stride)
{
    wIn[2] = *(cmsUInt16Number*) accum; accum+= 2; // K
    wIn[1] = *(cmsUInt16Number*) accum; accum+= 2; // Y
    wIn[0] = *(cmsUInt16Number*) accum; accum+= 2; // M
    wIn[3] = *(cmsUInt16Number*) accum; accum+= 2; // C

    return accum;
}

static
cmsUInt8Number* Unroll3Words(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wIn[], 
                             register cmsUInt8Number* accum,
                             register cmsUInt32Number Stride)
{
    wIn[0] = *(cmsUInt16Number*) accum; accum+= 2;  // C R
    wIn[1] = *(cmsUInt16Number*) accum; accum+= 2;  // M G
    wIn[2] = *(cmsUInt16Number*) accum; accum+= 2;  // Y B
    return accum;
}

static
cmsUInt8Number* Unroll3WordsSwap(register _cmsTRANSFORM* info, 
                                 register cmsUInt16Number wIn[], 
                                 register cmsUInt8Number* accum,
                                 register cmsUInt32Number Stride)
{
    wIn[2] = *(cmsUInt16Number*) accum; accum+= 2;  // C R
    wIn[1] = *(cmsUInt16Number*) accum; accum+= 2;  // M G
    wIn[0] = *(cmsUInt16Number*) accum; accum+= 2;  // Y B
    return accum;
}

static
cmsUInt8Number* Unroll3WordsSkip1Swap(register _cmsTRANSFORM* info, 
                                      register cmsUInt16Number wIn[], 
                                      register cmsUInt8Number* accum,
                                      register cmsUInt32Number Stride)
{
    accum += 2; // A
    wIn[2] = *(cmsUInt16Number*) accum; accum += 2; // R
    wIn[1] = *(cmsUInt16Number*) accum; accum += 2; // G
    wIn[0] = *(cmsUInt16Number*) accum; accum += 2; // B

    return accum;
}

static
cmsUInt8Number* Unroll3WordsSkip1SwapFirst(register _cmsTRANSFORM* info, 
                                           register cmsUInt16Number wIn[], 
                                           register cmsUInt8Number* accum,
                                           register cmsUInt32Number Stride)
{
    accum += 2; // A
    wIn[0] = *(cmsUInt16Number*) accum; accum += 2; // R
    wIn[1] = *(cmsUInt16Number*) accum; accum += 2; // G
    wIn[2] = *(cmsUInt16Number*) accum; accum += 2; // B

    return accum;
}

static
cmsUInt8Number* Unroll1Word(register _cmsTRANSFORM* info, 
                            register cmsUInt16Number wIn[], 
                            register cmsUInt8Number* accum,
                            register cmsUInt32Number Stride)
{
    wIn[0] = wIn[1] = wIn[2] = *(cmsUInt16Number*) accum; accum+= 2;   // L
    return accum;
}

static
cmsUInt8Number* Unroll1WordReversed(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wIn[], 
                                    register cmsUInt8Number* accum,
                                    register cmsUInt32Number Stride)
{
    wIn[0] = wIn[1] = wIn[2] = REVERSE_FLAVOR_16(*(cmsUInt16Number*) accum); accum+= 2;
    return accum;
}

static
cmsUInt8Number* Unroll1WordSkip3(register _cmsTRANSFORM* info, 
                                 register cmsUInt16Number wIn[], 
                                 register cmsUInt8Number* accum,
                                 register cmsUInt32Number Stride)
{
    wIn[0] = wIn[1] = wIn[2] = *(cmsUInt16Number*) accum; 

    accum += 8;
    return accum;
}

static
cmsUInt8Number* Unroll2Words(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wIn[], 
                             register cmsUInt8Number* accum,
                             register cmsUInt32Number Stride)
{
    wIn[0] = *(cmsUInt16Number*) accum; accum += 2;    // ch1
    wIn[1] = *(cmsUInt16Number*) accum; accum += 2;    // ch2

    return accum;
}


// This is a conversion of Lab double to 16 bits
static
cmsUInt8Number* UnrollLabDoubleTo16(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wIn[], 
                                    register cmsUInt8Number* accum,
                                    register cmsUInt32Number  Stride)
{       
    if (T_PLANAR(info -> InputFormat)) {

        cmsFloat64Number* Pt = (cmsFloat64Number*) accum;

        cmsCIELab Lab;

        Lab.L = Pt[0];
        Lab.a = Pt[Stride];
        Lab.b = Pt[Stride*2];

        cmsFloat2LabEncoded(wIn, &Lab);
        return accum + sizeof(cmsFloat64Number);
    }
    else {

        cmsFloat2LabEncoded(wIn, (cmsCIELab*) accum);
        accum += sizeof(cmsCIELab) + T_EXTRA(info ->InputFormat) * sizeof(cmsFloat64Number);
        return accum;
    }
}

// This is a conversion of XYZ double to 16 bits
static
cmsUInt8Number* UnrollXYZDoubleTo16(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wIn[], 
                                    register cmsUInt8Number* accum,
                                    register cmsUInt32Number Stride)
{   
    if (T_PLANAR(info -> InputFormat)) {

        cmsFloat64Number* Pt = (cmsFloat64Number*) accum;
        cmsCIEXYZ XYZ;

        XYZ.X = Pt[0];
        XYZ.Y = Pt[Stride];
        XYZ.Z = Pt[Stride*2];
        cmsFloat2XYZEncoded(wIn, &XYZ);

        return accum + sizeof(cmsFloat64Number);

    }

    else {
        cmsFloat2XYZEncoded(wIn, (cmsCIEXYZ*) accum);
        accum += sizeof(cmsCIEXYZ) + T_EXTRA(info ->InputFormat) * sizeof(cmsFloat64Number);

        return accum;
    }
}

// Check if space is marked as ink
cmsINLINE cmsBool IsInkSpace(cmsUInt32Number Type)
{
    switch (T_COLORSPACE(Type)) {

     case PT_CMY:       
     case PT_CMYK:      
     case PT_MCH5:      
     case PT_MCH6:
     case PT_MCH7:     
     case PT_MCH8:     
     case PT_MCH9:     
     case PT_MCH10:    
     case PT_MCH11:    
     case PT_MCH12:    
     case PT_MCH13:    
     case PT_MCH14:    
     case PT_MCH15: return TRUE;

     default: return FALSE;
    }
}

// Inks does come in percentage, remaining cases are between 0..1.0, again to 16 bits
static
cmsUInt8Number* UnrollDoubleTo16(register _cmsTRANSFORM* info, 
                                 register cmsUInt16Number wIn[], 
                                 register cmsUInt8Number* accum,
                                 register cmsUInt32Number Stride)
{
    cmsFloat64Number* Inks = (cmsFloat64Number*) accum;
    int nChan  = T_CHANNELS(info -> InputFormat);
    int Planar = T_PLANAR(info -> InputFormat);
    int i;
    cmsFloat64Number v;
    cmsFloat64Number maximum = IsInkSpace(info ->InputFormat) ? 655.35 : 65535.0;

    for (i=0; i < nChan; i++) {

        if (Planar)

            v = Inks[i * Stride];
        else
            v = Inks[i];

        wIn[i] = _cmsQuickSaturateWord(v * maximum);
    }

    if (T_PLANAR(info -> InputFormat))
        return accum + sizeof(cmsFloat64Number);
    else
        return accum + (nChan + T_EXTRA(info ->InputFormat)) * sizeof(cmsFloat64Number);
}

static
cmsUInt8Number* UnrollFloatTo16(register _cmsTRANSFORM* info, 
                                register cmsUInt16Number wIn[], 
                                register cmsUInt8Number* accum,
                                register cmsUInt32Number Stride)
{
    cmsFloat32Number* Inks = (cmsFloat32Number*) accum;
    int nChan  = T_CHANNELS(info -> InputFormat);
    int Planar = T_PLANAR(info -> InputFormat);
    int i;
    cmsFloat32Number v;
    cmsFloat64Number maximum = IsInkSpace(info ->InputFormat) ? 655.35 : 65535.0;

    for (i=0; i < nChan; i++) {

        if (Planar)

            v = Inks[i * Stride];
        else
            v = Inks[i];

        wIn[i] = _cmsQuickSaturateWord(v * maximum);
    }

    if (T_PLANAR(info -> InputFormat))
        return accum + sizeof(cmsFloat32Number);
    else
        return accum + (nChan + T_EXTRA(info ->InputFormat)) * sizeof(cmsFloat32Number);
}


// For 1 channel, we need to duplicate data (it comes in 0..1.0 range)
static
cmsUInt8Number* UnrollDouble1Chan(register _cmsTRANSFORM* info, 
                                  register cmsUInt16Number wIn[], 
                                  register cmsUInt8Number* accum,
                                  register cmsUInt32Number Stride)
{
    cmsFloat64Number* Inks = (cmsFloat64Number*) accum;

    wIn[0] = wIn[1] = wIn[2] = _cmsQuickSaturateWord(Inks[0] * 65535.0);

    return accum + sizeof(cmsFloat64Number);    
}

//-------------------------------------------------------------------------------------------------------------------

// True float transformation. 

// For anything going from cmsFloat32Number 
static
cmsUInt8Number* UnrollFloatsToFloat(_cmsTRANSFORM* info, 
                                    cmsFloat32Number wIn[], 
                                    cmsUInt8Number* accum,
                                    cmsUInt32Number Stride)
{
    cmsFloat32Number* Inks = (cmsFloat32Number*) accum;
    int nChan  = T_CHANNELS(info -> InputFormat);
    int Planar = T_PLANAR(info -> InputFormat);
    int i;
    cmsFloat64Number maximum = IsInkSpace(info ->InputFormat) ? 100.0 : 1.0;


    for (i=0; i <  nChan; i++) {

        if (Planar)
            wIn[i] = (cmsFloat32Number) (Inks[i * Stride] / maximum);
        else
            wIn[i] = (cmsFloat32Number) (Inks[i] / maximum);                            
    }

    if (T_PLANAR(info -> InputFormat))
        return accum + sizeof(cmsFloat32Number);
    else
        return accum + (nChan + T_EXTRA(info ->InputFormat)) * sizeof(cmsFloat32Number);
}

// For anything going from double
static
cmsUInt8Number* UnrollDoublesToFloat(_cmsTRANSFORM* info, 
                                     cmsFloat32Number wIn[], 
                                     cmsUInt8Number* accum,
                                     cmsUInt32Number Stride)
{
    cmsFloat64Number* Inks = (cmsFloat64Number*) accum;
    int nChan  = T_CHANNELS(info -> InputFormat);
    int Planar = T_PLANAR(info -> InputFormat);
    int i;
    cmsFloat64Number maximum = IsInkSpace(info ->InputFormat) ? 100.0 : 1.0;

    for (i=0; i <  nChan; i++) {

        if (Planar)
            wIn[i] = (cmsFloat32Number) (Inks[i * Stride] / maximum);
        else
            wIn[i] = (cmsFloat32Number) (Inks[i] / maximum);                            
    }

    if (T_PLANAR(info -> InputFormat))
        return accum + sizeof(cmsFloat64Number);
    else
        return accum + (nChan + T_EXTRA(info ->InputFormat)) * sizeof(cmsFloat64Number);
}


// From Lab double to cmsFloat32Number
static
cmsUInt8Number* UnrollLabDoubleToFloat(_cmsTRANSFORM* info,  
                                       cmsFloat32Number wIn[], 
                                       cmsUInt8Number* accum,
                                       cmsUInt32Number Stride)
{  
    cmsFloat64Number* Pt = (cmsFloat64Number*) accum;

    if (T_PLANAR(info -> InputFormat)) {

        wIn[0] = (cmsFloat32Number) (Pt[0] / 100.0);                            // from 0..100 to 0..1 
        wIn[1] = (cmsFloat32Number) ((Pt[Stride] + 128) / 255.0);    // form -128..+127 to 0..1
        wIn[2] = (cmsFloat32Number) ((Pt[Stride*2] + 128) / 255.0);

        return accum + sizeof(cmsFloat64Number);
    }
    else {

        wIn[0] = (cmsFloat32Number) (Pt[0] / 100.0);             // from 0..100 to 0..1 
        wIn[1] = (cmsFloat32Number) ((Pt[1] + 128) / 255.0);    // form -128..+127 to 0..1
        wIn[2] = (cmsFloat32Number) ((Pt[2] + 128) / 255.0);

        accum += sizeof(cmsFloat64Number)*(3 + T_EXTRA(info ->InputFormat));
        return accum;
    }
}

// From Lab double to cmsFloat32Number
static
cmsUInt8Number* UnrollLabFloatToFloat(_cmsTRANSFORM* info, 
                                      cmsFloat32Number wIn[], 
                                      cmsUInt8Number* accum,
                                      cmsUInt32Number Stride)
{  
    cmsFloat32Number* Pt = (cmsFloat32Number*) accum;

    if (T_PLANAR(info -> InputFormat)) {

        wIn[0] = (cmsFloat32Number) (Pt[0] / 100.0);                            // from 0..100 to 0..1 
        wIn[1] = (cmsFloat32Number) ((Pt[Stride] + 128) / 255.0);    // form -128..+127 to 0..1
        wIn[2] = (cmsFloat32Number) ((Pt[Stride*2] + 128) / 255.0);

        return accum + sizeof(cmsFloat32Number);
    }
    else {

        wIn[0] = (cmsFloat32Number) (Pt[0] / 100.0);             // from 0..100 to 0..1 
        wIn[1] = (cmsFloat32Number) ((Pt[1] + 128) / 255.0);    // form -128..+127 to 0..1
        wIn[2] = (cmsFloat32Number) ((Pt[2] + 128) / 255.0);

        accum += sizeof(cmsFloat32Number)*(3 + T_EXTRA(info ->InputFormat));
        return accum;
    }
}


// 1.15 fixed point, that means maximum value is MAX_ENCODEABLE_XYZ (0xFFFF)
static
cmsUInt8Number* UnrollXYZDoubleToFloat(_cmsTRANSFORM* info,  
                                       cmsFloat32Number wIn[], 
                                       cmsUInt8Number* accum,
                                       cmsUInt32Number Stride)
{  
    cmsFloat64Number* Pt = (cmsFloat64Number*) accum;

    if (T_PLANAR(info -> InputFormat)) {

        wIn[0] = (cmsFloat32Number) (Pt[0] / MAX_ENCODEABLE_XYZ);                   
        wIn[1] = (cmsFloat32Number) (Pt[Stride] / MAX_ENCODEABLE_XYZ);    
        wIn[2] = (cmsFloat32Number) (Pt[Stride*2] / MAX_ENCODEABLE_XYZ);

        return accum + sizeof(cmsFloat64Number);
    }
    else {

        wIn[0] = (cmsFloat32Number) (Pt[0] / MAX_ENCODEABLE_XYZ);            
        wIn[1] = (cmsFloat32Number) (Pt[1] / MAX_ENCODEABLE_XYZ);    
        wIn[2] = (cmsFloat32Number) (Pt[2] / MAX_ENCODEABLE_XYZ);

        accum += sizeof(cmsFloat64Number)*(3 + T_EXTRA(info ->InputFormat));
        return accum;
    }
}

static
cmsUInt8Number* UnrollXYZFloatToFloat(_cmsTRANSFORM* info,  
                                      cmsFloat32Number wIn[], 
                                      cmsUInt8Number* accum,
                                      cmsUInt32Number Stride)
{  
    cmsFloat32Number* Pt = (cmsFloat32Number*) accum;

    if (T_PLANAR(info -> InputFormat)) {

        wIn[0] = (cmsFloat32Number) (Pt[0] / MAX_ENCODEABLE_XYZ);                   
        wIn[1] = (cmsFloat32Number) (Pt[Stride] / MAX_ENCODEABLE_XYZ);    
        wIn[2] = (cmsFloat32Number) (Pt[Stride*2] / MAX_ENCODEABLE_XYZ);

        return accum + sizeof(cmsFloat32Number);
    }
    else {

        wIn[0] = (cmsFloat32Number) (Pt[0] / MAX_ENCODEABLE_XYZ);            
        wIn[1] = (cmsFloat32Number) (Pt[1] / MAX_ENCODEABLE_XYZ);    
        wIn[2] = (cmsFloat32Number) (Pt[2] / MAX_ENCODEABLE_XYZ);

        accum += sizeof(cmsFloat32Number)*(3 + T_EXTRA(info ->InputFormat));
        return accum;
    }
}

// Packing routines -----------------------------------------------------------------------------------------------------------


// Generic chunky for byte

static
cmsUInt8Number* PackAnyBytes(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wOut[], 
                             register cmsUInt8Number* output,
                             register cmsUInt32Number Stride)
{
    int nChan      = T_CHANNELS(info -> OutputFormat);
    int DoSwap     = T_DOSWAP(info ->OutputFormat);
    int Reverse    = T_FLAVOR(info ->OutputFormat);
    int Extra      = T_EXTRA(info -> OutputFormat);
    int SwapFirst  = T_SWAPFIRST(info -> OutputFormat);
    int ExtraFirst = DoSwap && !SwapFirst;
    cmsUInt8Number* swap1;
    cmsUInt8Number v = 0;
    int i;

    swap1 = output;

    if (ExtraFirst) {
        output += Extra;
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;

        v = FROM_16_TO_8(wOut[index]);

        if (Reverse)
            v = REVERSE_FLAVOR_8(v);

        *output++ = v;
    }

    if (!ExtraFirst) {
        output += Extra;
    }

    if (Extra == 0 && SwapFirst) {

        memmove(swap1 + 1, swap1, nChan-1);
        *swap1 = v;
    }


    return output;
}



static
cmsUInt8Number* PackAnyWords(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wOut[], 
                             register cmsUInt8Number* output,
                             register cmsUInt32Number Stride)
{
    int nChan      = T_CHANNELS(info -> OutputFormat);
    int SwapEndian = T_ENDIAN16(info -> InputFormat);
    int DoSwap     = T_DOSWAP(info ->OutputFormat);
    int Reverse    = T_FLAVOR(info ->OutputFormat);
    int Extra      = T_EXTRA(info -> OutputFormat);
    int SwapFirst  = T_SWAPFIRST(info -> OutputFormat);
    int ExtraFirst = DoSwap && !SwapFirst;
    cmsUInt16Number* swap1;
    cmsUInt16Number v = 0;
    int i;

    swap1 = (cmsUInt16Number*) output;

    if (ExtraFirst) {
        output += Extra * sizeof(cmsUInt16Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;

        v = wOut[index];

        if (SwapEndian)
            v = CHANGE_ENDIAN(v);

        if (Reverse)
            v = REVERSE_FLAVOR_16(v);

        *(cmsUInt16Number*) output = v;

        output += sizeof(cmsUInt16Number);
    }

    if (!ExtraFirst) {
        output += Extra * sizeof(cmsUInt16Number);
    }

    if (Extra == 0 && SwapFirst) {

        memmove(swap1 + 1, swap1, (nChan-1)* sizeof(cmsUInt16Number));
        *swap1 = v;
    }


    return output;
}


static
cmsUInt8Number* PackPlanarBytes(register _cmsTRANSFORM* info, 
                                register cmsUInt16Number wOut[], 
                                register cmsUInt8Number* output,
                                register cmsUInt32Number Stride)
{
    int nChan = T_CHANNELS(info -> OutputFormat);
    int DoSwap = T_DOSWAP(info ->OutputFormat);
    int Reverse= T_FLAVOR(info ->OutputFormat);
    int i;
    cmsUInt8Number* Init = output;

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;
        cmsUInt8Number v = FROM_16_TO_8(wOut[index]);

        *(cmsUInt8Number*)  output = (cmsUInt8Number) (Reverse ? REVERSE_FLAVOR_8(v) : v);
        output += Stride;
    }

    return (Init + 1);
}


static
cmsUInt8Number* PackPlanarWords(register _cmsTRANSFORM* info, 
                                register cmsUInt16Number wOut[], 
                                register cmsUInt8Number* output,
                                register cmsUInt32Number Stride)
{
    int nChan = T_CHANNELS(info -> OutputFormat);
    int DoSwap = T_DOSWAP(info ->OutputFormat);
    int Reverse= T_FLAVOR(info ->OutputFormat);
    int SwapEndian = T_ENDIAN16(info -> OutputFormat);
    int i;
    cmsUInt8Number* Init = output;
    cmsUInt16Number v;

    if (DoSwap) {
        output += T_EXTRA(info -> OutputFormat) * Stride * sizeof(cmsUInt16Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;

        v = wOut[index];

        if (SwapEndian)
            v = CHANGE_ENDIAN(v);

        if (Reverse) 
            v =  REVERSE_FLAVOR_16(v);

        *(cmsUInt16Number*) output = v;
        output += (Stride * sizeof(cmsUInt16Number));
    }

    return (Init + sizeof(cmsUInt16Number));
}

// CMYKcm (unrolled for speed)

static
cmsUInt8Number* Pack6Bytes(register _cmsTRANSFORM* info, 
                           register cmsUInt16Number wOut[], 
                           register cmsUInt8Number* output,
                           register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[0]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[3]);
    *output++ = FROM_16_TO_8(wOut[4]);
    *output++ = FROM_16_TO_8(wOut[5]);

    return output;
}

// KCMYcm

static
cmsUInt8Number* Pack6BytesSwap(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[5]);
    *output++ = FROM_16_TO_8(wOut[4]);
    *output++ = FROM_16_TO_8(wOut[3]);
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[0]);

    return output;
}

// CMYKcm
static
cmsUInt8Number* Pack6Words(register _cmsTRANSFORM* info, 
                           register cmsUInt16Number wOut[], 
                           register cmsUInt8Number* output,
                           register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[3];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[4];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[5];
    output+= 2;

    return output;
}

// KCMYcm
static
cmsUInt8Number* Pack6WordsSwap(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[5];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[4];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[3];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;

    return output;
}


static
cmsUInt8Number* Pack4Bytes(register _cmsTRANSFORM* info, 
                           register cmsUInt16Number wOut[], 
                           register cmsUInt8Number* output,
                           register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[0]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[3]);

    return output;
}

static
cmsUInt8Number* Pack4BytesReverse(register _cmsTRANSFORM* info, 
                                  register cmsUInt16Number wOut[], 
                                  register cmsUInt8Number* output,
                                  register cmsUInt32Number Stride)
{
    *output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[0]));
    *output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[1]));
    *output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[2]));
    *output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[3]));

    return output;
}


static
cmsUInt8Number* Pack4BytesSwapFirst(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wOut[], 
                                    register cmsUInt8Number* output,
                                    register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[3]);
    *output++ = FROM_16_TO_8(wOut[0]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[2]);

    return output;
}

// ABGR
static
cmsUInt8Number* Pack4BytesSwap(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[3]);
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[0]);

    return output;
}

static
cmsUInt8Number* Pack4BytesSwapSwapFirst(register _cmsTRANSFORM* info, 
                                        register cmsUInt16Number wOut[], 
                                        register cmsUInt8Number* output,
                                        register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[0]);
    *output++ = FROM_16_TO_8(wOut[3]);

    return output;
}

static
cmsUInt8Number* Pack4Words(register _cmsTRANSFORM* info, 
                           register cmsUInt16Number wOut[], 
                           register cmsUInt8Number* output,
                           register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[3];
    output+= 2;

    return output;
}

static
cmsUInt8Number* Pack4WordsReverse(register _cmsTRANSFORM* info, 
                                  register cmsUInt16Number wOut[], 
                                  register cmsUInt8Number* output,
                                  register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = REVERSE_FLAVOR_16(wOut[0]);
    output+= 2;
    *(cmsUInt16Number*) output = REVERSE_FLAVOR_16(wOut[1]);
    output+= 2;
    *(cmsUInt16Number*) output = REVERSE_FLAVOR_16(wOut[2]);
    output+= 2;
    *(cmsUInt16Number*) output = REVERSE_FLAVOR_16(wOut[3]);
    output+= 2;

    return output;
}

// ABGR
static
cmsUInt8Number* Pack4WordsSwap(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[3];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;

    return output;
}

// CMYK
static
cmsUInt8Number* Pack4WordsBigEndian(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wOut[], 
                                    register cmsUInt8Number* output,
                                    register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[0]);
    output+= 2;
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[1]);
    output+= 2;
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[2]);
    output+= 2;
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[3]);
    output+= 2;

    return output;
}


static
cmsUInt8Number* PackLabV2_8(register _cmsTRANSFORM* info, 
                            register cmsUInt16Number wOut[], 
                            register cmsUInt8Number* output,
                            register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[0]));
    *output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[1]));
    *output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[2]));

    return output;
}

static
cmsUInt8Number* PackALabV2_8(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wOut[], 
                             register cmsUInt8Number* output,
                             register cmsUInt32Number Stride)
{
    output++;
    *output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[0]));
    *output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[1]));
    *output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[2]));

    return output;
}

static
cmsUInt8Number* PackLabV2_16(register _cmsTRANSFORM* info, 
                             register cmsUInt16Number wOut[], 
                             register cmsUInt8Number* output,
                             register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = FomLabV4ToLabV2(wOut[0]);
    output += 2;
    *(cmsUInt16Number*) output = FomLabV4ToLabV2(wOut[1]);
    output += 2;
    *(cmsUInt16Number*) output = FomLabV4ToLabV2(wOut[2]);
    output += 2;

    return output;
}

static
cmsUInt8Number* Pack3Bytes(register _cmsTRANSFORM* info, 
                           register cmsUInt16Number wOut[], 
                           register cmsUInt8Number* output,
                           register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[0]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[2]);

    return output;
}

static
cmsUInt8Number* Pack3BytesOptimized(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wOut[], 
                                    register cmsUInt8Number* output,
                                    register cmsUInt32Number Stride)
{
    *output++ = (wOut[0] & 0xFF);
    *output++ = (wOut[1] & 0xFF);
    *output++ = (wOut[2] & 0xFF);

    return output;
}

static
cmsUInt8Number* Pack3BytesSwap(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[0]);

    return output;
}

static
cmsUInt8Number* Pack3BytesSwapOptimized(register _cmsTRANSFORM* info, 
                                        register cmsUInt16Number wOut[], 
                                        register cmsUInt8Number* output,
                                        register cmsUInt32Number Stride)
{
    *output++ = (wOut[2] & 0xFF);
    *output++ = (wOut[1] & 0xFF);
    *output++ = (wOut[0] & 0xFF);

    return output;
}


static
cmsUInt8Number* Pack3Words(register _cmsTRANSFORM* info, 
                           register cmsUInt16Number wOut[], 
                           register cmsUInt8Number* output,
                           register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;

    return output;
}

static
cmsUInt8Number* Pack3WordsSwap(register _cmsTRANSFORM* info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;

    return output;
}

static
cmsUInt8Number* Pack3WordsBigEndian(register _cmsTRANSFORM* info, 
                                    register cmsUInt16Number wOut[], 
                                    register cmsUInt8Number* output,
                                    register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[0]);
    output+= 2;
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[1]);
    output+= 2;
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[2]);
    output+= 2;

    return output;
}

static
cmsUInt8Number* Pack3BytesAndSkip1(register _cmsTRANSFORM* Info, 
                                   register cmsUInt16Number wOut[], 
                                   register cmsUInt8Number* output,
                                   register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[0]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[2]);
    output++;

    return output;
}

static
cmsUInt8Number* Pack3BytesAndSkip1Optimized(register _cmsTRANSFORM* Info, 
                                            register cmsUInt16Number wOut[], 
                                            register cmsUInt8Number* output,
                                            register cmsUInt32Number Stride)
{
    *output++ = (wOut[0] & 0xFF);
    *output++ = (wOut[1] & 0xFF);
    *output++ = (wOut[2] & 0xFF);
    output++;

    return output;
}


static
cmsUInt8Number* Pack3BytesAndSkip1SwapFirst(register _cmsTRANSFORM* Info, 
                                            register cmsUInt16Number wOut[], 
                                            register cmsUInt8Number* output,
                                            register cmsUInt32Number Stride)
{
    output++;
    *output++ = FROM_16_TO_8(wOut[0]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[2]);

    return output;
}

static
cmsUInt8Number* Pack3BytesAndSkip1SwapFirstOptimized(register _cmsTRANSFORM* Info, 
                                                     register cmsUInt16Number wOut[], 
                                                     register cmsUInt8Number* output,
                                                     register cmsUInt32Number Stride)
{
    output++;
    *output++ = (wOut[0] & 0xFF);
    *output++ = (wOut[1] & 0xFF);
    *output++ = (wOut[2] & 0xFF);

    return output;
}

static
cmsUInt8Number* Pack3BytesAndSkip1Swap(register _cmsTRANSFORM* Info, 
                                       register cmsUInt16Number wOut[], 
                                       register cmsUInt8Number* output,
                                       register cmsUInt32Number Stride)
{
    output++;
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[0]);

    return output;
}

static
cmsUInt8Number* Pack3BytesAndSkip1SwapOptimized(register _cmsTRANSFORM* Info, 
                                                register cmsUInt16Number wOut[], 
                                                register cmsUInt8Number* output,
                                                register cmsUInt32Number Stride)
{
    output++;
    *output++ = (wOut[2] & 0xFF);
    *output++ = (wOut[1] & 0xFF);
    *output++ = (wOut[0] & 0xFF);

    return output;
}


static
cmsUInt8Number* Pack3BytesAndSkip1SwapSwapFirst(register _cmsTRANSFORM* Info, 
                                                register cmsUInt16Number wOut[], 
                                                register cmsUInt8Number* output,
                                                register cmsUInt32Number Stride)
{       
    *output++ = FROM_16_TO_8(wOut[2]);
    *output++ = FROM_16_TO_8(wOut[1]);
    *output++ = FROM_16_TO_8(wOut[0]);
    output++;

    return output;
}

static
cmsUInt8Number* Pack3BytesAndSkip1SwapSwapFirstOptimized(register _cmsTRANSFORM* Info, 
                                                         register cmsUInt16Number wOut[], 
                                                         register cmsUInt8Number* output,
                                                         register cmsUInt32Number Stride)
{       
    *output++ = (wOut[2] & 0xFF);
    *output++ = (wOut[1] & 0xFF);
    *output++ = (wOut[0] & 0xFF);
    output++;

    return output;
}

static
cmsUInt8Number* Pack3WordsAndSkip1(register _cmsTRANSFORM* Info, 
                                   register cmsUInt16Number wOut[], 
                                   register cmsUInt8Number* output,
                                   register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    output+= 2;

    return output;
}

static
cmsUInt8Number* Pack3WordsAndSkip1Swap(register _cmsTRANSFORM* Info, 
                                       register cmsUInt16Number wOut[], 
                                       register cmsUInt8Number* output,
                                       register cmsUInt32Number Stride)
{
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;

    return output;
}


static
cmsUInt8Number* Pack3WordsAndSkip1SwapFirst(register _cmsTRANSFORM* Info, 
                                            register cmsUInt16Number wOut[], 
                                            register cmsUInt8Number* output,
                                            register cmsUInt32Number Stride)
{   
    output+= 2;
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;

    return output;
}


static
cmsUInt8Number* Pack3WordsAndSkip1SwapSwapFirst(register _cmsTRANSFORM* Info, 
                                                register cmsUInt16Number wOut[], 
                                                register cmsUInt8Number* output,
                                                register cmsUInt32Number Stride)
{      
    *(cmsUInt16Number*) output = wOut[2];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[1];
    output+= 2;
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;
    output+= 2;

    return output;
}



static
cmsUInt8Number* Pack1Byte(register _cmsTRANSFORM* Info, 
                          register cmsUInt16Number wOut[], 
                          register cmsUInt8Number* output,
                          register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[0]);
    return output;
}


static
cmsUInt8Number* Pack1ByteReversed(register _cmsTRANSFORM* Info, 
                                  register cmsUInt16Number wOut[], 
                                  register cmsUInt8Number* output,
                                  register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(REVERSE_FLAVOR_16(wOut[0]));
    return output;
}


static
cmsUInt8Number* Pack1ByteSkip1(register _cmsTRANSFORM* Info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *output++ = FROM_16_TO_8(wOut[0]);
    output++;
    return output;
}


static
cmsUInt8Number* Pack1ByteSkip1SwapFirst(register _cmsTRANSFORM* Info, 
                                        register cmsUInt16Number wOut[], 
                                        register cmsUInt8Number* output,
                                        register cmsUInt32Number Stride)
{
    output++;
    *output++ = FROM_16_TO_8(wOut[0]);

    return output;
}

static
cmsUInt8Number* Pack1Word(register _cmsTRANSFORM* Info, 
                          register cmsUInt16Number wOut[], 
                          register cmsUInt8Number* output,
                          register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;

    return output;
}


static
cmsUInt8Number* Pack1WordReversed(register _cmsTRANSFORM* Info, 
                                  register cmsUInt16Number wOut[], 
                                  register cmsUInt8Number* output,
                                  register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = REVERSE_FLAVOR_16(wOut[0]);
    output+= 2;

    return output;
}

static
cmsUInt8Number* Pack1WordBigEndian(register _cmsTRANSFORM* Info, 
                                   register cmsUInt16Number wOut[], 
                                   register cmsUInt8Number* output,
                                   register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = CHANGE_ENDIAN(wOut[0]);
    output+= 2;

    return output;
}


static
cmsUInt8Number* Pack1WordSkip1(register _cmsTRANSFORM* Info, 
                               register cmsUInt16Number wOut[], 
                               register cmsUInt8Number* output,
                               register cmsUInt32Number Stride)
{
    *(cmsUInt16Number*) output = wOut[0];
    output+= 4;

    return output;
}

static
cmsUInt8Number* Pack1WordSkip1SwapFirst(register _cmsTRANSFORM* Info, 
                                        register cmsUInt16Number wOut[], 
                                        register cmsUInt8Number* output,
                                        register cmsUInt32Number Stride)
{
    output += 2; 
    *(cmsUInt16Number*) output = wOut[0];
    output+= 2;

    return output;
}


// Unencoded Float values -- don't try optimize speed
static
cmsUInt8Number* PackLabDoubleFrom16(register _cmsTRANSFORM* Info, 
                                    register cmsUInt16Number wOut[], 
                                    register cmsUInt8Number* output, 
                                    register cmsUInt32Number Stride)
{

    if (T_PLANAR(Info -> OutputFormat)) {

        cmsCIELab  Lab;
        cmsFloat64Number* Out = (cmsFloat64Number*) output;
        cmsLabEncoded2Float(&Lab, wOut);

        Out[0]        = Lab.L;
        Out[Stride]   = Lab.a;
        Out[Stride*2] = Lab.b;

        return output + sizeof(cmsFloat64Number);
    }
    else {

        cmsLabEncoded2Float((cmsCIELab*) output, wOut);
        return output + (sizeof(cmsCIELab) + T_EXTRA(Info ->OutputFormat) * sizeof(cmsFloat64Number));            
    }

}

static
cmsUInt8Number* PackXYZDoubleFrom16(register _cmsTRANSFORM* Info, 
                                    register cmsUInt16Number wOut[], 
                                    register cmsUInt8Number* output,
                                    register cmsUInt32Number Stride)
{
    if (T_PLANAR(Info -> OutputFormat)) {

        cmsCIEXYZ XYZ;
        cmsFloat64Number* Out = (cmsFloat64Number*) output;
        cmsXYZEncoded2Float(&XYZ, wOut);

        Out[0]                  = XYZ.X;
        Out[Stride]   = XYZ.Y;
        Out[Stride*2] = XYZ.Z;

        return output + sizeof(cmsFloat64Number);

    }
    else {

        cmsXYZEncoded2Float((cmsCIEXYZ*) output, wOut);

        return output + (sizeof(cmsCIEXYZ) + T_EXTRA(Info ->OutputFormat) * sizeof(cmsFloat64Number));
    }
}

static
cmsUInt8Number* PackDoubleFrom16(register _cmsTRANSFORM* Info, 
                                 register cmsUInt16Number wOut[], 
                                 register cmsUInt8Number* output,
                                 register cmsUInt32Number Stride)
{
    cmsFloat64Number* Inks = (cmsFloat64Number*) output;
    int nChan = T_CHANNELS(Info -> OutputFormat);
    int i;
    cmsFloat64Number maximum = IsInkSpace(Info ->InputFormat) ? 655.35 : 65535.0;

    if (T_PLANAR(Info -> OutputFormat)) {

        for (i=0; i <  nChan; i++) {

            Inks[i*Stride] = wOut[i] / maximum;
        }

        return output + sizeof(cmsFloat64Number);
    } 
    else {

        for (i=0; i <  nChan; i++) {

            Inks[i] = wOut[i] /  maximum;
        }


        return output + (nChan + T_EXTRA(Info ->OutputFormat)) * sizeof(cmsFloat64Number);
    }

}

static
cmsUInt8Number* PackFloatFrom16(register _cmsTRANSFORM* Info, 
                                register cmsUInt16Number wOut[], 
                                register cmsUInt8Number* output,
                                register cmsUInt32Number Stride)
{
    cmsFloat32Number* Inks = (cmsFloat32Number*) output;
    int nChan = T_CHANNELS(Info -> OutputFormat);
    int i;
    cmsFloat64Number maximum = IsInkSpace(Info ->OutputFormat) ? 655.35 : 65535.0;

    if (T_PLANAR(Info -> OutputFormat)) {

        for (i=0; i <  nChan; i++) {

            Inks[i*Stride] = (cmsFloat32Number) (wOut[i] / maximum);
        }

        return output + sizeof(cmsFloat32Number);
    } 
    else {

        for (i=0; i <  nChan; i++) {

            Inks[i] = (cmsFloat32Number) (wOut[i] /  maximum);
        }


        return output + (nChan + T_EXTRA(Info ->OutputFormat)) * sizeof(cmsFloat32Number);
    }

}


// --------------------------------------------------------------------------------------------------------

static
cmsUInt8Number* PackChunkyFloatsFromFloat(_cmsTRANSFORM* info, 
                                          cmsFloat32Number wOut[], 
                                          cmsUInt8Number* output,
                                          cmsUInt32Number Stride)
{
    int nChan      = T_CHANNELS(info -> OutputFormat);
    int DoSwap     = T_DOSWAP(info ->OutputFormat);
    int Reverse    = T_FLAVOR(info ->OutputFormat);
    int Extra      = T_EXTRA(info -> OutputFormat);
    int SwapFirst  = T_SWAPFIRST(info -> OutputFormat);
    int ExtraFirst = DoSwap && !SwapFirst;
    cmsFloat64Number maximum = IsInkSpace(info ->OutputFormat) ? 100.0 : 1.0;
    cmsFloat32Number* swap1;
    cmsFloat64Number v = 0;
    int i;

    swap1 = (cmsFloat32Number*) output;

    if (ExtraFirst) {
        output += Extra * sizeof(cmsFloat32Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;

        v = wOut[index] * maximum;

        if (Reverse)
            v = maximum - v;

        *(cmsFloat32Number*) output = (cmsFloat32Number) v;

        output += sizeof(cmsFloat32Number);
    }

    if (!ExtraFirst) {
        output += Extra * sizeof(cmsFloat32Number);
    }

    if (Extra == 0 && SwapFirst) {

        memmove(swap1 + 1, swap1, (nChan-1)* sizeof(cmsFloat32Number));
        *swap1 = (cmsFloat32Number) v;
    }


    return output;
}

static
cmsUInt8Number* PackPlanarFloatsFromFloat(_cmsTRANSFORM* info, 
                                          cmsFloat32Number wOut[], 
                                          cmsUInt8Number* output,
                                          cmsUInt32Number Stride)
{
    int nChan = T_CHANNELS(info -> OutputFormat);
    int DoSwap = T_DOSWAP(info ->OutputFormat);
    int Reverse= T_FLAVOR(info ->OutputFormat);
    int i;
    cmsUInt8Number* Init = output;
    cmsFloat64Number maximum = IsInkSpace(info ->OutputFormat) ? 100.0 : 1.0;
    cmsFloat64Number v;

    if (DoSwap) {
        output += T_EXTRA(info -> OutputFormat) * Stride * sizeof(cmsFloat32Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;

        v = wOut[index] * maximum;

        if (Reverse) 
            v =  maximum - v;

        *(cmsFloat32Number*) output = (cmsFloat32Number) v;
        output += (Stride * sizeof(cmsFloat32Number));
    }

    return (Init + sizeof(cmsFloat32Number));
}


static
cmsUInt8Number* PackChunkyDoublesFromFloat(_cmsTRANSFORM* info, 
                                           cmsFloat32Number wOut[], 
                                           cmsUInt8Number* output,
                                           cmsUInt32Number Stride)
{
    int nChan      = T_CHANNELS(info -> OutputFormat);
    int DoSwap     = T_DOSWAP(info ->OutputFormat);
    int Reverse    = T_FLAVOR(info ->OutputFormat);
    int Extra      = T_EXTRA(info -> OutputFormat);
    int SwapFirst  = T_SWAPFIRST(info -> OutputFormat);
    int ExtraFirst = DoSwap && !SwapFirst;
    cmsFloat64Number* swap1;
    cmsFloat64Number maximum = IsInkSpace(info ->OutputFormat) ? 100.0 : 1.0;
    cmsFloat64Number v = 0;
    int i;

    swap1 = (cmsFloat64Number*) output;

    if (ExtraFirst) {
        output += Extra * sizeof(cmsFloat64Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;

        v = (cmsFloat64Number) wOut[index] * maximum;

        if (Reverse)
            v = maximum - v;

        *(cmsFloat64Number*) output = v;

        output += sizeof(cmsFloat64Number);
    }

    if (!ExtraFirst) {
        output += Extra * sizeof(cmsFloat64Number);
    }

    if (Extra == 0 && SwapFirst) {

        memmove(swap1 + 1, swap1, (nChan-1)* sizeof(cmsFloat64Number));
        *swap1 = v;
    }


    return output;
}

static
cmsUInt8Number* PackPlanarDoublesFromFloat(_cmsTRANSFORM* info, 
                                           cmsFloat32Number wOut[], 
                                           cmsUInt8Number* output,
                                           cmsUInt32Number Stride)
{
    int nChan = T_CHANNELS(info -> OutputFormat);
    int DoSwap = T_DOSWAP(info ->OutputFormat);
    int Reverse= T_FLAVOR(info ->OutputFormat);
    int i;
    cmsUInt8Number* Init = output;
    cmsFloat64Number maximum = IsInkSpace(info ->OutputFormat) ? 100.0 : 1.0;
    cmsFloat64Number v;

    if (DoSwap) {
        output += T_EXTRA(info -> OutputFormat) * Stride * sizeof(cmsFloat64Number);
    }

    for (i=0; i < nChan; i++) {

        int index = DoSwap ? (nChan - i - 1) : i;

        v = (cmsFloat64Number) wOut[index] * maximum;

        if (Reverse) 
            v =  maximum - v;

        *(cmsFloat64Number*) output = v;
        output += (Stride * sizeof(cmsFloat64Number));
    }

    return (Init + sizeof(cmsFloat64Number));
}




static
cmsUInt8Number* PackLabFloatFromFloat(_cmsTRANSFORM* Info, 
                                      cmsFloat32Number wOut[], 
                                      cmsUInt8Number* output,
                                      cmsUInt32Number Stride)
{        
    cmsFloat32Number* Out = (cmsFloat32Number*) output;

    if (T_PLANAR(Info -> OutputFormat)) {

        Out[0]        = (cmsFloat32Number) (wOut[0] * 100.0);
        Out[Stride]   = (cmsFloat32Number) (wOut[1] * 255.0 - 128.0);
        Out[Stride*2] = (cmsFloat32Number) (wOut[2] * 255.0 - 128.0);

        return output + sizeof(cmsFloat32Number);
    }
    else {

        Out[0] = (cmsFloat32Number) (wOut[0] * 100.0);
        Out[1] = (cmsFloat32Number) (wOut[1] * 255.0 - 128.0);
        Out[2] = (cmsFloat32Number) (wOut[2] * 255.0 - 128.0);

        return output + (sizeof(cmsFloat32Number)*3 + T_EXTRA(Info ->OutputFormat) * sizeof(cmsFloat32Number));            
    }

}

static
cmsUInt8Number* PackLabDoubleFromFloat(_cmsTRANSFORM* Info, 
                                       cmsFloat32Number wOut[], 
                                       cmsUInt8Number* output,
                                       cmsUInt32Number Stride)
{        
    cmsFloat64Number* Out = (cmsFloat64Number*) output;

    if (T_PLANAR(Info -> OutputFormat)) {

        Out[0]        = (cmsFloat64Number) (wOut[0] * 100.0);
        Out[Stride]   = (cmsFloat64Number) (wOut[1] * 255.0 - 128.0);
        Out[Stride*2] = (cmsFloat64Number) (wOut[2] * 255.0 - 128.0);

        return output + sizeof(cmsFloat64Number);
    }
    else {

        Out[0] = (cmsFloat64Number) (wOut[0] * 100.0);
        Out[1] = (cmsFloat64Number) (wOut[1] * 255.0 - 128.0);
        Out[2] = (cmsFloat64Number) (wOut[2] * 255.0 - 128.0);

        return output + (sizeof(cmsFloat64Number)*3 + T_EXTRA(Info ->OutputFormat) * sizeof(cmsFloat64Number));            
    }

}


// From 0..1 range to 0..MAX_ENCODEABLE_XYZ
static
cmsUInt8Number* PackXYZFloatFromFloat(_cmsTRANSFORM* Info, 
                                      cmsFloat32Number wOut[], 
                                      cmsUInt8Number* output,
                                      cmsUInt32Number Stride)
{        
    cmsFloat32Number* Out = (cmsFloat32Number*) output;

    if (T_PLANAR(Info -> OutputFormat)) {

        Out[0]        = (cmsFloat32Number) (wOut[0] * MAX_ENCODEABLE_XYZ);
        Out[Stride]   = (cmsFloat32Number) (wOut[1] * MAX_ENCODEABLE_XYZ);
        Out[Stride*2] = (cmsFloat32Number) (wOut[2] * MAX_ENCODEABLE_XYZ);

        return output + sizeof(cmsFloat32Number);
    }
    else {

        Out[0] = (cmsFloat32Number) (wOut[0] * MAX_ENCODEABLE_XYZ);
        Out[1] = (cmsFloat32Number) (wOut[1] * MAX_ENCODEABLE_XYZ);
        Out[2] = (cmsFloat32Number) (wOut[2] * MAX_ENCODEABLE_XYZ);

        return output + (sizeof(cmsFloat32Number)*3 + T_EXTRA(Info ->OutputFormat) * sizeof(cmsFloat32Number));            
    }

}


// Same, but convert to double
static
cmsUInt8Number* PackXYZDoubleFromFloat(_cmsTRANSFORM* Info, 
                                       cmsFloat32Number wOut[], 
                                       cmsUInt8Number* output,
                                       cmsUInt32Number Stride)
{        
    cmsFloat64Number* Out = (cmsFloat64Number*) output;

    if (T_PLANAR(Info -> OutputFormat)) {

        Out[0]        = (cmsFloat64Number) (wOut[0] * MAX_ENCODEABLE_XYZ);
        Out[Stride]   = (cmsFloat64Number) (wOut[1] * MAX_ENCODEABLE_XYZ);
        Out[Stride*2] = (cmsFloat64Number) (wOut[2] * MAX_ENCODEABLE_XYZ);

        return output + sizeof(cmsFloat64Number);
    }
    else {

        Out[0] = (cmsFloat64Number) (wOut[0] * MAX_ENCODEABLE_XYZ);
        Out[1] = (cmsFloat64Number) (wOut[1] * MAX_ENCODEABLE_XYZ);
        Out[2] = (cmsFloat64Number) (wOut[2] * MAX_ENCODEABLE_XYZ);

        return output + (sizeof(cmsFloat64Number)*3 + T_EXTRA(Info ->OutputFormat) * sizeof(cmsFloat64Number));            
    }

}


// ----------------------------------------------------------------------------------------------------------------


static cmsFormatters16 InputFormatters16[] = {

    //    Type                                          Mask                  Function
    //  ----------------------------   ------------------------------------  ----------------------------
    { TYPE_Lab_DBL,                                 ANYPLANAR|ANYEXTRA,   UnrollLabDoubleTo16},
    { TYPE_XYZ_DBL,                                 ANYPLANAR|ANYEXTRA,   UnrollXYZDoubleTo16},
    { TYPE_GRAY_DBL,                                                 0,   UnrollDouble1Chan},
    { FLOAT_SH(1)|BYTES_SH(0), ANYCHANNELS|ANYPLANAR|ANYEXTRA|ANYSPACE,   UnrollDoubleTo16},
    { FLOAT_SH(1)|BYTES_SH(4), ANYCHANNELS|ANYPLANAR|ANYEXTRA|ANYSPACE,   UnrollFloatTo16},


    { CHANNELS_SH(1)|BYTES_SH(1),                              ANYSPACE,  Unroll1Byte}, 
    { CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(1),                  ANYSPACE,  Unroll1ByteSkip1},
    { CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(2),                  ANYSPACE,  Unroll1ByteSkip2},
    { CHANNELS_SH(1)|BYTES_SH(1)|FLAVOR_SH(1),                 ANYSPACE,  Unroll1ByteReversed},
    { COLORSPACE_SH(PT_MCH2)|CHANNELS_SH(2)|BYTES_SH(1),              0,  Unroll2Bytes},

    { TYPE_LabV2_8,                                                   0,  UnrollLabV2_8 },
    { TYPE_ALabV2_8,                                                  0,  UnrollALabV2_8 },
    { TYPE_LabV2_16,                                                  0,  UnrollLabV2_16 },

    { CHANNELS_SH(3)|BYTES_SH(1),                              ANYSPACE,  Unroll3Bytes},
    { CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1),                 ANYSPACE,  Unroll3BytesSwap},
    { CHANNELS_SH(3)|EXTRA_SH(1)|BYTES_SH(1)|DOSWAP_SH(1),     ANYSPACE,  Unroll3BytesSkip1Swap},
    { CHANNELS_SH(3)|EXTRA_SH(1)|BYTES_SH(1)|SWAPFIRST_SH(1),  ANYSPACE,  Unroll3BytesSkip1SwapFirst},

    { CHANNELS_SH(4)|BYTES_SH(1),                              ANYSPACE,  Unroll4Bytes},
    { CHANNELS_SH(4)|BYTES_SH(1)|FLAVOR_SH(1),                 ANYSPACE,  Unroll4BytesReverse},
    { CHANNELS_SH(4)|BYTES_SH(1)|SWAPFIRST_SH(1),              ANYSPACE,  Unroll4BytesSwapFirst}, 
    { CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1),                 ANYSPACE,  Unroll4BytesSwap}, 
    { CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1), ANYSPACE,  Unroll4BytesSwapSwapFirst}, 

    { BYTES_SH(1)|PLANAR_SH(1),    ANYFLAVOR|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, UnrollPlanarBytes},
    { BYTES_SH(1),    ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, UnrollChunkyBytes},     


    { CHANNELS_SH(1)|BYTES_SH(2),                              ANYSPACE,  Unroll1Word},
    { CHANNELS_SH(1)|BYTES_SH(2)|FLAVOR_SH(1),                 ANYSPACE,  Unroll1WordReversed},
    { CHANNELS_SH(1)|BYTES_SH(2)|EXTRA_SH(3),                  ANYSPACE,  Unroll1WordSkip3},

    { CHANNELS_SH(2)|BYTES_SH(2),                              ANYSPACE,  Unroll2Words},
    { CHANNELS_SH(3)|BYTES_SH(2),                              ANYSPACE,  Unroll3Words},
    { CHANNELS_SH(4)|BYTES_SH(2),                              ANYSPACE,  Unroll4Words},

    { CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1),                 ANYSPACE,  Unroll3WordsSwap},
    { CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|SWAPFIRST_SH(1),  ANYSPACE,  Unroll3WordsSkip1SwapFirst},
    { CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|DOSWAP_SH(1),     ANYSPACE,  Unroll3WordsSkip1Swap},
    { CHANNELS_SH(4)|BYTES_SH(2)|FLAVOR_SH(1),                 ANYSPACE,  Unroll4WordsReverse},
    { CHANNELS_SH(4)|BYTES_SH(2)|SWAPFIRST_SH(1),              ANYSPACE,  Unroll4WordsSwapFirst}, 
    { CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1),                 ANYSPACE,  Unroll4WordsSwap}, 
    { CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1)|SWAPFIRST_SH(1), ANYSPACE,  Unroll4WordsSwapSwapFirst}, 


    { BYTES_SH(2)|PLANAR_SH(1),  ANYFLAVOR|ANYSWAP|ANYENDIAN|ANYEXTRA|ANYCHANNELS|ANYSPACE,  UnrollPlanarWords },
    { BYTES_SH(2),  ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYENDIAN|ANYEXTRA|ANYCHANNELS|ANYSPACE,  UnrollAnyWords}, 
};



static cmsFormattersFloat InputFormattersFloat[] = {

    //    Type                                          Mask                  Function
    //  ----------------------------   ------------------------------------  ----------------------------
    {     TYPE_Lab_DBL,                                ANYPLANAR|ANYEXTRA,   UnrollLabDoubleToFloat},
    {     TYPE_Lab_FLT,                                ANYPLANAR|ANYEXTRA,   UnrollLabFloatToFloat},
    {     TYPE_XYZ_DBL,                                ANYPLANAR|ANYEXTRA,   UnrollXYZDoubleToFloat},
    {     TYPE_XYZ_FLT,                                ANYPLANAR|ANYEXTRA,   UnrollXYZFloatToFloat},

    {     FLOAT_SH(1)|BYTES_SH(4), ANYPLANAR|ANYEXTRA|ANYCHANNELS|ANYSPACE,  UnrollFloatsToFloat},
    {     FLOAT_SH(1)|BYTES_SH(0), ANYPLANAR|ANYEXTRA|ANYCHANNELS|ANYSPACE,  UnrollDoublesToFloat},
};


// Bit fields set to one in the mask are not compared
static
cmsFormatter _cmsGetStockInputFormatter(cmsUInt32Number dwInput, cmsUInt32Number dwFlags)
{
    cmsUInt32Number i;
    cmsFormatter fr;


    if (!(dwFlags & CMS_PACK_FLAGS_FLOAT)) {

        for (i=0; i < sizeof(InputFormatters16) / sizeof(cmsFormatters16); i++) {
            cmsFormatters16* f = InputFormatters16 + i;

            if ((dwInput & ~f ->Mask) == f ->Type) {
                fr.Fmt16 = f ->Frm;
                return fr;
            }
        }
    }
    else {
        for (i=0; i < sizeof(InputFormattersFloat) / sizeof(cmsFormattersFloat); i++) {
            cmsFormattersFloat* f = InputFormattersFloat + i;

            if ((dwInput & ~f ->Mask) == f ->Type) {
                fr.FmtFloat = f ->Frm;
                return fr;
            }
        }
    }

    fr.Fmt16 = NULL;
    return fr;
}

static cmsFormatters16 OutputFormatters16[] = {
    //    Type                                          Mask                  Function
    //  ----------------------------   ------------------------------------  ----------------------------

    { TYPE_Lab_DBL,                                      ANYPLANAR|ANYEXTRA,  PackLabDoubleFrom16},
    { TYPE_XYZ_DBL,                                      ANYPLANAR|ANYEXTRA,  PackXYZDoubleFrom16},
    { FLOAT_SH(1)|BYTES_SH(0),      ANYCHANNELS|ANYPLANAR|ANYEXTRA|ANYSPACE,  PackDoubleFrom16},
    { FLOAT_SH(1)|BYTES_SH(4),      ANYCHANNELS|ANYPLANAR|ANYEXTRA|ANYSPACE,  PackFloatFrom16},

    { CHANNELS_SH(1)|BYTES_SH(1),                                  ANYSPACE,  Pack1Byte},   
    { CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(1),                      ANYSPACE,  Pack1ByteSkip1},
    { CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack1ByteSkip1SwapFirst},

    { CHANNELS_SH(1)|BYTES_SH(1)|FLAVOR_SH(1),                     ANYSPACE,  Pack1ByteReversed},

    { TYPE_LabV2_8,                                                       0,  PackLabV2_8 },
    { TYPE_ALabV2_8,                                                      0,  PackALabV2_8 },
    { TYPE_LabV2_16,                                                      0,  PackLabV2_16 },

    { CHANNELS_SH(3)|BYTES_SH(1)|OPTIMIZED_SH(1),                  ANYSPACE,  Pack3BytesOptimized},
    { CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|OPTIMIZED_SH(1),      ANYSPACE,  Pack3BytesAndSkip1Optimized},
    { CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|SWAPFIRST_SH(1)|OPTIMIZED_SH(1),
                                                                   ANYSPACE,  Pack3BytesAndSkip1SwapFirstOptimized},
    { CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1)|OPTIMIZED_SH(1),  
                                                                   ANYSPACE,  Pack3BytesAndSkip1SwapSwapFirstOptimized},
    { CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1)|EXTRA_SH(1)|OPTIMIZED_SH(1),         
                                                                   ANYSPACE,  Pack3BytesAndSkip1SwapOptimized},
    { CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1)|OPTIMIZED_SH(1),     ANYSPACE,  Pack3BytesSwapOptimized},



    { CHANNELS_SH(3)|BYTES_SH(1),                                  ANYSPACE,  Pack3Bytes},
    { CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1),                      ANYSPACE,  Pack3BytesAndSkip1},
    { CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack3BytesAndSkip1SwapFirst},
    { CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1),  
                                                                   ANYSPACE,  Pack3BytesAndSkip1SwapSwapFirst},
    { CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1)|EXTRA_SH(1),         ANYSPACE,  Pack3BytesAndSkip1Swap},
    { CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1),                     ANYSPACE,  Pack3BytesSwap},
    { CHANNELS_SH(6)|BYTES_SH(1),                                  ANYSPACE,  Pack6Bytes},
    { CHANNELS_SH(6)|BYTES_SH(1)|DOSWAP_SH(1),                     ANYSPACE,  Pack6BytesSwap},
    { CHANNELS_SH(4)|BYTES_SH(1),                                  ANYSPACE,  Pack4Bytes},
    { CHANNELS_SH(4)|BYTES_SH(1)|FLAVOR_SH(1),                     ANYSPACE,  Pack4BytesReverse},
    { CHANNELS_SH(4)|BYTES_SH(1)|SWAPFIRST_SH(1),                  ANYSPACE,  Pack4BytesSwapFirst}, 
    { CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1),                     ANYSPACE,  Pack4BytesSwap}, 
    { CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1),     ANYSPACE,  Pack4BytesSwapSwapFirst}, 

    { BYTES_SH(1),                 ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackAnyBytes},     
    { BYTES_SH(1)|PLANAR_SH(1),    ANYFLAVOR|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackPlanarBytes},   

    { CHANNELS_SH(1)|BYTES_SH(2),                                  ANYSPACE,  Pack1Word},
    { CHANNELS_SH(1)|BYTES_SH(2)|EXTRA_SH(1),                      ANYSPACE,  Pack1WordSkip1},
    { CHANNELS_SH(1)|BYTES_SH(2)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack1WordSkip1SwapFirst},
    { CHANNELS_SH(1)|BYTES_SH(2)|FLAVOR_SH(1),                     ANYSPACE,  Pack1WordReversed},
    { CHANNELS_SH(1)|BYTES_SH(2)|ENDIAN16_SH(1),                   ANYSPACE,  Pack1WordBigEndian},
    { CHANNELS_SH(3)|BYTES_SH(2),                                  ANYSPACE,  Pack3Words},
    { CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1),                     ANYSPACE,  Pack3WordsSwap},
    { CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1),                   ANYSPACE,  Pack3WordsBigEndian},
    { CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1),                      ANYSPACE,  Pack3WordsAndSkip1},
    { CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|DOSWAP_SH(1),         ANYSPACE,  Pack3WordsAndSkip1Swap},
    { CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack3WordsAndSkip1SwapFirst},

    { CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1),             
                                                                   ANYSPACE,  Pack3WordsAndSkip1SwapSwapFirst},

    { CHANNELS_SH(4)|BYTES_SH(2),                                  ANYSPACE,  Pack4Words},
    { CHANNELS_SH(4)|BYTES_SH(2)|FLAVOR_SH(1),                     ANYSPACE,  Pack4WordsReverse},
    { CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1),                     ANYSPACE,  Pack4WordsSwap}, 
    { CHANNELS_SH(4)|BYTES_SH(2)|ENDIAN16_SH(1),                   ANYSPACE,  Pack4WordsBigEndian},

    { CHANNELS_SH(6)|BYTES_SH(2),                                  ANYSPACE,  Pack6Words},
    { CHANNELS_SH(6)|BYTES_SH(2)|DOSWAP_SH(1),                     ANYSPACE,  Pack6WordsSwap},

    { BYTES_SH(2)|PLANAR_SH(1),     ANYFLAVOR|ANYENDIAN|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackPlanarWords}, 
    { BYTES_SH(2),                  ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYENDIAN|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackAnyWords}

};


static cmsFormattersFloat OutputFormattersFloat[] = {
    //    Type                                          Mask                                 Function
    //  ----------------------------   ---------------------------------------------------  ----------------------------
    {     TYPE_Lab_FLT,                                                ANYPLANAR|ANYEXTRA,   PackLabFloatFromFloat},
    {     TYPE_XYZ_FLT,                                                ANYPLANAR|ANYEXTRA,   PackXYZFloatFromFloat},
    {     TYPE_Lab_DBL,                                                ANYPLANAR|ANYEXTRA,   PackLabDoubleFromFloat},
    {     TYPE_XYZ_DBL,                                                ANYPLANAR|ANYEXTRA,   PackXYZDoubleFromFloat},
    {     FLOAT_SH(1)|BYTES_SH(4), 
                             ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE,   PackChunkyFloatsFromFloat }, 
    {     FLOAT_SH(1)|BYTES_SH(4)|PLANAR_SH(1),             ANYEXTRA|ANYCHANNELS|ANYSPACE,   PackPlanarFloatsFromFloat},
    {     FLOAT_SH(1)|BYTES_SH(0),
                             ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE,   PackChunkyDoublesFromFloat }, 
    {     FLOAT_SH(1)|BYTES_SH(0)|PLANAR_SH(1),             ANYEXTRA|ANYCHANNELS|ANYSPACE,   PackPlanarDoublesFromFloat},


};


// Bit fields set to one in the mask are not compared
cmsFormatter _cmsGetStockOutputFormatter(cmsUInt32Number dwInput, cmsUInt32Number dwFlags)
{
    cmsUInt32Number i;
    cmsFormatter fr;


    if (dwFlags & CMS_PACK_FLAGS_FLOAT) {

        for (i=0; i < sizeof(OutputFormattersFloat) / sizeof(cmsFormattersFloat); i++) {
            cmsFormattersFloat* f = OutputFormattersFloat + i;

            if ((dwInput & ~f ->Mask) == f ->Type) {
                fr.FmtFloat = f ->Frm;
                return fr;
            }
        }

    }
    else {

        for (i=0; i < sizeof(OutputFormatters16) / sizeof(cmsFormatters16); i++) {
            cmsFormatters16* f = OutputFormatters16 + i;

            if ((dwInput & ~f ->Mask) == f ->Type) {
                fr.Fmt16 = f ->Frm;
                return fr;
            }
        }
    }

    fr.Fmt16 = NULL;
    return fr;
}


typedef struct _cms_formatters_factory_list {

    cmsFormatterFactory Factory;
    struct _cms_formatters_factory_list *Next;

} cmsFormattersFactoryList;

static cmsFormattersFactoryList* FactoryList = NULL;


// Formatters management
cmsBool  _cmsRegisterFormattersPlugin(cmsPluginBase* Data)
{
    cmsPluginFormatters* Plugin = (cmsPluginFormatters*) Data;
    cmsFormattersFactoryList* fl ;

    // Reset
    if (Data == NULL) {

          FactoryList = NULL;
          return TRUE;
    }
    
    fl = (cmsFormattersFactoryList*) _cmsPluginMalloc(sizeof(cmsFormattersFactoryList));
    if (fl == NULL) return FALSE;

    fl ->Factory    = Plugin ->FormattersFactory;

    fl ->Next = FactoryList;
    FactoryList = fl;

    return TRUE;
}

cmsFormatter _cmsGetFormatter(cmsUInt32Number Type,         // Specific type, i.e. TYPE_RGB_8
                             cmsFormatterDirection Dir, 
                             cmsUInt32Number dwFlags)       // Float or 16 bits
{
    cmsFormattersFactoryList* f;

    for (f = FactoryList; f != NULL; f = f ->Next) {

        cmsFormatter fn = f ->Factory(Type, Dir, dwFlags);
        if (fn.Fmt16 != NULL) return fn;
    }

    // Revert to default
    if (Dir == cmsFormatterInput) 
        return _cmsGetStockInputFormatter(Type, dwFlags);
    else 
        return _cmsGetStockOutputFormatter(Type, dwFlags);
}


// Return whatever given formatter refers to float values
cmsBool  _cmsFormatterIsFloat(cmsUInt32Number Type)
{
    return T_FLOAT(Type) ? TRUE : FALSE;
}

// Return whatever given formatter refers to 8 bits
cmsBool  _cmsFormatterIs8bit(cmsUInt32Number Type)
{
    int Bytes = T_BYTES(Type);

    return (Bytes == 1);
}

// Build a suitable formatter for the colorspace of this profile
cmsUInt32Number CMSEXPORT cmsFormatterForColorspaceOfProfile(cmsHPROFILE hProfile, cmsUInt32Number nBytes, cmsBool lIsFloat)
{
 
    cmsColorSpaceSignature ColorSpace      = cmsGetColorSpace(hProfile);
    cmsUInt32Number        ColorSpaceBits  = _cmsLCMScolorSpace(ColorSpace);
    cmsUInt32Number        nOutputChans    = cmsChannelsOf(ColorSpace);
    cmsUInt32Number        Float           = lIsFloat ? 1 : 0;

    // Create a fake formatter for result
    return FLOAT_SH(Float) | COLORSPACE_SH(ColorSpaceBits) | BYTES_SH(nBytes) | CHANNELS_SH(nOutputChans);
}

// Build a suitable formatter for the colorspace of this profile
cmsUInt32Number CMSEXPORT cmsFormatterForPCSOfProfile(cmsHPROFILE hProfile, cmsUInt32Number nBytes, cmsBool lIsFloat)
{
 
    cmsColorSpaceSignature ColorSpace      = cmsGetPCS(hProfile);
    int                    ColorSpaceBits  = _cmsLCMScolorSpace(ColorSpace);
    cmsUInt32Number        nOutputChans    = cmsChannelsOf(ColorSpace);
    cmsUInt32Number        Float           = lIsFloat ? 1 : 0;

    // Create a fake formatter for result
    return FLOAT_SH(Float) | COLORSPACE_SH(ColorSpaceBits) | BYTES_SH(nBytes) | CHANNELS_SH(nOutputChans);
}

