/** @file
File:       icProfileHeader.h

Contains:   ICC profile definitions and structures including Version 4 extensions

Copyright:  © see ICC Software License

* <b>
* This version of the header file corresponds to the profile
* specification version 4.2 as defined in ICC Specificion ICC.1:2004-04.
*
* Some definitions only provided by version 2.x profiles are also included.
*
* This header file should not be considered as a replacement for the ICC
* profile specification.  The ICC profile specification should always be
* considered the ULTIMATE authority related to the specifiation for
* contents in ICC profile file.  Conflicts between this header file
* and the ICC profile specification (if they exist) should be deferred
* to the ICC profile specification.
* </b>
*
* All header file entries are pre-fixed with "ic" to help
* avoid name space collisions. Signatures are pre-fixed with
* icSig.
*
* Note: This header assumes that int is at least a 32 bit quantity
*
* The structures defined in this header file were created to
* represent a description of an ICC profile on disk. Rather
* than use pointers a technique is used where a single byte array
* was placed at the end of each structure. This allows us in "C"
* to extend the structure by allocating more data than is needed
* to account for variable length structures.
*
* This also ensures that data following is allocated
* contiguously and makes it easier to write and read data from
* the file.
*
* For example to allocate space for a 256 count length UCR
* and BG array, and fill the allocated data.  Note strlen + 1
* to remember NULL terminator.
*
icUcrBgCurve    *ucrCurve, *bgCurve;
int             ucr_nbytes, bg_nbytes, string_bytes;
icUcrBg         *ucrBgWrite;
char            ucr_string[100], *ucr_char;

strcpy(ucr_string, "Example ucrBG curves");
ucr_nbytes = sizeof(icUInt32Number) +
(UCR_CURVE_SIZE * sizeof(icUInt16Number));
bg_nbytes = sizeof(icUInt32Number) +
(BG_CURVE_SIZE * sizeof(icUInt16Number));
string_bytes = strlen(ucr_string) + 1;

ucrBgWrite = (icUcrBg *)malloc(
(ucr_nbytes + bg_nbytes + string_bytes));

ucrCurve = (icUcrBgCurve *)ucrBgWrite->data;
ucrCurve->count = UCR_CURVE_SIZE;
for (i=0; i<ucrCurve->count; i++)
ucrCurve->curve[i] = (icUInt16Number)i;

bgCurve = (icUcrBgCurve *)((char *)ucrCurve + ucr_nbytes);
bgCurve->count = BG_CURVE_SIZE;
for (i=0; i<bgCurve->count; i++)
bgCurve->curve[i] = 255 - (icUInt16Number)i;

ucr_char = (char *)((char *)bgCurve + bg_nbytes);
memcpy(ucr_char, ucr_string, string_bytes);

* Many of the structures contain variable length arrays. This
* is represented by the use of the convention.
*
*      type    data[icAny];
*/

/*
*
*
* Copyright (c) 2003-2006 The International Color Consortium.
*
*
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED.  IN NO EVENT SHALL THE INTERNATIONAL COLOR CONSORTIUM OR
* ITS CONTRIBUTING MEMBERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
* USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
* ====================================================================
*
* This software consists of voluntary contributions made by many
* individuals on behalf of the The International Color Consortium.
*
*
* Membership in the ICC is encouraged when this software is used for
* commercial purposes.
*
*
* For more information on The International Color Consortium, please
* see <http://www.color.org/>.
*
*
*/


/*****************************************************************
Copyright (c) 2002  Heidelberger Druckmaschinen AG

* THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED.  IN NO EVENT SHALL THE INTERNATIONAL COLOR CONSORTIUM OR
* ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
* USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
******************************************************************/

/*****************************************************************
Copyright (c) 1994 SunSoft, Inc.

Rights Reserved

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restrict-
ion, including without limitation the rights to use, copy, modify,
merge, publish distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-
INFRINGEMENT.  IN NO EVENT SHALL SUNSOFT, INC. OR ITS PARENT
COMPANY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of SunSoft, Inc.
shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without written
authorization from SunSoft Inc.
******************************************************************/



/* Header file guard bands */
#ifndef icPROFILEHEADER_H
#define icPROFILEHEADER_H


/* In order for structures to work it is important to ensure that
* the structure packing is set to 1.  On many compilers this
* can be accomplished using #pragma pack(1).  Define the macro
* ICSETBYTEPACKING to enable the following.*/
#ifdef ICSETBYTEPACKING
#pragma pack(1)
#endif

/*------------------------------------------------------------------------*/

/**
* Defines used in the specification
*/
#define icMagicNumber                   0x61637370     /* 'acsp' */
#define icVersionNumber                 0x02000000     /* 2.0, BCD */
#define icVersionNumberV2_1             0x02100000     /* 2.1, BCD */
#define icVersionNumberV4               0x04000000     /* 4.0, BCD */
#define icVersionNumberV4_2             0x04200000     /* 4.2, BCD */

/** Screening Encodings */
#define icPrtrDefaultScreensFalse       0x00000000     /* Bit position 0 */
#define icPrtrDefaultScreensTrue        0x00000001     /* Bit position 0 */
#define icLinesPerInch                  0x00000002     /* Bit position 1 */
#define icLinesPerCm                    0x00000000     /* Bit position 1 */

/**
* Device attributes, currently defined values correspond
* to the low 4 bytes of the 8 byte attribute quantity, see
* the header for their location.
*/
#define icReflective                    0x00000000     /* Bit position 0 */
#define icTransparency                  0x00000001     /* Bit position 0 */
#define icGlossy                        0x00000000     /* Bit position 1 */
#define icMatte                         0x00000002     /* Bit position 1 */
#define icMediaPositive                 0x00000000     /* Bit position 2 */
#define icMediaNegative                 0x00000004     /* Bit position 2 */
#define icMediaColour                   0x00000000     /* Bit position 3 */
#define icMediaBlackAndWhite            0x00000008     /* Bit position 3 */

/**
* Profile header flags, the low 16 bits are reserved for consortium
* use.
*/
#define icEmbeddedProfileFalse          0x00000000     /* Bit position 0 */
#define icEmbeddedProfileTrue           0x00000001     /* Bit position 0 */
#define icUseAnywhere                   0x00000000     /* Bit position 1 */
#define icUseWithEmbeddedDataOnly       0x00000002     /* Bit position 1 */

/** Ascii or Binary data */
#define icAsciiData                     0x00000000     /* Used in dataType */
#define icBinaryData                    0x00000001

/**
* Define used to indicate that this is a variable length array
*/
#define icAny                               1

/**
* Number definitions
*
* NOTE:
*  Integer definitions vary from compiler to compiler.  Rather than
*  provide complex checking for compiler and system, default implementations
*  are provided with the ability to redefine actual meaning based upon
*  macros.  This can be accomplished in a separate header file that first defines
*  the macros and then includes this header, or by defining macro values on
*  a project level.
*/

/** Unsigned integer numbers */
#ifdef ICUINT8TYPE
typedef ICUINT8TYPE          icUInt8Number;
#else
typedef unsigned char        icUInt8Number;
#endif

#ifdef ICUINT16TYPE
typedef ICUINT16TYPE         icUInt16Number;
#else
typedef unsigned short       icUInt16Number;
#endif

#ifdef ICUINT32TYPE
typedef ICUINT32TYPE         icUInt32Number;
#else
typedef unsigned long        icUInt32Number;
#endif

