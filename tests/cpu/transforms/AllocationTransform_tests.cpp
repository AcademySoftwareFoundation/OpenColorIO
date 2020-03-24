// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/AllocationTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(AllocationTransform, allocation)
{
    OCIO::AllocationTransformRcPtr al = OCIO::AllocationTransform::Create();

    al->setAllocation(OCIO::ALLOCATION_UNIFORM);
    OCIO_CHECK_NO_THROW(al->validate());

    std::vector<float> envs(2, 0.0f);
    al->setVars(static_cast<int>(envs.size()), &envs[0]);
    OCIO_CHECK_NO_THROW(al->validate());

    envs.push_back(0.01f);
    al->setVars(static_cast<int>(envs.size()), &envs[0]);
    OCIO_CHECK_THROW(al->validate(), OCIO::Exception);

    al->setAllocation(OCIO::ALLOCATION_LG2);
    OCIO_CHECK_NO_THROW(al->validate());

    envs.push_back(0.1f);
    al->setVars(static_cast<int>(envs.size()), &envs[0]);
    OCIO_CHECK_THROW(al->validate(), OCIO::Exception);

    al->setVars(0, 0x0);
    OCIO_CHECK_NO_THROW(al->validate());
}

