// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PathUtils.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

// A custom compute hash function for testing.
std::string CustomComputeHash(const std::string & filename)
{
    std::ostringstream testHasher;
    testHasher << std::hash<std::string>{}(filename + "this is custom hash");
    return testHasher.str();
}

} //anon.

// A class for testing the working of custom callback for compute hash.
class ComputeHashGuard
{

public:

    ComputeHashGuard();

    ComputeHashGuard(ComputeHashGuard &) = delete;

    ~ComputeHashGuard();

};

ComputeHashGuard::ComputeHashGuard()
{
    SetComputeHashFunction(CustomComputeHash);
}

ComputeHashGuard::~ComputeHashGuard()
{
    ResetComputeHashFunction();
}

}

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(PathUtils, compute_hash)
{

    const std::string file1 = OCIO::GetTestFilesDir() + "/lut1d_4.spi1d";
    const std::string file2 = OCIO::GetTestFilesDir() + "/lut1d_5.spi1d";

    OCIO_CHECK_EQUAL(OCIO::g_hashFunction(file1), OCIO::g_hashFunction(file1));

    OCIO_CHECK_NE(OCIO::g_hashFunction(file1), OCIO::g_hashFunction(file2));

    const std::string result1 = OCIO::g_hashFunction(file1);
    const std::string result2 = OCIO::g_hashFunction(file2);
    std::string result3, result4;

    {

        OCIO::ComputeHashGuard tester;

        OCIO_CHECK_NE(OCIO::g_hashFunction(file1), result1);

        OCIO_CHECK_NE(OCIO::g_hashFunction(file2), result2);

        OCIO_CHECK_EQUAL(OCIO::g_hashFunction(file1), OCIO::g_hashFunction(file1));

        result3 = OCIO::g_hashFunction(file1);

        result4 = OCIO::g_hashFunction(file2);

    }

    OCIO_CHECK_NE(result3, OCIO::g_hashFunction(file1));

    OCIO_CHECK_NE(result4, OCIO::g_hashFunction(file2));

}
