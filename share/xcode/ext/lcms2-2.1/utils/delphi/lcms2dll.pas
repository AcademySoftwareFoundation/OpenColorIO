//
//  Little cms DELPHI wrapper
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
// Version 2.1
//

UNIT lcms2dll;

INTERFACE

USES Windows;

 TYPE 

  Uint8   = Byte;
  Int8    = Shortint;
  UInt16  = Word;
  Int16   = Smallint;
  UInt32  = LongWord;
  Int32   = Longint;

 TYPE 
     cmsUInt8Number   = Uint8;
     cmsInt8Number    = Int8;
     cmsUInt16Number  = UInt16;
     cmsInt16Number   = Int16;

     cmsUInt32Number  = UInt32;
     cmsInt32Number   = Int32;
     cmsInt64Number   = Int64;
     cmsUInt64Number  = UInt64;

     cmsFloat32Number = Single;
     cmsFloat64Number = Double;

     LPcmsUInt8Number    = ^cmsUInt8Number;
     LPcmsInt8Number     = ^cmsInt8Number;  
     LPcmsUInt16Number   = ^cmsUInt16Number;  
     LPcmsInt16Number    = ^cmsInt16Number;  
     
     LPcmsUInt32Number   = ^cmsUInt32Number;  
     LPcmsInt32Number    = ^cmsInt32Number;  
     LPcmsInt64Number    = ^cmsInt64Number;  
     LPcmsUInt64Number   = ^cmsUInt64Number;  
     
     LPcmsFloat32Number  = ^cmsFloat32Number; 
     LPcmsFloat64Number  = ^cmsFloat64Number; 


     // Derivative types
     cmsSignature        = cmsUInt32Number;
     cmsU8Fixed8Number   = cmsUInt16Number;
     cmsS15Fixed16Number = cmsInt32Number;
     cmsU16Fixed16Number = cmsUInt32Number;

     // Boolean type, which will be using the native integer
     cmsBool = Boolean;

 CONST

    // Some common definitions
    cmsMAX_PATH     = 256;

    // D50 XYZ normalized to Y=1.0
    cmsD50X             = 0.9642;
    cmsD50Y             = 1.0;
    cmsD50Z             = 0.8249;

    // V4 perceptual black
    cmsPERCEPTUAL_BLACK_X  = 0.00336;
    cmsPERCEPTUAL_BLACK_Y  = 0.0034731;
    cmsPERCEPTUAL_BLACK_Z  = 0.00287;

    // Definitions in ICC spec
    cmsMagicNumber      = $61637370;     // 'acsp'
    lcmsSignature       = $6c636d73;     // 'lcms'


TYPE

// Base ICC type definitions
cmsTagTypeSignature = (
  cmsSigChromaticityType                  = $6368726D,  // 'chrm'
  cmsSigColorantOrderType                 = $636C726F,  // 'clro'
  cmsSigColorantTableType                 = $636C7274,  // 'clrt'
  cmsSigCrdInfoType                       = $63726469,  // 'crdi'
  cmsSigCurveType                         = $63757276,  // 'curv'
  cmsSigDataType                          = $64617461,  // 'data'
  cmsSigDateTimeType                      = $6474696D,  // 'dtim'
  cmsSigDeviceSettingsType                = $64657673,  // 'devs'
  cmsSigLut16Type                         = $6d667432,  // 'mft2'
  cmsSigLut8Type                          = $6d667431,  // 'mft1'
  cmsSigLutAtoBType                       = $6d414220,  // 'mAB '
  cmsSigLutBtoAType                       = $6d424120,  // 'mBA '
  cmsSigMeasurementType                   = $6D656173,  // 'meas'
  cmsSigMultiLocalizedUnicodeType         = $6D6C7563,  // 'mluc'
  cmsSigMultiProcessElementType           = $6D706574,  // 'mpet'
  cmsSigNamedColorType                    = $6E636f6C,  // 'ncol' -- DEPRECATED!
  cmsSigNamedColor2Type                   = $6E636C32,  // 'ncl2'
  cmsSigParametricCurveType               = $70617261,  // 'para'
  cmsSigProfileSequenceDescType           = $70736571,  // 'pseq'
  cmsSigProfileSequenceIdType             = $70736964,  // 'psid'
  cmsSigResponseCurveSet16Type            = $72637332,  // 'rcs2'
  cmsSigS15Fixed16ArrayType               = $73663332,  // 'sf32'
  cmsSigScreeningType                     = $7363726E,  // 'scrn'
  cmsSigSignatureType                     = $73696720,  // 'sig '
  cmsSigTextType                          = $74657874,  // 'text'
  cmsSigTextDescriptionType               = $64657363,  // 'desc'
  cmsSigU16Fixed16ArrayType               = $75663332,  // 'uf32'
  cmsSigUcrBgType                         = $62666420,  // 'bfd '
  cmsSigUInt16ArrayType                   = $75693136,  // 'ui16'
  cmsSigUInt32ArrayType                   = $75693332,  // 'ui32'
  cmsSigUInt64ArrayType                   = $75693634,  // 'ui64'
  cmsSigUInt8ArrayType                    = $75693038,  // 'ui08'
  cmsSigViewingConditionsType             = $76696577,  // 'view'
  cmsSigXYZType                           = $58595A20,  // 'XYZ '
  cmsSigVcgtType                          = $76636774   // 'vcgt'
  );

// Base ICC tag definitions
cmsTagSignature = (
    cmsSigAToB0Tag                          = $41324230,  // 'A2B0'
    cmsSigAToB1Tag                          = $41324231,  // 'A2B1'
    cmsSigAToB2Tag                          = $41324232,  // 'A2B2'
    cmsSigBlueColorantTag                   = $6258595A,  // 'bXYZ'
    cmsSigBlueMatrixColumnTag               = $6258595A,  // 'bXYZ'
    cmsSigBlueTRCTag                        = $62545243,  // 'bTRC'
    cmsSigBToA0Tag                          = $42324130,  // 'B2A0'
    cmsSigBToA1Tag                          = $42324131,  // 'B2A1'
    cmsSigBToA2Tag                          = $42324132,  // 'B2A2'
    cmsSigCalibrationDateTimeTag            = $63616C74,  // 'calt'
    cmsSigCharTargetTag                     = $74617267,  // 'targ'
    cmsSigChromaticAdaptationTag            = $63686164,  // 'chad'
    cmsSigChromaticityTag                   = $6368726D,  // 'chrm'
    cmsSigColorantOrderTag                  = $636C726F,  // 'clro'
    cmsSigColorantTableTag                  = $636C7274,  // 'clrt'
    cmsSigColorantTableOutTag               = $636C6F74,  // 'clot'
    cmsSigColorimetricIntentImageStateTag   = $63696973,  // 'ciis'
    cmsSigCopyrightTag                      = $63707274,  // 'cprt'
    cmsSigCrdInfoTag                        = $63726469,  // 'crdi'
    cmsSigDataTag                           = $64617461,  // 'data'
    cmsSigDateTimeTag                       = $6474696D,  // 'dtim'
    cmsSigDeviceMfgDescTag                  = $646D6E64,  // 'dmnd'
    cmsSigDeviceModelDescTag                = $646D6464,  // 'dmdd'
    cmsSigDeviceSettingsTag                 = $64657673,  // 'devs'
    cmsSigDToB0Tag                          = $44324230,  // 'D2B0'
    cmsSigDToB1Tag                          = $44324231,  // 'D2B1'
    cmsSigDToB2Tag                          = $44324232,  // 'D2B2'
    cmsSigDToB3Tag                          = $44324233,  // 'D2B3'
    cmsSigBToD0Tag                          = $42324430,  // 'B2D0'
    cmsSigBToD1Tag                          = $42324431,  // 'B2D1'
    cmsSigBToD2Tag                          = $42324432,  // 'B2D2'
    cmsSigBToD3Tag                          = $42324433,  // 'B2D3'
    cmsSigGamutTag                          = $67616D74,  // 'gamt'
    cmsSigGrayTRCTag                        = $6b545243,  // 'kTRC'
    cmsSigGreenColorantTag                  = $6758595A,  // 'gXYZ'
    cmsSigGreenMatrixColumnTag              = $6758595A,  // 'gXYZ'
    cmsSigGreenTRCTag                       = $67545243,  // 'gTRC'
    cmsSigLuminanceTag                      = $6C756d69,  // 'lumi'
    cmsSigMeasurementTag                    = $6D656173,  // 'meas'
    cmsSigMediaBlackPointTag                = $626B7074,  // 'bkpt'
    cmsSigMediaWhitePointTag                = $77747074,  // 'wtpt'
    cmsSigNamedColorTag                     = $6E636f6C,  // 'ncol' // Deprecated by the ICC
    cmsSigNamedColor2Tag                    = $6E636C32,  // 'ncl2'
    cmsSigOutputResponseTag                 = $72657370,  // 'resp'
    cmsSigPerceptualRenderingIntentGamutTag = $72696730,  // 'rig0'
    cmsSigPreview0Tag                       = $70726530,  // 'pre0'
    cmsSigPreview1Tag                       = $70726531,  // 'pre1'
    cmsSigPreview2Tag                       = $70726532,  // 'pre2'
    cmsSigProfileDescriptionTag             = $64657363,  // 'desc'
    cmsSigProfileSequenceDescTag            = $70736571,  // 'pseq'
    cmsSigProfileSequenceIdTag              = $70736964,  // 'psid'
    cmsSigPs2CRD0Tag                        = $70736430,  // 'psd0'
    cmsSigPs2CRD1Tag                        = $70736431,  // 'psd1'
    cmsSigPs2CRD2Tag                        = $70736432,  // 'psd2'
    cmsSigPs2CRD3Tag                        = $70736433,  // 'psd3'
    cmsSigPs2CSATag                         = $70733273,  // 'ps2s'
    cmsSigPs2RenderingIntentTag             = $70733269,  // 'ps2i'
    cmsSigRedColorantTag                    = $7258595A,  // 'rXYZ'
    cmsSigRedMatrixColumnTag                = $7258595A,  // 'rXYZ'
    cmsSigRedTRCTag                         = $72545243,  // 'rTRC'
    cmsSigSaturationRenderingIntentGamutTag = $72696732,  // 'rig2'
    cmsSigScreeningDescTag                  = $73637264,  // 'scrd'
    cmsSigScreeningTag                      = $7363726E,  // 'scrn'
    cmsSigTechnologyTag                     = $74656368,  // 'tech'
    cmsSigUcrBgTag                          = $62666420,  // 'bfd '
    cmsSigViewingCondDescTag                = $76756564,  // 'vued'
    cmsSigViewingConditionsTag              = $76696577,  // 'view'
    cmsSigVcgtTag                           = $76636774   // 'vcgt'
);

// ICC Technology tag
cmsTechnologySignature = (
    cmsSigDigitalCamera                     = $6463616D,  // 'dcam'
    cmsSigFilmScanner                       = $6673636E,  // 'fscn'
    cmsSigReflectiveScanner                 = $7273636E,  // 'rscn'
    cmsSigInkJetPrinter                     = $696A6574,  // 'ijet'
    cmsSigThermalWaxPrinter                 = $74776178,  // 'twax'
    cmsSigElectrophotographicPrinter        = $6570686F,  // 'epho'
    cmsSigElectrostaticPrinter              = $65737461,  // 'esta'
    cmsSigDyeSublimationPrinter             = $64737562,  // 'dsub'
    cmsSigPhotographicPaperPrinter          = $7270686F,  // 'rpho'
    cmsSigFilmWriter                        = $6670726E,  // 'fprn'
    cmsSigVideoMonitor                      = $7669646D,  // 'vidm'
    cmsSigVideoCamera                       = $76696463,  // 'vidc'
    cmsSigProjectionTelevision              = $706A7476,  // 'pjtv'
    cmsSigCRTDisplay                        = $43525420,  // 'CRT '
    cmsSigPMDisplay                         = $504D4420,  // 'PMD '
    cmsSigAMDisplay                         = $414D4420,  // 'AMD '
    cmsSigPhotoCD                           = $4B504344,  // 'KPCD'
    cmsSigPhotoImageSetter                  = $696D6773,  // 'imgs'
    cmsSigGravure                           = $67726176,  // 'grav'
    cmsSigOffsetLithography                 = $6F666673,  // 'offs'
    cmsSigSilkscreen                        = $73696C6B,  // 'silk'
    cmsSigFlexography                       = $666C6578,  // 'flex'
    cmsSigMotionPictureFilmScanner          = $6D706673,  // 'mpfs'
    cmsSigMotionPictureFilmRecorder         = $6D706672,  // 'mpfr'
    cmsSigDigitalMotionPictureCamera        = $646D7063,  // 'dmpc'
    cmsSigDigitalCinemaProjector            = $64636A70   // 'dcpj'
);


// ICC Color spaces
cmsColorSpaceSignature = (
    cmsSigXYZData                           = $58595A20,  // 'XYZ '
    cmsSigLabData                           = $4C616220,  // 'Lab '
    cmsSigLuvData                           = $4C757620,  // 'Luv '
    cmsSigYCbCrData                         = $59436272,  // 'YCbr'
    cmsSigYxyData                           = $59787920,  // 'Yxy '
    cmsSigRgbData                           = $52474220,  // 'RGB '
    cmsSigGrayData                          = $47524159,  // 'GRAY'
    cmsSigHsvData                           = $48535620,  // 'HSV '
    cmsSigHlsData                           = $484C5320,  // 'HLS '
    cmsSigCmykData                          = $434D594B,  // 'CMYK'
    cmsSigCmyData                           = $434D5920,  // 'CMY '
    cmsSigMCH1Data                          = $4D434831,  // 'MCH1'
    cmsSigMCH2Data                          = $4D434832,  // 'MCH2'
    cmsSigMCH3Data                          = $4D434833,  // 'MCH3'
    cmsSigMCH4Data                          = $4D434834,  // 'MCH4'
    cmsSigMCH5Data                          = $4D434835,  // 'MCH5'
    cmsSigMCH6Data                          = $4D434836,  // 'MCH6'
    cmsSigMCH7Data                          = $4D434837,  // 'MCH7'
    cmsSigMCH8Data                          = $4D434838,  // 'MCH8'
    cmsSigMCH9Data                          = $4D434839,  // 'MCH9'
    cmsSigMCHAData                          = $4D43483A,  // 'MCHA'
    cmsSigMCHBData                          = $4D43483B,  // 'MCHB'
    cmsSigMCHCData                          = $4D43483C,  // 'MCHC'
    cmsSigMCHDData                          = $4D43483D,  // 'MCHD'
    cmsSigMCHEData                          = $4D43483E,  // 'MCHE'
    cmsSigMCHFData                          = $4D43483F,  // 'MCHF'
    cmsSigNamedData                         = $6e6d636c,  // 'nmcl'
    cmsSig1colorData                        = $31434C52,  // '1CLR'
    cmsSig2colorData                        = $32434C52,  // '2CLR'
    cmsSig3colorData                        = $33434C52,  // '3CLR'
    cmsSig4colorData                        = $34434C52,  // '4CLR'
    cmsSig5colorData                        = $35434C52,  // '5CLR'
    cmsSig6colorData                        = $36434C52,  // '6CLR'
    cmsSig7colorData                        = $37434C52,  // '7CLR'
    cmsSig8colorData                        = $38434C52,  // '8CLR'
    cmsSig9colorData                        = $39434C52,  // '9CLR'
    cmsSig10colorData                       = $41434C52,  // 'ACLR'
    cmsSig11colorData                       = $42434C52,  // 'BCLR'
    cmsSig12colorData                       = $43434C52,  // 'CCLR'
    cmsSig13colorData                       = $44434C52,  // 'DCLR'
    cmsSig14colorData                       = $45434C52,  // 'ECLR'
    cmsSig15colorData                       = $46434C52,  // 'FCLR'
    cmsSigLuvKData                          = $4C75764B   // 'LuvK'
);

// ICC Profile Class
cmsProfileClassSignature = (
    cmsSigInputClass                        = $73636E72,  // 'scnr'
    cmsSigDisplayClass                      = $6D6E7472,  // 'mntr'
    cmsSigOutputClass                       = $70727472,  // 'prtr'
    cmsSigLinkClass                         = $6C696E6B,  // 'link'
    cmsSigAbstractClass                     = $61627374,  // 'abst'
    cmsSigColorSpaceClass                   = $73706163,  // 'spac'
    cmsSigNamedColorClass                   = $6e6d636c   // 'nmcl'
);


// ICC Platforms
cmsPlatformSignature = (
    cmsSigMacintosh                         = $4150504C,  // 'APPL'
    cmsSigMicrosoft                         = $4D534654,  // 'MSFT'
    cmsSigSolaris                           = $53554E57,  // 'SUNW'
    cmsSigSGI                               = $53474920,  // 'SGI '
    cmsSigTaligent                          = $54474E54,  // 'TGNT'
    cmsSigUnices                            = $2A6E6978   // '*nix'   // From argyll -- Not official
);

