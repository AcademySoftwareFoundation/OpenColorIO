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

#include "OpenColorIO_PS_Version.h"

// Definitions
#define plugInName          "OpenColorIO"
#define plugInCopyrightYear OpenColorIO_PS_Copyright_Year
#define plugInDescription  OpenColorIO_PS_Description
#define VersionString   OpenColorIO_PS_Version_String
#define ReleaseString   OpenColorIO_PS_Build_Date_Manual
#define CurrentYear     OpenColorIO_PS_Build_Year

// dictionary (aete) definitions
#define vendorName          "fnord"
#define plugInAETEComment   "OpenColorIO"

#define plugInSuiteID       'ocio'
#define plugInClassID       plugInSuiteID
#define plugInEventID       plugInClassID

#include "PIDefines.h"

#ifdef __PIMac__
    #include "Types.r"
    #include "SysTypes.r"
    #include "PIGeneral.r"
    #include "PIUtilities.r"
#elif defined(__PIWin__)
    #define Rez
    #include "PIGeneral.h"
    #include "PIUtilities.r"
#endif

#include "PITerminology.h"
#include "PIActions.h"

#include "OpenColorIO_PS_Terminology.h"


resource 'PiPL' ( 16000, "OpenColorIO", purgeable )
{
    {
        Kind { Filter },
        Name { plugInName "..."},
        Category { "OpenColorIO" },
        Version { (latestFilterVersion << 16) | latestFilterSubVersion },

        #ifdef __PIMac__
            #if (defined(__x86_64__))
                CodeMacIntel64 { "PluginMain" },
            #endif
            #if (defined(__i386__))
                CodeMacIntel32 { "PluginMain" },
            #endif
            #if (defined(__ppc__))
                CodeMachOPowerPC { 0, 0, "PluginMain" },
            #endif
        #else
            #if defined(_WIN64)
                CodeWin64X86 { "PluginMain" },
            #else
                CodeWin32X86 { "PluginMain" },
            #endif
        #endif

        // ClassID, eventID, aete ID, uniqueString:
        HasTerminology { plugInClassID, plugInEventID, ResourceID, vendorName " " plugInName },

        SupportedModes
        {
            noBitmap, noGrayScale,
            noIndexedColor, doesSupportRGBColor,
            noCMYKColor, noHSLColor,
            noHSBColor, noMultichannel,
            noDuotone, noLABColor
        },
            
        EnableInfo
        {
            "in (PSHOP_ImageMode, RGBMode, RGB48Mode, RGB96Mode)"
        },

        FilterLayerSupport {doesSupportFilterLayers}, // lets us work on smart objects

        PlugInMaxSize { 2000000, 2000000 },

        FilterCaseInfo {
            {   /* array: 7 elements */
                /* Flat data, no selection */
                inStraightData,
                outStraightData,
                doNotWriteOutsideSelection,
                doesNotFilterLayerMasks,
                doesNotWorkWithBlankData,
                copySourceToDestination,
                /* Flat data with selection */
                inStraightData,
                outStraightData,
                doNotWriteOutsideSelection,
                doesNotFilterLayerMasks,
                doesNotWorkWithBlankData,
                copySourceToDestination,
                /* Floating selection */
                inStraightData,
                outStraightData,
                doNotWriteOutsideSelection,
                doesNotFilterLayerMasks,
                doesNotWorkWithBlankData,
                copySourceToDestination,
                /* Editable transparency, no selection */
                inStraightData,
                outStraightData,
                doNotWriteOutsideSelection,
                doesNotFilterLayerMasks,
                doesNotWorkWithBlankData,
                copySourceToDestination,
                /* Editable transparency, with selection */
                inStraightData,
                outStraightData,
                doNotWriteOutsideSelection,
                doesNotFilterLayerMasks,
                doesNotWorkWithBlankData,
                copySourceToDestination,
                /* Preserved transparency, no selection */
                inStraightData,
                outStraightData,
                doNotWriteOutsideSelection,
                doesNotFilterLayerMasks,
                doesNotWorkWithBlankData,
                copySourceToDestination,
                /* Preserved transparency, with selection */
                inStraightData,
                outStraightData,
                doNotWriteOutsideSelection,
                doesNotFilterLayerMasks,
                doesNotWorkWithBlankData,
                copySourceToDestination
            }
        }
    }
};

