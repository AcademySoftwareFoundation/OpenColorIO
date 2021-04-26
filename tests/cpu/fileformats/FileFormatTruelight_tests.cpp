// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatTruelight.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatTruelight, shaper_and_lut_3d)
{
    // This lowers the red channel by 0.5, other channels are unaffected.
    const char * luttext = "# Truelight Cube v2.0\n"
       "# iDims 3\n"
       "# oDims 3\n"
       "# width 3 3 3\n"
       "# lutLength 5\n"
       "# InputLUT\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.500000 0.500000 0.500000\n"
       " 1.000000 1.000000 1.000000\n"
       " 1.500000 1.500000 1.500000\n"
       " 2.000000 2.000000 2.000000\n"
       "\n"
       "# Cube\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.250000 0.000000 0.000000\n"
       " 0.500000 0.000000 0.000000\n"
       " 0.000000 0.500000 0.000000\n"
       " 0.250000 0.500000 0.000000\n"
       " 0.500000 0.500000 0.000000\n"
       " 0.000000 1.000000 0.000000\n"
       " 0.250000 1.000000 0.000000\n"
       " 0.500000 1.000000 0.000000\n"
       " 0.000000 0.000000 0.500000\n"
       " 0.250000 0.000000 0.500000\n"
       " 0.500000 0.000000 0.500000\n"
       " 0.000000 0.500000 0.500000\n"
       " 0.250000 0.500000 0.500000\n"
       " 0.500000 0.500000 0.500000\n"
       " 0.000000 1.000000 0.500000\n"
       " 0.250000 1.000000 0.500000\n"
       " 0.500000 1.000000 0.500000\n"
       " 0.000000 0.000000 1.000000\n"
       " 0.250000 0.000000 1.000000\n"
       " 0.500000 0.000000 1.000000\n"
       " 0.000000 0.500000 1.000000\n"
       " 0.250000 0.500000 1.000000\n"
       " 0.500000 0.500000 1.000000\n"
       " 0.000000 1.000000 1.000000\n"
       " 0.250000 1.000000 1.000000\n"
       " 0.500000 1.000000 1.000000\n"
       "\n"
       "# end\n"
       "\n"
       "# Truelight profile\n"
       "title{madeup on some display}\n"
       "print{someprint}\n"
       "display{some}\n"
       "cubeFile{madeup.cube}\n"
       "\n"
       " # This last line confirms 'end' tag is obeyed\n"
       " 1.23456 1.23456 1.23456\n";

    std::istringstream lutIStream;
    lutIStream.str(luttext);

    // Read file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = tester.read(lutIStream, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);

    OCIO_REQUIRE_ASSERT(lut);

    OCIO_REQUIRE_ASSERT(lut->lut1D);
    OCIO_REQUIRE_ASSERT(lut->lut3D);
    OCIO_CHECK_EQUAL(lut->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(lut->lut3D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };

    float result[4*3] = { 0.05f, 0.2f, 0.3f, 0.0f,
                          0.50f, 0.5f, 0.123456f, 0.0f,
                          0.0f, 1.0f, 0.5f, 0.0f };

    OCIO::OpRcPtrVec ops;
    if(lut->lut1D)
    {
        CreateLut1DOp(ops, lut->lut1D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->lut3D)
    {
        CreateLut3DOp(ops, lut->lut3D, OCIO::TRANSFORM_DIR_FORWARD);
    }

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }

    for(int i=0; i<4*3; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}

OCIO_ADD_TEST(FileFormatTruelight, shaper)
{
    const char * luttext = "# Truelight Cube v2.0\n"
       "# lutLength 11\n"
       "# iDims 3\n"
       "\n"
       "\n"
       "# InputLUT\n"
       " 0.000 0.000 -0.000\n"
       " 0.200 0.010 -0.100\n"
       " 0.400 0.040 -0.200\n"
       " 0.600 0.090 -0.300\n"
       " 0.800 0.160 -0.400\n"
       " 1.000 0.250 -0.500\n"
       " 1.200 0.360 -0.600\n"
       " 1.400 0.490 -0.700\n"
       " 1.600 0.640 -0.800\n"
       " 1.800 0.820 -0.900\n"
       " 2.000 1.000 -1.000\n"
       "\n\n\n"
       "# end\n";

    std::istringstream lutIStream;
    lutIStream.str(luttext);

    // Read file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = tester.read(lutIStream, emptyString, OCIO::INTERP_DEFAULT));

    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);

    OCIO_CHECK_ASSERT(lut->lut1D);
    OCIO_CHECK_ASSERT(!lut->lut3D);

    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };

    float result[4*3] = { 0.2f, 0.04f, -0.3f, 0.0f,
                          2.0f, 0.25f, -0.123456f, 0.0f,
                          0.0f, 1.0f, -0.5f, 0.0f };

    OCIO::OpRcPtrVec ops;
    if(lut->lut1D)
    {
        CreateLut1DOp(ops, lut->lut1D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->lut3D)
    {
        CreateLut3DOp(ops, lut->lut3D, OCIO::TRANSFORM_DIR_FORWARD);
    }

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }

    for(int i=0; i<4*3; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}


OCIO_ADD_TEST(FileFormatTruelight, lut_3d)
{
    // This lowers the red channel by 0.5, other channels are unaffected.
    const char * luttext = "# Truelight Cube v2.0\n"
       "# iDims 3\n"
       "# oDims 3\n"
       "# width 3 3 3\n"
       "\n\n\n"
       "# Cube\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.250000 0.000000 0.000000\n"
       " 0.500000 0.000000 0.000000\n"
       " 0.000000 0.500000 0.000000\n"
       " 0.250000 0.500000 0.000000\n"
       " 0.500000 0.500000 0.000000\n"
       " 0.000000 1.000000 0.000000\n"
       " 0.250000 1.000000 0.000000\n"
       " 0.500000 1.000000 0.000000\n"
       " 0.000000 0.000000 0.500000\n"
       " 0.250000 0.000000 0.500000\n"
       " 0.500000 0.000000 0.500000\n"
       " 0.000000 0.500000 0.500000\n"
       " 0.250000 0.500000 0.500000\n"
       " 0.500000 0.500000 0.500000\n"
       " 0.000000 1.000000 0.500000\n"
       " 0.250000 1.000000 0.500000\n"
       " 0.500000 1.000000 0.500000\n"
       " 0.000000 0.000000 1.000000\n"
       " 0.250000 0.000000 1.000000\n"
       " 0.500000 0.000000 1.000000\n"
       " 0.000000 0.500000 1.000000\n"
       " 0.250000 0.500000 1.000000\n"
       " 0.500000 0.500000 1.000000\n"
       " 0.000000 1.000000 1.000000\n"
       " 0.250000 1.000000 1.000000\n"
       " 0.500000 1.000000 1.000000\n"
       "\n"
       "# end\n";

    std::istringstream lutIStream;
    lutIStream.str(luttext);

    // Read file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = tester.read(lutIStream, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);

    OCIO_CHECK_ASSERT(!lut->lut1D);
    OCIO_CHECK_ASSERT(lut->lut3D);

    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };

    float result[4*3] = { 0.05f, 0.2f, 0.3f, 0.0f,
                          0.50f, 0.5f, 0.123456f, 0.0f,
                          0.0f, 1.0f, 0.5f, 0.0f };

    OCIO::OpRcPtrVec ops;
    if(lut->lut1D)
    {
        CreateLut1DOp(ops, lut->lut1D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->lut3D)
    {
        CreateLut3DOp(ops, lut->lut3D, OCIO::TRANSFORM_DIR_FORWARD);
    }

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }

    for(int i=0; i<4*3; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}