#ifdef ICUINT64TYPE
typedef ICUINT64TYPE         icUInt64Number;
#else
typedef icUInt32Number       icUInt64Number[2];
#endif

typedef icUInt32Number       icSignature;


/** Signed numbers */
#ifdef ICINT8TYPE
typedef ICINT8TYPE           icInt8Number;
#else
typedef char                 icInt8Number;
#endif

#ifdef ICINT16TYPE
typedef ICINT16TYPE          icInt16Number;
#else
typedef short                icInt16Number;
#endif

#ifdef ICINT32TYPE
typedef ICINT32TYPE          icInt32Number;
#else
typedef long                 icInt32Number;
#endif

#ifdef ICINT64TYPE
typedef ICINT64TYPE          icInt64Number;
#else
typedef icInt32Number        icInt64Number[2];
#endif


/** Fixed numbers */
typedef icInt32Number        icS15Fixed16Number;
typedef icUInt32Number       icU16Fixed16Number;



/*------------------------------------------------------------------------*/

/**
* public tags and sizes
*/
typedef enum {
    icSigAToB0Tag = 0x41324230,  /* 'A2B0' */
    icSigAToB1Tag = 0x41324231,  /* 'A2B1' */
    icSigAToB2Tag = 0x41324232,  /* 'A2B2' */
    icSigBlueColorantTag = 0x6258595A,  /* 'bXYZ' */
    icSigBlueMatrixColumnTag = 0x6258595A,  /* 'bXYZ' */
    icSigBlueTRCTag = 0x62545243,  /* 'bTRC' */
    icSigBToA0Tag = 0x42324130,  /* 'B2A0' */
    icSigBToA1Tag = 0x42324131,  /* 'B2A1' */
    icSigBToA2Tag = 0x42324132,  /* 'B2A2' */
    icSigCalibrationDateTimeTag = 0x63616C74,  /* 'calt' */
    icSigCharTargetTag = 0x74617267,  /* 'targ' */
    icSigChromaticAdaptationTag = 0x63686164,  /* 'chad' */
    icSigChromaticityTag = 0x6368726D,  /* 'chrm' */
    icSigColorantOrderTag = 0x636C726F,  /* 'clro' */
    icSigColorantTableTag = 0x636C7274,  /* 'clrt' */
    icSigColorantTableOutTag = 0x636C6F74,  /* 'clot' */
    icSigCopyrightTag = 0x63707274,  /* 'cprt' */
    icSigCrdInfoTag = 0x63726469,  /* 'crdi' Removed in V4 */
    icSigDataTag = 0x64617461,  /* 'data' Removed in V4 */
    icSigDateTimeTag = 0x6474696D,  /* 'dtim' Removed in V4 */
    icSigDeviceMfgDescTag = 0x646D6E64,  /* 'dmnd' */
    icSigDeviceModelDescTag = 0x646D6464,  /* 'dmdd' */
    icSigDeviceSettingsTag = 0x64657673,  /* 'devs' Removed in V4 */
    icSigGamutTag = 0x67616D74,  /* 'gamt' */
    icSigGrayTRCTag = 0x6b545243,  /* 'kTRC' */
    icSigGreenColorantTag = 0x6758595A,  /* 'gXYZ' */
    icSigGreenMatrixColumnTag = 0x6758595A,  /* 'gXYZ' */
    icSigGreenTRCTag = 0x67545243,  /* 'gTRC' */
    icSigLuminanceTag = 0x6C756d69,  /* 'lumi' */
    icSigMeasurementTag = 0x6D656173,  /* 'meas' */
    icSigMediaBlackPointTag = 0x626B7074,  /* 'bkpt' */
    icSigMediaWhitePointTag = 0x77747074,  /* 'wtpt' */
    /*icSigNamedColorTag                    = 0x6E636f6C,  / 'ncol' OBSOLETE, use ncl2 */
    icSigNamedColor2Tag = 0x6E636C32,  /* 'ncl2' */
    icSigOutputResponseTag = 0x72657370,  /* 'resp' */
    icSigPerceptualRenderingIntentGamutTag = 0x72696730,  /* 'rig0' */
    icSigPreview0Tag = 0x70726530,  /* 'pre0' */
    icSigPreview1Tag = 0x70726531,  /* 'pre1' */
    icSigPreview2Tag = 0x70726532,  /* 'pre2' */
    icSigProfileDescriptionTag = 0x64657363,  /* 'desc' */
    icSigProfileDescriptionMLTag = 0x6473636d,  /* 'dscm' Custom Apple Tag */
    icSigProfileSequenceDescTag = 0x70736571,  /* 'pseq' */
    icSigPs2CRD0Tag = 0x70736430,  /* 'psd0' Removed in V4 */
    icSigPs2CRD1Tag = 0x70736431,  /* 'psd1' Removed in V4 */
    icSigPs2CRD2Tag = 0x70736432,  /* 'psd2' Removed in V4 */
    icSigPs2CRD3Tag = 0x70736433,  /* 'psd3' Removed in V4 */
    icSigPs2CSATag = 0x70733273,  /* 'ps2s' Removed in V4 */
    icSigPs2RenderingIntentTag = 0x70733269,  /* 'ps2i' Removed in V4 */
    icSigRedColorantTag = 0x7258595A,  /* 'rXYZ' */
    icSigRedMatrixColumnTag = 0x7258595A,  /* 'rXYZ' */
    icSigRedTRCTag = 0x72545243,  /* 'rTRC' */
    icSigSaturationRenderingIntentGamutTag = 0x72696732,  /* 'rig2' */
    icSigScreeningDescTag = 0x73637264,  /* 'scrd' Removed in V4 */
    icSigScreeningTag = 0x7363726E,  /* 'scrn' Removed in V4 */
    icSigTechnologyTag = 0x74656368,  /* 'tech' */
    icSigUcrBgTag = 0x62666420,  /* 'bfd ' Removed in V4 */
    icSigViewingCondDescTag = 0x76756564,  /* 'vued' */
    icSigViewingConditionsTag = 0x76696577,  /* 'view' */
} icTagSignature;

/** Convenience Enum Definitions - Not defined in ICC specification*/
#define icSigUnknownTag    ((icTagSignature) 0x3f3f3f3f)  /* '????' */
#define icMaxEnumTag       ((icTagSignature) 0xFFFFFFFF)



/**
* technology signature descriptions
*/
typedef enum {
    icSigDigitalCamera = 0x6463616D,  /* 'dcam' */
    icSigFilmScanner = 0x6673636E,  /* 'fscn' */
    icSigReflectiveScanner = 0x7273636E,  /* 'rscn' */
    icSigInkJetPrinter = 0x696A6574,  /* 'ijet' */
    icSigThermalWaxPrinter = 0x74776178,  /* 'twax' */
    icSigElectrophotographicPrinter = 0x6570686F,  /* 'epho' */
    icSigElectrostaticPrinter = 0x65737461,  /* 'esta' */
    icSigDyeSublimationPrinter = 0x64737562,  /* 'dsub' */
    icSigPhotographicPaperPrinter = 0x7270686F,  /* 'rpho' */
    icSigFilmWriter = 0x6670726E,  /* 'fprn' */
    icSigVideoMonitor = 0x7669646D,  /* 'vidm' */
    icSigVideoCamera = 0x76696463,  /* 'vidc' */
    icSigProjectionTelevision = 0x706A7476,  /* 'pjtv' */
    icSigCRTDisplay = 0x43525420,  /* 'CRT ' */
    icSigPMDisplay = 0x504D4420,  /* 'PMD ' */
    icSigAMDisplay = 0x414D4420,  /* 'AMD ' */
    icSigPhotoCD = 0x4B504344,  /* 'KPCD' */
    icSigPhotoImageSetter = 0x696D6773,  /* 'imgs' */
    icSigGravure = 0x67726176,  /* 'grav' */
    icSigOffsetLithography = 0x6F666673,  /* 'offs' */
    icSigSilkscreen = 0x73696C6B,  /* 'silk' */
    icSigFlexography = 0x666C6578,  /* 'flex' */
} icTechnologySignature;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumTechnology ((icTechnologySignature) 0xFFFFFFFF)



