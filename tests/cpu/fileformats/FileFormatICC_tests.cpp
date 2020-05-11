// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatICC.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatICC, types)
{
    // Test types used
    OCIO_CHECK_EQUAL(1, sizeof(icUInt8Number));
    OCIO_CHECK_EQUAL(2, sizeof(icUInt16Number));
    OCIO_CHECK_EQUAL(4, sizeof(icUInt32Number));

    OCIO_CHECK_EQUAL(4, sizeof(icInt32Number));

    OCIO_CHECK_EQUAL(4, sizeof(icS15Fixed16Number));
}

OCIO::LocalCachedFileRcPtr LoadICCFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::binary);
}

OCIO_ADD_TEST(FileFormatICC, test_file)
{
    OCIO::LocalCachedFileRcPtr iccFile;
    {
        // This example uses a profile with a 1024-entry LUT for the TRC.
        const std::string iccFileName("icc-test-3.icm");
        OCIO::OpRcPtrVec ops;
        OCIO::ContextRcPtr context = OCIO::Context::Create();
        OCIO_CHECK_NO_THROW(BuildOpsTest(ops, iccFileName, context,
                                         OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(4, ops.size());
        OCIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
        OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[1]->getInfo());
        OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[2]->getInfo());
        OCIO_CHECK_EQUAL("<Lut1DOp>", ops[3]->getInfo());
        OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));
        // No-ops are removed.
        OCIO_REQUIRE_EQUAL(3, ops.size());

        std::vector<float> v0(4, 0.0f);
        v0[0] = 1.0f;
        std::vector<float> v1(4, 0.0f);
        v1[1] = 1.0f;
        std::vector<float> v2(4, 0.0f);
        v2[2] = 1.0f;
        std::vector<float> v3(4, 0.0f);
        v3[3] = 1.0f;

        std::vector<float> tmp = v0;
        ops[0]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(1.04788959f, tmp[0]);
        OCIO_CHECK_EQUAL(0.0295844227f, tmp[1]);
        OCIO_CHECK_EQUAL(-0.00925218873f, tmp[2]);
        OCIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v1;
        ops[0]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(0.0229206420f, tmp[0]);
        OCIO_CHECK_EQUAL(0.990481913f, tmp[1]);
        OCIO_CHECK_EQUAL(0.0150730424f, tmp[2]);
        OCIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v2;
        ops[0]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(-0.0502183065f, tmp[0]);
        OCIO_CHECK_EQUAL(-0.0170795303f, tmp[1]);
        OCIO_CHECK_EQUAL(0.751668930f, tmp[2]);
        OCIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v3;
        ops[0]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(0.0f, tmp[0]);
        OCIO_CHECK_EQUAL(0.0f, tmp[1]);
        OCIO_CHECK_EQUAL(0.0f, tmp[2]);
        OCIO_CHECK_EQUAL(1.0f, tmp[3]);

        tmp = v0;
        ops[1]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(3.13411215332385f, tmp[0]);
        OCIO_CHECK_EQUAL(-0.978787296139183f, tmp[1]);
        OCIO_CHECK_EQUAL(0.0719830443856949f, tmp[2]);
        OCIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v1;
        ops[1]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(-1.61739245955187f, tmp[0]);
        OCIO_CHECK_EQUAL(1.91627958642662f, tmp[1]);
        OCIO_CHECK_EQUAL(-0.228985850247545f, tmp[2]);
        OCIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v2;
        ops[1]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(-0.49063340456472f, tmp[0]);
        OCIO_CHECK_EQUAL(0.033454714231382f, tmp[1]);
        OCIO_CHECK_EQUAL(1.4053851315845f, tmp[2]);
        OCIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v3;
        ops[1]->apply(&tmp[0], 1);
        OCIO_CHECK_EQUAL(0.0f, tmp[0]);
        OCIO_CHECK_EQUAL(0.0f, tmp[1]);
        OCIO_CHECK_EQUAL(0.0f, tmp[2]);
        OCIO_CHECK_EQUAL(1.0f, tmp[3]);

        // Knowing the LUT has 1024 elements
        // and is inverted, verify values for a given index
        // are converted to index * step
        const float error = 1e-5f;

        // value at index 200
        tmp[0] = 0.0317235067f;
        tmp[1] = 0.0317235067f;
        tmp[2] = 0.0317235067f;
        ops[2]->apply(&tmp[0], 1);
        OCIO_CHECK_CLOSE(200.0f / 1023.0f, tmp[0], error);
        OCIO_CHECK_CLOSE(200.0f / 1023.0f, tmp[1], error);
        OCIO_CHECK_CLOSE(200.0f / 1023.0f, tmp[2], error);


        // Get cached file to access LUT size
        OCIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OCIO_REQUIRE_ASSERT(iccFile);
        OCIO_REQUIRE_ASSERT(iccFile->lut);

        OCIO_CHECK_EQUAL(iccFile->lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);

        const auto & lutArray = iccFile->lut->getArray();
        OCIO_CHECK_EQUAL(1024, lutArray.getLength());

        OCIO_CHECK_EQUAL(0.0317235067f, lutArray[200 * 3 + 0]);
        OCIO_CHECK_EQUAL(0.0317235067f, lutArray[200 * 3 + 1]);
        OCIO_CHECK_EQUAL(0.0317235067f, lutArray[200 * 3 + 2]);
    }

    {
        // This test uses a profile where the TRC is a 1-entry curve,
        // to be interpreted as a gamma value.
        const std::string iccFileName("icc-test-1.icc");
        OCIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OCIO_CHECK_ASSERT((bool)iccFile);
        OCIO_CHECK_ASSERT(!(bool)(iccFile->lut));

        OCIO_CHECK_EQUAL(0.609741211f, iccFile->mMatrix44[0]);
        OCIO_CHECK_EQUAL(0.205276489f, iccFile->mMatrix44[1]);
        OCIO_CHECK_EQUAL(0.149185181f, iccFile->mMatrix44[2]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[3]);

        OCIO_CHECK_EQUAL(0.311111450f, iccFile->mMatrix44[4]);
        OCIO_CHECK_EQUAL(0.625671387f, iccFile->mMatrix44[5]);
        OCIO_CHECK_EQUAL(0.0632171631f, iccFile->mMatrix44[6]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[7]);

        OCIO_CHECK_EQUAL(0.0194702148f, iccFile->mMatrix44[8]);
        OCIO_CHECK_EQUAL(0.0608673096f, iccFile->mMatrix44[9]);
        OCIO_CHECK_EQUAL(0.744567871f, iccFile->mMatrix44[10]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[11]);

        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[12]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[13]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[14]);
        OCIO_CHECK_EQUAL(1.0f, iccFile->mMatrix44[15]);

        OCIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[0]);
        OCIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[1]);
        OCIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[2]);
        OCIO_CHECK_EQUAL(1.0f, iccFile->mGammaRGB[3]);
    }

    {
        // This test uses a profile where the TRC is 
        // a parametric curve of type 0 (a single gamma value).
        const std::string iccFileName("icc-test-2.pf");
        OCIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OCIO_CHECK_ASSERT((bool)iccFile);
        OCIO_CHECK_ASSERT(!(bool)(iccFile->lut));

        OCIO_CHECK_EQUAL(0.504470825f, iccFile->mMatrix44[0]);
        OCIO_CHECK_EQUAL(0.328125000f, iccFile->mMatrix44[1]);
        OCIO_CHECK_EQUAL(0.131607056f, iccFile->mMatrix44[2]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[3]);

        OCIO_CHECK_EQUAL(0.264923096f, iccFile->mMatrix44[4]);
        OCIO_CHECK_EQUAL(0.682678223f, iccFile->mMatrix44[5]);
        OCIO_CHECK_EQUAL(0.0523834229f, iccFile->mMatrix44[6]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[7]);

        OCIO_CHECK_EQUAL(0.0144805908f, iccFile->mMatrix44[8]);
        OCIO_CHECK_EQUAL(0.0871734619f, iccFile->mMatrix44[9]);
        OCIO_CHECK_EQUAL(0.723556519f, iccFile->mMatrix44[10]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[11]);

        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[12]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[13]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[14]);
        OCIO_CHECK_EQUAL(1.0f, iccFile->mMatrix44[15]);

        OCIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[0]);
        OCIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[1]);
        OCIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[2]);
        OCIO_CHECK_EQUAL(1.0f, iccFile->mGammaRGB[3]);
    }
}