CONST 

    // Reference gamut
    cmsSigPerceptualReferenceMediumGamut         = $70726d67;  //'prmg'

    // For cmsSigColorimetricIntentImageStateTag
    cmsSigSceneColorimetryEstimates              = $73636F65;  //'scoe'
    cmsSigSceneAppearanceEstimates               = $73617065;  //'sape'
    cmsSigFocalPlaneColorimetryEstimates         = $66706365;  //'fpce'
    cmsSigReflectionHardcopyOriginalColorimetry  = $72686F63;  //'rhoc'
    cmsSigReflectionPrintOutputColorimetry       = $72706F63;  //'rpoc'

TYPE

// Multi process elements types
cmsStageSignature = (
    cmsSigCurveSetElemType              = $63767374,  //'cvst'
    cmsSigMatrixElemType                = $6D617466,  //'matf'
    cmsSigCLutElemType                  = $636C7574,  //'clut'

    cmsSigBAcsElemType                  = $62414353,  // 'bACS'
    cmsSigEAcsElemType                  = $65414353,  // 'eACS'

    // Custom from here, not in the ICC Spec
    cmsSigXYZ2LabElemType               = $6C327820,  // 'l2x '
    cmsSigLab2XYZElemType               = $78326C20,  // 'x2l '
    cmsSigNamedColorElemType            = $6E636C20,  // 'ncl '
    cmsSigLabV2toV4                     = $32203420,  // '2 4 '
    cmsSigLabV4toV2                     = $34203220,  // '4 2 '

    // Identities
    cmsSigIdentityElemType              = $69646E20   // 'idn '
);

// Types of CurveElements
cmsCurveSegSignature = (

    cmsSigFormulaCurveSeg               = $70617266, // 'parf'
    cmsSigSampledCurveSeg               = $73616D66, // 'samf'
    cmsSigSegmentedCurve                = $63757266  // 'curf'
);

CONST 

    // Used in ResponseCurveType
    cmsSigStatusA                    = $53746141; //'StaA'
    cmsSigStatusE                    = $53746145; //'StaE'
    cmsSigStatusI                    = $53746149; //'StaI'
    cmsSigStatusT                    = $53746154; //'StaT'
    cmsSigStatusM                    = $5374614D; //'StaM'
    cmsSigDN                         = $444E2020; //'DN  '
    cmsSigDNP                        = $444E2050; //'DN P'
    cmsSigDNN                        = $444E4E20; //'DNN '
    cmsSigDNNP                       = $444E4E50; //'DNNP'

    // Device attributes, currently defined values correspond to the low 4 bytes
    // of the 8 byte attribute quantity
    cmsReflective     = 0;
    cmsTransparency   = 1;
    cmsGlossy         = 0;
    cmsMatte          = 2;

TYPE

// Common structures in ICC tags
cmsICCData = PACKED RECORD
     len  :    cmsUInt32Number;
     flag :    cmsUInt32Number;
     data : Array [0..1] of cmsUInt8Number;
    END;

// ICC date time
cmsDateTimeNumber = PACKED RECORD
    year:     cmsUInt16Number;     
    month:    cmsUInt16Number;     
    day:      cmsUInt16Number;     
    hours:    cmsUInt16Number;     
    minutes:  cmsUInt16Number;     
    seconds:  cmsUInt16Number;                 
END;

// ICC XYZ

cmsEncodedXYZNumber = PACKED RECORD
      X: cmsS15Fixed16Number;
      Y: cmsS15Fixed16Number;
      Z: cmsS15Fixed16Number;
END;


// Profile ID as computed by MD5 algorithm
cmsProfileID = PACKED RECORD
    CASE Integer OF
    1: (ID8: Array[0..15] OF cmsUInt8Number);
    2: (ID16: Array[0..7] OF cmsUInt16Number);
    3: (ID32: Array[0..3] OF cmsUInt32Number);
END;



// ----------------------------------------------------------------------------------------------
// ICC profile internal base types. Strictly, shouldn't be declared in this unit, but maybe
// somebody want to use this info for accessing profile header directly, so here it is.

// Profile header -- it is 32-bit aligned, so no issues are expected on alignment
cmsICCHeader = PACKED RECORD
         size:           cmsUInt32Number;          // Profile size in bytes
         cmmId:          cmsSignature;             // CMM for this profile
         version:        cmsUInt32Number;          // Format version number
         deviceClass:    cmsProfileClassSignature; // Type of profile
         colorSpace:     cmsColorSpaceSignature;   // Color space of data
         pcs:            cmsColorSpaceSignature;   // PCS, XYZ or Lab only
         date:           cmsDateTimeNumber;        // Date profile was created
         magic:          cmsSignature;             // Magic Number to identify an ICC profile
         platform:       cmsPlatformSignature;     // Primary Platform
         flags:          cmsUInt32Number;          // Various bit settings
         manufacturer:   cmsSignature;             // Device manufacturer
         model:          cmsUInt32Number;          // Device model number
         attributes:     cmsUInt64Number;          // Device attributes
         renderingIntent:cmsUInt32Number;          // Rendering intent
         illuminant:     cmsEncodedXYZNumber;      // Profile illuminant
         creator:        cmsSignature;             // Profile creator
         profileID:      cmsProfileID;             // Profile ID using MD5
         reserved: array [0..27] of cmsInt8Number; // Reserved for future use
END;

// ICC base tag
cmsTagBase = PACKED RECORD
     sig:         cmsTagTypeSignature; 
     reserved:    array[0..3] of cmsInt8Number;       
END;

// A tag entry in directory
cmsTagEntry = PACKED RECORD
    sig:    cmsTagSignature;   // The tag signature
    offset: cmsUInt32Number;   // Start of tag
    size:   cmsUInt32Number;   // Size in bytes
END;


cmsContext    = Pointer;              // Context identifier for multithreaded environments
cmsHANDLE     = Pointer;              // Generic handle
cmsHPROFILE   = Pointer;             // Opaque typedefs to hide internals
cmsHTRANSFORM = Pointer;


CONST

     cmsMAXCHANNELS  = 16;                // Maximum number of channels in ICC profiles

// Format of pixel is defined by one cmsUInt32Number, using bit fields as follows
//
//            A O TTTTT U Y F P X S EEE CCCC BBB
//
//            A: Floating point -- With this flag we can differentiate 16 bits as float and as int
//            O: Optimized -- previous optimization already returns the final 8-bit value
//            T: Pixeltype
//            F: Flavor  0=MinIsBlack(Chocolate) 1=MinIsWhite(Vanilla)
//            P: Planar? 0=Chunky, 1=Planar
//            X: swap 16 bps endianess?
//            S: Do swap? ie, BGR, KYMC
//            E: Extra samples
//            C: Channels (Samples per pixel)
//            B: bytes per sample
//            Y: Swap first - changes ABGR to BGRA and KCMY to CMYK

    FUNCTION FLOAT_SH(a: cmsUInt32Number): cmsUInt32Number;
    FUNCTION OPTIMIZED_SH(s: cmsUInt32Number): cmsUInt32Number;
    FUNCTION COLORSPACE_SH(s: cmsUInt32Number):cmsUInt32Number;
    FUNCTION SWAPFIRST_SH(s: cmsUInt32Number):cmsUInt32Number;
    FUNCTION FLAVOR_SH(s: cmsUInt32Number):cmsUInt32Number;
    FUNCTION PLANAR_SH(p: cmsUInt32Number):cmsUInt32Number;
    FUNCTION ENDIAN16_SH(e: cmsUInt32Number):cmsUInt32Number;
    FUNCTION DOSWAP_SH(e: cmsUInt32Number):cmsUInt32Number;
    FUNCTION EXTRA_SH(e: cmsUInt32Number):cmsUInt32Number;
    FUNCTION CHANNELS_SH(c: cmsUInt32Number):cmsUInt32Number;
    FUNCTION BYTES_SH(b: cmsUInt32Number):cmsUInt32Number;


    FUNCTION T_FLOAT(a: cmsUInt32Number): cmsUInt32Number;          
    FUNCTION T_OPTIMIZED(o: cmsUInt32Number): cmsUInt32Number;      
    FUNCTION T_COLORSPACE(s: cmsUInt32Number): cmsUInt32Number;     
    FUNCTION T_SWAPFIRST(s: cmsUInt32Number): cmsUInt32Number;      
    FUNCTION T_FLAVOR(s: cmsUInt32Number): cmsUInt32Number;         
    FUNCTION T_PLANAR(p: cmsUInt32Number): cmsUInt32Number;         
    FUNCTION T_ENDIAN16(e: cmsUInt32Number): cmsUInt32Number;       
    FUNCTION T_DOSWAP(e: cmsUInt32Number): cmsUInt32Number;         
    FUNCTION T_EXTRA(e: cmsUInt32Number): cmsUInt32Number;          
    FUNCTION T_CHANNELS(c: cmsUInt32Number): cmsUInt32Number;       
    FUNCTION T_BYTES(b: cmsUInt32Number): cmsUInt32Number;          

CONST


// Pixel types

    PT_ANY     =  0;    // Don't check colorspace
                      // 1 & 2 are reserved
    PT_GRAY    =  3;
    PT_RGB     =  4;
    PT_CMY     =  5;
    PT_CMYK    =  6;
    PT_YCbCr   =  7;
    PT_YUV     =  8;      // Lu'v'
    PT_XYZ     =  9;
    PT_Lab     =  10;
    PT_YUVK    =  11;     // Lu'v'K
    PT_HSV     =  12;
    PT_HLS     =  13;
    PT_Yxy     =  14;
                
    PT_MCH1    =  15;
    PT_MCH2    =  16;
    PT_MCH3    =  17;
    PT_MCH4    =  18;
    PT_MCH5    =  19;
    PT_MCH6    =  20;
    PT_MCH7    =  21;
    PT_MCH8    =  22;
    PT_MCH9    =  23;
    PT_MCH10   =  24;
    PT_MCH11   =  25;
    PT_MCH12   =  26;
    PT_MCH13   =  27;
    PT_MCH14   =  28;
    PT_MCH15   =  29;

    PT_LabV2   =  30;     // Identical to PT_Lab, but using the V2 old encoding

    
    // Format descriptors
    TYPE_GRAY_8          = $030009;
    TYPE_GRAY_8_REV      = $032009;
    TYPE_GRAY_16         = $03000a;
    TYPE_GRAY_16_REV     = $03200a;
    TYPE_GRAY_16_SE      = $03080a;
    TYPE_GRAYA_8         = $030089;
    TYPE_GRAYA_16        = $03008a;
    TYPE_GRAYA_16_SE     = $03088a;
    TYPE_GRAYA_8_PLANAR  = $031089;
    TYPE_GRAYA_16_PLANAR = $03108a;
    TYPE_RGB_8           = $040019;
    TYPE_RGB_8_PLANAR    = $041019;
    TYPE_BGR_8           = $040419;
    TYPE_BGR_8_PLANAR    = $041419;
    TYPE_RGB_16          = $04001a;
    TYPE_RGB_16_PLANAR   = $04101a;
    TYPE_RGB_16_SE       = $04081a;
    TYPE_BGR_16          = $04041a;
    TYPE_BGR_16_PLANAR   = $04141a;
    TYPE_BGR_16_SE       = $040c1a;
    TYPE_RGBA_8          = $040099;
    TYPE_RGBA_8_PLANAR   = $041099;
    TYPE_RGBA_16         = $04009a;
    TYPE_RGBA_16_PLANAR  = $04109a;
    TYPE_RGBA_16_SE      = $04089a;
    TYPE_ARGB_8          = $044099;
    TYPE_ARGB_16         = $04409a;
    TYPE_ABGR_8          = $040499;
    TYPE_ABGR_16         = $04049a;
    TYPE_ABGR_16_PLANAR  = $04149a;
    TYPE_ABGR_16_SE      = $040c9a;
    TYPE_BGRA_8          = $044499;
    TYPE_BGRA_16         = $04449a;
    TYPE_BGRA_16_SE      = $04489a;
    TYPE_CMY_8           = $050019;
    TYPE_CMY_8_PLANAR    = $051019;
    TYPE_CMY_16          = $05001a;
    TYPE_CMY_16_PLANAR   = $05101a;
    TYPE_CMY_16_SE       = $05081a;
    TYPE_CMYK_8          = $060021;
    TYPE_CMYKA_8         = $0600a1;
    TYPE_CMYK_8_REV      = $062021;
    TYPE_YUVK_8          = $062021;
    TYPE_CMYK_8_PLANAR   = $061021;
    TYPE_CMYK_16         = $060022;
    TYPE_CMYK_16_REV     = $062022;
    TYPE_YUVK_16         = $062022;
    TYPE_CMYK_16_PLANAR  = $061022;
    TYPE_CMYK_16_SE      = $060822;
    TYPE_KYMC_8          = $060421;
    TYPE_KYMC_16         = $060422;
    TYPE_KYMC_16_SE      = $060c22;
    TYPE_KCMY_8          = $064021;
    TYPE_KCMY_8_REV      = $066021;
    TYPE_KCMY_16         = $064022;
    TYPE_KCMY_16_REV     = $066022;
    TYPE_KCMY_16_SE      = $064822;
    TYPE_CMYK5_8         = $130029;
    TYPE_CMYK5_16        = $13002a;
    TYPE_CMYK5_16_SE     = $13082a;
    TYPE_KYMC5_8         = $130429;
    TYPE_KYMC5_16        = $13042a;
    TYPE_KYMC5_16_SE     = $130c2a;
    TYPE_CMYK6_8         = $140031;
    TYPE_CMYK6_8_PLANAR  = $141031;
    TYPE_CMYK6_16        = $140032;
    TYPE_CMYK6_16_PLANAR = $141032;
    TYPE_CMYK6_16_SE     = $140832;
    TYPE_CMYK7_8         = $150039;
    TYPE_CMYK7_16        = $15003a;
    TYPE_CMYK7_16_SE     = $15083a;
    TYPE_KYMC7_8         = $150439;
    TYPE_KYMC7_16        = $15043a;
    TYPE_KYMC7_16_SE     = $150c3a;
    TYPE_CMYK8_8         = $160041;
    TYPE_CMYK8_16        = $160042;
    TYPE_CMYK8_16_SE     = $160842;
    TYPE_KYMC8_8         = $160441;
    TYPE_KYMC8_16        = $160442;
    TYPE_KYMC8_16_SE     = $160c42;
    TYPE_CMYK9_8         = $170049;
    TYPE_CMYK9_16        = $17004a;
    TYPE_CMYK9_16_SE     = $17084a;
    TYPE_KYMC9_8         = $170449;
    TYPE_KYMC9_16        = $17044a;
    TYPE_KYMC9_16_SE     = $170c4a;
    TYPE_CMYK10_8        = $180051;
    TYPE_CMYK10_16       = $180052;
    TYPE_CMYK10_16_SE    = $180852;
    TYPE_KYMC10_8        = $180451;
    TYPE_KYMC10_16       = $180452;
    TYPE_KYMC10_16_SE    = $180c52;
    TYPE_CMYK11_8        = $190059;
    TYPE_CMYK11_16       = $19005a;
    TYPE_CMYK11_16_SE    = $19085a;
    TYPE_KYMC11_8        = $190459;
    TYPE_KYMC11_16       = $19045a;
    TYPE_KYMC11_16_SE    = $190c5a;
    TYPE_CMYK12_8        = $1a0061;
    TYPE_CMYK12_16       = $1a0062;
    TYPE_CMYK12_16_SE    = $1a0862;
    TYPE_KYMC12_8        = $1a0461;
    TYPE_KYMC12_16       = $1a0462;
    TYPE_KYMC12_16_SE    = $1a0c62;
    TYPE_XYZ_16          = $09001a;
    TYPE_Lab_8           = $0a0019;
    TYPE_ALab_8          = $0a0499;
    TYPE_Lab_16          = $0a001a;
    TYPE_Yxy_16          = $0e001a;
    TYPE_YCbCr_8         = $070019;
    TYPE_YCbCr_8_PLANAR  = $071019;
    TYPE_YCbCr_16        = $07001a;
    TYPE_YCbCr_16_PLANAR = $07101a;
    TYPE_YCbCr_16_SE     = $07081a;
    TYPE_YUV_8           = $080019;
    TYPE_YUV_8_PLANAR    = $081019;
    TYPE_YUV_16          = $08001a;
    TYPE_YUV_16_PLANAR   = $08101a;
    TYPE_YUV_16_SE       = $08081a;
    TYPE_HLS_8           = $0d0019;
    TYPE_HLS_8_PLANAR    = $0d1019;
    TYPE_HLS_16          = $0d001a;
    TYPE_HLS_16_PLANAR   = $0d101a;
    TYPE_HLS_16_SE       = $0d081a;
    TYPE_HSV_8           = $0c0019;
    TYPE_HSV_8_PLANAR    = $0c1019;
    TYPE_HSV_16          = $0c001a;
    TYPE_HSV_16_PLANAR   = $0c101a;
    TYPE_HSV_16_SE       = $0c081a;

    TYPE_NAMED_COLOR_INDEX = $000A;

    TYPE_XYZ_FLT         = $49001c;
    TYPE_Lab_FLT         = $4a001c;
    TYPE_GRAY_FLT        = $43000c;
    TYPE_RGB_FLT         = $44001c;
    TYPE_CMYK_FLT        = $460024;
    TYPE_XYZA_FLT        = $49009c;
    TYPE_LabA_FLT        = $4a009c;
    TYPE_RGBA_FLT        = $44009c;
    TYPE_XYZ_DBL         = $490018;
    TYPE_Lab_DBL         = $4a0018;
    TYPE_GRAY_DBL        = $430008;
    TYPE_RGB_DBL         = $440018;
    TYPE_CMYK_DBL        = $460020;
    TYPE_LabV2_8         = $1e0019;
    TYPE_ALabV2_8        = $1e0499;
    TYPE_LabV2_16        = $1e001a;

    
