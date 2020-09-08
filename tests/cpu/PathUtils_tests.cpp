// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "PathUtils.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(PathUtils, compute_hash)
{
    const std::string file1 = OCIO::GetTestFilesDir() + "/lut1d_4.spi1d";
    const std::string file2 = OCIO::GetTestFilesDir() + "/lut1d_5.spi1d";

    OCIO_CHECK_EQUAL(OCIO::ComputeHash(file1), OCIO::ComputeHash(file1));

    OCIO_CHECK_NE(OCIO::ComputeHash(file1), OCIO::ComputeHash(file2));
}
