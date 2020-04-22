// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "UnitTestOptimFlags.h"

#include "Platform.h"

namespace OCIO = OCIO_NAMESPACE;

OCIOOptimizationFlagsEnvGuard::OCIOOptimizationFlagsEnvGuard(const char * newValue)
{
    std::string newString{ newValue ? newValue : "" };
    m_optimizationFlagEnv = OCIO::GetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR);
    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, newString.c_str());
}

OCIOOptimizationFlagsEnvGuard::~OCIOOptimizationFlagsEnvGuard()
{
    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, m_optimizationFlagEnv.c_str());
}