OCIO_ADD_TEST(FileFormatICC, test_apply)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    {
        const std::string iccFileName("icc-test-3.icm");
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(BuildOpsTest(ops, iccFileName, context,
                                         OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_DEFAULT));

        // apply ops
        float srcImage[] = {
            -0.1f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            0.7f, 1.0f, 1.9f, 1.0f };

        const float dstImage[] = {
            0.013221f, 0.005287f, 0.069636f, 0.0f,
            0.188847f, 0.204323f, 0.330955f, 0.5f,
            0.722887f, 0.882591f, 1.078655f, 1.0f };
        const float error = 1e-5f;

        OCIO::OpRcPtrVec::size_type numOps = ops.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            ops[i]->apply(srcImage, 3);
        }

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], dstImage[i], error);
        }

        // inverse
        OCIO::OpRcPtrVec opsInv;
        OCIO_CHECK_NO_THROW(BuildOpsTest(opsInv, iccFileName, context,
                                         OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(opsInv.finalize(OCIO::OPTIMIZATION_DEFAULT));

        numOps = opsInv.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            opsInv[i]->apply(srcImage, 3);
        }

        const float bckImage[] = {
            // neg values are clamped by the LUT and won't round-trip
            0.0f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            // >1 values are clamped by the LUT and won't round-trip
            0.7f, 1.0f, 1.0f, 1.0f };

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], bckImage[i], error);
        }
    }

    {
        const std::string iccFileName("icc-test-2.pf");
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(BuildOpsTest(ops, iccFileName, context,
                                         OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_DEFAULT));

        // apply ops
        float srcImage[] = {
            -0.1f, 0.0f, 0.3f, 0.0f,
             0.4f, 0.5f, 0.6f, 0.5f,
             0.7f, 1.0f, 1.9f, 1.0f };

        const float dstImage[] = {
            0.012437f, 0.004702f, 0.070333f, 0.0f,
            0.188392f, 0.206965f, 0.343595f, 0.5f,

// Gamma SSE vs. not SEE implementations explain the differences.
#ifdef USE_SSE
            1.210458f, 1.058771f, 4.003655f, 1.0f };
#else
            1.210462f, 1.058761f, 4.003706f, 1.0f };