/**
* type signatures
*/
typedef enum {
    icSigChromaticityType = 0x6368726D,  /* 'chrm' */
    icSigColorantOrderType = 0x636C726F,  /* 'clro' */
    icSigColorantTableType = 0x636C7274,  /* 'clrt' */
    icSigCrdInfoType = 0x63726469,  /* 'crdi' Removed in V4 */
    icSigCurveType = 0x63757276,  /* 'curv' */
    icSigDataType = 0x64617461,  /* 'data' */
    icSigDateTimeType = 0x6474696D,  /* 'dtim' */
    icSigDeviceSettingsType = 0x64657673,  /* 'devs' Removed in V4 */
    icSigLut16Type = 0x6d667432,  /* 'mft2' */
    icSigLut8Type = 0x6d667431,  /* 'mft1' */
    icSigLutAtoBType = 0x6d414220,  /* 'mAB ' */
    icSigLutBtoAType = 0x6d424120,  /* 'mBA ' */
    icSigMeasurementType = 0x6D656173,  /* 'meas' */
    icSigMultiLocalizedUnicodeType = 0x6D6C7563,  /* 'mluc' */
    /*icSigNamedColorType                 = 0x6E636f6C,   'ncol' OBSOLETE, use ncl2 */
    icSigNamedColor2Type = 0x6E636C32,  /* 'ncl2' */
    icSigParametricCurveType = 0x70617261,  /* 'para' */
    icSigProfileSequenceDescType = 0x70736571,  /* 'pseq' */
    icSigResponseCurveSet16Type = 0x72637332,  /* 'rcs2' */
    icSigS15Fixed16ArrayType = 0x73663332,  /* 'sf32' */
    icSigScreeningType = 0x7363726E,  /* 'scrn' Removed in V4 */
    icSigSignatureType = 0x73696720,  /* 'sig ' */
    icSigTextType = 0x74657874,  /* 'text' */
    icSigTextDescriptionType = 0x64657363,  /* 'desc' Removed in V4 */
    icSigU16Fixed16ArrayType = 0x75663332,  /* 'uf32' */
    icSigUcrBgType = 0x62666420,  /* 'bfd ' Removed in V4 */
    icSigUInt16ArrayType = 0x75693136,  /* 'ui16' */
    icSigUInt32ArrayType = 0x75693332,  /* 'ui32' */
    icSigUInt64ArrayType = 0x75693634,  /* 'ui64' */
    icSigUInt8ArrayType = 0x75693038,  /* 'ui08' */
    icSigViewingConditionsType = 0x76696577,  /* 'view' */
    icSigXYZType = 0x58595A20,  /* 'XYZ ' */
    icSigXYZArrayType = 0x58595A20,  /* 'XYZ ' */
} icTagTypeSignature;

/** Convenience Enum Definitions - Not defined in ICC specification*/
#define icSigUnknownType ((icTagTypeSignature) 0x3f3f3f3f)  /* '????' */
#define icMaxEnumType    ((icTagTypeSignature) 0xFFFFFFFF)



/**
* Color Space Signatures.
* Note that only icSigXYZData and icSigLabData are valid
* Profile Connection Spaces (PCSs)
*/
typedef enum {
    icSigXYZData = 0x58595A20,  /* 'XYZ ' */
    icSigLabData = 0x4C616220,  /* 'Lab ' */
    icSigLuvData = 0x4C757620,  /* 'Luv ' */
    icSigYCbCrData = 0x59436272,  /* 'YCbr' */
    icSigYxyData = 0x59787920,  /* 'Yxy ' */
    icSigRgbData = 0x52474220,  /* 'RGB ' */
    icSigGrayData = 0x47524159,  /* 'GRAY' */
    icSigHsvData = 0x48535620,  /* 'HSV ' */
    icSigHlsData = 0x484C5320,  /* 'HLS ' */
    icSigCmykData = 0x434D594B,  /* 'CMYK' */
    icSigCmyData = 0x434D5920,  /* 'CMY ' */

    icSigMCH2Data = 0x32434C52,  /* '2CLR' */
    icSigMCH3Data = 0x33434C52,  /* '3CLR' */
    icSigMCH4Data = 0x34434C52,  /* '4CLR' */
    icSigMCH5Data = 0x35434C52,  /* '5CLR' */
    icSigMCH6Data = 0x36434C52,  /* '6CLR' */
    icSigMCH7Data = 0x37434C52,  /* '7CLR' */
    icSigMCH8Data = 0x38434C52,  /* '8CLR' */
    icSigMCH9Data = 0x39434C52,  /* '9CLR' */
    icSigMCHAData = 0x41434C52,  /* 'ACLR' */
    icSigMCHBData = 0x42434C52,  /* 'BCLR' */
    icSigMCHCData = 0x43434C52,  /* 'CCLR' */
    icSigMCHDData = 0x44434C52,  /* 'DCLR' */
    icSigMCHEData = 0x45434C52,  /* 'ECLR' */
    icSigMCHFData = 0x46434C52,  /* 'FCLR' */
    icSigNamedData = 0x6e6d636c,  /* 'nmcl' */

    icSig2colorData = 0x32434C52,  /* '2CLR' */
    icSig3colorData = 0x33434C52,  /* '3CLR' */
    icSig4colorData = 0x34434C52,  /* '4CLR' */
    icSig5colorData = 0x35434C52,  /* '5CLR' */
    icSig6colorData = 0x36434C52,  /* '6CLR' */
    icSig7colorData = 0x37434C52,  /* '7CLR' */
    icSig8colorData = 0x38434C52,  /* '8CLR' */
    icSig9colorData = 0x39434C52,  /* '9CLR' */
    icSig10colorData = 0x41434C52,  /* 'ACLR' */
    icSig11colorData = 0x42434C52,  /* 'BCLR' */
    icSig12colorData = 0x43434C52,  /* 'CCLR' */
    icSig13colorData = 0x44434C52,  /* 'DCLR' */
    icSig14colorData = 0x45434C52,  /* 'ECLR' */
    icSig15colorData = 0x46434C52,  /* 'FCLR' */

} icColorSpaceSignature;

/** Defined by previous versions of header file but not defined in ICC specification */
#define icSigMCH1Data       ((icColorSpaceSignature) 0x31434C52)  /* '1CLR' */        
#define icSigMCHGData       ((icColorSpaceSignature) 0x47434C52)  /* 'GCLR' */
#define icSig1colorData     ((icColorSpaceSignature) 0x31434C52)  /* '1CLR' */        
#define icSig16colorData    ((icColorSpaceSignature) 0x47434C52)  /* 'GCLR' */

