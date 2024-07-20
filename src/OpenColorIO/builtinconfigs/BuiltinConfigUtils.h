// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_BUILTIN_CONFIG_UTILS_H
#define INCLUDED_OCIO_BUILTIN_CONFIG_UTILS_H

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{
    namespace BUILTIN_CONFIG_UTILS
    {
        void AddColorSpace(ConfigRcPtr& cfg
            , const char* name
            , ReferenceSpaceType type
            , const char** aliases
            , BitDepth bitDepth
            , const char** categories
            , const char* encoding
            , const char* eqGroup
            , const char* family
            , bool isData
            , ConstTransformRcPtr trFrom
            , ConstTransformRcPtr trTo
            , const char* desc);

        void AddNamedTramsform(ConfigRcPtr& cfg
            , const char* name
            , const char** aliases
            , const char** categories
            , const char* encoding
            , const char* family
            , ConstTransformRcPtr trFwd
            , ConstTransformRcPtr trInv
            , const char* desc);
    }
}

#endif