#endif

        OCIO::OpRcPtrVec::size_type numOps = ops.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            ops[i]->apply(srcImage, 3);
        }

        // Compare results
        const float error = 2e-5f;

        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], dstImage[i], error);
        }

        // inverse
        OCIO::OpRcPtrVec opsInv;
        OCIO_CHECK_NO_THROW(BuildOpsTest(opsInv, iccFileName, context,
                                         OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(opsInv.finalize(OCIO::OPTIMIZATION_DEFAULT));

        numOps = opsInv.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            opsInv[i]->apply(srcImage, 3);
        }

        // Negative values are clamped by the LUT and won't round-trip.
        const float bckImage[] = {
            0.0f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            0.7f, 1.0f, 1.9f, 1.0f };

        // Compare results
        const float error2 = 2e-4f;

        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], bckImage[i], error2);
        }
    }

}

OCIO_ADD_TEST(FileFormatICC, endian)
{
    unsigned char test[8];
    test[0] = 0x11;
    test[1] = 0x22;
    test[2] = 0x33;
    test[3] = 0x44;
    test[4] = 0x55;
    test[5] = 0x66;
    test[6] = 0x77;
    test[7] = 0x88;

    SampleICC::Swap32Array((void*)test, 2);

    OCIO_CHECK_EQUAL(test[0], 0x44);
    OCIO_CHECK_EQUAL(test[1], 0x33);
    OCIO_CHECK_EQUAL(test[2], 0x22);
    OCIO_CHECK_EQUAL(test[3], 0x11);

    OCIO_CHECK_EQUAL(test[4], 0x88);
    OCIO_CHECK_EQUAL(test[5], 0x77);
    OCIO_CHECK_EQUAL(test[6], 0x66);
    OCIO_CHECK_EQUAL(test[7], 0x55);

    SampleICC::Swap16Array((void*)test, 4);

    OCIO_CHECK_EQUAL(test[0], 0x33);
    OCIO_CHECK_EQUAL(test[1], 0x44);

    OCIO_CHECK_EQUAL(test[2], 0x11);
    OCIO_CHECK_EQUAL(test[3], 0x22);

    OCIO_CHECK_EQUAL(test[4], 0x77);
    OCIO_CHECK_EQUAL(test[5], 0x88);

    OCIO_CHECK_EQUAL(test[6], 0x55);
    OCIO_CHECK_EQUAL(test[7], 0x66);

    SampleICC::Swap64Array((void*)test, 1);

    OCIO_CHECK_EQUAL(test[7], 0x33);
    OCIO_CHECK_EQUAL(test[6], 0x44);
    OCIO_CHECK_EQUAL(test[5], 0x11);
    OCIO_CHECK_EQUAL(test[4], 0x22);
    OCIO_CHECK_EQUAL(test[3], 0x77);
    OCIO_CHECK_EQUAL(test[2], 0x88);
    OCIO_CHECK_EQUAL(test[1], 0x55);
    OCIO_CHECK_EQUAL(test[0], 0x66);

}