/** Convenience Enum Definitions - Not defined in ICC specification*/
#define icSigGamutData      ((icColorSpaceSignature) 0x67616D74)  /* 'gamt' */
#define icSigUnknownData    ((icColorSpaceSignature) 0x3f3f3f3f)  /* '????' */
#define icMaxEnumData       ((icColorSpaceSignature) 0xFFFFFFFF)



/** profileClass enumerations */
typedef enum {
    icSigInputClass = 0x73636E72,  /* 'scnr' */
    icSigDisplayClass = 0x6D6E7472,  /* 'mntr' */
    icSigOutputClass = 0x70727472,  /* 'prtr' */
    icSigLinkClass = 0x6C696E6B,  /* 'link' */
    icSigAbstractClass = 0x61627374,  /* 'abst' */
    icSigColorSpaceClass = 0x73706163,  /* 'spac' */
    icSigNamedColorClass = 0x6e6d636c,  /* 'nmcl' */
} icProfileClassSignature;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumClass ((icProfileClassSignature) 0xFFFFFFFF)



/** Platform Signatures */
typedef enum {
    icSigMacintosh = 0x4150504C,  /* 'APPL' */
    icSigMicrosoft = 0x4D534654,  /* 'MSFT' */
    icSigSolaris = 0x53554E57,  /* 'SUNW' */
    icSigSGI = 0x53474920,  /* 'SGI ' */
    icSigTaligent = 0x54474E54,  /* 'TGNT' */
} icPlatformSignature;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumPlatform ((icPlatformSignature) 0xFFFFFFFF)