TYPE


  // Colorimetric spaces

      cmsCIEXYZ = PACKED RECORD
                        X, Y, Z : cmsFloat64Number;
                    END;
      LPcmsCIEXYZ = ^cmsCIEXYZ;

      cmsCIExyY = PACKED RECORD
                        x, y, YY : cmsFloat64Number
                        END;
      LPcmsCIExyY = ^cmsCIEXYY;

      cmsCIELab = PACKED RECORD
                  L, a, b: cmsFloat64Number
                  END;
      LPcmsCIELab = ^cmsCIELab;

     cmsCIELCh = PACKED RECORD
                  L, C, h : cmsFloat64Number
                  END;
     LPcmsCIELCh = ^cmsCIELCh;

     cmsJCh = PACKED RECORD
                  J, C, h : cmsFloat64Number
                  END;
     LPcmsJCh = ^cmsJCH;


     cmsCIEXYZTRIPLE = PACKED RECORD
                        Red, Green, Blue : cmsCIEXYZ
                        END;
     LPcmsCIEXYZTRIPLE = ^cmsCIEXYZTRIPLE;


      cmsCIExyYTRIPLE = PACKED RECORD
                        Red, Green, Blue : cmsCIExyY
                        END;
      LPcmsCIExyYTRIPLE = ^cmsCIExyYTRIPLE;


CONST

    // Illuminant types for structs below
    cmsILLUMINANT_TYPE_UNKNOWN = $0000000;
    cmsILLUMINANT_TYPE_D50     = $0000001;
    cmsILLUMINANT_TYPE_D65     = $0000002;
    cmsILLUMINANT_TYPE_D93     = $0000003;
    cmsILLUMINANT_TYPE_F2      = $0000004;
    cmsILLUMINANT_TYPE_D55     = $0000005;
    cmsILLUMINANT_TYPE_A       = $0000006;
    cmsILLUMINANT_TYPE_E       = $0000007;
    cmsILLUMINANT_TYPE_F8      = $0000008;
    
TYPE

    cmsICCMeasurementConditions = PACKED RECORD

        Observer: cmsUInt32Number;       // 0 = unknown, 1=CIE 1931, 2=CIE 1964
        Backing:  cmsCIEXYZ;             // Value of backing
        Geometry: cmsUInt32Number;       // 0=unknown, 1=45/0, 0/45 2=0d, d/0
        Flare:    cmsFloat64Number;      // 0..1.0
        IlluminantType: cmsUInt32Number;

    END;

   cmsICCViewingConditions = PACKED RECORD
        IlluminantXYZ: cmsCIEXYZ;         // Not the same struct as CAM02,
        SurroundXYZ: cmsCIEXYZ;           // This is for storing the tag
        IlluminantType: cmsUInt32Number;  // viewing condition
    END;


// Plug-In registering  ---------------------------------------------------------------------------------------------------

FUNCTION  cmsPlugin(Plugin: Pointer): cmsBool; StdCall;
PROCEDURE cmsUnregisterPlugins; StdCall;

// Error logging ----------------------------------------------------------------------------------------------------------

// There is no error handling at all. When a function fails, it returns proper value.
// For example, all create functions does return NULL on failure. Other may return FALSE.
// It may be interesting, for the developer, to know why the function is failing.
// for that reason, lcms2 does offer a logging function. This function will get
// an ENGLISH string with some clues on what is going wrong. You can show this
// info to the end user if you wish, or just create some sort of log on disk.
// The logging function should NOT terminate the program, as this obviously can leave
// unfreed resources. It is the programmer's responsibility to check each function
// return code to make sure it didn't fail.

CONST

    cmsERROR_UNDEFINED                  =  0;
    cmsERROR_FILE                       =  1;
    cmsERROR_RANGE                      =  2;
    cmsERROR_INTERNAL                   =  3;
    cmsERROR_NULL                       =  4;
    cmsERROR_READ                       =  5;
    cmsERROR_SEEK                       =  6;
    cmsERROR_WRITE                      =  7;
    cmsERROR_UNKNOWN_EXTENSION          =  8;
    cmsERROR_COLORSPACE_CHECK           =  9;
    cmsERROR_ALREADY_DEFINED            =  10;
    cmsERROR_BAD_SIGNATURE              =  11;
    cmsERROR_CORRUPTION_DETECTED        =  12;
    cmsERROR_NOT_SUITABLE               =  13;

// Error logger is called with the ContextID when a message is raised. This gives the
// chance to know which thread is responsible of the warning and any environment associated
// with it. Non-multithreading applications may safely ignore this parameter.
// Note that under certain special circumstances, ContextID may be NULL.

TYPE

    cmsLogErrorHandlerFunction = PROCEDURE( ContextID: cmsContext; ErrorCode: cmsUInt32Number; Text: PAnsiChar); CDecl;

    // Allows user to set any specific logger
    PROCEDURE cmsSetLogErrorHandler(Fn: cmsLogErrorHandlerFunction); StdCall;


// Conversions --------------------------------------------------------------------------------------------------------------


// Returns pointers to constant structs
FUNCTION cmsD50_XYZ: LPcmsCIEXYZ; StdCall;
FUNCTION cmsD50_xyY: LPcmsCIExyY; StdCall;

// Colorimetric space conversions
PROCEDURE cmsXYZ2xyY(Dest: LPcmsCIExyY; Source: LPcmsCIEXYZ); StdCall;
PROCEDURE cmsxyY2XYZ(Dest: LPcmsCIEXYZ; Source: LPcmsCIExyY); StdCall;
PROCEDURE cmsLab2XYZ(WhitePoint: LPcmsCIEXYZ; xyz: LPcmsCIEXYZ; Lab: LPcmsCIELab); StdCall;
PROCEDURE cmsXYZ2Lab(WhitePoint: LPcmsCIEXYZ; Lab: LPcmsCIELab; xyz: LPcmsCIEXYZ); StdCall;
PROCEDURE cmsLab2LCh(LCh: LPcmsCIELCh; Lab: LPcmsCIELab); StdCall;
PROCEDURE cmsLCh2Lab(Lab: LPcmsCIELab; LCh: LPcmsCIELCh); StdCall;

// Encoding /Decoding on PCS
PROCEDURE cmsLabEncoded2Float(Lab: LPcmsCIELab; wLab: Pointer); StdCall;
PROCEDURE cmsLabEncoded2FloatV2(Lab: LPcmsCIELab; wLab: Pointer); StdCall;
PROCEDURE cmsFloat2LabEncoded(wLab: Pointer; Lab: LPcmsCIELab); StdCall;
PROCEDURE cmsFloat2LabEncodedV2(wLab: Pointer; Lab: LPcmsCIELab); StdCall;
PROCEDURE cmsXYZEncoded2Float(fxyz : LPcmsCIEXYZ; XYZ: Pointer); StdCall;
PROCEDURE cmsFloat2XYZEncoded(XYZ: Pointer; fXYZ: LPcmsCIEXYZ); StdCall;


// DeltaE metrics
FUNCTION cmsDeltaE(Lab1, Lab2: LPcmsCIELab): Double; StdCall;
FUNCTION cmsCIE94DeltaE(Lab1, Lab2: LPcmsCIELab): Double; StdCall;
FUNCTION cmsBFDdeltaE(Lab1, Lab2: LPcmsCIELab): Double; StdCall;
FUNCTION cmsCMCdeltaE(Lab1, Lab2: LPcmsCIELab): Double; StdCall;
FUNCTION cmsCIE2000DeltaE(Lab1, Lab2: LPcmsCIELab; Kl, Kc, Kh: Double): Double; StdCall;


// Temperature <-> Chromaticity (Black body)
FUNCTION  cmsWhitePointFromTemp(var WhitePoint: cmsCIExyY; TempK: cmsFloat64Number) : cmsBool; StdCall;
FUNCTION  cmsTempFromWhitePoint(var TeampK: cmsFloat64Number; var WhitePoint: cmsCIExyY) : cmsBool; StdCall;


// Chromatic adaptation
FUNCTION cmsAdaptToIlluminant(Result: LPcmsCIEXYZ; SourceWhitePt: LPcmsCIEXYZ;
                              Illuminant: LPcmsCIEXYZ; Value: LPcmsCIEXYZ): cmsBool; StdCall;


// CIECAM02 ---------------------------------------------------------------------------------------------------

// Viewing conditions. Please note those are CAM model viewing conditions, and not the ICC tag viewing
// conditions, which I'm naming cmsICCViewingConditions to make differences evident. Unfortunately, the tag
// cannot deal with surround La, Yb and D value so is basically useless to store CAM02 viewing conditions.

 CONST

    AVG_SURROUND       = 1;
    DIM_SURROUND       = 2;
    DARK_SURROUND      = 3;
    CUTSHEET_SURROUND  = 4;

    D_CALCULATE        = -1;

  TYPE

    cmsViewingConditions = PACKED RECORD

                WhitePoint: cmsCIEXYZ;
                Yb        : cmsFloat64Number;
                La        : cmsFloat64Number;
                surround  : Integer;
                D_value   : cmsFloat64Number
              END;


    LPcmsViewingConditions = ^cmsViewingConditions;
        
FUNCTION    cmsCIECAM02Init(pVC : LPcmsViewingConditions ) : Pointer; StdCall;
PROCEDURE   cmsCIECAM02Done(hModel : Pointer); StdCall;
PROCEDURE   cmsCIECAM02Forward(hModel: Pointer; pIn: LPcmsCIEXYZ; pOut: LPcmsJCh ); StdCall;
PROCEDURE   cmsCIECAM02Reverse(hModel: Pointer; pIn: LPcmsJCh;   pOut: LPcmsCIEXYZ ); StdCall;

// Tone curves -----------------------------------------------------------------------------------------

// This describes a curve segment. For a table of supported types, see the manual. User can increase the number of
// available types by using a proper plug-in. Parametric segments allow 10 parameters at most

TYPE 
cmsCurveSegment = PACKED RECORD
       x0, x1: cmsFloat32Number;                       // Domain; for x0 < x <= x1
         PType: cmsInt32Number;                        // Parametric type, Type == 0 means sampled segment. Negative values are reserved
       Params: array [0..9] of cmsFloat64Number;       // Parameters if Type != 0
    nGridPoints: cmsUInt32Number;                      // Number of grid points if Type == 0
    SampledPoints: LPcmsFloat32Number;                 // Points to an array of floats if Type == 0
END;

LPcmsToneCurve = Pointer;
LPcmsCurveSegmentArray = ^cmsCurveSegmentArray;
cmsCurveSegmentArray = array[0..0] of cmsCurveSegment;

LPcmsFloat64NumberArray = ^cmsFloat64NumberArray;
cmsFloat64NumberArray = array[0..0] of cmsFloat64Number;

LPcmsUInt16NumberArray = ^cmsUInt16NumberArray;
cmsUInt16NumberArray = array[0..0] of cmsUInt16Number;

LPcmsFloat32NumberArray = ^cmsFloat32NumberArray;
cmsFloat32NumberArray = array[0..0] of cmsFloat32Number;

LPLPcmsToneCurveArray = ^LPcmsToneCurveArray;
LPcmsToneCurveArray = array[0..0] of LPcmsToneCurve;

LPcmsUInt32NumberArray = ^cmsUInt32NumberArray;
cmsUInt32NumberArray = array[0..0] of cmsUInt32Number;

FUNCTION  cmsBuildSegmentedToneCurve(ContextID: cmsContext; nSegments: cmsInt32Number; Segments: LPcmsCurveSegmentArray): LPcmsToneCurve; StdCall;
FUNCTION  cmsBuildParametricToneCurve(ContextID: cmsContext;  CType: cmsInt32Number; Params: LPcmsFloat64NumberArray): LPcmsToneCurve; StdCall;
FUNCTION  cmsBuildGamma(ContextID: cmsContext; Gamma: cmsFloat64Number): LPcmsToneCurve; StdCall;
FUNCTION  cmsBuildTabulatedToneCurve16(ContextID: cmsContext; nEntries: cmsInt32Number; values: LPcmsUInt16NumberArray): LPcmsToneCurve; StdCall;
FUNCTION  cmsBuildTabulatedToneCurveFloat(ContextID: cmsContext; nEntries: cmsUInt32Number; values: LPcmsFloat32NumberArray): LPcmsToneCurve; StdCall;
PROCEDURE cmsFreeToneCurve(Curve: LPcmsToneCurve); StdCall;
PROCEDURE cmsFreeToneCurveTriple(Curve: LPLPcmsToneCurveArray); StdCall;
FUNCTION  cmsDupToneCurve(Src: LPcmsToneCurve): LPcmsToneCurve; StdCall;
FUNCTION  cmsReverseToneCurve(InGamma: LPcmsToneCurve): LPcmsToneCurve; StdCall;
FUNCTION  cmsReverseToneCurveEx(nResultSamples: cmsInt32Number; InGamma: LPcmsToneCurve): LPcmsToneCurve; StdCall;
FUNCTION  cmsJoinToneCurve(ContextID: cmsContext; X, Y: LPcmsToneCurve; nPoints: cmsUInt32Number ): LPcmsToneCurve; StdCall;
FUNCTION  cmsSmoothToneCurve(Tab: LPcmsToneCurve; lambda: cmsFloat64Number): cmsBool; StdCall;
FUNCTION  cmsEvalToneCurveFloat(Curve: LPcmsToneCurve; v: cmsFloat32Number):cmsFloat32Number; StdCall;
FUNCTION  cmsEvalToneCurve16(Curve: LPcmsToneCurve; v:cmsUInt16Number):cmsUInt16Number; StdCall;
FUNCTION  cmsIsToneCurveMultisegment(InGamma: LPcmsToneCurve):cmsBool; StdCall;
FUNCTION  cmsIsToneCurveLinear(Curve: LPcmsToneCurve):cmsBool; StdCall;
FUNCTION  cmsIsToneCurveMonotonic(t: LPcmsToneCurve):cmsBool; StdCall;
FUNCTION  cmsIsToneCurveDescending(t: LPcmsToneCurve):cmsBool; StdCall;
FUNCTION  cmsGetToneCurveParametricType(t: LPcmsToneCurve):cmsInt32Number; StdCall;
FUNCTION  cmsEstimateGamma(t: LPcmsToneCurve; Precision:cmsFloat64Number):cmsFloat64Number; StdCall;


// Implements pipelines of multi-processing elements -------------------------------------------------------------

TYPE
    LPcmsPipeline = Pointer;
    LPcmsStage    = Pointer;
    LPLPcmsStage   = ^LPcmsStage;
 
// Those are hi-level pipelines
FUNCTION  cmsPipelineAlloc(ContextID: cmsContext; InputChannels, OutputChannels: cmsUInt32Number): LPcmsPipeline; StdCall;
PROCEDURE cmsPipelineFree(lut: LPcmsPipeline); StdCall;
FUNCTION  cmsPipelineDup(Orig: LPcmsPipeline): LPcmsPipeline; StdCall; 
FUNCTION  cmsPipelineInputChannels(lut: LPcmsPipeline): cmsUInt32Number; StdCall;
FUNCTION  cmsPipelineOutputChannels(lut: LPcmsPipeline): cmsUInt32Number; StdCall;

FUNCTION cmsPipelineStageCount(lut: LPcmsPipeline): cmsUInt32Number; StdCall;
FUNCTION cmsPipelineGetPtrToFirstStage(lut: LPcmsPipeline): LPcmsStage; StdCall;
FUNCTION cmsPipelineGetPtrToLastStage(lut: LPcmsPipeline): LPcmsStage; StdCall;

