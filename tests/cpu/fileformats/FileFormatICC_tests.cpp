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

        static const std::string iccFileName("icc-test-3.icm");
        OCIO::ContextRcPtr context = OCIO::Context::Create();

        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(BuildOpsTest(ops, iccFileName, context, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(ops.validate());
        OCIO_REQUIRE_EQUAL(4, ops.size());
        OCIO_CHECK_EQUAL("<FileNoOp>",       ops[0]->getInfo());
        OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[1]->getInfo());
        OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[2]->getInfo());
        OCIO_CHECK_EQUAL("<Lut1DOp>",        ops[3]->getInfo());
    
        // No-ops are removed even without any optimizations.
        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));
        OCIO_REQUIRE_EQUAL(3, ops.size());

        std::vector<float> v0 = {1.0f, 0.0f, 0.0f, 0.0f};
        std::vector<float> v1 = {0.0f, 1.0f, 0.0f, 0.0f};
        std::vector<float> v2 = {0.0f, 0.0f, 1.0f, 0.0f};
        std::vector<float> v3 = {0.0f, 0.0f, 0.0f, 1.0f};

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
        constexpr float error = 1e-5f;

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

        static const std::string iccFileName("icc-test-1.icc");
        OCIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OCIO_CHECK_ASSERT(iccFile);
        OCIO_CHECK_ASSERT(!iccFile->lut); // No 1D LUT.

        OCIO_CHECK_EQUAL(0.609741211f, iccFile->mMatrix44[0]);
        OCIO_CHECK_EQUAL(0.205276489f, iccFile->mMatrix44[1]);
        OCIO_CHECK_EQUAL(0.149185181f, iccFile->mMatrix44[2]);
        OCIO_CHECK_EQUAL(0.0f,         iccFile->mMatrix44[3]);

        OCIO_CHECK_EQUAL(0.311111450f,  iccFile->mMatrix44[4]);
        OCIO_CHECK_EQUAL(0.625671387f,  iccFile->mMatrix44[5]);
        OCIO_CHECK_EQUAL(0.0632171631f, iccFile->mMatrix44[6]);
        OCIO_CHECK_EQUAL(0.0f,          iccFile->mMatrix44[7]);

        OCIO_CHECK_EQUAL(0.0194702148f, iccFile->mMatrix44[8]);
        OCIO_CHECK_EQUAL(0.0608673096f, iccFile->mMatrix44[9]);
        OCIO_CHECK_EQUAL(0.744567871f,  iccFile->mMatrix44[10]);
        OCIO_CHECK_EQUAL(0.0f,          iccFile->mMatrix44[11]);

        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[12]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[13]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[14]);
        OCIO_CHECK_EQUAL(1.0f, iccFile->mMatrix44[15]);

        OCIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[0]);
        OCIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[1]);
        OCIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[2]);
        OCIO_CHECK_EQUAL(1.0f,        iccFile->mGammaRGB[3]);
    }

    {
        // This test uses a profile where the TRC is 
        // a parametric curve of type 0 (a single gamma value).

        static const std::string iccFileName("icc-test-2.pf");
        OCIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OCIO_CHECK_ASSERT(iccFile);
        OCIO_CHECK_ASSERT(!iccFile->lut); // No 1D LUT.

        OCIO_CHECK_EQUAL(0.504470825f, iccFile->mMatrix44[0]);
        OCIO_CHECK_EQUAL(0.328125000f, iccFile->mMatrix44[1]);
        OCIO_CHECK_EQUAL(0.131607056f, iccFile->mMatrix44[2]);
        OCIO_CHECK_EQUAL(0.0f,         iccFile->mMatrix44[3]);

        OCIO_CHECK_EQUAL(0.264923096f,  iccFile->mMatrix44[4]);
        OCIO_CHECK_EQUAL(0.682678223f,  iccFile->mMatrix44[5]);
        OCIO_CHECK_EQUAL(0.0523834229f, iccFile->mMatrix44[6]);
        OCIO_CHECK_EQUAL(0.0f,          iccFile->mMatrix44[7]);

        OCIO_CHECK_EQUAL(0.0144805908f, iccFile->mMatrix44[8]);
        OCIO_CHECK_EQUAL(0.0871734619f, iccFile->mMatrix44[9]);
        OCIO_CHECK_EQUAL(0.723556519f,  iccFile->mMatrix44[10]);
        OCIO_CHECK_EQUAL(0.0f,          iccFile->mMatrix44[11]);

        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[12]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[13]);
        OCIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[14]);
        OCIO_CHECK_EQUAL(1.0f, iccFile->mMatrix44[15]);

        OCIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[0]);
        OCIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[1]);
        OCIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[2]);
        OCIO_CHECK_EQUAL(1.0f,        iccFile->mGammaRGB[3]);
    }

    {
        // This test uses profiles where the TRC is a parametric curve of type 1-4.

        static const std::vector<std::string> iccFileNames {
            "icc-test-pc1.icc",
            "icc-test-pc2.icc",
            "icc-test-pc3.icc",
            "icc-test-pc4.icc"
        };

        for (const auto & iccFileName: iccFileNames)
        {
            OCIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

            OCIO_CHECK_ASSERT(iccFile);
            OCIO_REQUIRE_ASSERT(iccFile->lut);

            OCIO_CHECK_EQUAL(iccFile->lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

            const auto & lutArray = iccFile->lut->getArray();
            OCIO_CHECK_EQUAL(1024, lutArray.getLength());
        }
    }
}