/** Rendering Intent Gamut Signatures */
typedef enum {
    icSigPerceptualReferenceMediumGamut = 0x70726d67,  /* 'prmg' */
} icReferenceMediumGamutSignature;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumReferenceMediumGamut ((icReferenceMediumGamutSignature 0xFFFFFFFF)



/*------------------------------------------------------------------------*/

/**
* Other enums
*/

/** Measurement Flare, used in the measurmentType tag */
typedef enum {
    icFlare0 = 0x00000000,  /* 0% flare */
    icFlare100 = 0x00000001,  /* 100% flare */
} icMeasurementFlare;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumFlare ((icMeasurementFlare) 0xFFFFFFFF)
#define icMaxFlare     ((icMeasurementFlare) 0xFFFFFFFF) /* as defined by earlier versions */



/** Measurement Geometry, used in the measurmentType tag */
typedef enum {
    icGeometryUnknown = 0x00000000,  /* Unknown geometry */
    icGeometry045or450 = 0x00000001,  /* 0/45, 45/0 */
    icGeometry0dord0 = 0x00000002,  /* 0/d or d/0 */
} icMeasurementGeometry;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumGeometry ((icMeasurementGeometry) 0xFFFFFFFF)
#define icMaxGeometry     ((icMeasurementGeometry) 0xFFFFFFFF)



/** Rendering Intents, used in the profile header */
typedef enum {
    icPerceptual = 0,
    icRelativeColorimetric = 1,
    icSaturation = 2,
    icAbsoluteColorimetric = 3,
} icRenderingIntent;

/** Convenience Enum Definitions - Not defined in ICC specification*/
#define icUnknownIntent ((icRenderingIntent) 0x3f3f3f3f)  /* '????' */
#define icMaxEnumIntent ((icRenderingIntent) 0xFFFFFFFF)



/** Different Spot Shapes currently defined, used for screeningType */
typedef enum {
    icSpotShapeUnknown = 0,
    icSpotShapePrinterDefault = 1,
    icSpotShapeRound = 2,
    icSpotShapeDiamond = 3,
    icSpotShapeEllipse = 4,
    icSpotShapeLine = 5,
    icSpotShapeSquare = 6,
    icSpotShapeCross = 7,
} icSpotShape;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumSpot ((icSpotShape)  0xFFFFFFFF)



/** Standard Observer, used in the measurmentType tag */
typedef enum {
    icStdObsUnknown = 0x00000000,  /* Unknown observer */
    icStdObs1931TwoDegrees = 0x00000001,  /* 1931 two degrees */
    icStdObs1964TenDegrees = 0x00000002,  /* 1961 ten degrees */
} icStandardObserver;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumStdObs ((icStandardObserver) 0xFFFFFFFF)
#define icMaxStdObs     ((icStandardObserver) 0xFFFFFFFF) /* as defined by earlier versions */



/** Pre-defined illuminants, used in measurement and viewing conditions type */
typedef enum {
    icIlluminantUnknown = 0x00000000,
    icIlluminantD50 = 0x00000001,
    icIlluminantD65 = 0x00000002,
    icIlluminantD93 = 0x00000003,
    icIlluminantF2 = 0x00000004,
    icIlluminantD55 = 0x00000005,
    icIlluminantA = 0x00000006,
    icIlluminantEquiPowerE = 0x00000007,  /* Equi-Power (E) */
    icIlluminantF8 = 0x00000008,
} icIlluminant;

/** Convenience Enum Definitions - Not defined in ICC specification*/
#define icMaxEnumIlluminant ((icIlluminant) 0xFFFFFFFF)
#define icMaxEnumIluminant  ((icIlluminant) 0xFFFFFFFF)   /* as defined by earlier versions */



/** A not so exhaustive list of language codes */
typedef enum {
    icLanguageCodeEnglish = 0x656E, /* 'en' */
    icLanguageCodeGerman = 0x6465, /* 'de' */
    icLanguageCodeItalian = 0x6974, /* 'it' */
    icLanguageCodeDutch = 0x6E6C, /* 'nl' */
    icLanguageCodeSweden = 0x7376, /* 'sv' */
    icLanguageCodeSpanish = 0x6573, /* 'es' */
    icLanguageCodeDanish = 0x6461, /* 'da' */
    icLanguageCodeNorwegian = 0x6E6F, /* 'no' */
    icLanguageCodeJapanese = 0x6A61, /* 'ja' */
    icLanguageCodeFinnish = 0x6669, /* 'fi' */
    icLanguageCodeTurkish = 0x7472, /* 'tr' */
    icLanguageCodeKorean = 0x6B6F, /* 'ko' */
    icLanguageCodeChinese = 0x7A68, /* 'zh' */
    icLanguageCodeFrench = 0x6672, /* 'fr' */
} icEnumLanguageCode;
typedef icUInt16Number icLanguageCode;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumLanguageCode ((icEnumLanguageCode) 0xFFFF)



/**
* A not so exhaustive list of country codes.
* Helpful website: http://dev.krook.org  ld.html */
typedef enum {
    icCountryCodeUSA = 0x5553, /* 'US' */
    icCountryCodeUnitedKingdom = 0x554B, /* 'UK' */
    icCountryCodeGermany = 0x4445, /* 'DE' */
    icCountryCodeItaly = 0x4954, /* 'IT' */
    icCountryCodeNetherlands = 0x4E4C, /* 'NL' */
    icCountryCodeSpain = 0x4543, /* 'ES' */
    icCountryCodeDenmark = 0x444B, /* 'DK' */
    icCountryCodeNorway = 0x4E4F, /* 'NO' */
    icCountryCodeJapan = 0x4A50, /* 'JP' */
    icCountryCodeFinland = 0x4649, /* 'FI' */
    icCountryCodeTurkey = 0x5452, /* 'TR' */
    icCountryCodeKorea = 0x4B52, /* 'KR' */
    icCountryCodeChina = 0x434E, /* 'CN' */
    icCountryCodeTaiwan = 0x5457, /* 'TW' */
    icCountryCodeFrance = 0x4652, /* 'FR' */
} icEnumCountryCode;
typedef icUInt16Number icCountryCode;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumCountryCode ((icEnumCountryCode) 0xFFFF)



/** Measurement Unit Signatures used in ResponseCurveSet16Type */
typedef enum {
    icSigStatusA = 0x53746141, /* 'StaA' */
    icSigStatusE = 0x53746145, /* 'StaE' */
    icSigStatusI = 0x53746149, /* 'StaI' */
    icSigStatusT = 0x53746154, /* 'StaT' */
    icSigStatusM = 0x5374614D, /* 'StaM' */
    icSigDN = 0x444E2020, /* 'DN  ' */
    icSigDNP = 0x444E2050, /* 'DN P' */
    icSigDNN = 0x444E4E20, /* 'DNN ' */
    icSigDNNP = 0x444E4E50, /* 'DNNP' */
} icMeasurementUnitSig;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumMeasurmentUnitSig ((icMeasurementUnitSig) 0xffffffff)



/** Colorant and Phosphor Encodings used in chromaticity type */
typedef enum {
    icColorantUnknown = 0x0000, /* Unknown */
    icColorantITU = 0x0001, /* ITU-R BT.709 */
    icColorantSMPTE = 0x0002, /* SMPTE RP145-1994 */
    icColorantEBU = 0x0003, /* EBU Tech.3213-E */
    icColorantP22 = 0x0004, /* P22 */
} icColorantEncoding;

/** Convenience Enum Definition - Not defined in ICC specification*/
#define icMaxEnumColorant ((icColorantEncoding) 0xFFFF)


/**
* Note: The next three enum types are for DeviceSettingType structures
* supported by V2 profiles.  The DeviceSettingsType was removed in the
* V4 specificaiton.*/

/** DeviceSettingsType structure ID signatures for Microsoft 'msft' platform*/
typedef enum {
    icMSFTDevSetResolution = 0x72736C6E, /* 'rsln' */
    icMSFTDevSetMediaType = 0x6D747970, /* 'mtyp' */
    icMSFTDevSetMediaHalftone = 0x6866746E, /* 'hftn' */
} icMSFTDevSetSig;

/** DeviceSettingsType media encodings for Microsoft 'msft' platform */
typedef enum {
    icDMMediaStandard = 0x0001, /* Standard paper */
    icDMMediaTransparancy = 0x0002, /* Transparency */
    icDMMediaGlossy = 0x0003, /* Glossy paper */
    icDMMediaUser = 0x0100, /* Device-specific type media
                            are >= 256 */
} icDMMediaType;

/** DeviceSettingsType media encodings for Microsoft 'msft' platform */
typedef enum {
    icDMDitherNone = 0x0001, /* No dithering */
    icDMDitherCoarse = 0x0002, /* Dither with a coarse brush */
    icDMDitherFine = 0x0003, /* Dither with a fine brush */
    icDMDitherLineArt = 0x0004, /* LineArt dithering */
    icDMDitherErrorDiffusion = 0x0005, /* Error Diffusion */
    icDMDitherReserved6 = 0x0006,
    icDMDitherReserved7 = 0x0007,
    icDMDitherReserved8 = 0x0008,
    icDMDitherReserved9 = 0x0009,
    icDMDitherGrayscale = 0x000A, /* Device does grayscaling */
    icDMDitherUser = 0x0100, /* Device-specifice halftones
                             are >= 256 */
} icDMHalftoneType;

/**
*------------------------------------------------------------------------
*
* Arrays of numbers
*/

/** Int8 Array */
typedef struct {
    icInt8Number        data[icAny];    /* Variable array of values */
} icInt8Array;

/** UInt8 Array */
typedef struct {
    icUInt8Number       data[icAny];    /* Variable array of values */
} icUInt8Array;

/** uInt16 Array */
typedef struct {
    icUInt16Number      data[icAny];    /* Variable array of values */
} icUInt16Array;

/** Int16 Array */
typedef struct {
    icInt16Number       data[icAny];    /* Variable array of values */
} icInt16Array;

/** uInt32 Array */
typedef struct {
    icUInt32Number      data[icAny];    /* Variable array of values */
} icUInt32Array;

/** Int32 Array */
typedef struct {
    icInt32Number       data[icAny];    /* Variable array of values */
} icInt32Array;

/** UInt64 Array */
typedef struct {
    icUInt64Number      data[icAny];    /* Variable array of values */
} icUInt64Array;

/** Int64 Array */
typedef struct {
    icInt64Number       data[icAny];    /* Variable array of values */
} icInt64Array;

/** u16Fixed16 Array */
typedef struct {
    icU16Fixed16Number  data[icAny];    /* Variable array of values */
} icU16Fixed16Array;

/** s15Fixed16 Array */
typedef struct {
    icS15Fixed16Number  data[icAny];    /* Variable array of values */
} icS15Fixed16Array;

/** The base date time number */
typedef struct {
    icUInt16Number      year;
    icUInt16Number      month;
    icUInt16Number      day;
    icUInt16Number      hours;
    icUInt16Number      minutes;
    icUInt16Number      seconds;
} icDateTimeNumber;

/** XYZ Number  */
typedef struct {
    icS15Fixed16Number  X;
    icS15Fixed16Number  Y;
    icS15Fixed16Number  Z;
} icXYZNumber;

/** XYZ Array */
typedef struct {
    icXYZNumber         data[icAny];    /* Variable array of XYZ numbers */
} icXYZArray;

/** xy Chromaticity Number  */
typedef struct {
    icU16Fixed16Number x;
    icU16Fixed16Number y;
} icChromaticityNumber;

/** response16Number  */
typedef struct {
    icUInt16Number deviceCode;
    icUInt16Number reserved;
    icS15Fixed16Number measurementValue;
} icResponse16Number;

/** Curve */
typedef struct {
    icUInt32Number      count;          /* Number of entries */
    icUInt16Number      data[icAny];    /* The actual table data, real
                                        * number is determined by count
                                        * Interpretation depends on how
                                        * data is used with a given tag
                                        */
} icCurve;

/** Parametric Curve */
typedef struct {
    icUInt16Number      funcType;       /* Function Type                */
                                        /* 0 = gamma only               */
    icUInt16Number      pad;            /* Padding for byte alignment   */
    icS15Fixed16Number  gamma;          /* x°gamma                      */
                                        /* up to 7 values Y,a,b,c,d,e,f */
} icParametricCurve;

/** Parametric Curve */
typedef struct {
    icUInt16Number      funcType;       /* Function Type                */
                                        /* 0 = gamma only               */
    icUInt16Number      pad;            /* Padding for byte alignment   */
    icS15Fixed16Number  gamma;          /* x°gamma                      */
    icS15Fixed16Number  a;              /* a                            */
    icS15Fixed16Number  b;              /* b                            */
    icS15Fixed16Number  c;              /* c                            */
    icS15Fixed16Number  d;              /* d                            */
    icS15Fixed16Number  e;              /* e                            */
    icS15Fixed16Number  f;              /* f                            */
} icParametricCurveFull;

/** Data */
typedef struct {
    icUInt32Number      dataFlag;       /* 0 = ascii, 1 = binary */
    icInt8Number        data[icAny];    /* Data, size determined from tag */
} icData;

/** lut16 */
typedef struct {
    icUInt8Number       inputChan;      /* Number of input channels */
    icUInt8Number       outputChan;     /* Number of output channels */
    icUInt8Number       clutPoints;     /* Number of clutTable grid points */
    icInt8Number        pad;            /* Padding for byte alignment */
    icS15Fixed16Number  e00;            /* e00 in the 3 * 3 */
    icS15Fixed16Number  e01;            /* e01 in the 3 * 3 */
    icS15Fixed16Number  e02;            /* e02 in the 3 * 3 */
    icS15Fixed16Number  e10;            /* e10 in the 3 * 3 */
    icS15Fixed16Number  e11;            /* e11 in the 3 * 3 */
    icS15Fixed16Number  e12;            /* e12 in the 3 * 3 */
    icS15Fixed16Number  e20;            /* e20 in the 3 * 3 */
    icS15Fixed16Number  e21;            /* e21 in the 3 * 3 */
    icS15Fixed16Number  e22;            /* e22 in the 3 * 3 */
    icUInt16Number      inputEnt;       /* Number of input table entries */
    icUInt16Number      outputEnt;      /* Number of output table entries */
    icUInt16Number      data[icAny];    /* Data follows see spec for size */
                                        /**
                                        *  Data that follows is of this form
                                        *
                                        *  icUInt16Number      inputTable[inputChan][icAny];   * The input table
                                        *  icUInt16Number      clutTable[icAny];               * The clut table
                                        *  icUInt16Number      outputTable[outputChan][icAny]; * The output table
                                        */
} icLut16;

/** lut8, input & output tables are always 256 bytes in length */
typedef struct {
    icUInt8Number       inputChan;      /* Number of input channels */
    icUInt8Number       outputChan;     /* Number of output channels */
    icUInt8Number       clutPoints;     /* Number of clutTable grid points */
    icInt8Number        pad;
    icS15Fixed16Number  e00;            /* e00 in the 3 * 3 */
    icS15Fixed16Number  e01;            /* e01 in the 3 * 3 */
    icS15Fixed16Number  e02;            /* e02 in the 3 * 3 */
    icS15Fixed16Number  e10;            /* e10 in the 3 * 3 */
    icS15Fixed16Number  e11;            /* e11 in the 3 * 3 */
    icS15Fixed16Number  e12;            /* e12 in the 3 * 3 */
    icS15Fixed16Number  e20;            /* e20 in the 3 * 3 */
    icS15Fixed16Number  e21;            /* e21 in the 3 * 3 */
    icS15Fixed16Number  e22;            /* e22 in the 3 * 3 */
    icUInt8Number       data[icAny];    /* Data follows see spec for size */
                                        /**
                                        *  Data that follows is of this form
                                        *
                                        *  icUInt8Number       inputTable[inputChan][256];     * The input table
                                        *  icUInt8Number       clutTable[icAny];               * The clut table
                                        *  icUInt8Number       outputTable[outputChan][256];   * The output table
                                        */
} icLut8;

/** icLutAToB  */
typedef struct {
    icUInt8Number       gridPoints[16]; /* Number of grid points in each dimension.  */
    icUInt8Number       prec;           /* Precision of data elements in bytes.      */
    icUInt8Number       pad1;
    icUInt8Number       pad2;
    icUInt8Number       pad3;
    /*icUInt8Number     data[icAny];     Data follows see spec for size */
} icCLutStruct;

/** icLutAtoB  */
typedef struct {
    icUInt8Number       inputChan;      /* Number of input channels     */
    icUInt8Number       outputChan;     /* Number of output channels    */
    icUInt8Number       pad1;
    icUInt8Number       pad2;
    icUInt32Number      offsetB;        /* Offset to first "B" curve    */
    icUInt32Number      offsetMat;      /* Offset to matrix             */
    icUInt32Number      offsetM;        /* Offset to first "M" curve    */
    icUInt32Number      offsetC;        /* Offset to CLUT               */
    icUInt32Number      offsetA;        /* Offset to first "A" curve    */
                                        /*icUInt8Number     data[icAny];     Data follows see spec for size */
} icLutAtoB;

/** icLutBtoA  */
typedef struct {
    icUInt8Number       inputChan;      /* Number of input channels     */
    icUInt8Number       outputChan;     /* Number of output channels    */
    icUInt8Number       pad1;
    icUInt8Number       pad2;
    icUInt32Number      offsetB;        /* Offset to first "B" curve    */
    icUInt32Number      offsetMat;      /* Offset to matrix             */
    icUInt32Number      offsetM;        /* Offset to first "M" curve    */
    icUInt32Number      offsetC;        /* Offset to CLUT               */
    icUInt32Number      offsetA;        /* Offset to first "A" curve    */
                                        /*icUInt8Number     data[icAny];     Data follows see spec for size */
} icLutBtoA;

/** Measurement Data */
typedef struct {
    icStandardObserver          stdObserver;    /* Standard observer */
    icXYZNumber                 backing;        /* XYZ for backing material */
    icMeasurementGeometry       geometry;       /* Measurement geometry */
    icMeasurementFlare          flare;          /* Measurement flare */
    icIlluminant                illuminant;     /* Illuminant */
} icMeasurement;

/**
* Named color
*/

/** Entry format for each named color */
typedef struct {
    icUInt8Number       rootName[32];        /* Root name for first color */
    icUInt16Number      pcsCoords[3];        /* PCS coordinates of color (only Lab or XYZ allowed)*/
    icUInt16Number      deviceCoords[icAny]; /* Device coordinates of color */
} icNamedColor2Entry;

/**
* icNamedColor2 takes the place of icNamedColor
*/
typedef struct {
    icUInt32Number      vendorFlag;     /* Bottom 16 bits for IC use */
    icUInt32Number      count;          /* Count of named colors */
    icUInt32Number      nDeviceCoords;  /* Number of device coordinates */
    icInt8Number        prefix[32];     /* Prefix for each color name */
    icInt8Number        suffix[32];     /* Suffix for each color name */
    icInt8Number        data[icAny];    /* Named color data follows */
                                        /**
                                        *  Data that follows is of this form
                                        *
                                        *   icInt8Number         root1[32];              * Root name for first color
                                        *   icUInt16Number       pcsCoords1[icAny];      * PCS coordinates of first color
                                        *   icUInt16Number       deviceCoords1[icAny];   * Device coordinates of first color
                                        *   icInt8Number         root2[32];              * Root name for second color
                                        *   icUInt16Number       pcsCoords2[icAny];      * PCS coordinates of first color
                                        *   icUInt16Number       deviceCoords2[icAny];   * Device coordinates of first color
                                        *                      :
                                        *                      :
                                        *
                                        * Alternatively written if byte packing is assumed with no padding between
                                        * structures then data can take the following form
                                        *
                                        *   icNamedColor2Entry  entry0; // Entry for first color
                                        *   icNamedColor2Entry  entry1; // Entry for second color
                                        *                      :
                                        *                      :
                                        * In either case repeat for name and PCS and device color coordinates up to (count-1)
                                        *
                                        * NOTES:
                                        * PCS and device space can be determined from the header.
                                        *
                                        * PCS coordinates are icUInt16 numbers and are described in Annex A of
                                        * the ICC spec. Only 16 bit L*a*b* and XYZ are allowed. The number of
                                        * coordinates is consistent with the headers PCS.
                                        *
                                        * Device coordinates are icUInt16 numbers where 0x0000 represents
                                        * the minimum value and 0xFFFF represents the maximum value.
                                        * If the nDeviceCoords value is 0 this field is not given.
                                        */
} icNamedColor2;

/** Profile sequence structure */
typedef struct {
    icSignature                 deviceMfg;      /* Device Manufacturer */
    icSignature                 deviceModel;    /* Decvice Model */
    icUInt64Number              attributes;     /* Device attributes */
    icTechnologySignature       technology;     /* Technology signature */
    icInt8Number                data[icAny];    /* Descriptions text follows */
                                                /**
                                                *  Data that follows is of this form, this is an icInt8Number
                                                *  to avoid problems with a compiler generating  bad code as
                                                *  these arrays are variable in length.
                                                *
                                                * icTextDescription            deviceMfgDesc;  * Manufacturer text
                                                * icTextDescription            modelDesc;      * Model text
                                                */
} icDescStruct;

/** Profile sequence description */
typedef struct {
    icUInt32Number      count;          /* Number of descriptions */
    icUInt8Number       data[icAny];    /* Array of description struct */
} icProfileSequenceDesc;

/** textDescription */
typedef struct {
    icUInt32Number      count;          /* Description length */
    icInt8Number        data[icAny];    /* Descriptions follow */
                                        /**
                                        *  Data that follows is of this form
                                        *
                                        * icInt8Number         desc[count]     * NULL terminated ascii string
                                        * icUInt32Number       ucLangCode;     * UniCode language code
                                        * icUInt32Number       ucCount;        * UniCode description length
                                        * icInt16Number        ucDesc[ucCount];* The UniCode description
                                        * icUInt16Number       scCode;         * ScriptCode code
                                        * icUInt8Number        scCount;        * ScriptCode count
                                        * icInt8Number         scDesc[67];     * ScriptCode Description
                                        */
} icTextDescription;

/** Screening Data */
typedef struct {
    icS15Fixed16Number  frequency;      /* Frequency */
    icS15Fixed16Number  angle;          /* Screen angle */
    icSpotShape         spotShape;      /* Spot Shape encodings below */
} icScreeningData;

/** screening */
typedef struct {
    icUInt32Number      screeningFlag;  /* Screening flag */
    icUInt32Number      channels;       /* Number of channels */
    icScreeningData     data[icAny];    /* Array of screening data */
} icScreening;

/** Text Data */
typedef struct {
    icInt8Number        data[icAny];    /* Variable array of characters */
} icText;

/** Structure describing either a UCR or BG curve */
typedef struct {
    icUInt32Number      count;          /* Curve length */
    icUInt16Number      curve[icAny];   /* The array of curve values */
} icUcrBgCurve;

/** Under color removal, black generation */
typedef struct {
    icInt8Number        data[icAny];    /* The Ucr BG data */
                                        /**
                                        *  Data that follows is of this form, this is a icInt8Number
                                        *  to avoid problems with a compiler generating  bad code as
                                        *  these arrays are variable in length.
                                        *
                                        * icUcrBgCurve         ucr;            * Ucr curve
                                        * icUcrBgCurve         bg;             * Bg curve
                                        * icInt8Number         string;         * UcrBg description
                                        */
} icUcrBg;

/** viewingConditionsType */
typedef struct {
    icXYZNumber         illuminant;     /* In candelas per metre sq'd */
    icXYZNumber         surround;       /* In candelas per metre sq'd */
    icIlluminant        stdIluminant;   /* See icIlluminant defines */
} icViewingCondition;

/** CrdInfo type */
typedef struct {
    icUInt32Number      count;          /* Char count includes NULL */
    icInt8Number        desc[icAny];    /* Null terminated string */
} icCrdInfo;

/** ColorantOrder type */
typedef struct {
    icUInt32Number      count;          /* Count of colorants           */
    icUInt8Number        data[icAny];    /* One-based number of the
                                         colorant to be printed first,
                                         second...                    */
} icColorantOrder;

/** ColorantTable Entry */
typedef struct {
    icInt8Number        name[32];       /* First colorant name              */
    icUInt16Number       data[3];        /* 16 bit PCS Lab value for first   */
} icColorantTableEntry;

/** ColorantTable */
typedef struct {
    icUInt32Number          count;          /* Count of colorants           */
    icColorantTableEntry    entry[icAny];   /* N colorant entries           */
} icColorantTable;

/*------------------------------------------------------------------------*/

/**
* Tag Type definitions
*/


/** The base part of each tag */
typedef struct {
    icTagTypeSignature  sig;            /* Signature */
    icInt8Number        reserved[4];    /* Reserved, set to 0 */
} icTagBase;

/** curveType */
typedef struct {
    icTagBase           base;           /* Signature, "curv" */
    icCurve             curve;          /* The curve data */
} icCurveType;

/** ParametricCurveType */
typedef struct {
    icTagBase           base;           /* Signature, "para"        */
    icParametricCurve   curve;          /* The Parametric curve data*/
} icParametricCurveType;

/** ParametricCurveFullType */
typedef struct {
    icTagBase           base;           /* Signature, "para"        */
    icParametricCurveFull curve;        /* The Parametric curve data*/
} icParametricCurveFullType;

/** dataType */
typedef struct {
    icTagBase           base;           /* Signature, "data" */
    icData              data;           /* The data structure */
} icDataType;

/** dateTimeType */
typedef struct {
    icTagBase           base;           /* Signature, "dtim" */
    icDateTimeNumber    date;           /* The date */
} icDateTimeType;

/** lut16Type */
typedef struct {
    icTagBase           base;           /* Signature, "mft2" */
    icLut16             lut;            /* Lut16 data */
} icLut16Type;

/** lut8Type, input & output tables are always 256 bytes in length */
typedef struct {
    icTagBase           base;           /* Signature, "mft1" */
    icLut8              lut;            /* Lut8 data */
} icLut8Type;

/** lutAtoBType new format */
typedef struct {
    icTagBase           base;           /* Signature, "mAB " */
    icLutAtoB           lut;            /* icLutAtoB data    */
} icLutAtoBType;

/** lutBtoAType new format */
typedef struct {
    icTagBase           base;           /* Signature, "mBA " */
    icLutBtoA           lut;            /* icLutBtoA data    */
} icLutBtoAType;

/** Measurement Type */
typedef struct {
    icTagBase           base;           /* Signature, "meas" */
    icMeasurement       measurement;    /* Measurement data  */
} icMeasurementType;

/**
* Named color type
*/

/** icNamedColor2Type, replaces icNamedColorType */
typedef struct {
    icTagBase           base;           /* Signature, "ncl2" */
    icNamedColor2       ncolor;         /* Named color data  */
} icNamedColor2Type;

/** Profile sequence description type */
typedef struct {
    icTagBase               base;       /* Signature, "pseq" */
    icProfileSequenceDesc   desc;       /* The seq description */
} icProfileSequenceDescType;

/** textDescriptionType */
typedef struct {
    icTagBase           base;           /* Signature, "desc" */
    icTextDescription   desc;           /* The description */
} icTextDescriptionType;

/** s15Fixed16Type */
typedef struct {
    icTagBase           base;           /* Signature, "sf32" */
    icS15Fixed16Array   data;           /* Array of values */
} icS15Fixed16ArrayType;

/** screeningType */
typedef struct {
    icTagBase           base;           /* Signature, "scrn" */
    icScreening         screen;         /* Screening structure */
} icScreeningType;

/** sigType */
typedef struct {
    icTagBase           base;           /* Signature, "sig" */
    icSignature         signature;      /* The signature data */
} icSignatureType;

/** textType */
typedef struct {
    icTagBase           base;           /* Signature, "text" */
    icText              data;           /* Variable array of characters */
} icTextType;

/** u16Fixed16Type */
typedef struct {
    icTagBase           base;           /* Signature, "uf32" */
    icU16Fixed16Array   data;           /* Variable array of values */
} icU16Fixed16ArrayType;

/** Under color removal, black generation type */
typedef struct {
    icTagBase           base;           /* Signature, "bfd " */
    icUcrBg             data;           /* ucrBg structure */
} icUcrBgType;

/** uInt16Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui16" */
    icUInt16Array       data;           /* Variable array of values */
} icUInt16ArrayType;