PROCEDURE cmsPipelineEval16(Inv, Outv: LPcmsUInt16NumberArray; lut: LPcmsPipeline); StdCall;
PROCEDURE cmsPipelineEvalFloat(Inv, Outv: LPcmsFloat32NumberArray; lut: LPcmsPipeline); StdCall;

FUNCTION cmsPipelineEvalReverseFloat(Target, Result, Hint: LPcmsFloat32NumberArray; lut: LPcmsPipeline): cmsBool; StdCall;
FUNCTION cmsPipelineCat(l1, l2: LPcmsPipeline): cmsBool; StdCall;
FUNCTION cmsPipelineSetSaveAs8bitsFlag(lut: LPcmsPipeline; On: cmsBool): cmsBool; StdCall;

// Where to place/locate the stages in the pipeline chain
TYPE
    cmsStageLoc = (cmsAT_BEGIN = 0, cmsAT_END = 1 );

PROCEDURE cmsPipelineInsertStage(lut: LPcmsPipeline; loc: cmsStageLoc; mpe: LPcmsStage); StdCall;
PROCEDURE cmsPipelineUnlinkStage(lut: LPcmsPipeline; loc: cmsStageLoc; mpe: LPLPcmsStage); StdCall;

// This function is quite useful to analyze the structure of a Pipeline and retrieve the Stage elements
// that conform the Pipeline. It should be called with the Pipeline, the number of expected elements and
// then a list of expected types followed with a list of double pointers to Stage elements. If
// the function founds a match with current pipeline, it fills the pointers and returns TRUE
// if not, returns FALSE without touching anything.
// FUNCTION cmsPipelineCheckAndRetreiveStages(const cmsPipeline* Lut, n: cmsUInt32Number, ...): cmsBool; StdCall;

// Matrix has double precision and CLUT has only float precision. That is because an ICC profile can encode
// matrices with far more precision that CLUTS
FUNCTION  cmsStageAllocIdentity(ContextID: cmsContext; nChannels: cmsUInt32Number): LPcmsStage; StdCall;
FUNCTION  cmsStageAllocToneCurves(ContextID: cmsContext; nChannels: cmsUInt32Number; Curves: LPLPcmsToneCurveArray): LPcmsStage; StdCall;
FUNCTION  cmsStageAllocMatrix(ContextID: cmsContext; Rows, Cols: cmsUInt32Number; Matrix, Offset: LPcmsFloat64NumberArray): LPcmsStage; StdCall;

FUNCTION  cmsStageAllocCLut16bit(ContextID: cmsContext; nGridPoints: cmsUInt32Number; inputChan, outputChan: cmsUInt32Number; Table: LPcmsUInt16NumberArray): LPcmsStage; StdCall;
FUNCTION  cmsStageAllocCLutFloat(ContextID: cmsContext; nGridPoints: cmsUInt32Number; inputChan, outputChan: cmsUInt32Number; Table: LPcmsFloat32NumberArray): LPcmsStage; StdCall;

FUNCTION  cmsStageAllocCLut16bitGranular(ContextID: cmsContext; nGridPoints: LPcmsUInt32NumberArray; inputChan, outputChan: cmsUInt32Number; Table: LPcmsUInt16NumberArray): LPcmsStage; StdCall;
FUNCTION  cmsStageAllocCLutFloatGranular(ContextID: cmsContext; nGridPoints: LPcmsUInt32NumberArray; inputChan, outputChan: cmsUInt32Number; Table: LPcmsFloat32NumberArray): LPcmsStage; StdCall;


FUNCTION  cmsStageDup(mpe: LPcmsStage): LPcmsStage; StdCall;
PROCEDURE cmsStageFree(mpe: LPcmsStage); StdCall;
FUNCTION  cmsStageNext(mpe: LPcmsStage): LPcmsStage; StdCall;

FUNCTION cmsStageInputChannels(mpe: LPcmsStage): cmsUInt32Number; StdCall;
FUNCTION cmsStageOutputChannels(mpe: LPcmsStage): cmsUInt32Number; StdCall;
FUNCTION cmsStageType(mpe: LPcmsStage): cmsStageSignature; StdCall;
FUNCTION cmsStageData(mpe: LPcmsStage): Pointer; StdCall;

// Sampling
 
Type
    cmsSAMPLER16    = FUNCTION (Inp, Outp: LPcmsUInt16NumberArray; Cargo: Pointer): cmsInt32Number; CDecl;
    cmsSAMPLERFLOAT = FUNCTION (Inp, Outp: LPcmsFloat32NumberArray; Cargo: Pointer): cmsInt32Number; CDecl;

// Use this flag to prevent changes being written to destination

Const

SAMPLER_INSPECT     = $01000000;


// For CLUT only
FUNCTION cmsStageSampleCLut16bit(mpe: LPcmsStage;  Sampler: cmsSAMPLER16;    Cargo: Pointer; dwFlags: cmsUInt32Number): cmsBool; StdCall;
FUNCTION cmsStageSampleCLutFloat(mpe: LPcmsStage;  Sampler: cmsSAMPLERFLOAT; Cargo: Pointer; dwFlags: cmsUInt32Number): cmsBool; StdCall;


// Slicers
FUNCTION  cmsSliceSpace16(nInputs: cmsUInt32Number; clutPoints: LPcmsUInt32NumberArray;
                                                   Sampler: cmsSAMPLER16; Cargo: Pointer): cmsBool; StdCall;

FUNCTION cmsSliceSpaceFloat(nInputs: cmsUInt32Number; clutPoints: LPcmsUInt32NumberArray;
                                                   Sampler: cmsSAMPLERFLOAT; Cargo: Pointer): cmsBool; StdCall;

// Multilocalized Unicode management ---------------------------------------------------------------------------------------

Type
   LPcmsMLU = Pointer;

Const

cmsNoLanguage = #0#0#0;
cmsNoCountry  = #0#0#0;


FUNCTION  cmsMLUalloc(ContextID: cmsContext; nItems: cmsUInt32Number): LPcmsMLU; StdCall; 
PROCEDURE cmsMLUfree(mlu: LPcmsMLU); StdCall;
FUNCTION  cmsMLUdup(mlu: LPcmsMLU): LPcmsMLU; StdCall; 

FUNCTION  cmsMLUsetASCII(mlu: LPcmsMLU; LanguageCode, CountryCode, ASCIIString: PAnsiChar): cmsBool; StdCall;
FUNCTION  cmsMLUsetWide(mlu: LPcmsMLU; LanguageCode, CountryCode: PAnsiChar; WideString: PWChar): cmsBool; StdCall;

FUNCTION cmsMLUgetASCII(mlu: LPcmsMLU; LanguageCode, CountryCode: PAnsiChar; Buffer: PAnsiChar; BufferSize: cmsUInt32Number): cmsUInt32Number; StdCall;

FUNCTION cmsMLUgetWide(mlu: LPcmsMLU; LanguageCode, CountryCode: PAnsiChar; Buffer: PWChar; BufferSize: cmsUInt32Number): cmsUInt32Number; StdCall;
                                                 
FUNCTION cmsMLUgetTranslation(mlu: LPcmsMLU; LanguageCode, CountryCode, ObtainedLanguage, ObtainedCountry: PAnsiChar): cmsBool; StdCall;

// Undercolorremoval & black generation -------------------------------------------------------------------------------------

Type

cmsUcrBg = PACKED RECORD
            Ucr, Bg: LPcmsToneCurve;
            Desc: LPcmsMLU;
            END;


// Screening ----------------------------------------------------------------------------------------------------------------

Const

 cmsPRINTER_DEFAULT_SCREENS    = $0001;
 cmsFREQUENCE_UNITS_LINES_CM   = $0000;
 cmsFREQUENCE_UNITS_LINES_INCH = $0002;

 cmsSPOT_UNKNOWN         = 0;
 cmsSPOT_PRINTER_DEFAULT = 1;
 cmsSPOT_ROUND           = 2;
 cmsSPOT_DIAMOND         = 3;
 cmsSPOT_ELLIPSE         = 4;
 cmsSPOT_LINE            = 5;
 cmsSPOT_SQUARE          = 6;
 cmsSPOT_CROSS           = 7;
                            

Type

cmsScreeningChannel = PACKED RECORD

      Frequency, 
      ScreenAngle: cmsFloat64Number;
      SpotShape: cmsUInt32Number;

END;

cmsScreening = PACKED RECORD

    Flag,
    nChannels : cmsUInt32Number;
    Channels: Array [0..cmsMAXCHANNELS-1] OF cmsScreeningChannel;
END;


// Named color -----------------------------------------------------------------------------------------------------------------


LPcmsNAMEDCOLORLIST = Pointer;

FUNCTION cmsAllocNamedColorList(ContextID: cmsContext; n, ColorantCount :cmsUInt32Number;
                                                           Prefix, Suffix: PAnsiChar): LPcmsNAMEDCOLORLIST; StdCall;

PROCEDURE cmsFreeNamedColorList(v: LPcmsNAMEDCOLORLIST); StdCall;
FUNCTION  cmsDupNamedColorList(v: LPcmsNAMEDCOLORLIST): LPcmsNAMEDCOLORLIST; StdCall;
FUNCTION  cmsAppendNamedColor(v: LPcmsNAMEDCOLORLIST; Name: PAnsiChar;
                                                             PCS, Colorant : LPcmsUInt16NumberArray): cmsBool; StdCall;

FUNCTION cmsNamedColorCount(v: LPcmsNAMEDCOLORLIST): cmsUInt32Number; StdCall;
FUNCTION cmsNamedColorIndex(v: LPcmsNAMEDCOLORLIST; Name: PAnsiChar): cmsInt32Number; StdCall;

FUNCTION cmsNamedColorInfo(v: LPcmsNAMEDCOLORLIST; nColor : cmsUInt32Number;
                                                      Name,Prefix, Suffix : PAnsiChar;
                                                       PCS, Colorant : LPcmsUInt16NumberArray): cmsBool; StdCall;

// Retrieve named color list from transform
FUNCTION cmsGetNamedColorList(xform: cmsHTRANSFORM ): LPcmsNAMEDCOLORLIST; StdCall;

// Profile sequence -----------------------------------------------------------------------------------------------------

Type

// Profile sequence descriptor. Some fields come from profile sequence descriptor tag, others
// come from Profile Sequence Identifier Tag

cmsPSEQDESC = PACKED RECORD
   deviceMfg, deviceModel: cmsSignature;
    
   attributes: cmsUInt64Number;
   technology: cmsTechnologySignature;
   ProfileID: cmsProfileID;
   Manufacturer, 
   Model, 
   Description : LPcmsMLU;
 END;

 LPcmsSEQDESC = ^cmsPSEQDESC;

cmsSEQ = PACKED RECORD

    n: cmsUInt32Number;
    ContextID: cmsContext;
    seq: LPcmsSEQDESC;
END;

LPcmsSEQ = ^cmsSEQ;

FUNCTION   cmsAllocProfileSequenceDescription(ContextID: cmsContext; n: cmsUInt32Number):LPcmsSEQ; StdCall;
FUNCTION   cmsDupProfileSequenceDescription(pseq: LPcmsSEQ):LPcmsSEQ; StdCall;
PROCEDURE  cmsFreeProfileSequenceDescription(pseq: LPcmsSEQ); StdCall;

// Access to Profile data ----------------------------------------------------------------------------------------------
FUNCTION cmsCreateProfilePlaceholder(ContextID: cmsContext): cmsHPROFILE; StdCall;

FUNCTION cmsGetProfileContextID(hProfile: cmsHPROFILE):cmsContext; StdCall;
FUNCTION cmsGetTagCount(hProfile: cmsHPROFILE): cmsInt32Number; StdCall;
FUNCTION cmsGetTagSignature(hProfile: cmsHPROFILE; n: cmsUInt32Number): cmsTagSignature; StdCall;
FUNCTION cmsIsTag(hProfile: cmsHPROFILE; sig: cmsTagSignature ): cmsBool; StdCall;

// Read and write pre-formatted data
FUNCTION cmsReadTag(hProfile: cmsHPROFILE; sig: cmsTagSignature ): Pointer; StdCall;
FUNCTION cmsWriteTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; data: Pointer): cmsBool; StdCall;
FUNCTION cmsLinkTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; dest: cmsTagSignature): cmsBool; StdCall;
FUNCTION cmsTagLinkedTo(hProfile: cmsHPROFILE; sig: cmsTagSignature):cmsTagSignature; StdCall;

// Read and write raw data
FUNCTION cmsReadRawTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; Buffer: Pointer; BufferSize: cmsUInt32Number): cmsInt32Number; StdCall;
FUNCTION cmsWriteRawTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; data: Pointer; Size: cmsUInt32Number): cmsBool; StdCall;

// Access header data
Const

   cmsEmbeddedProfileFalse    = $00000000;
   cmsEmbeddedProfileTrue     = $00000001;
   cmsUseAnywhere             = $00000000;
   cmsUseWithEmbeddedDataOnly = $00000002;

FUNCTION  cmsGetHeaderFlags(hProfile: cmsHPROFILE): cmsUInt32Number; StdCall;
PROCEDURE cmsGetHeaderAttributes(hProfile: cmsHPROFILE; Flags: LPcmsUInt64Number); StdCall;
PROCEDURE cmsGetHeaderProfileID(hProfile: cmsHPROFILE; ProfileID: LPcmsUInt8Number); StdCall;

// TODO:
// FUNCTION  cmsGetHeaderCreationDateTime(hProfile: cmsHPROFILE; struct tm *Dest): cmsBool; StdCall;

FUNCTION  cmsGetHeaderRenderingIntent(hProfile: cmsHPROFILE): cmsUInt32Number; StdCall;
PROCEDURE cmsSetHeaderFlags(hProfile: cmsHPROFILE; Flags: cmsUInt32Number); StdCall;
FUNCTION  cmsGetHeaderManufacturer(hProfile: cmsHPROFILE): cmsUInt32Number; StdCall;
PROCEDURE cmsSetHeaderManufacturer(hProfile: cmsHPROFILE; manufacturer: cmsUInt32Number ); StdCall;
FUNCTION  cmsGetHeaderModel(hProfile: cmsHPROFILE): cmsUInt32Number; StdCall;
PROCEDURE cmsSetHeaderModel(hProfile: cmsHPROFILE; model: cmsUInt32Number ); StdCall;
PROCEDURE cmsSetHeaderAttributes(hProfile: cmsHPROFILE; Flags: cmsUInt64Number); StdCall;
PROCEDURE cmsSetHeaderProfileID(hProfile: cmsHPROFILE; ProfileID: LPcmsUInt8Number); StdCall;
PROCEDURE cmsSetHeaderRenderingIntent(hProfile: cmsHPROFILE; RenderingIntent: cmsUInt32Number ); StdCall;

FUNCTION  cmsGetPCS(hProfile: cmsHPROFILE):cmsColorSpaceSignature; StdCall;
PROCEDURE cmsSetPCS(hProfile: cmsHPROFILE; pcs: cmsColorSpaceSignature); StdCall;
FUNCTION  cmsGetColorSpace(hProfile: cmsHPROFILE): cmsColorSpaceSignature; StdCall;
PROCEDURE cmsSetColorSpace(hProfile: cmsHPROFILE; sig: cmsColorSpaceSignature); StdCall;
FUNCTION  cmsGetDeviceClass(hProfile: cmsHPROFILE): cmsProfileClassSignature; StdCall;
PROCEDURE cmsSetDeviceClass(hProfile: cmsHPROFILE; sig: cmsProfileClassSignature); StdCall;
PROCEDURE cmsSetProfileVersion(hProfile: cmsHPROFILE; Version: cmsFloat64Number); StdCall;
FUNCTION  cmsGetProfileVersion(hProfile: cmsHPROFILE): cmsFloat64Number; StdCall;

FUNCTION  cmsGetEncodedICCversion(hProfile: cmsHPROFILE): cmsUInt32Number; StdCall;
PROCEDURE cmsSetEncodedICCversion(hProfile: cmsHPROFILE; Version: cmsUInt32Number); StdCall;


Const

    // How profiles may be used
    LCMS_USED_AS_INPUT     = 0;
    LCMS_USED_AS_OUTPUT    = 1;
    LCMS_USED_AS_PROOF     = 2;

FUNCTION   cmsIsIntentSupported(hProfile: cmsHPROFILE; Intent: cmsUInt32Number; UsedDirection: cmsUInt32Number): cmsBool; StdCall;
FUNCTION   cmsIsMatrixShaper(hProfile: cmsHPROFILE): cmsBool; StdCall;
FUNCTION   cmsIsCLUT(hProfile: cmsHPROFILE; Intent: cmsUInt32Number; UsedDirection: cmsUInt32Number): cmsBool; StdCall;

