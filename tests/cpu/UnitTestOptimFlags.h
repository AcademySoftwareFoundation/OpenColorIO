// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <string>

class OCIOOptimizationFlagsEnvGuard
{
public:
    OCIOOptimizationFlagsEnvGuard() = delete;
    OCIOOptimizationFlagsEnvGuard(const char * value);
    ~OCIOOptimizationFlagsEnvGuard();

private:
    std::string m_optimizationFlagEnv{ "" };
};

