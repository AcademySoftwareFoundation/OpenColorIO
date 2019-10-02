// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "AEConfig.h"

#include "AE_EffectVers.h"

#ifndef AE_OS_WIN
    #include "AE_General.r"
#endif

resource 'PiPL' (16000) {
    {   /* array properties: 12 elements */
        /* [1] */
        Kind {
            AEEffect
        },
        /* [2] */
        Name {
            "OpenColorIO"
        },
        /* [3] */
        Category {
            "Utility"
        },
        
#ifdef AE_OS_WIN
    #ifdef AE_PROC_INTELx64
        CodeWin64X86 {"PluginMain"},
    #else
        CodeWin32X86 {"PluginMain"},
    #endif  
#else
    #ifdef AE_PROC_INTELx64
        CodeMacIntel64 {"PluginMain"},
    #else
        CodeMachOPowerPC {"PluginMain"},
        CodeMacIntel32 {"PluginMain"},
    #endif
#endif      /* [6] */
        AE_PiPL_Version {
            2,
            0
        },
        /* [7] */
        AE_Effect_Spec_Version {
            PF_PLUG_IN_VERSION,
            PF_PLUG_IN_SUBVERS
        },
        /* [8] */
        AE_Effect_Version {
            525824 /* 2.0 */
        },
        /* [9] */
        AE_Effect_Info_Flags {
            0
        },
        /* [10] */
        AE_Effect_Global_OutFlags {
            50365504
        },
        AE_Effect_Global_OutFlags_2 {
            37896
        },
        /* [11] */
        AE_Effect_Match_Name {
            "OpenColorIO"
        },
        /* [12] */
        AE_Reserved_Info {
            0
        }
    }
};