// Translate form/to our notation to ICC
FUNCTION _cmsICCcolorSpace(OurNotation: Integer): cmsColorSpaceSignature; StdCall;
FUNCTION _cmsLCMScolorSpace(ProfileSpace: cmsColorSpaceSignature): Integer; StdCall;

FUNCTION cmsChannelsOf( ColorSpace: cmsColorSpaceSignature): cmsUInt32Number; StdCall;

// Build a suitable formatter for the colorspace of this profile
FUNCTION cmsFormatterForColorspaceOfProfile(hProfile: cmsHPROFILE; nBytes: cmsUInt32Number; lIsFloat: cmsBool): cmsUInt32Number; StdCall;
FUNCTION cmsFormatterForPCSOfProfile(hProfile: cmsHPROFILE; nBytes: cmsUInt32Number; lIsFloat: cmsBool): cmsUInt32Number; StdCall;

Type

// Localized info
cmsInfoType = (
             cmsInfoDescription  = 0,
             cmsInfoManufacturer = 1,
             cmsInfoModel        = 2,
             cmsInfoCopyright    = 3
);

FUNCTION cmsGetProfileInfo(hProfile: cmsHPROFILE; Info: cmsInfoType; LanguageCode, CountryCode: PAnsiChar;
                                                            Buffer: PWChar; BufferSize: cmsUInt32Number): cmsUInt32Number; StdCall;

FUNCTION cmsGetProfileInfoASCII(hProfile: cmsHPROFILE; Info: cmsInfoType; LanguageCode, CountryCode: PAnsiChar;
                                                            Buffer: PAnsiChar; BufferSize: cmsUInt32Number): cmsUInt32Number; StdCall;

// IO handlers ----------------------------------------------------------------------------------------------------------

Type

LPcmsIOHANDLER = Pointer;

FUNCTION cmsOpenIOhandlerFromFile(ContextID: cmsContext; FileName, AccessMode: PAnsiChar): LPcmsIOHANDLER; StdCall;
// FUNCTION cmsOpenIOhandlerFromStream(ContextID: cmsContext; FILE* Stream): LPcmsIOHANDLER; StdCall;
FUNCTION cmsOpenIOhandlerFromMem(ContextID: cmsContext; Buffer: Pointer; size: cmsUInt32Number; AccessMode: PAnsiChar): LPcmsIOHANDLER; StdCall;
FUNCTION cmsOpenIOhandlerFromNULL(ContextID: cmsContext): LPcmsIOHANDLER; StdCall;
FUNCTION cmsCloseIOhandler(io: LPcmsIOHANDLER): cmsBool; StdCall;

// MD5 message digest --------------------------------------------------------------------------------------------------

FUNCTION cmsMD5computeID(hProfile: cmsHPROFILE): cmsBool; StdCall;

// Profile high level funtions ------------------------------------------------------------------------------------------

FUNCTION   cmsOpenProfileFromFile(ICCProfile : PAnsiChar; sAccess: PAnsiChar): cmsHPROFILE; StdCall;
FUNCTION   cmsOpenProfileFromFileTHR(ContextID: cmsContext; ICCProfile, sAccess: PAnsiChar): cmsHPROFILE; StdCall;
// FUNCTION      CMSEXPORT cmsOpenProfileFromStream(FILE* ICCProfile, const char* sAccess): cmsHPROFILE; StdCall;
// FUNCTION      CMSEXPORT cmsOpenProfileFromStreamTHR(ContextID: cmsContext; FILE* ICCProfile, const char* sAccess): cmsHPROFILE; StdCall;
FUNCTION   cmsOpenProfileFromMem(MemPtr: Pointer; dwSize: cmsUInt32Number): cmsHPROFILE; StdCall;
FUNCTION   cmsOpenProfileFromMemTHR(ContextID: cmsContext; MemPtr: Pointer; dwSize: cmsUInt32Number): cmsHPROFILE; StdCall;
FUNCTION   cmsOpenProfileFromIOhandlerTHR(ContextID: cmsContext; io: LPcmsIOHANDLER): cmsHPROFILE; StdCall;
FUNCTION   cmsCloseProfile(hProfile: cmsHPROFILE): cmsBool; StdCall;

FUNCTION   cmsSaveProfileToFile(hProfile: cmsHPROFILE; FileName: PAnsiChar): cmsBool; StdCall;
// FUNCTION         CMSEXPORT cmsSaveProfileToStream(hProfile: cmsHPROFILE, FILE* Stream): cmsBool; StdCall;
FUNCTION   cmsSaveProfileToMem(hProfile: cmsHPROFILE; MemPtr: Pointer; BytesNeeded: LPcmsUInt32Number): cmsBool; StdCall;
FUNCTION   cmsSaveProfileToIOhandler(hProfile: cmsHPROFILE; io: LPcmsIOHANDLER):cmsUInt32Number; StdCall;

// Predefined virtual profiles ------------------------------------------------------------------------------------------

FUNCTION  cmsCreateRGBProfileTHR(ContextID: cmsContext;
                                                   WhitePoint: LPcmsCIExyY;
                                                   Primaries: LPcmsCIExyYTRIPLE;
                                                   TransferFunction: LPLPcmsToneCurveArray): cmsHPROFILE; StdCall;

FUNCTION  cmsCreateRGBProfile(WhitePoint: LPcmsCIExyY;
                                                   Primaries: LPcmsCIExyYTRIPLE;
                                                   TransferFunction: LPLPcmsToneCurveArray): cmsHPROFILE; StdCall;

FUNCTION cmsCreateGrayProfileTHR(ContextID: cmsContext;
                                                    WhitePoint: LPcmsCIExyY;
                                                    TransferFunction: LPcmsToneCurve): cmsHPROFILE; StdCall;

FUNCTION cmsCreateGrayProfile(WhitePoint: LPcmsCIExyY;
                                                     TransferFunction: LPcmsToneCurve): cmsHPROFILE; StdCall;

FUNCTION cmsCreateLinearizationDeviceLinkTHR(ContextID: cmsContext;
                                                                 ColorSpace: cmsColorSpaceSignature;
                                                                 TransferFunctions: LPLPcmsToneCurveArray): cmsHPROFILE; StdCall;

FUNCTION cmsCreateLinearizationDeviceLink(ColorSpace: cmsColorSpaceSignature;
                                                                 TransferFunctions: LPLPcmsToneCurveArray): cmsHPROFILE; StdCall;

FUNCTION cmsCreateInkLimitingDeviceLinkTHR(ContextID: cmsContext;
                                                              ColorSpace: cmsColorSpaceSignature; Limit: cmsFloat64Number): cmsHPROFILE; StdCall;

FUNCTION cmsCreateInkLimitingDeviceLink(ColorSpace: cmsColorSpaceSignature; Limit: cmsFloat64Number): cmsHPROFILE; StdCall;


FUNCTION cmsCreateLab2ProfileTHR(ContextID: cmsContext; WhitePoint: LPcmsCIExyY): cmsHPROFILE; StdCall;
FUNCTION cmsCreateLab2Profile(WhitePoint: LPcmsCIExyY): cmsHPROFILE; StdCall;
FUNCTION cmsCreateLab4ProfileTHR(ContextID: cmsContext; WhitePoint: LPcmsCIExyY): cmsHPROFILE; StdCall;
FUNCTION cmsCreateLab4Profile(WhitePoint: LPcmsCIExyY): cmsHPROFILE; StdCall;

FUNCTION cmsCreateXYZProfileTHR(ContextID: cmsContext): cmsHPROFILE; StdCall;
FUNCTION cmsCreateXYZProfile: cmsHPROFILE; StdCall;

FUNCTION cmsCreate_sRGBProfileTHR(ContextID: cmsContext): cmsHPROFILE; StdCall;
FUNCTION cmsCreate_sRGBProfile: cmsHPROFILE; StdCall;

FUNCTION cmsCreateBCHSWabstractProfileTHR(ContextID: cmsContext;
                                                             nLUTPoints: Integer;
                                                             Bright,
                                                             Contrast,
                                                             Hue,
                                                             Saturation: cmsFloat64Number;
                                                             TempSrc,
                                                             TempDest: Integer): cmsHPROFILE; StdCall;

FUNCTION cmsCreateBCHSWabstractProfile(   nLUTPoints: Integer;
                                                             Bright,
                                                             Contrast,
                                                             Hue,
                                                             Saturation: cmsFloat64Number;
                                                             TempSrc,
                                                             TempDest: Integer): cmsHPROFILE; StdCall;

FUNCTION  cmsCreateNULLProfileTHR(ContextID: cmsContext): cmsHPROFILE; StdCall;
FUNCTION  cmsCreateNULLProfile: cmsHPROFILE; StdCall;

// Converts a transform to a devicelink profile
FUNCTION  cmsTransform2DeviceLink(hTransform: cmsHTRANSFORM; Version: cmsFloat64Number; dwFlags: cmsUInt32Number): cmsHPROFILE; StdCall;

// Intents ----------------------------------------------------------------------------------------------

Const

// ICC Intents
INTENT_PERCEPTUAL                              = 0;
INTENT_RELATIVE_COLORIMETRIC                   = 1;
INTENT_SATURATION                              = 2;
INTENT_ABSOLUTE_COLORIMETRIC                   = 3;

// Non-ICC intents
INTENT_PRESERVE_K_ONLY_PERCEPTUAL             = 10;
INTENT_PRESERVE_K_ONLY_RELATIVE_COLORIMETRIC  = 11;
INTENT_PRESERVE_K_ONLY_SATURATION             = 12;
INTENT_PRESERVE_K_PLANE_PERCEPTUAL            = 13;
INTENT_PRESERVE_K_PLANE_RELATIVE_COLORIMETRIC = 14;
INTENT_PRESERVE_K_PLANE_SATURATION            = 15;
                                                  
Type
LPPAnsiChar = ^PAnsiChar;

// Call with NULL as parameters to get the intent count
FUNCTION cmsGetSupportedIntents(nMax: cmsUInt32Number; Codes: LPcmsUInt32Number; Descriptions: LPPAnsiChar): cmsUInt32Number; StdCall;

Const

// Flags

cmsFLAGS_NOCACHE                  = $0040;    // Inhibit 1-pixel cache
cmsFLAGS_NOOPTIMIZE               = $0100;    // Inhibit optimizations
cmsFLAGS_NULLTRANSFORM            = $0200;    // Don't transform anyway

// Proofing flags
cmsFLAGS_GAMUTCHECK               = $1000;    // Out of Gamut alarm
cmsFLAGS_SOFTPROOFING             = $4000;    // Do softproofing

// Misc
cmsFLAGS_BLACKPOINTCOMPENSATION   = $2000;
cmsFLAGS_NOWHITEONWHITEFIXUP      = $0004;    // Don't fix scum dot
cmsFLAGS_HIGHRESPRECALC           = $0400;    // Use more memory to give better accurancy
cmsFLAGS_LOWRESPRECALC            = $0800;    // Use less memory to minimize resouces

// For devicelink creation
cmsFLAGS_8BITS_DEVICELINK         = $0008;   // Create 8 bits devicelinks
cmsFLAGS_GUESSDEVICECLASS         = $0020;   // Guess device class (for transform2devicelink)
cmsFLAGS_KEEP_SEQUENCE            = $0080;   // Keep profile sequence for devicelink creation

// Specific to a particular optimizations
cmsFLAGS_FORCE_CLUT               = $0002;    // Force CLUT optimization
cmsFLAGS_CLUT_POST_LINEARIZATION  = $0001;    // create postlinearization tables if possible
cmsFLAGS_CLUT_PRE_LINEARIZATION   = $0010;    // create prelinearization tables if possible

// CRD special
cmsFLAGS_NODEFAULTRESOURCEDEF     = $01000000;

// Fine-tune control over number of gridpoints
FUNCTION cmsFLAGS_GRIDPOINTS(n: Integer): Integer;


// Transforms ---------------------------------------------------------------------------------------------------

type
  LPcmsHPROFILEArray = ^cmsHPROFILEArray;
  cmsHPROFILEArray = array[0..0] of cmsHPROFILE;

  LPcmsBoolArray = ^cmsBoolArray;
  cmsBoolArray = array[0..0] of cmsBool;