resource 'aete' (ResourceID, plugInName " dictionary", purgeable)
{
    1, 0, english, roman,                                   /* aete version and language specifiers */
    {
        vendorName,                                         /* vendor suite name */
        "OpenColorIO",                                 /* optional description */
        plugInSuiteID,                                      /* suite ID */
        1,                                                  /* suite code, must be 1 */
        1,                                                  /* suite level, must be 1 */
        {                                                   /* structure for filters */
            plugInName,                                     /* unique filter name */
            plugInAETEComment,                              /* optional description */
            plugInClassID,                                  /* class ID, must be unique or Suite ID */
            plugInEventID,                                  /* event ID, must be unique */
            
            NO_REPLY,                                       /* never a reply */
            IMAGE_DIRECT_PARAMETER,                         /* direct parameter, used by Photoshop */
            {                                               /* parameters here, if any */
                "Source",                                   /* parameter name */
                ocioKeySource,                                  /* parameter key ID */
                typeSource,                                 /* parameter type ID */
                "Source of the OCIO configuration",         /* optional description */
                flagsEnumeratedParameter,                   /* parameter flags */               

                "Configuration",
                ocioKeyConfigName,
                typeChar,
                "OCIO Configuration Name",
                flagsSingleParameter,       

                "File",
                ocioKeyConfigFileHandle,
                typePlatformFilePath,
                "OCIO Configuration File",
                flagsSingleParameter,       

                "Action",
                ocioKeyAction,
                typeAction,
                "OCIO Action",
                flagsEnumeratedParameter,
                
                "Invert",
                ocioKeyInvert,
                typeBoolean,
                "Invert LUT",
                flagsSingleParameter,
                
                "Interpolation",
                ocioKeyInterpolation,
                typeInterpolation,
                "OCIO Interpolation",
                flagsEnumeratedParameter,
                
                "Input Space",
                ocioKeyInputSpace,
                typeChar,
                "OCIO Input Space",
                flagsSingleParameter,
                
                "Output Space",
                ocioKeyOutputSpace,
                typeChar,
                "OCIO Output Space",
                flagsSingleParameter,
                
                "Transform",
                ocioKeyTransform,
                typeChar,
                "OCIO Transform",
                flagsSingleParameter,
                
                "Device",
                ocioKeyDevice,
                typeChar,
                "OCIO Device",
                flagsSingleParameter
            }
        },
        {},                                                 /* non-filter plug-in class here */
        {},                                                 /* comparison ops (not supported) */
        {                                                   /* any enumerations */
            typeSource,
            {
                "Environment",
                sourceEnvironment,
                "$OCIO Environment Variable",
                
                "Standard",
                sourceStandard,
                "Standard OCIO Configuration",
                
                "Custom",
                sourceCustom,
                "Custom Configuration Path"
            },
            
            typeAction,
            {
                "LUT",
                actionLUT,
                "Apply a LUT",
                
                "Convert",
                actionConvert,
                "Convert Transform",
                
                "Display",
                actionDisplay,
                "Display Transform"
            },
            
            typeInterpolation,
            {
                "Nearest",
                interpNearest,
                "Nearest Interpolation",
                
                "Linear",
                interpLinear,
                "Linear Interpolation",
                
                "Tetrahedral",
                interpTetrahedral,
                "Tetrahedral Interpolation",
                
                "Best",
                interpBest,
                "Best Interpolation"
            }
        }
    }
};