/** uInt32Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui32" */
    icUInt32Array       data;           /* Variable array of values */
} icUInt32ArrayType;

/** uInt64Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui64" */
    icUInt64Array       data;           /* Variable array of values */
} icUInt64ArrayType;

/** uInt8Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui08" */
    icUInt8Array        data;           /* Variable array of values */
} icUInt8ArrayType;

/** viewingConditionsType */
typedef struct {
    icTagBase           base;           /* Signature, "view" */
    icViewingCondition  view;           /* Viewing conditions */
} icViewingConditionType;

/** XYZ Type */
typedef struct {
    icTagBase           base;           /* Signature, "XYZ" */
    icXYZArray          data;           /* Variable array of XYZ numbers */
} icXYZType;

/**
* CRDInfoType where [0] is the CRD product name count and string and
* [1] -[5] are the rendering intents 0-4 counts and strings
*/
typedef struct {
    icTagBase           base;           /* Signature, "crdi" */
    icCrdInfo           info;           /* 5 sets of counts & strings */
}icCrdInfoType;
/*   icCrdInfo       productName;     PS product count/string */
/*   icCrdInfo       CRDName0;        CRD name for intent 0 */
/*   icCrdInfo       CRDName1;        CRD name for intent 1 */
/*   icCrdInfo       CRDName2;        CRD name for intent 2 */
/*   icCrdInfo       CRDName3;        CRD name for intent 3 */