FUNCTION   cmsCreateTransformTHR(ContextID: cmsContext;
                                                  Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; StdCall;

FUNCTION   cmsCreateTransform(Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; StdCall;

FUNCTION   cmsCreateProofingTransformTHR(ContextID: cmsContext;
                                                  Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Proofing: cmsHPROFILE;
                                                  Intent: cmsUInt32Number;
                                                  ProofingIntent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; StdCall;

FUNCTION   cmsCreateProofingTransform(Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Proofing: cmsHPROFILE;
                                                  Intent: cmsUInt32Number;
                                                  ProofingIntent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; StdCall;

FUNCTION   cmsCreateMultiprofileTransformTHR(ContextID: cmsContext;
                                                  hProfiles: LPcmsHPROFILEArray;
                                                  nProfiles: cmsUInt32Number;
                                                  InputFormat: cmsUInt32Number;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; StdCall;


FUNCTION   cmsCreateMultiprofileTransform( hProfiles: LPcmsHPROFILEArray;
                                                  nProfiles: cmsUInt32Number;
                                                  InputFormat: cmsUInt32Number;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; StdCall;


FUNCTION   cmsCreateExtendedTransform(ContextID: cmsContext;
                                                   nProfiles: cmsUInt32Number;
                                                   hProfiles: LPcmsHPROFILEArray;
                                                   BPC: LPcmsBoolArray;
                                                   Intents: LPcmsUInt32NumberArray;
                                                   AdaptationStates: LPcmsFloat64NumberArray;
                                                   hGamutProfile: cmsHPROFILE;
                                                   nGamutPCSposition: cmsUInt32Number;
                                                   InputFormat,
                                                   OutputFormat: cmsUInt32Number;
                                                   dwFlags: cmsUInt32Number): cmsHTRANSFORM; StdCall;

PROCEDURE  cmsDeleteTransform(hTransform: cmsHTRANSFORM); StdCall;

PROCEDURE  cmsDoTransform(Transform: cmsHTRANSFORM; InputBuffer, OutputBuffer: Pointer; size: cmsUInt32Number);  StdCall;

PROCEDURE  cmsSetAlarmCodes( NewAlarm: LPcmsUInt16NumberArray);  StdCall;
PROCEDURE  cmsGetAlarmCodes(NewAlarm: LPcmsUInt16NumberArray); StdCall;

// Adaptation state for absolute colorimetric intent
FUNCTION  cmsSetAdaptationState(d: cmsFloat64Number):cmsFloat64Number; StdCall;

// Grab the ContextID from an open transform. Returns NULL if a NULL transform is passed
FUNCTION  cmsGetTransformContextID(hTransform: cmsHTRANSFORM):cmsContext; StdCall;

// For backwards compatibility
FUNCTION  cmsChangeBuffersFormat(hTransform: cmsHTRANSFORM; InputFormat, OutputFormat: cmsUInt32Number): cmsBool; StdCall;



// PostScript ColorRenderingDictionary and ColorSpaceArray ----------------------------------------------------

Type

cmsPSResourceType = (cmsPS_RESOURCE_CSA, cmsPS_RESOURCE_CRD ) ;

// lcms2 unified method to access postscript color resources
FUNCTION cmsGetPostScriptColorResource(ContextID: cmsContext;   RType: cmsPSResourceType;
                                                                hProfile: cmsHPROFILE;
                                                                Intent: cmsUInt32Number;
                                                                dwFlags: cmsUInt32Number;
                                                                io: LPcmsIOHANDLER): cmsUInt32Number; StdCall;

FUNCTION cmsGetPostScriptCSA(ContextID: cmsContext; hProfile: cmsHPROFILE; Intent: cmsUInt32Number; dwFlags: cmsUInt32Number; Buffer: Pointer; dwBufferLen: cmsUInt32Number ): cmsUInt32Number; StdCall;
FUNCTION cmsGetPostScriptCRD(ContextID: cmsContext; hProfile: cmsHPROFILE; Intent: cmsUInt32Number; dwFlags: cmsUInt32Number; Buffer: Pointer; dwBufferLen: cmsUInt32Number): cmsUInt32Number; StdCall;


// IT8.7 / CGATS.17-20$ handling -----------------------------------------------------------------------------


// CGATS.13 parser

FUNCTION  cmsIT8Alloc: cmsHANDLE; StdCall;
PROCEDURE cmsIT8Free(hIT8: cmsHANDLE); StdCall;

// Tables

FUNCTION  cmsIT8TableCount(hIT8: cmsHANDLE): Integer; StdCall;
FUNCTION  cmsIT8SetTable(hIT8: cmsHANDLE; nTable: Integer): Integer; StdCall;

// Persistence
FUNCTION  cmsIT8LoadFromFile(cFileName: PAnsiChar): cmsHANDLE; StdCall;
FUNCTION  cmsIT8LoadFromMem(Ptr: Pointer; size :DWord): cmsHANDLE; StdCall;

FUNCTION cmsIT8SaveToFile(hIT8: cmsHANDLE; cFileName: PAnsiChar): cmsBool; StdCall;
FUNCTION cmsIT8SaveToMem(hIT8: cmsHANDLE; MemPtr: Pointer; BytesNeeded: LPcmsUInt32Number): cmsBool; StdCall;
// Properties

FUNCTION cmsIT8GetSheetType(hIT8: cmsHANDLE): PAnsiChar; StdCall;
FUNCTION cmsIT8SetSheetType(hIT8: cmsHANDLE; TheType: PAnsiChar): cmsBool; StdCall;

FUNCTION cmsIT8SetComment(hIT8: cmsHANDLE; cComment: PAnsiChar): cmsBool; StdCall;

FUNCTION cmsIT8SetPropertyStr(hIT8: cmsHANDLE; cProp, Str: PAnsiChar): cmsBool; StdCall;
FUNCTION cmsIT8SetPropertyDbl(hIT8: cmsHANDLE; cProp: PAnsiChar; Val: Double): cmsBool; StdCall;
FUNCTION cmsIT8SetPropertyHex(hIT8: cmsHANDLE; cProp: PAnsiChar; Val: Integer): cmsBool; StdCall;
FUNCTION cmsIT8SetPropertyUncooked(hIT8: cmsHANDLE; Key, Buffer: PAnsiChar): cmsBool; StdCall;


FUNCTION cmsIT8GetProperty(hIT8: cmsHANDLE; cProp: PAnsiChar): PAnsiChar; StdCall;
FUNCTION cmsIT8GetPropertyDbl(hIT8: cmsHANDLE; cProp: PAnsiChar): Double; StdCall;
FUNCTION cmsIT8EnumProperties(hIT8: cmsHANDLE; var PropertyNames: LPPAnsiChar): Integer; StdCall;

// Datasets

FUNCTION cmsIT8GetDataRowCol(hIT8: cmsHANDLE; row, col: Integer): PAnsiChar; StdCall;
FUNCTION cmsIT8GetDataRowColDbl(hIT8: cmsHANDLE; row, col: Integer): Double; StdCall;

FUNCTION cmsIT8SetDataRowCol(hIT8: cmsHANDLE; row, col: Integer; Val: PAnsiChar): cmsBool; StdCall;
FUNCTION cmsIT8SetDataRowColDbl(hIT8: cmsHANDLE; row, col: Integer; Val: Double): cmsBool; StdCall;

FUNCTION cmsIT8GetData(hIT8: cmsHANDLE; cPatch, cSample: PAnsiChar): PAnsiChar; StdCall;

FUNCTION cmsIT8GetDataDbl(hIT8: cmsHANDLE;cPatch, cSample: PAnsiChar): Double; StdCall;

FUNCTION cmsIT8SetData(hIT8: cmsHANDLE; cPatch, cSample, Val: PAnsiChar): cmsBool; StdCall;

FUNCTION cmsIT8SetDataDbl(hIT8: cmsHANDLE; cPatch, cSample: PAnsiChar; Val: Double): cmsBool; StdCall;

FUNCTION cmsIT8SetDataFormat(hIT8: cmsHANDLE; n: Integer; Sample: PAnsiChar): cmsBool; StdCall;
FUNCTION cmsIT8EnumDataFormat(hIT8: cmsHANDLE; var SampleNames: LPPAnsiChar): Integer; StdCall;
FUNCTION cmsIT8GetPatchName(hIT8: cmsHANDLE; nPatch: Integer; Buffer: PAnsiChar): PAnsiChar; StdCall;

// The LABEL extension
FUNCTION cmsIT8SetTableByLabel(hIT8: cmsHANDLE; cSet, cField, ExpectedType: PAnsiChar): Integer; StdCall;

FUNCTION cmsIT8FindDataFormat(hIT8: cmsHANDLE; cSample: PAnsiChar): Integer; StdCall;

// Formatter for double
PROCEDURE  cmsIT8DefineDblFormat(hIT8: cmsHANDLE; Formatter: PAnsiChar);  StdCall;

// Gamut boundary description routines ------------------------------------------------------------------------------

FUNCTION  cmsGBDAlloc(ContextID: cmsContext):cmsHANDLE; StdCall;
PROCEDURE cmsGBDFree(hGBD: cmsHANDLE); StdCall;
FUNCTION  cmsGDBAddPoint(hGBD: cmsHANDLE; Lab: LPcmsCIELab): cmsBool; StdCall;
FUNCTION  cmsGDBCompute(hGDB: cmsHANDLE; dwFlags: cmsUInt32Number): cmsBool; StdCall;
FUNCTION  cmsGDBCheckPoint(hGBD: cmsHANDLE; Lab: LPcmsCIELab): cmsBool; StdCall;

// Feature detection  ----------------------------------------------------------------------------------------------

// Estimate the black point
FUNCTION cmsDetectBlackPoint( BlackPoint: LPcmsCIEXYZ; hProfile: cmsHPROFILE; Intent: cmsUInt32Number; dwFlags: cmsUInt32Number): cmsBool; StdCall;

// Estimate total area coverage
FUNCTION cmsDetectTAC(hProfile: cmsHPROFILE): cmsFloat64Number; StdCall;


// Poor man's gamut mapping
FUNCTION  cmsDesaturateLab(Lab: LPcmsCIELab; amax, amin, bmax, bmin: cmsFloat64Number): cmsBool; StdCall;
                                                   

IMPLEMENTATION



    FUNCTION FLOAT_SH(a: cmsUInt32Number): cmsUInt32Number;        begin  FLOAT_SH :=       ((a)  shl  22) end;
    FUNCTION OPTIMIZED_SH(s: cmsUInt32Number): cmsUInt32Number;    begin  OPTIMIZED_SH :=   ((s)  shl  21) end;
    FUNCTION COLORSPACE_SH(s: cmsUInt32Number):cmsUInt32Number;    begin  COLORSPACE_SH :=  ((s)  shl  16) end;
    FUNCTION SWAPFIRST_SH(s: cmsUInt32Number):cmsUInt32Number;     begin  SWAPFIRST_SH :=   ((s)  shl  14) end;
    FUNCTION FLAVOR_SH(s: cmsUInt32Number):cmsUInt32Number;        begin  FLAVOR_SH :=      ((s)  shl  13) end;
    FUNCTION PLANAR_SH(p: cmsUInt32Number):cmsUInt32Number;        begin  PLANAR_SH :=      ((p)  shl  12) end;
    FUNCTION ENDIAN16_SH(e: cmsUInt32Number):cmsUInt32Number;      begin  ENDIAN16_SH :=    ((e)  shl  11) end;
    FUNCTION DOSWAP_SH(e: cmsUInt32Number):cmsUInt32Number;        begin  DOSWAP_SH :=      ((e)  shl  10) end;
    FUNCTION EXTRA_SH(e: cmsUInt32Number):cmsUInt32Number;         begin  EXTRA_SH :=       ((e)  shl  7) end;
    FUNCTION CHANNELS_SH(c: cmsUInt32Number):cmsUInt32Number;      begin  CHANNELS_SH :=    ((c)  shl  3) end;
    FUNCTION BYTES_SH(b: cmsUInt32Number):cmsUInt32Number;         begin  BYTES_SH :=       (b) end;


    FUNCTION T_FLOAT(a: cmsUInt32Number): cmsUInt32Number;          begin  T_FLOAT :=        (((a) shr 22) and 1) end;  
    FUNCTION T_OPTIMIZED(o: cmsUInt32Number): cmsUInt32Number;      begin  T_OPTIMIZED :=    (((o) shr 21) and 1) end;    
    FUNCTION T_COLORSPACE(s: cmsUInt32Number): cmsUInt32Number;     begin  T_COLORSPACE :=   (((s) shr 16) and 31) end;   
    FUNCTION T_SWAPFIRST(s: cmsUInt32Number): cmsUInt32Number;      begin  T_SWAPFIRST :=    (((s) shr 14) and 1) end;    
    FUNCTION T_FLAVOR(s: cmsUInt32Number): cmsUInt32Number;         begin  T_FLAVOR :=       (((s) shr 13) and 1) end;
    FUNCTION T_PLANAR(p: cmsUInt32Number): cmsUInt32Number;         begin  T_PLANAR :=       (((p) shr 12) and 1) end;  
    FUNCTION T_ENDIAN16(e: cmsUInt32Number): cmsUInt32Number;       begin  T_ENDIAN16 :=     (((e) shr 11) and 1) end;  
    FUNCTION T_DOSWAP(e: cmsUInt32Number): cmsUInt32Number;         begin  T_DOSWAP :=       (((e) shr 10) and 1) end; 
    FUNCTION T_EXTRA(e: cmsUInt32Number): cmsUInt32Number;          begin  T_EXTRA :=        (((e) shr 7) and 7) end;   
    FUNCTION T_CHANNELS(c: cmsUInt32Number): cmsUInt32Number;       begin  T_CHANNELS :=     (((c) shr 3) and 15) end; 
    FUNCTION T_BYTES(b: cmsUInt32Number): cmsUInt32Number;          begin  T_BYTES :=        ((b) and 7) end;



// 


FUNCTION  cmsPlugin(Plugin: Pointer): cmsBool; external 'lcms2.dll';
PROCEDURE cmsUnregisterPlugins; external 'lcms2.dll';
PROCEDURE cmsSetLogErrorHandler(Fn: cmsLogErrorHandlerFunction); external 'lcms2.dll';
FUNCTION cmsD50_XYZ: LPcmsCIEXYZ; external 'lcms2.dll';
FUNCTION cmsD50_xyY: LPcmsCIExyY; external 'lcms2.dll';
PROCEDURE cmsXYZ2xyY(Dest: LPcmsCIExyY; Source: LPcmsCIEXYZ); external 'lcms2.dll';
PROCEDURE cmsxyY2XYZ(Dest: LPcmsCIEXYZ; Source: LPcmsCIExyY); external 'lcms2.dll';
PROCEDURE cmsLab2XYZ(WhitePoint: LPcmsCIEXYZ; xyz: LPcmsCIEXYZ; Lab: LPcmsCIELab); external 'lcms2.dll';
PROCEDURE cmsXYZ2Lab(WhitePoint: LPcmsCIEXYZ; Lab: LPcmsCIELab; xyz: LPcmsCIEXYZ); external 'lcms2.dll';
PROCEDURE cmsLab2LCh(LCh: LPcmsCIELCh; Lab: LPcmsCIELab); external 'lcms2.dll';
PROCEDURE cmsLCh2Lab(Lab: LPcmsCIELab; LCh: LPcmsCIELCh); external 'lcms2.dll';
PROCEDURE cmsLabEncoded2Float(Lab: LPcmsCIELab; wLab: Pointer); external 'lcms2.dll';
PROCEDURE cmsLabEncoded2FloatV2(Lab: LPcmsCIELab; wLab: Pointer); external 'lcms2.dll';
PROCEDURE cmsFloat2LabEncoded(wLab: Pointer; Lab: LPcmsCIELab); external 'lcms2.dll';
PROCEDURE cmsFloat2LabEncodedV2(wLab: Pointer; Lab: LPcmsCIELab); external 'lcms2.dll';
PROCEDURE cmsXYZEncoded2Float(fxyz : LPcmsCIEXYZ; XYZ: Pointer); external 'lcms2.dll';
PROCEDURE cmsFloat2XYZEncoded(XYZ: Pointer; fXYZ: LPcmsCIEXYZ); external 'lcms2.dll';
FUNCTION cmsDeltaE(Lab1, Lab2: LPcmsCIELab): Double; external 'lcms2.dll';
FUNCTION cmsCIE94DeltaE(Lab1, Lab2: LPcmsCIELab): Double; external 'lcms2.dll';
FUNCTION cmsBFDdeltaE(Lab1, Lab2: LPcmsCIELab): Double; external 'lcms2.dll';
FUNCTION cmsCMCdeltaE(Lab1, Lab2: LPcmsCIELab): Double; external 'lcms2.dll';
FUNCTION cmsCIE2000DeltaE(Lab1, Lab2: LPcmsCIELab; Kl, Kc, Kh: Double): Double; external 'lcms2.dll';
FUNCTION  cmsWhitePointFromTemp(var WhitePoint: cmsCIExyY; TempK: cmsFloat64Number) : cmsBool; external 'lcms2.dll';
FUNCTION  cmsTempFromWhitePoint(var TeampK: cmsFloat64Number; var WhitePoint: cmsCIExyY) : cmsBool; external 'lcms2.dll';
FUNCTION cmsAdaptToIlluminant(Result: LPcmsCIEXYZ; SourceWhitePt: LPcmsCIEXYZ;
                              Illuminant: LPcmsCIEXYZ; Value: LPcmsCIEXYZ): cmsBool; external 'lcms2.dll';        
FUNCTION    cmsCIECAM02Init(pVC : LPcmsViewingConditions ) : Pointer; external 'lcms2.dll';
PROCEDURE   cmsCIECAM02Done(hModel : Pointer); external 'lcms2.dll';
PROCEDURE   cmsCIECAM02Forward(hModel: Pointer; pIn: LPcmsCIEXYZ; pOut: LPcmsJCh ); external 'lcms2.dll';
PROCEDURE   cmsCIECAM02Reverse(hModel: Pointer; pIn: LPcmsJCh;   pOut: LPcmsCIEXYZ ); external 'lcms2.dll';
FUNCTION  cmsBuildSegmentedToneCurve(ContextID: cmsContext; nSegments: cmsInt32Number; Segments: LPcmsCurveSegmentArray): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsBuildParametricToneCurve(ContextID: cmsContext;  CType: cmsInt32Number; Params: LPcmsFloat64NumberArray): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsBuildGamma(ContextID: cmsContext; Gamma: cmsFloat64Number): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsBuildTabulatedToneCurve16(ContextID: cmsContext; nEntries: cmsInt32Number; values: LPcmsUInt16NumberArray): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsBuildTabulatedToneCurveFloat(ContextID: cmsContext; nEntries: cmsUInt32Number; values: LPcmsFloat32NumberArray): LPcmsToneCurve; external 'lcms2.dll';
PROCEDURE cmsFreeToneCurve(Curve: LPcmsToneCurve); external 'lcms2.dll';
PROCEDURE cmsFreeToneCurveTriple(Curve: LPLPcmsToneCurveArray); external 'lcms2.dll';
FUNCTION  cmsDupToneCurve(Src: LPcmsToneCurve): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsReverseToneCurve(InGamma: LPcmsToneCurve): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsReverseToneCurveEx(nResultSamples: cmsInt32Number; InGamma: LPcmsToneCurve): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsJoinToneCurve(ContextID: cmsContext; X, Y: LPcmsToneCurve; nPoints: cmsUInt32Number ): LPcmsToneCurve; external 'lcms2.dll';
FUNCTION  cmsSmoothToneCurve(Tab: LPcmsToneCurve; lambda: cmsFloat64Number): cmsBool; external 'lcms2.dll';
FUNCTION  cmsEvalToneCurveFloat(Curve: LPcmsToneCurve; v: cmsFloat32Number):cmsFloat32Number; external 'lcms2.dll';
FUNCTION  cmsEvalToneCurve16(Curve: LPcmsToneCurve; v:cmsUInt16Number):cmsUInt16Number; external 'lcms2.dll';
FUNCTION  cmsIsToneCurveMultisegment(InGamma: LPcmsToneCurve):cmsBool; external 'lcms2.dll';
FUNCTION  cmsIsToneCurveLinear(Curve: LPcmsToneCurve):cmsBool; external 'lcms2.dll';
FUNCTION  cmsIsToneCurveMonotonic(t: LPcmsToneCurve):cmsBool; external 'lcms2.dll';
FUNCTION  cmsIsToneCurveDescending(t: LPcmsToneCurve):cmsBool; external 'lcms2.dll';
FUNCTION  cmsGetToneCurveParametricType(t: LPcmsToneCurve):cmsInt32Number; external 'lcms2.dll';
FUNCTION  cmsEstimateGamma(t: LPcmsToneCurve; Precision:cmsFloat64Number):cmsFloat64Number; external 'lcms2.dll';
FUNCTION  cmsPipelineAlloc(ContextID: cmsContext; InputChannels, OutputChannels: cmsUInt32Number): LPcmsPipeline; external 'lcms2.dll';
PROCEDURE cmsPipelineFree(lut: LPcmsPipeline); external 'lcms2.dll';
FUNCTION  cmsPipelineDup(Orig: LPcmsPipeline): LPcmsPipeline; external 'lcms2.dll'; 
FUNCTION  cmsPipelineInputChannels(lut: LPcmsPipeline): cmsUInt32Number; external 'lcms2.dll';
FUNCTION  cmsPipelineOutputChannels(lut: LPcmsPipeline): cmsUInt32Number; external 'lcms2.dll';
FUNCTION cmsPipelineStageCount(lut: LPcmsPipeline): cmsUInt32Number; external 'lcms2.dll';
FUNCTION cmsPipelineGetPtrToFirstStage(lut: LPcmsPipeline): LPcmsStage; external 'lcms2.dll';
FUNCTION cmsPipelineGetPtrToLastStage(lut: LPcmsPipeline): LPcmsStage; external 'lcms2.dll';

PROCEDURE cmsPipelineEval16(Inv, Outv: LPcmsUInt16NumberArray; lut: LPcmsPipeline); external 'lcms2.dll';
PROCEDURE cmsPipelineEvalFloat(Inv, Outv: LPcmsFloat32NumberArray; lut: LPcmsPipeline); external 'lcms2.dll';

FUNCTION cmsPipelineEvalReverseFloat(Target, Result, Hint: LPcmsFloat32NumberArray; lut: LPcmsPipeline): cmsBool; external 'lcms2.dll';
FUNCTION cmsPipelineCat(l1, l2: LPcmsPipeline): cmsBool; external 'lcms2.dll';
FUNCTION cmsPipelineSetSaveAs8bitsFlag(lut: LPcmsPipeline; On: cmsBool): cmsBool; external 'lcms2.dll';
PROCEDURE cmsPipelineInsertStage(lut: LPcmsPipeline; loc: cmsStageLoc; mpe: LPcmsStage); external 'lcms2.dll';
PROCEDURE cmsPipelineUnlinkStage(lut: LPcmsPipeline; loc: cmsStageLoc; mpe: LPLPcmsStage); external 'lcms2.dll';
FUNCTION  cmsStageAllocIdentity(ContextID: cmsContext; nChannels: cmsUInt32Number): LPcmsStage; external 'lcms2.dll';
FUNCTION  cmsStageAllocToneCurves(ContextID: cmsContext; nChannels: cmsUInt32Number; Curves: LPLPcmsToneCurveArray): LPcmsStage; external 'lcms2.dll';
FUNCTION  cmsStageAllocMatrix(ContextID: cmsContext; Rows, Cols: cmsUInt32Number; Matrix, Offset: LPcmsFloat64NumberArray): LPcmsStage; external 'lcms2.dll';
FUNCTION  cmsStageAllocCLut16bit(ContextID: cmsContext; nGridPoints: cmsUInt32Number; inputChan, outputChan: cmsUInt32Number; Table: LPcmsUInt16NumberArray): LPcmsStage; external 'lcms2.dll';
FUNCTION  cmsStageAllocCLutFloat(ContextID: cmsContext; nGridPoints: cmsUInt32Number; inputChan, outputChan: cmsUInt32Number; Table: LPcmsFloat32NumberArray): LPcmsStage; external 'lcms2.dll';
FUNCTION  cmsStageAllocCLut16bitGranular(ContextID: cmsContext; nGridPoints: LPcmsUInt32NumberArray; inputChan, outputChan: cmsUInt32Number; Table: LPcmsUInt16NumberArray): LPcmsStage; external 'lcms2.dll';
FUNCTION  cmsStageAllocCLutFloatGranular(ContextID: cmsContext; nGridPoints: LPcmsUInt32NumberArray; inputChan, outputChan: cmsUInt32Number; Table: LPcmsFloat32NumberArray): LPcmsStage; external 'lcms2.dll';
FUNCTION  cmsStageDup(mpe: LPcmsStage): LPcmsStage; external 'lcms2.dll';
PROCEDURE cmsStageFree(mpe: LPcmsStage); external 'lcms2.dll';
FUNCTION  cmsStageNext(mpe: LPcmsStage): LPcmsStage; external 'lcms2.dll';
FUNCTION cmsStageInputChannels(mpe: LPcmsStage): cmsUInt32Number; external 'lcms2.dll';
FUNCTION cmsStageOutputChannels(mpe: LPcmsStage): cmsUInt32Number; external 'lcms2.dll';
FUNCTION cmsStageType(mpe: LPcmsStage): cmsStageSignature; external 'lcms2.dll';
FUNCTION cmsStageData(mpe: LPcmsStage): Pointer; external 'lcms2.dll';
FUNCTION cmsStageSampleCLut16bit(mpe: LPcmsStage;  Sampler: cmsSAMPLER16;    Cargo: Pointer; dwFlags: cmsUInt32Number): cmsBool; external 'lcms2.dll';
FUNCTION cmsStageSampleCLutFloat(mpe: LPcmsStage;  Sampler: cmsSAMPLERFLOAT; Cargo: Pointer; dwFlags: cmsUInt32Number): cmsBool; external 'lcms2.dll';
FUNCTION  cmsSliceSpace16(nInputs: cmsUInt32Number; clutPoints: LPcmsUInt32NumberArray;
                                                   Sampler: cmsSAMPLER16; Cargo: Pointer): cmsBool; external 'lcms2.dll';

FUNCTION cmsSliceSpaceFloat(nInputs: cmsUInt32Number; clutPoints: LPcmsUInt32NumberArray;
                                                   Sampler: cmsSAMPLERFLOAT; Cargo: Pointer): cmsBool; external 'lcms2.dll';
FUNCTION  cmsMLUalloc(ContextID: cmsContext; nItems: cmsUInt32Number): LPcmsMLU; external 'lcms2.dll'; 
PROCEDURE cmsMLUfree(mlu: LPcmsMLU); external 'lcms2.dll';
FUNCTION  cmsMLUdup(mlu: LPcmsMLU): LPcmsMLU; external 'lcms2.dll'; 

FUNCTION  cmsMLUsetASCII(mlu: LPcmsMLU; LanguageCode, CountryCode, ASCIIString: PAnsiChar): cmsBool; external 'lcms2.dll';
FUNCTION  cmsMLUsetWide(mlu: LPcmsMLU; LanguageCode, CountryCode: PAnsiChar; WideString: PWChar): cmsBool; external 'lcms2.dll';

FUNCTION cmsMLUgetASCII(mlu: LPcmsMLU; LanguageCode, CountryCode: PAnsiChar; Buffer: PAnsiChar; BufferSize: cmsUInt32Number): cmsUInt32Number; external 'lcms2.dll';

FUNCTION cmsMLUgetWide(mlu: LPcmsMLU; LanguageCode, CountryCode: PAnsiChar; Buffer: PWChar; BufferSize: cmsUInt32Number): cmsUInt32Number; external 'lcms2.dll';
                                                 
FUNCTION cmsMLUgetTranslation(mlu: LPcmsMLU; LanguageCode, CountryCode, ObtainedLanguage, ObtainedCountry: PAnsiChar): cmsBool; external 'lcms2.dll';

FUNCTION cmsAllocNamedColorList(ContextID: cmsContext; n, ColorantCount :cmsUInt32Number;
                                                           Prefix, Suffix: PAnsiChar): LPcmsNAMEDCOLORLIST; external 'lcms2.dll';

PROCEDURE cmsFreeNamedColorList(v: LPcmsNAMEDCOLORLIST); external 'lcms2.dll';
FUNCTION  cmsDupNamedColorList(v: LPcmsNAMEDCOLORLIST): LPcmsNAMEDCOLORLIST; external 'lcms2.dll';
FUNCTION  cmsAppendNamedColor(v: LPcmsNAMEDCOLORLIST; Name: PAnsiChar;
                                                             PCS, Colorant : LPcmsUInt16NumberArray): cmsBool; external 'lcms2.dll';

FUNCTION cmsNamedColorCount(v: LPcmsNAMEDCOLORLIST): cmsUInt32Number; external 'lcms2.dll';
FUNCTION cmsNamedColorIndex(v: LPcmsNAMEDCOLORLIST; Name: PAnsiChar): cmsInt32Number; external 'lcms2.dll';

FUNCTION cmsNamedColorInfo(v: LPcmsNAMEDCOLORLIST; nColor : cmsUInt32Number;
                                                      Name,Prefix, Suffix : PAnsiChar;
                                                       PCS, Colorant : LPcmsUInt16NumberArray): cmsBool; external 'lcms2.dll';

FUNCTION cmsGetNamedColorList(xform: cmsHTRANSFORM ): LPcmsNAMEDCOLORLIST; external 'lcms2.dll';

FUNCTION   cmsAllocProfileSequenceDescription(ContextID: cmsContext; n: cmsUInt32Number):LPcmsSEQ; external 'lcms2.dll';
FUNCTION   cmsDupProfileSequenceDescription(pseq: LPcmsSEQ):LPcmsSEQ; external 'lcms2.dll';
PROCEDURE  cmsFreeProfileSequenceDescription(pseq: LPcmsSEQ); external 'lcms2.dll';
FUNCTION cmsCreateProfilePlaceholder(ContextID: cmsContext): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsGetProfileContextID(hProfile: cmsHPROFILE):cmsContext; external 'lcms2.dll';
FUNCTION cmsGetTagCount(hProfile: cmsHPROFILE): cmsInt32Number; external 'lcms2.dll';
FUNCTION cmsGetTagSignature(hProfile: cmsHPROFILE; n: cmsUInt32Number): cmsTagSignature; external 'lcms2.dll';
FUNCTION cmsIsTag(hProfile: cmsHPROFILE; sig: cmsTagSignature ): cmsBool; external 'lcms2.dll';

FUNCTION cmsReadTag(hProfile: cmsHPROFILE; sig: cmsTagSignature ): Pointer; external 'lcms2.dll';
FUNCTION cmsWriteTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; data: Pointer): cmsBool; external 'lcms2.dll';
FUNCTION cmsLinkTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; dest: cmsTagSignature): cmsBool; external 'lcms2.dll';
FUNCTION cmsTagLinkedTo(hProfile: cmsHPROFILE; sig: cmsTagSignature):cmsTagSignature; external 'lcms2.dll';

FUNCTION cmsReadRawTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; Buffer: Pointer; BufferSize: cmsUInt32Number): cmsInt32Number; external 'lcms2.dll';
FUNCTION cmsWriteRawTag(hProfile: cmsHPROFILE; sig: cmsTagSignature; data: Pointer; Size: cmsUInt32Number): cmsBool; external 'lcms2.dll';

FUNCTION  cmsGetHeaderFlags(hProfile: cmsHPROFILE): cmsUInt32Number; external 'lcms2.dll';
PROCEDURE cmsGetHeaderAttributes(hProfile: cmsHPROFILE; Flags: LPcmsUInt64Number); external 'lcms2.dll';
PROCEDURE cmsGetHeaderProfileID(hProfile: cmsHPROFILE; ProfileID: LPcmsUInt8Number); external 'lcms2.dll';

FUNCTION  cmsGetHeaderRenderingIntent(hProfile: cmsHPROFILE): cmsUInt32Number; external 'lcms2.dll';
PROCEDURE cmsSetHeaderFlags(hProfile: cmsHPROFILE; Flags: cmsUInt32Number); external 'lcms2.dll';
FUNCTION  cmsGetHeaderManufacturer(hProfile: cmsHPROFILE): cmsUInt32Number; external 'lcms2.dll';
PROCEDURE cmsSetHeaderManufacturer(hProfile: cmsHPROFILE; manufacturer: cmsUInt32Number ); external 'lcms2.dll';
FUNCTION  cmsGetHeaderModel(hProfile: cmsHPROFILE): cmsUInt32Number; external 'lcms2.dll';
PROCEDURE cmsSetHeaderModel(hProfile: cmsHPROFILE; model: cmsUInt32Number ); external 'lcms2.dll';
PROCEDURE cmsSetHeaderAttributes(hProfile: cmsHPROFILE; Flags: cmsUInt64Number); external 'lcms2.dll';
PROCEDURE cmsSetHeaderProfileID(hProfile: cmsHPROFILE; ProfileID: LPcmsUInt8Number); external 'lcms2.dll';
PROCEDURE cmsSetHeaderRenderingIntent(hProfile: cmsHPROFILE; RenderingIntent: cmsUInt32Number ); external 'lcms2.dll';

FUNCTION  cmsGetPCS(hProfile: cmsHPROFILE):cmsColorSpaceSignature; external 'lcms2.dll';
PROCEDURE cmsSetPCS(hProfile: cmsHPROFILE; pcs: cmsColorSpaceSignature); external 'lcms2.dll';
FUNCTION  cmsGetColorSpace(hProfile: cmsHPROFILE): cmsColorSpaceSignature; external 'lcms2.dll';
PROCEDURE cmsSetColorSpace(hProfile: cmsHPROFILE; sig: cmsColorSpaceSignature); external 'lcms2.dll';
FUNCTION  cmsGetDeviceClass(hProfile: cmsHPROFILE): cmsProfileClassSignature; external 'lcms2.dll';
PROCEDURE cmsSetDeviceClass(hProfile: cmsHPROFILE; sig: cmsProfileClassSignature); external 'lcms2.dll';
PROCEDURE cmsSetProfileVersion(hProfile: cmsHPROFILE; Version: cmsFloat64Number); external 'lcms2.dll';
FUNCTION  cmsGetProfileVersion(hProfile: cmsHPROFILE): cmsFloat64Number; external 'lcms2.dll';

FUNCTION  cmsGetEncodedICCversion(hProfile: cmsHPROFILE): cmsUInt32Number; external 'lcms2.dll';
PROCEDURE cmsSetEncodedICCversion(hProfile: cmsHPROFILE; Version: cmsUInt32Number); external 'lcms2.dll';


FUNCTION   cmsIsIntentSupported(hProfile: cmsHPROFILE; Intent: cmsUInt32Number; UsedDirection: cmsUInt32Number): cmsBool; external 'lcms2.dll';
FUNCTION   cmsIsMatrixShaper(hProfile: cmsHPROFILE): cmsBool; external 'lcms2.dll';
FUNCTION   cmsIsCLUT(hProfile: cmsHPROFILE; Intent: cmsUInt32Number; UsedDirection: cmsUInt32Number): cmsBool; external 'lcms2.dll';
FUNCTION _cmsICCcolorSpace(OurNotation: Integer): cmsColorSpaceSignature; external 'lcms2.dll';
FUNCTION _cmsLCMScolorSpace(ProfileSpace: cmsColorSpaceSignature): Integer; external 'lcms2.dll';

FUNCTION cmsChannelsOf( ColorSpace: cmsColorSpaceSignature): cmsUInt32Number; external 'lcms2.dll';

FUNCTION cmsFormatterForColorspaceOfProfile(hProfile: cmsHPROFILE; nBytes: cmsUInt32Number; lIsFloat: cmsBool): cmsUInt32Number; external 'lcms2.dll';
FUNCTION cmsFormatterForPCSOfProfile(hProfile: cmsHPROFILE; nBytes: cmsUInt32Number; lIsFloat: cmsBool): cmsUInt32Number; external 'lcms2.dll';


FUNCTION cmsGetProfileInfo(hProfile: cmsHPROFILE; Info: cmsInfoType; LanguageCode, CountryCode: PAnsiChar;
                                                            Buffer: PWChar; BufferSize: cmsUInt32Number): cmsUInt32Number; external 'lcms2.dll';

FUNCTION cmsGetProfileInfoASCII(hProfile: cmsHPROFILE; Info: cmsInfoType; LanguageCode, CountryCode: PAnsiChar;
                                                            Buffer: PAnsiChar; BufferSize: cmsUInt32Number): cmsUInt32Number; external 'lcms2.dll';


FUNCTION cmsOpenIOhandlerFromFile(ContextID: cmsContext; FileName, AccessMode: PAnsiChar): LPcmsIOHANDLER; external 'lcms2.dll';
// FUNCTION cmsOpenIOhandlerFromStream(ContextID: cmsContext; FILE* Stream): LPcmsIOHANDLER; external 'lcms2.dll';
FUNCTION cmsOpenIOhandlerFromMem(ContextID: cmsContext; Buffer: Pointer; size: cmsUInt32Number; AccessMode: PAnsiChar): LPcmsIOHANDLER; external 'lcms2.dll';
FUNCTION cmsOpenIOhandlerFromNULL(ContextID: cmsContext): LPcmsIOHANDLER; external 'lcms2.dll';
FUNCTION cmsCloseIOhandler(io: LPcmsIOHANDLER): cmsBool; external 'lcms2.dll';

FUNCTION cmsMD5computeID(hProfile: cmsHPROFILE): cmsBool; external 'lcms2.dll';

FUNCTION   cmsOpenProfileFromFile(ICCProfile : PAnsiChar; sAccess: PAnsiChar): cmsHPROFILE; external 'lcms2.dll';
FUNCTION   cmsOpenProfileFromFileTHR(ContextID: cmsContext; ICCProfile, sAccess: PAnsiChar): cmsHPROFILE; external 'lcms2.dll';
// FUNCTION      CMSEXPORT cmsOpenProfileFromStream(FILE* ICCProfile, const char* sAccess): cmsHPROFILE; external 'lcms2.dll';
// FUNCTION      CMSEXPORT cmsOpenProfileFromStreamTHR(ContextID: cmsContext; FILE* ICCProfile, const char* sAccess): cmsHPROFILE; external 'lcms2.dll';
FUNCTION   cmsOpenProfileFromMem(MemPtr: Pointer; dwSize: cmsUInt32Number): cmsHPROFILE; external 'lcms2.dll';
FUNCTION   cmsOpenProfileFromMemTHR(ContextID: cmsContext; MemPtr: Pointer; dwSize: cmsUInt32Number): cmsHPROFILE; external 'lcms2.dll';
FUNCTION   cmsOpenProfileFromIOhandlerTHR(ContextID: cmsContext; io: LPcmsIOHANDLER): cmsHPROFILE; external 'lcms2.dll';
FUNCTION   cmsCloseProfile(hProfile: cmsHPROFILE): cmsBool; external 'lcms2.dll';

FUNCTION   cmsSaveProfileToFile(hProfile: cmsHPROFILE; FileName: PAnsiChar): cmsBool; external 'lcms2.dll';
// FUNCTION         CMSEXPORT cmsSaveProfileToStream(hProfile: cmsHPROFILE, FILE* Stream): cmsBool; external 'lcms2.dll';
FUNCTION   cmsSaveProfileToMem(hProfile: cmsHPROFILE; MemPtr: Pointer; BytesNeeded: LPcmsUInt32Number): cmsBool; external 'lcms2.dll';
FUNCTION   cmsSaveProfileToIOhandler(hProfile: cmsHPROFILE; io: LPcmsIOHANDLER):cmsUInt32Number; external 'lcms2.dll';

FUNCTION  cmsCreateRGBProfileTHR(ContextID: cmsContext;
                                                   WhitePoint: LPcmsCIExyY;
                                                   Primaries: LPcmsCIExyYTRIPLE;
                                                   TransferFunction: LPLPcmsToneCurveArray): cmsHPROFILE; external 'lcms2.dll';

FUNCTION  cmsCreateRGBProfile(WhitePoint: LPcmsCIExyY;
                                                   Primaries: LPcmsCIExyYTRIPLE;
                                                   TransferFunction: LPLPcmsToneCurveArray): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateGrayProfileTHR(ContextID: cmsContext;
                                                    WhitePoint: LPcmsCIExyY;
                                                    TransferFunction: LPcmsToneCurve): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateGrayProfile(WhitePoint: LPcmsCIExyY;
                                                     TransferFunction: LPcmsToneCurve): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateLinearizationDeviceLinkTHR(ContextID: cmsContext;
                                                                 ColorSpace: cmsColorSpaceSignature;
                                                                 TransferFunctions: LPLPcmsToneCurveArray): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateLinearizationDeviceLink(ColorSpace: cmsColorSpaceSignature;
                                                                 TransferFunctions: LPLPcmsToneCurveArray): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateInkLimitingDeviceLinkTHR(ContextID: cmsContext;
                                                              ColorSpace: cmsColorSpaceSignature; Limit: cmsFloat64Number): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateInkLimitingDeviceLink(ColorSpace: cmsColorSpaceSignature; Limit: cmsFloat64Number): cmsHPROFILE; external 'lcms2.dll';


FUNCTION cmsCreateLab2ProfileTHR(ContextID: cmsContext; WhitePoint: LPcmsCIExyY): cmsHPROFILE; external 'lcms2.dll';
FUNCTION cmsCreateLab2Profile(WhitePoint: LPcmsCIExyY): cmsHPROFILE; external 'lcms2.dll';
FUNCTION cmsCreateLab4ProfileTHR(ContextID: cmsContext; WhitePoint: LPcmsCIExyY): cmsHPROFILE; external 'lcms2.dll';
FUNCTION cmsCreateLab4Profile(WhitePoint: LPcmsCIExyY): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateXYZProfileTHR(ContextID: cmsContext): cmsHPROFILE; external 'lcms2.dll';
FUNCTION cmsCreateXYZProfile: cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreate_sRGBProfileTHR(ContextID: cmsContext): cmsHPROFILE; external 'lcms2.dll';
FUNCTION cmsCreate_sRGBProfile: cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateBCHSWabstractProfileTHR(ContextID: cmsContext;
                                                             nLUTPoints: Integer;
                                                             Bright,
                                                             Contrast,
                                                             Hue,
                                                             Saturation: cmsFloat64Number;
                                                             TempSrc,
                                                             TempDest: Integer): cmsHPROFILE; external 'lcms2.dll';

FUNCTION cmsCreateBCHSWabstractProfile(   nLUTPoints: Integer;
                                                             Bright,
                                                             Contrast,
                                                             Hue,
                                                             Saturation: cmsFloat64Number;
                                                             TempSrc,
                                                             TempDest: Integer): cmsHPROFILE; external 'lcms2.dll';

FUNCTION  cmsCreateNULLProfileTHR(ContextID: cmsContext): cmsHPROFILE; external 'lcms2.dll';
FUNCTION  cmsCreateNULLProfile: cmsHPROFILE; external 'lcms2.dll';

// Converts a transform to a devicelink profile
FUNCTION  cmsTransform2DeviceLink(hTransform: cmsHTRANSFORM; Version: cmsFloat64Number; dwFlags: cmsUInt32Number): cmsHPROFILE; external 'lcms2.dll';

// Call with NULL as parameters to get the intent count
FUNCTION cmsGetSupportedIntents(nMax: cmsUInt32Number; Codes: LPcmsUInt32Number; Descriptions: LPPAnsiChar): cmsUInt32Number; external 'lcms2.dll';

FUNCTION cmsFLAGS_GRIDPOINTS(n: Integer): Integer; begin cmsFLAGS_GRIDPOINTS :=  (((n) and $FF) shl 16) end;


FUNCTION   cmsCreateTransformTHR(ContextID: cmsContext;
                                                  Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; external 'lcms2.dll';

FUNCTION   cmsCreateTransform(Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; external 'lcms2.dll';

FUNCTION   cmsCreateProofingTransformTHR(ContextID: cmsContext;
                                                  Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Proofing: cmsHPROFILE;
                                                  Intent: cmsUInt32Number;
                                                  ProofingIntent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; external 'lcms2.dll';

FUNCTION   cmsCreateProofingTransform(Input: cmsHPROFILE;
                                                  InputFormat: cmsUInt32Number;
                                                  Output: cmsHPROFILE;
                                                  OutputFormat: cmsUInt32Number;
                                                  Proofing: cmsHPROFILE;
                                                  Intent: cmsUInt32Number;
                                                  ProofingIntent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; external 'lcms2.dll';

FUNCTION   cmsCreateMultiprofileTransformTHR(ContextID: cmsContext;
                                                  hProfiles: LPcmsHPROFILEArray;
                                                  nProfiles: cmsUInt32Number;
                                                  InputFormat: cmsUInt32Number;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; external 'lcms2.dll';


FUNCTION   cmsCreateMultiprofileTransform( hProfiles: LPcmsHPROFILEArray;
                                                  nProfiles: cmsUInt32Number;
                                                  InputFormat: cmsUInt32Number;
                                                  OutputFormat: cmsUInt32Number;
                                                  Intent: cmsUInt32Number;
                                                  dwFlags: cmsUInt32Number): cmsHTRANSFORM; external 'lcms2.dll';


FUNCTION   cmsCreateExtendedTransform(ContextID: cmsContext;
                                                   nProfiles: cmsUInt32Number;
                                                   hProfiles: LPcmsHPROFILEArray;
                                                   BPC: LPcmsBoolArray;
                                                   Intents: LPcmsUInt32NumberArray;
                                                   AdaptationStates: LPcmsFloat64NumberArray;
                                                   hGamutProfile: cmsHPROFILE;
                                                   nGamutPCSposition: cmsUInt32Number;
                                                   InputFormat,
                                                   OutputFormat: cmsUInt32Number;
                                                   dwFlags: cmsUInt32Number): cmsHTRANSFORM; external 'lcms2.dll';

PROCEDURE  cmsDeleteTransform(hTransform: cmsHTRANSFORM); external 'lcms2.dll';

PROCEDURE  cmsDoTransform(Transform: cmsHTRANSFORM; InputBuffer, OutputBuffer: Pointer; size: cmsUInt32Number);  external 'lcms2.dll';

PROCEDURE  cmsSetAlarmCodes( NewAlarm: LPcmsUInt16NumberArray);  external 'lcms2.dll';
PROCEDURE  cmsGetAlarmCodes(NewAlarm: LPcmsUInt16NumberArray); external 'lcms2.dll';

// Adaptation state for absolute colorimetric intent
FUNCTION  cmsSetAdaptationState(d: cmsFloat64Number):cmsFloat64Number; external 'lcms2.dll';

// Grab the ContextID from an open transform. Returns NULL if a NULL transform is passed
FUNCTION  cmsGetTransformContextID(hTransform: cmsHTRANSFORM):cmsContext; external 'lcms2.dll';

// For backwards compatibility
FUNCTION  cmsChangeBuffersFormat(hTransform: cmsHTRANSFORM; InputFormat, OutputFormat: cmsUInt32Number): cmsBool; external 'lcms2.dll';


// lcms2 unified method to access postscript color resources
FUNCTION cmsGetPostScriptColorResource(ContextID: cmsContext;   RType: cmsPSResourceType;
                                                                hProfile: cmsHPROFILE;
                                                                Intent: cmsUInt32Number;
                                                                dwFlags: cmsUInt32Number;
                                                                io: LPcmsIOHANDLER): cmsUInt32Number; external 'lcms2.dll';

FUNCTION cmsGetPostScriptCSA(ContextID: cmsContext; hProfile: cmsHPROFILE; Intent: cmsUInt32Number; dwFlags: cmsUInt32Number; Buffer: Pointer; dwBufferLen: cmsUInt32Number ): cmsUInt32Number; external 'lcms2.dll';
FUNCTION cmsGetPostScriptCRD(ContextID: cmsContext; hProfile: cmsHPROFILE; Intent: cmsUInt32Number; dwFlags: cmsUInt32Number; Buffer: Pointer; dwBufferLen: cmsUInt32Number): cmsUInt32Number; external 'lcms2.dll';


// CGATS.13 parser

FUNCTION  cmsIT8Alloc: cmsHANDLE; external 'lcms2.dll';
PROCEDURE cmsIT8Free(hIT8: cmsHANDLE); external 'lcms2.dll';

// Tables

FUNCTION  cmsIT8TableCount(hIT8: cmsHANDLE): Integer; external 'lcms2.dll';
FUNCTION  cmsIT8SetTable(hIT8: cmsHANDLE; nTable: Integer): Integer; external 'lcms2.dll';

// Persistence
FUNCTION  cmsIT8LoadFromFile(cFileName: PAnsiChar): cmsHANDLE; external 'lcms2.dll';
FUNCTION  cmsIT8LoadFromMem(Ptr: Pointer; size :DWord): cmsHANDLE; external 'lcms2.dll';

FUNCTION cmsIT8SaveToFile(hIT8: cmsHANDLE; cFileName: PAnsiChar): cmsBool; external 'lcms2.dll';
FUNCTION cmsIT8SaveToMem(hIT8: cmsHANDLE; MemPtr: Pointer; BytesNeeded: LPcmsUInt32Number): cmsBool; external 'lcms2.dll';
// Properties

FUNCTION cmsIT8GetSheetType(hIT8: cmsHANDLE): PAnsiChar; external 'lcms2.dll';
FUNCTION cmsIT8SetSheetType(hIT8: cmsHANDLE; TheType: PAnsiChar): cmsBool; external 'lcms2.dll';

FUNCTION cmsIT8SetComment(hIT8: cmsHANDLE; cComment: PAnsiChar): cmsBool; external 'lcms2.dll';

FUNCTION cmsIT8SetPropertyStr(hIT8: cmsHANDLE; cProp, Str: PAnsiChar): cmsBool; external 'lcms2.dll';
FUNCTION cmsIT8SetPropertyDbl(hIT8: cmsHANDLE; cProp: PAnsiChar; Val: Double): cmsBool; external 'lcms2.dll';
FUNCTION cmsIT8SetPropertyHex(hIT8: cmsHANDLE; cProp: PAnsiChar; Val: Integer): cmsBool; external 'lcms2.dll';
FUNCTION cmsIT8SetPropertyUncooked(hIT8: cmsHANDLE; Key, Buffer: PAnsiChar): cmsBool; external 'lcms2.dll';


FUNCTION cmsIT8GetProperty(hIT8: cmsHANDLE; cProp: PAnsiChar): PAnsiChar; external 'lcms2.dll';
FUNCTION cmsIT8GetPropertyDbl(hIT8: cmsHANDLE; cProp: PAnsiChar): Double; external 'lcms2.dll';
FUNCTION cmsIT8EnumProperties(hIT8: cmsHANDLE; var PropertyNames: LPPAnsiChar): Integer; external 'lcms2.dll';

// Datasets

FUNCTION cmsIT8GetDataRowCol(hIT8: cmsHANDLE; row, col: Integer): PAnsiChar; external 'lcms2.dll';
FUNCTION cmsIT8GetDataRowColDbl(hIT8: cmsHANDLE; row, col: Integer): Double; external 'lcms2.dll';

FUNCTION cmsIT8SetDataRowCol(hIT8: cmsHANDLE; row, col: Integer; Val: PAnsiChar): cmsBool; external 'lcms2.dll';
FUNCTION cmsIT8SetDataRowColDbl(hIT8: cmsHANDLE; row, col: Integer; Val: Double): cmsBool; external 'lcms2.dll';

FUNCTION cmsIT8GetData(hIT8: cmsHANDLE; cPatch, cSample: PAnsiChar): PAnsiChar; external 'lcms2.dll';

FUNCTION cmsIT8GetDataDbl(hIT8: cmsHANDLE;cPatch, cSample: PAnsiChar): Double; external 'lcms2.dll';

FUNCTION cmsIT8SetData(hIT8: cmsHANDLE; cPatch, cSample, Val: PAnsiChar): cmsBool; external 'lcms2.dll';

FUNCTION cmsIT8SetDataDbl(hIT8: cmsHANDLE; cPatch, cSample: PAnsiChar; Val: Double): cmsBool; external 'lcms2.dll';

FUNCTION cmsIT8SetDataFormat(hIT8: cmsHANDLE; n: Integer; Sample: PAnsiChar): cmsBool; external 'lcms2.dll';
FUNCTION cmsIT8EnumDataFormat(hIT8: cmsHANDLE; var SampleNames: LPPAnsiChar): Integer; external 'lcms2.dll';
FUNCTION cmsIT8GetPatchName(hIT8: cmsHANDLE; nPatch: Integer; Buffer: PAnsiChar): PAnsiChar; external 'lcms2.dll';

// The LABEL extension

FUNCTION cmsIT8SetTableByLabel(hIT8: cmsHANDLE; cSet, cField, ExpectedType: PAnsiChar): Integer; external 'lcms2.dll';

FUNCTION cmsIT8FindDataFormat(hIT8: cmsHANDLE; cSample: PAnsiChar): Integer; external 'lcms2.dll';

// Formatter for double
PROCEDURE  cmsIT8DefineDblFormat(hIT8: cmsHANDLE; Formatter: PAnsiChar);  external 'lcms2.dll';

FUNCTION  cmsGBDAlloc(ContextID: cmsContext):cmsHANDLE; external 'lcms2.dll';
PROCEDURE cmsGBDFree(hGBD: cmsHANDLE); external 'lcms2.dll';
FUNCTION  cmsGDBAddPoint(hGBD: cmsHANDLE; Lab: LPcmsCIELab): cmsBool; external 'lcms2.dll';
FUNCTION  cmsGDBCompute(hGDB: cmsHANDLE; dwFlags: cmsUInt32Number): cmsBool; external 'lcms2.dll';
FUNCTION  cmsGDBCheckPoint(hGBD: cmsHANDLE; Lab: LPcmsCIELab): cmsBool; external 'lcms2.dll';

FUNCTION cmsDetectBlackPoint( BlackPoint: LPcmsCIEXYZ; hProfile: cmsHPROFILE; Intent: cmsUInt32Number; dwFlags: cmsUInt32Number): cmsBool; external 'lcms2.dll';

FUNCTION cmsDetectTAC(hProfile: cmsHPROFILE): cmsFloat64Number; external 'lcms2.dll';

FUNCTION  cmsDesaturateLab(Lab: LPcmsCIELab; amax, amin, bmax, bmin: cmsFloat64Number): cmsBool; external 'lcms2.dll';
                                                   
END.
