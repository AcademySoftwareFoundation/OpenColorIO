// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_COLORSPACE_UTILS_H
#define INCLUDED_OCIO_COLORSPACE_UTILS_H

#include <vector>
#include <string>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{
ConstProcessorRcPtr getProcessorToOrFromBuiltinColorSpace(ConstConfigRcPtr srcConfig,
                                                    const char * srcColorSpaceName, 
                                                    const char * builtinColorSpaceName,
                                                    TransformDirection direction);
}

#endif