OCIO_ADD_TEST(FileFormatICC, test_apply)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    {
        static const std::string iccFileName("icc-test-3.icm");
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(BuildOpsTest(ops, iccFileName, context, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_LOSSLESS));

        // apply ops
        float srcImage[] = {
            -0.1f, 0.0f, 0.3f, 0.0f,
             0.4f, 0.5f, 0.6f, 0.5f,
             0.7f, 1.0f, 1.9f, 1.0f };

        static constexpr float dstImage[] = {
            0.013221f, 0.005287f, 0.069636f, 0.0f,
            0.188847f, 0.204323f, 0.330955f, 0.5f,
            0.722887f, 0.882591f, 1.078655f, 1.0f };

        static constexpr float error = 1e-5f;

        for (const auto & op : ops)
        {
            op->apply(srcImage, 3);
        }

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], dstImage[i], error);
        }

        // Invert the processing.

        OCIO::OpRcPtrVec opsInv;
        OCIO_CHECK_NO_THROW(BuildOpsTest(opsInv, iccFileName, context, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(opsInv.finalize());
        OCIO_CHECK_NO_THROW(opsInv.optimize(OCIO::OPTIMIZATION_LOSSLESS));

        for (const auto & op : opsInv)
        {
            // Note: This apply call hard-codes the FastLogExpPow optimization setting to false.
            op->apply(srcImage, 3);
        }

        // Values outside [0.0, 1.0] are clamped and won't round-trip.
        static constexpr float bckImage[] = {
            0.0f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            0.7f, 1.0f, 1.0f, 1.0f };

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], bckImage[i], error);
        }
    }

    {
        static const std::string iccFileName("icc-test-2.pf");
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(BuildOpsTest(ops, iccFileName, context, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_LOSSLESS));

        // apply ops
        float srcImage[] = {
            -0.1f, 0.0f, 0.3f, 0.0f,
             0.4f, 0.5f, 0.6f, 0.5f,
             0.7f, 1.0f, 1.9f, 1.0f };

        const float dstImage[] = {
            0.012437f, 0.004702f, 0.070333f, 0.0f,
            0.188392f, 0.206965f, 0.343595f, 0.5f,
            0.693246f, 0.863199f, 1.07867f , 1.0f };

        for (const auto & op : ops)
        {
            op->apply(srcImage, 3);
        }

        // Compare results
        const float error = 2e-5f;

        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], dstImage[i], error);
        }

        // Invert the processing.

        OCIO::OpRcPtrVec opsInv;
        OCIO_CHECK_NO_THROW(BuildOpsTest(opsInv, iccFileName, context, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(opsInv.finalize());
        OCIO_CHECK_NO_THROW(opsInv.optimize(OCIO::OPTIMIZATION_LOSSLESS));

        for (const auto & op : opsInv)
        {
            op->apply(srcImage, 3);
        }

        // Values outside [0.0, 1.0] are clamped and won't round-trip.
        const float bckImage[] = {
            0.0f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            0.7f, 1.0f, 1.0f, 1.0f };

        // Compare results
        const float error2 = 2e-4f;

        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(srcImage[i], bckImage[i], error2);
        }
    }

}