/** ColorantOrderType type */
typedef struct {
    icTagBase           base;           /* Signature, "clro" */
    icColorantOrder     order;          /* ColorantOrder     */
}icColorantOrderType;

/** ColorantTableType type */
typedef struct {
    icTagBase           base;           /* Signature, "clrt" */
    icColorantTable     table;          /* ColorantTable     */
}icColorantTableType;

/** ChromaticAdaptation type */
typedef struct {
    icTagBase           base;           /* Signature, "chad"            */
    icS15Fixed16Number  matrix[9];      /* ChromaticAdaptation Matrix   */
}icChromaticAdaptationType;

/** MultiLocalizedUnicodeEntry type */
typedef struct {
    icUInt16Number      languageCode;   /* name language code ISO-639           */
    icUInt16Number      countryCode;    /* name country code ISO-3166           */
    icUInt32Number      len;            /* string length in bytes               */
    icUInt32Number      off;            /* offset in bytes from start of tag    */
}icMultiLocalizedUnicodeEntry;

/** MultiLocalizedUnicode type */
typedef struct {
    icTagBase           base;           /* Signature, "mluc"            */
    icUInt32Number      count;          /* Count of name records        */
    icUInt32Number      size;           /* name record size             */
}icMultiLocalizedUnicodeType;