// Apply the ICC profile in forward then inverse direction (OCIO inverted interpretation)
// and compare against expected values at each steps. Expects RGBA pixel layout.
void ValidateRoundtripProfile(const std::string & iccFileName,
                              unsigned int numPixels,
                              float * srcImage,
                              const float * dstImage,
                              const float * bckImage,
                              unsigned lineNo,
                              int fwd_op_idx=-1,
                              int bck_op_idx=-1,
                              float error=2e-5f,
                              float error_bck=2e-5f)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();

    // PCS to Device direction
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, iccFileName, context, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_LOSSLESS));

    // Apply ops
    if (fwd_op_idx >= 0)
    {
        ops[fwd_op_idx]->apply(srcImage, numPixels);
    }
    else
    {
        for (const auto & op : ops)
        {
            op->apply(srcImage, numPixels);
        }
    }

    // Compare results
    for (unsigned int i = 0; i < numPixels * 4; ++i)
    {
        OCIO_CHECK_CLOSE(srcImage[i], dstImage[i], error);
    }

    // Invert the processing.

    // Device to PCS direction
    OCIO::OpRcPtrVec opsInv;
    OCIO_CHECK_NO_THROW(BuildOpsTest(opsInv, iccFileName, context, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(opsInv.finalize());
    OCIO_CHECK_NO_THROW(opsInv.optimize(OCIO::OPTIMIZATION_LOSSLESS));

    // apply ops
    if (bck_op_idx >= 0)
    {
        opsInv[bck_op_idx]->apply(srcImage, numPixels);
    }
    else
    {
        for (const auto & op : opsInv)
        {
            op->apply(srcImage, numPixels);
        }
    }

    // Compare results
    for (unsigned int i = 0; i < numPixels * 4; ++i)
    {
        OCIO_CHECK_CLOSE_FROM(srcImage[i], bckImage[i], error_bck, lineNo);
    }
}

OCIO_ADD_TEST(FileFormatICC, test_apply_para_t1)
{
    // Check processing of ParaCurve type 1.
    // g = 2.4, a = 1.1, b = -0.1
    {
        static const std::string iccFileName("icc-test-pc1.icc");

        float srcImage[] = {
           -1.0f,  -1.0f,  -1.0f,  1.0f,
            0.0f,   0.0f,   0.0f,  1.0f,
            0.02f,  0.02f,  0.02f, 1.0f,
            0.18f,  0.18f,  0.18f, 1.0f,
            0.5f,   0.5f,   0.5f,  1.0f,
            0.75f,  0.75f,  0.75f, 1.0f,
            1.0f,   1.0f,   1.0f,  1.0f,
            2.0f,   2.0f,   2.0f,  1.0f };

        const float dstImage[] = {
            0.09090909f, 0.09090909f, 0.09090909f, 1.0f,
            0.09090909f, 0.09090909f, 0.09090909f, 1.0f,
            0.26902518f, 0.26902518f, 0.26902518f, 1.0f,
            0.53586119f, 0.53586119f, 0.53586119f, 1.0f,
            0.77196938f, 0.77196938f, 0.77196938f, 1.0f,
            0.89732236f, 0.89732236f, 0.89732236f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f };

        // Negative and values above 1.0 are clamped by the LUT and won't round-trip.
        const float bckImage[] = {
            0.0f,  0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  0.0f,  1.0f,
            0.02f, 0.02f, 0.02f, 1.0f,
            0.18f, 0.18f, 0.18f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f,
            0.75f, 0.75f, 0.75f, 1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f };

        ValidateRoundtripProfile(iccFileName, 7, srcImage, dstImage, bckImage, __LINE__, 1, 0);
    }
}

OCIO_ADD_TEST(FileFormatICC, test_apply_para_t2)
{
    // Check processing of ParaCurve type 2.
    // g = 2.4, a = 1.057, b = -0.1, c = 0.1
    {
        static const std::string iccFileName("icc-test-pc2.icc");

        float srcImage[] = {
           -1.0f,  -1.0f,  -1.0f,  1.0f,
            0.0f,   0.0f,   0.0f,  1.0f,
            0.02f,  0.02f,  0.02f, 1.0f,
            0.18f,  0.18f,  0.18f, 1.0f,
            0.5f,   0.5f,   0.5f,  1.0f,
            0.75f,  0.75f,  0.75f, 1.0f,
            1.0f,   1.0f,   1.0f,  1.0f,
            2.0f,   2.0f,   2.0f,  1.0f };

        const float dstImage[] = {
            0.09481915f, 0.09481915f, 0.09481915f, 1.0f,
            0.09481915f, 0.09481915f, 0.09481915f, 1.0f,
            0.09481915f, 0.09481915f, 0.09481915f, 1.0f,
            0.42486829f, 0.42486829f, 0.42486829f, 1.0f,
            0.74041277f, 0.74041277f, 0.74041277f, 1.0f,
            0.88520885f, 0.88520885f, 0.88520885f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f };

        // Values below the curve flat segment and above 1.0 are clamped by the LUT and won't round-trip.
        const float bckImage[] = {
            0.1f,  0.1f,  0.1f,  1.0f,
            0.1f,  0.1f,  0.1f,  1.0f,
            0.1f,  0.1f,  0.1f,  1.0f,
            0.18f, 0.18f, 0.18f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f,
            0.75f, 0.75f, 0.75f, 1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f };

        ValidateRoundtripProfile(iccFileName, 7, srcImage, dstImage, bckImage, __LINE__, 1, 0);
    }
}

OCIO_ADD_TEST(FileFormatICC, test_apply_para_t3)
{
    // Check processing of ParaCurve type 3.
    // g = 2.4, a = 1/1.055, b = 0.055/1.055, c = 1/12.92, d = 0.04045
    {
        static const std::string iccFileName("icc-test-pc3.icc");

        float srcImage[] = {
           -1.0f,  -1.0f,  -1.0f,  1.0f,
            0.0f,   0.0f,   0.0f,  1.0f,
            0.02f,  0.02f,  0.02f, 1.0f,
            0.18f,  0.18f,  0.18f, 1.0f,
            0.5f,   0.5f,   0.5f,  1.0f,
            0.75f,  0.75f,  0.75f, 1.0f,
            1.0f,   1.0f,   1.0f,  1.0f,
            2.0f,   2.0f,   2.0f,  1.0f };

        const float dstImage[] = {
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
            0.15170372f, 0.15170372f, 0.15170372f, 1.0f,
            0.46136194f, 0.46136194f, 0.46136194f, 1.0f,
            0.73536557f, 0.73536557f, 0.73536557f, 1.0f,
            0.88083965f, 0.88083965f, 0.88083965f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f };

        // Negative and values above 1.0 are clamped by the LUT and won't round-trip.
        const float bckImage[] = {
            0.0f,  0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  0.0f,  1.0f,
            0.02f, 0.02f, 0.02f, 1.0f,
            0.18f, 0.18f, 0.18f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f,
            0.75f, 0.75f, 0.75f, 1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f };

        ValidateRoundtripProfile(iccFileName, 7, srcImage, dstImage, bckImage, __LINE__, 1, 0);
    }
}

OCIO_ADD_TEST(FileFormatICC, test_apply_para_t4)
{
    // Check processing of ParaCurve type 4.
    // g = 2.4, a = 0.905, b = 0.052, c = 0.073, d = 0.04, e = 0.1, f = 0.1
    {
        static const std::string iccFileName("icc-test-pc4.icc");

        float srcImage[] = {
           -1.0f,  -1.0f,  -1.0f,  1.0f,
            0.0f,   0.0f,   0.0f,  1.0f,
            0.02f,  0.02f,  0.02f, 1.0f,
            0.18f,  0.18f,  0.18f, 1.0f,
            0.5f,   0.5f,   0.5f,  1.0f,
            0.75f,  0.75f,  0.75f, 1.0f,
            1.0f,   1.0f,   1.0f,  1.0f,
            2.0f,   2.0f,   2.0f,  1.0f };

        const float dstImage[] = {
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
            0.32816601f, 0.32816601f, 0.32816601f, 1.0f,
            0.69675589f, 0.69675589f, 0.69675589f, 1.0f,
            0.86589807f, 0.86589807f, 0.86589807f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f };

        // Values below the forward minimum and above 1.0 are clamped by the LUT and won't round-trip.
        const float bckImage[] = {
            0.1f,  0.1f,  0.1f,  1.0f,
            0.1f,  0.1f,  0.1f,  1.0f,
            0.1f,  0.1f,  0.1f,  1.0f,
            0.18f, 0.18f, 0.18f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f,
            0.75f, 0.75f, 0.75f, 1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f };

        ValidateRoundtripProfile(iccFileName, 7, srcImage, dstImage, bckImage, __LINE__, 1, 0, 4e-5f, 4e-5f);
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