/*------------------------------------------------------------------------*/

/**
* Lists of tags, tags, profile header and profile structure
*/

/** A tag */
typedef struct {
    icTagSignature      sig;            /* The tag signature */
    icUInt32Number      offset;         /* Start of tag relative to
                                        * start of header, Spec
                                        * Clause 5 */
    icUInt32Number      size;           /* Size in bytes */
} icTag;

/** A Structure that may be used independently for a list of tags */
typedef struct {
    icUInt32Number      count;          /* Number of tags in the profile */
    icTag               tags[icAny];    /* Variable array of tags */
} icTagList;

/** Profile ID */
typedef union {
    icUInt8Number         ID8[16];
    icUInt16Number        ID16[8];
    icUInt32Number        ID32[4];
} icProfileID;

/** The Profile header */
typedef struct {
    icUInt32Number              size;           /* Profile size in bytes */
    icSignature                 cmmId;          /* CMM for this profile */
    icUInt32Number              version;        /* Format version number */
    icProfileClassSignature     deviceClass;    /* Type of profile */
    icColorSpaceSignature       colorSpace;     /* Color space of data */
    icColorSpaceSignature       pcs;            /* PCS, XYZ or Lab only */
    icDateTimeNumber            date;           /* Date profile was created */
    icSignature                 magic;          /* icMagicNumber */
    icPlatformSignature         platform;       /* Primary Platform */
    icUInt32Number              flags;          /* Various bit settings */
    icSignature                 manufacturer;   /* Device manufacturer */
    icUInt32Number              model;          /* Device model number */
    icUInt64Number              attributes;     /* Device attributes */
    icUInt32Number              renderingIntent;/* Rendering intent */
    icXYZNumber                 illuminant;     /* Profile illuminant */
    icSignature                 creator;        /* Profile creator */
    icProfileID                 profileID;      /* Profile ID using RFC 1321 MD5 128bit fingerprinting */
    icInt8Number                reserved[28];   /* Reserved for future use */
} icHeader;

/**
* A profile,
* we can't use icTagList here because its not at the end of the structure
*/
typedef struct {
    icHeader            header;         /* The header */
    icTagList           tagList;        /* with tagList */
                                        /* Original:
                                        icHeader            header;             The header
                                        icUInt32Number      count;              Number of tags in the profile
                                        icInt8Number        data[icAny];        The tagTable and tagData */
                                        /*
                                        * Data that follows is of the form
                                        *
                                        * icTag        tagTable[icAny];        * The tag table
                                        * icInt8Number tagData[icAny];         * The tag data
                                        */
} icProfile;

/*------------------------------------------------------------------------*/
/* Obsolete entries */

/* icNamedColor was replaced with icNamedColor2 *
typedef struct {
icUInt32Number      vendorFlag;     / Bottom 16 bits for IC use *
icUInt32Number      count;          / Count of named colors *
icInt8Number        data[icAny];    / Named color data follows *
*
*  Data that follows is of this form
*
* icInt8Number         prefix[icAny];      * Prefix for the color name, max = 32
* icInt8Number         suffix[icAny];      * Suffix for the color name, max = 32
* icInt8Number         root1[icAny];       * Root name for first color, max = 32
* icInt8Number         coords1[icAny];     * Color coordinates of first color
* icInt8Number         root2[icAny];       * Root name for first color, max = 32
* icInt8Number         coords2[icAny];     * Color coordinates of first color
*                      :
*                      :
* Repeat for root name and color coordinates up to (count-1)
*
} icNamedColor; */

/* icNamedColorType was replaced by icNamedColor2Type *
typedef struct {
icTagBase           base;           / Signature, "ncol" *
icNamedColor        ncolor;         / Named color data *
} icNamedColorType; */

#endif /* icPROFILEHEADER_H */




