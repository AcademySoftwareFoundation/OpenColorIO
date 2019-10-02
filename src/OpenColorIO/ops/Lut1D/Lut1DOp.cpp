// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "HashUtils.h"
#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut1D/Lut1DOpCPU.h"
#include "ops/Lut1D/Lut1DOpGPU.h"
#include "ops/Matrix/MatrixOps.h"
#include "OpTools.h"
#include "SSE.h"

OCIO_NAMESPACE_ENTER
{

    namespace
    {
        class Lut1DOp;
        typedef OCIO_SHARED_PTR<Lut1DOp> Lut1DOpRcPtr;
        typedef OCIO_SHARED_PTR<const Lut1DOp> ConstLut1DOpRcPtr;

        class Lut1DOp : public Op
        {
        public:
            Lut1DOp() = delete;
            Lut1DOp(const Lut1DOp &) = delete;
            explicit Lut1DOp(Lut1DOpDataRcPtr & lutData);
            virtual ~Lut1DOp();

            TransformDirection getDirection() const noexcept override { return lut1DData()->getDirection(); }

            OpRcPtr clone() const override;

            std::string getInfo() const override;

            bool isSameType(ConstOpRcPtr & op) const override;
            bool isInverse(ConstOpRcPtr & op) const override;
            bool hasChannelCrosstalk() const override;
            void finalize(FinalizationFlags fFlags) override;

            ConstOpCPURcPtr getCPUOp() const override;

            bool supportedByLegacyShader() const override { return false; }
            void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

            ConstLut1DOpDataRcPtr lut1DData() const { return DynamicPtrCast<const Lut1DOpData>(data()); }
            Lut1DOpDataRcPtr lut1DData() { return DynamicPtrCast<Lut1DOpData>(data()); }
        };

        Lut1DOp::Lut1DOp(Lut1DOpDataRcPtr & lut1D)
        {
            data() = lut1D;
        }

        Lut1DOp::~Lut1DOp()
        {
        }

        OpRcPtr Lut1DOp::clone() const
        {
            Lut1DOpDataRcPtr lut = lut1DData()->clone();
            return std::make_shared<Lut1DOp>(lut);
        }

        std::string Lut1DOp::getInfo() const
        {
            return "<Lut1DOp>";
        }

        bool Lut1DOp::isSameType(ConstOpRcPtr & op) const
        {
            ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(op);
            return (bool)typedRcPtr;
        }

        bool Lut1DOp::isInverse(ConstOpRcPtr & op) const
        {
            ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(op);
            if (typedRcPtr)
            {
                ConstLut1DOpDataRcPtr lutData = typedRcPtr->lut1DData();
                return lut1DData()->isInverse(lutData);
            }

            return false;
        }

        bool Lut1DOp::hasChannelCrosstalk() const
        {
            return lut1DData()->hasChannelCrosstalk();
        }

        void Lut1DOp::finalize(FinalizationFlags fFlags)
        {
            Lut1DOpDataRcPtr lutData = lut1DData();

            lutData->setInversionQuality(
                fFlags==FINALIZATION_FAST ? LUT_INVERSION_FAST: LUT_INVERSION_EXACT);

            lutData->finalize();

            // Rebuild the cache identifier
            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut1D ";
            cacheIDStream << lutData->getCacheID() << " ";
            cacheIDStream << ">";

            m_cacheID = cacheIDStream.str();
        }

        ConstOpCPURcPtr Lut1DOp::getCPUOp() const
        {
            ConstLut1DOpDataRcPtr data = lut1DData();
            return GetLut1DRenderer(data, BIT_DEPTH_F32, BIT_DEPTH_F32);
        }

        void Lut1DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if (getInputBitDepth() != BIT_DEPTH_F32 || getOutputBitDepth() != BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            ConstLut1DOpDataRcPtr lutData = lut1DData();
            if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
            {
                // TODO: Add GPU renderer for EXACT mode.

                Lut1DOpDataRcPtr newLut = Lut1DOpData::MakeFastLut1DFromInverse(lutData, true);
                if (!newLut)
                {
                    throw Exception("Cannot apply Lut1DOp, inversion failed.");
                }

                Lut1DOp invLut(newLut);
                invLut.finalize(FINALIZATION_EXACT);
                invLut.extractGpuShaderInfo(shaderDesc);
            }
            else
            {
                GetLut1DGPUShaderProgram(shaderDesc, lutData);
            }
        }
    }

    void CreateLut1DOp(OpRcPtrVec & ops,
                       Lut1DOpDataRcPtr & lut,
                       TransformDirection direction)
    {
        // TODO: Detect if 1D LUT can be exactly approximated as y = mx + b
        // If so, return a mtx instead.

        if (direction != TRANSFORM_DIR_FORWARD
            && direction != TRANSFORM_DIR_INVERSE)
        {
            throw Exception("Cannot apply Lut1DOp op, "
                            "unspecified transform direction.");
        }

        if (direction == TRANSFORM_DIR_FORWARD)
        {
            ops.push_back(std::make_shared<Lut1DOp>(lut));
        }
        else
        {
            Lut1DOpDataRcPtr data = lut->inverse();
            ops.push_back(std::make_shared<Lut1DOp>(data));
        }
    }

    void GenerateIdentityLut1D(float* img, int numElements, int numChannels)
    {
        if(!img) return;
        int numChannelsToFill = std::min(3, numChannels);
        
        float scale = 1.0f / ((float) numElements - 1.0f);
        for(int i=0; i<numElements; i++)
        {
            for(int c=0; c<numChannelsToFill; ++c)
            {
                img[numChannels*i+c] = scale * (float)(i);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////

    void CreateLut1DTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
    {
        auto lut = DynamicPtrCast<const Lut1DOp>(op);
        if (!lut)
        {
            throw Exception("CreateLut1DTransform: op has to be a Lut1DOp");
        }
        auto lutData = DynamicPtrCast<const Lut1DOpData>(op->data());
        auto lutTransform = LUT1DTransform::Create();

        lutTransform->setFileOutputBitDepth(lutData->getFileOutputBitDepth());

        const auto dir = lutData->getDirection();
        lutTransform->setDirection(dir);

        auto & formatMetadata = lutTransform->getFormatMetadata();
        auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
        metadata = lutData->getFormatMetadata();

        const bool inputHalf = lutData->isInputHalfDomain();
        const bool outputHalf = lutData->isOutputRawHalfs();
        const LUT1DHueAdjust hue = lutData->getHueAdjust();
        const Interpolation interp = lutData->getInterpolation();

        lutTransform->setInputHalfDomain(inputHalf);
        lutTransform->setOutputRawHalfs(outputHalf);
        lutTransform->setHueAdjust(hue);
        lutTransform->setInterpolation(interp);

        // Scale back to F32.
        const float scale = 1.0f /
            ((dir == TRANSFORM_DIR_FORWARD) ? (float)GetBitDepthMaxValue(lutData->getOutputBitDepth())
                                            : (float)GetBitDepthMaxValue(lutData->getInputBitDepth()));

        auto & lutArray = lutData->getArray();
        const unsigned long l = lutArray.getLength();
        lutTransform->setLength(l);
        for (unsigned int i = 0; i < l; ++i)
        {
            lutTransform->setValue(i, scale * lutArray[3 * i],
                                      scale * lutArray[3 * i + 1],
                                      scale * lutArray[3 * i + 2]);
        }

        group->push_back(lutTransform);
    }

    void BuildLut1DOps(OpRcPtrVec & ops,
                       const Config & config,
                       const LUT1DTransform & transform,
                       TransformDirection dir)
    {
        TransformDirection combinedDir =
            CombineTransformDirections(dir,
                                       transform.getDirection());

        const unsigned long length = transform.getLength();
        auto halfFlags = (Lut1DOpData::HalfFlags)(
            (transform.getInputHalfDomain() ? Lut1DOpData::LUT_INPUT_HALF_CODE :
                                              Lut1DOpData::LUT_STANDARD) |
                                              (transform.getOutputRawHalfs() ? Lut1DOpData::LUT_OUTPUT_HALF_CODE :
                                                                               Lut1DOpData::LUT_STANDARD));

        auto data = std::make_shared<Lut1DOpData>(
            BIT_DEPTH_F32, BIT_DEPTH_F32,
            transform.getFormatMetadata(),
            transform.getInterpolation(),
            halfFlags,
            length);

        data->setFileOutputBitDepth(transform.getFileOutputBitDepth());

        data->setHueAdjust(transform.getHueAdjust());
        for (unsigned long i = 0; i < length; ++i)
        {
            float r = 0.f;
            float g = 0.f;
            float b = 0.f;
            transform.getValue(i, r, g, b);
            data->getArray()[3 * i] = r;
            data->getArray()[3 * i + 1] = g;
            data->getArray()[3 * i + 2] = b;
        }
        CreateLut1DOp(ops, data, combinedDir);
    }

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include <cstring>

namespace OCIO = OCIO_NAMESPACE;
#include "OpBuilders.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace
{

OCIO_ADD_TEST(Lut1DOp, extrapolation_errors)
{
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(3);
    auto & lutArray = lut->getArray();

    // Simple y=x+0.1 LUT.
    for (int i = 0; i < 3; ++i)
    {
        for (int c = 0; c < 3; ++c)
        {
            lutArray[c + i * 3] += 0.1f;
        }
    }

    bool isIdentity = true;
    OCIO_CHECK_NO_THROW(isIdentity = lut->isNoOp());
    OCIO_CHECK_ASSERT(!isIdentity);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));

    const int PIXELS = 5;
    float inputBuffer_linearforward[PIXELS * 4] = {
        -0.1f,   -0.2f, -10.0f, 0.0f,
         0.5f,    1.0f,   1.1f, 0.0f,
        10.1f,   55.0f,   2.3f, 0.0f,
         9.1f,  1.0e6f, 1.0e9f, 0.0f,
        4.0e9f, 9.5e7f,   0.5f, 0.0f };
    const float outputBuffer_linearforward[PIXELS * 4] = {
        0.1f, 0.1f, 0.1f, 0.0f,
        0.6f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 0.6f, 0.0f };

    ops[0]->apply(inputBuffer_linearforward, PIXELS);
    for (size_t i = 0; i < sizeof(inputBuffer_linearforward) / sizeof(inputBuffer_linearforward[0]); ++i)
    {
        OCIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
    }
}

OCIO_ADD_TEST(Lut1DOp, inverse)
{
    OCIO::Lut1DOpDataRcPtr luta = std::make_shared<OCIO::Lut1DOpData>(3);
    auto & lutaArray = luta->getArray();
    lutaArray[0] = 0.1f;

    OCIO::Lut1DOpDataRcPtr lutb = luta->clone();

    OCIO::Lut1DOpDataRcPtr lutc = luta->clone();
    auto & lutcArray = lutc->getArray();
    lutcArray[0] = 0.2f;

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, luta, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, luta, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutb, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutb, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutc, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutc, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_EQUAL(op0->isInverse(op1), true);
    OCIO_CHECK_EQUAL(op2->isInverse(op3), true);
    OCIO_CHECK_EQUAL(op4->isInverse(op5), true);

    OCIO_CHECK_EQUAL(op0->isInverse(op2), false);
    OCIO_CHECK_EQUAL(op0->isInverse(op3), true);
    OCIO_CHECK_EQUAL(op1->isInverse(op2), true);
    OCIO_CHECK_EQUAL(op1->isInverse(op3), false);

    OCIO_CHECK_EQUAL(op0->isInverse(op4), false);
    OCIO_CHECK_EQUAL(op0->isInverse(op5), false);
    OCIO_CHECK_EQUAL(op1->isInverse(op4), false);
    OCIO_CHECK_EQUAL(op1->isInverse(op5), false);

    OCIO_CHECK_EQUAL(ops[0]->getCacheID(), ops[2]->getCacheID());
    OCIO_CHECK_EQUAL(ops[1]->getCacheID(), ops[3]->getCacheID());

    OCIO_CHECK_NE(ops[0]->getCacheID(), ops[4]->getCacheID());
    OCIO_CHECK_NE(ops[0]->getCacheID(), ops[5]->getCacheID());
    OCIO_CHECK_NE(ops[1]->getCacheID(), ops[4]->getCacheID());
    OCIO_CHECK_NE(ops[1]->getCacheID(), ops[5]->getCacheID());

    // Optimize will remove LUT forward and inverse (0+1, 2+3 and 4+5).
    // Clamping is lost.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
}

OCIO::Lut1DOpDataRcPtr CreateSquareLut()
{
    // Make a LUT that squares the input.
    static constexpr int size = 256;
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(size);
    auto & lutArray = lut->getArray();

    for (int i = 0; i < size; ++i)
    {
        float x = (float)i / (float)(size - 1);
        float x2 = x*x;

        for (int c = 0; c < 3; ++c)
        {
            lutArray[c + i * 3] = x2;
        }
    }
    return lut;
}
}

OCIO_ADD_TEST(Lut1DOp, finite_value)
{
    OCIO::Lut1DOpDataRcPtr lut = CreateSquareLut();

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

    float inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    const float outputBuffer_linearforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    ops[0]->apply(inputBuffer_linearforward, 1);
    for(int i=0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
    }

    const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    ops[1]->apply(outputBuffer_linearinverse, 1);
    for(int i=0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(inputBuffer_linearinverse[i], outputBuffer_linearinverse[i], 1e-5f);
    }
}

OCIO_ADD_TEST(Lut1DOp, gpu)
{
    OCIO::Lut1DOpDataRcPtr lut = CreateSquareLut();
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));
    
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->supportedByLegacyShader(), false);
}

OCIO_ADD_TEST(Lut1DOp, identity_lut_1d)
{
    int size = 3;
    int channels = 2;
    std::vector<float> data(size*channels);
    OCIO::GenerateIdentityLut1D(&data[0], size, channels);
    OCIO_CHECK_EQUAL(data[0], 0.0f);
    OCIO_CHECK_EQUAL(data[1], 0.0f);
    OCIO_CHECK_EQUAL(data[2], 0.5f);
    OCIO_CHECK_EQUAL(data[3], 0.5f);
    OCIO_CHECK_EQUAL(data[4], 1.0f);
    OCIO_CHECK_EQUAL(data[5], 1.0f);

    size = 4;
    channels = 3;
    data.resize(size*channels);
    OCIO::GenerateIdentityLut1D(&data[0], size, channels);
    for (int c = 0; c < channels; ++c)
    {
        OCIO_CHECK_EQUAL(data[0+c], 0.0f);
        OCIO_CHECK_EQUAL(data[channels+c], 0.33333333f);
        OCIO_CHECK_EQUAL(data[2*channels+c], 0.66666667f);
        OCIO_CHECK_EQUAL(data[3*channels+c], 1.0f);
    }
}

OCIO_ADD_TEST(Lut1D, basic)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(bitDepth, bitDepth,
                                            OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                                            OCIO::INTERP_LINEAR,
                                            OCIO::Lut1DOpData::LUT_STANDARD);

    OCIO::Lut1DOp lut(lutData);

    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());

    const float step = (float)(OCIO::GetBitDepthMaxValue(lutData->getInputBitDepth()))
                       / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = { 0.0f, 0.0f, 0.0f, 1.0f,
                         0.0f, 0.0f, step, 1.0f };

    const float error = 1e-6f;
    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_CLOSE(myImage[0], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[1], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[2], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[3], 1.0f, error);

        OCIO_CHECK_CLOSE(myImage[4], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[5], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[6], step, error);
        OCIO_CHECK_CLOSE(myImage[7], 1.0f, error);
    }

    // No more an 'identity LUT 1D'.
    const float arbitraryVal = 0.123456f;

    lutData->getArray()[5] = arbitraryVal;

    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());

    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_CLOSE(myImage[0], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[1], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[2], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[3], 1.0f, error);

        OCIO_CHECK_CLOSE(myImage[4], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[5], 0.0f, error);
        OCIO_CHECK_CLOSE(myImage[6], arbitraryVal, error);
        OCIO_CHECK_CLOSE(myImage[7], 1.0f, error);
    }

}

OCIO_ADD_TEST(Lut1D, half)
{
    OCIO::Lut1DOpDataRcPtr
        lutData(
            new OCIO::Lut1DOpData(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
                                  OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                                  OCIO::INTERP_LINEAR,
                                  OCIO::Lut1DOpData::LUT_STANDARD));

    OCIO::Lut1DOp lut(lutData);

    const float step = (float)(OCIO::GetBitDepthMaxValue(lutData->getInputBitDepth()))
                       / ((float)lutData->getArray().getLength() - 1.0f);

    // No more an 'identity LUT 1D'
    const float arbitraryVal = 0.123456f;
    lutData->getArray()[5] = arbitraryVal;
    OCIO_CHECK_ASSERT(!lutData->isIdentity());

    const half myImage[8] = { 0.1f, 0.3f, 0.4f, 1.0f,
                              0.0f, 0.9f, step, 0.0f };

    float resImage[8] = { 0.1f, 0.3f, 0.4f, 1.0f,
                          0.0f, 0.9f, step, 0.0f };

    // TODO: The SC test is intended to test half evaluation using myImage
    // as input.  Adjust after half support is added to apply.
    lut.setInputBitDepth(OCIO::BIT_DEPTH_F32);
    lut.setOutputBitDepth(OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(lut.apply(resImage, resImage, 2));

    const float error = 1e-4f;

    OCIO_CHECK_CLOSE(resImage[0], (float)myImage[0], error);
    OCIO_CHECK_CLOSE(resImage[1], (float)myImage[1], error);
    OCIO_CHECK_CLOSE(resImage[2], (float)myImage[2], error);
    OCIO_CHECK_CLOSE(resImage[3], (float)myImage[3], error);

    OCIO_CHECK_CLOSE(resImage[4], (float)myImage[4], error);
    OCIO_CHECK_CLOSE(resImage[5], (float)myImage[5], error);
    OCIO_CHECK_CLOSE(resImage[6], arbitraryVal, error);
    OCIO_CHECK_CLOSE(resImage[7], (float)myImage[7], error);
}

OCIO_ADD_TEST(Lut1D, nan)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'
    OCIO::Lut1DOpDataRcPtr
        lutData(
            new OCIO::Lut1DOpData(bitDepth, bitDepth,
                                  OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                                  OCIO::INTERP_LINEAR,
                                  OCIO::Lut1DOpData::LUT_STANDARD));

    OCIO::Lut1DOp lut(lutData);

    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(lut.isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());

    const float step = (float)(OCIO::GetBitDepthMaxValue(lutData->getInputBitDepth()))
                       / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = {
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f, 1.0f,
                                           0.0f, 0.0f, step, 1.0f };

    const float error = 1e-6f;
    OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

    OCIO_CHECK_CLOSE(myImage[0], 0.0f, error);
    OCIO_CHECK_CLOSE(myImage[1], 0.0f, error);
    OCIO_CHECK_CLOSE(myImage[2], 0.0f, error);
    OCIO_CHECK_CLOSE(myImage[3], 1.0f, error);

    OCIO_CHECK_CLOSE(myImage[4], 0.0f, error);
    OCIO_CHECK_CLOSE(myImage[5], 0.0f, error);
    OCIO_CHECK_CLOSE(myImage[6], step, error);
    OCIO_CHECK_CLOSE(myImage[7], 1.0f, error);
}

OCIO_ADD_TEST(Lut1D, finite_value_hue_adjust)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DOpDataRcPtr lutData = CreateSquareLut();
    lutData->setHueAdjust(OCIO::HUE_DW3);

    OCIO::Lut1DOp lut(lutData);

    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(!lut.isIdentity());

    const float outputBuffer_linearforward[4] = { 0.25f,
                                                  0.37000f, // (Hue adj modifies green here.)
                                                  0.49f,
                                                  0.5f };
    float lut1d_inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

    OCIO_CHECK_NO_THROW(lut.finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(lut.apply(lut1d_inputBuffer_linearforward, 1));
    for (int i = 0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_inputBuffer_linearforward[i],
                         outputBuffer_linearforward[i], 1e-5f);
    }

    OCIO::Lut1DOpDataRcPtr invData = lutData->inverse();
    OCIO::Lut1DOpDataRcPtr invDataExact = invData->clone();
    invDataExact->setInversionQuality(OCIO::LUT_INVERSION_BEST);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(
        CreateLut1DOp(ops, invData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(
        CreateLut1DOp(ops, invDataExact, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float lut1d_outputBuffer_linearinverse[4] = { 0.25f, 0.37f, 0.49f, 0.5f };
    float lut1d_outputBuffer_linearinverseEx[4] = { 0.25f, 0.37f, 0.49f, 0.5f };

    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(ops[0]->apply(lut1d_outputBuffer_linearinverse, 1)); // fast
    OCIO_CHECK_NO_THROW(ops[1]->apply(lut1d_outputBuffer_linearinverseEx, 1)); // exact
    for (int i = 0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_outputBuffer_linearinverse[i],
                         inputBuffer_linearinverse[i], 1e-5f);
        OCIO_CHECK_CLOSE(lut1d_outputBuffer_linearinverseEx[i],
                         inputBuffer_linearinverse[i], 1e-5f);
    }
}


//
// Unit tests using clf files.
//

namespace
{

void Apply(const OCIO::OpRcPtrVec & ops, float * img, long numPixels)
{
    for (auto op : ops)
    {
        op->apply(img, numPixels);
    }
}

}

OCIO_ADD_TEST(Lut1D, apply_half_domain_hue_adjust)
{
    const std::string ctfFile("lut1d_hd_hueAdjust.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, ctfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);

    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lut);

    float inputFrame[] = {
        0.05f, 0.18f, 1.1f, 0.5f,
        2.3f, 0.01f, 0.3f, 1.0f };

    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(Apply(ops, inputFrame, 2));

    const float rtol = 1e-6f;
    const float minExpected = 1e-3f;

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[0], 0.54780269f, rtol, minExpected));

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[1],
                                    9.57448578f,
                                    rtol, minExpected));  // would be 5.0 w/out hue adjust

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[2], 73.45562744f, rtol, minExpected));

    OCIO_CHECK_EQUAL(inputFrame[3], 0.5f);

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[4], 188.087067f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[5], 0.0324990489f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[6],
                                    23.8472710f,
                                    rtol, minExpected));  // would be 11.3372078 w/out hue adjust

    OCIO_CHECK_EQUAL(inputFrame[7], 1.0f);
}

OCIO_ADD_TEST(InvLut1D, apply_half)
{
    const OCIO::BitDepth inBD = OCIO::BIT_DEPTH_F32;
    const OCIO::BitDepth outBD = OCIO::BIT_DEPTH_F32;

    static const std::string ctfFile("lut1d_halfdom.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, ctfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);

    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lut);

    auto fwdLut = lut->clone();
    fwdLut->setInputBitDepth(outBD);
    fwdLut->setOutputBitDepth(inBD);

    OCIO::OpRcPtrVec ops1;
    auto fwdOp = std::make_shared<OCIO::Lut1DOp>(fwdLut);
    ops1.push_back(fwdOp);

    const float inImage[12] = {
         1.f,    1.f,   0.5f, 0.f,
         0.001f, 0.1f,  4.f,  0.5f,  // test positive half domain of R, G, B channels
        -0.08f, -1.f, -10.f,  1.f }; // test negative half domain of R, G, B channels

    float inImage1[12];
    memcpy(inImage1, inImage, 12 * sizeof(float));

    // Apply forward LUT.
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops1, OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(Apply(ops1, inImage1, 3));

    // Apply inverse LUT.
    OCIO::OpRcPtrVec ops2;
    auto invLut = lut->inverse();
    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    auto invOp = std::make_shared<OCIO::Lut1DOp>(invLut);
    ops2.push_back(invOp);

    float inImage2[12];
    memcpy(inImage2, inImage1, 12 * sizeof(float));

    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops2, OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(Apply(ops2, inImage2, 3));

    // Compare the two applys
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_ASSERT(!OCIO::FloatsDiffer(inImage2[i], inImage[i], 50, false));
    }

    // Repeat with style = LUT_INVERSION_FAST.
    OCIO::OpRcPtrVec ops3;
    invLut = lut->inverse();
    invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    invLut->setFileOutputBitDepth(inBD);
    invOp = std::make_shared<OCIO::Lut1DOp>(invLut);
    ops3.push_back(invOp);

    memcpy(inImage2, inImage1, 12 * sizeof(float));

    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops3, OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(Apply(ops3, inImage2, 3));

    // Compare the two applys
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_ASSERT(!OCIO::FloatsDiffer(inImage2[i], inImage[i], 50, false));
    }
}

OCIO_ADD_TEST(Lut1D, lut_1d_compose_with_bit_depth)
{
    const std::string ctfFile("lut1d_comp.clf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, ctfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut1 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lut1);
    op = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut2 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);

    {
        auto lutComposed = lut1->clone();
        OCIO::Lut1DOpData::Compose(lutComposed, lut2, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO);

        const float error = 1e-5f;
        OCIO_CHECK_EQUAL(lutComposed->getArray().getLength(), 2);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[0], 0.00744791f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[1], 0.03172233f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[2], 0.07058375f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[3], 0.3513808f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[4], 0.51819527f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[5], 0.67463773f, error);
    }
    {
        auto lutComposed = lut1->clone();
        OCIO::Lut1DOpData::Compose(lutComposed, lut2, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_INDEPTH);

        const float error = 1e-5f;
        OCIO_CHECK_EQUAL(lutComposed->getArray().getLength(), 256);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[0], 0.00744791f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[1], 0.03172233f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[2], 0.07058375f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[383], 0.28073114f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[384], 0.09914176f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[385], 0.1866852f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[765], 0.3513808f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[766], 0.51819527f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[767], 0.67463773f, error);
    }
}

OCIO_ADD_TEST(Lut1D, inverse_twice)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DOpDataRcPtr lut = CreateSquareLut();

    const float outputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

    // Create inverse lut.
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    const float lut1d_inputBuffer_reference[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    float lut1d_inputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(ops[0]->apply(lut1d_inputBuffer_linearinverse, 1));
    for (int i = 0; i < 4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
            outputBuffer_linearinverse[i], 1e-5f);
    }

    // Inverse the inverse.
    OCIO::Lut1DOp * pLut = dynamic_cast<OCIO::Lut1DOp*>(ops[0].get());
    OCIO_CHECK_ASSERT(pLut);
    OCIO::Lut1DOpDataRcPtr lutData = pLut->lut1DData()->inverse();
    OCIO_CHECK_NO_THROW(OCIO::CreateLut1DOp(ops, lutData,
                                            OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    // Apply the inverse.
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(ops[1]->apply(lut1d_inputBuffer_linearinverse, 1));

    // Verify we are back on the input.
    for (int i = 0; i < 4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
                         lut1d_inputBuffer_reference[i], 1e-5f);
    }
}

OCIO_ADD_TEST(Lut1D, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::BIT_DEPTH_UINT8,
        OCIO::BIT_DEPTH_UINT10,
        OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
        OCIO::INTERP_LINEAR,
        OCIO::Lut1DOpData::LUT_STANDARD,
        3);
    lut->setFileOutputBitDepth(lut->getOutputBitDepth());
    lut->getArray()[3] = 500.1f;
    lut->getArray()[4] = 500.2f;
    lut->getArray()[5] = 500.3f;

    auto & metadataSource = lut->getFormatMetadata();
    metadataSource.addAttribute(OCIO::METADATA_NAME, "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLut1DOp(ops, lut, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateLut1DTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::LUT1DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);

    const auto & metadata = lTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), OCIO::METADATA_NAME);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(lTransform->getDirection(), direction);
    OCIO_REQUIRE_EQUAL(lTransform->getLength(), 3);

    OCIO_CHECK_EQUAL(lTransform->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lTransform->getValue(1, r, g, b);

    // Transform LUT is always 32F.
    const float scale = (float)(OCIO::GetBitDepthMaxValue(lTransform->getFileOutputBitDepth()));
    OCIO_CHECK_EQUAL(r * scale, 500.1f);
    OCIO_CHECK_EQUAL(g * scale, 500.2f);
    OCIO_CHECK_EQUAL(b * scale, 500.3f);
}

OCIO_ADD_TEST(LUT1DTransform, build_op)
{
    const OCIO::LUT1DTransformRcPtr lut = OCIO::LUT1DTransform::Create();
    lut->setLength(3);

    const float r = 0.51f;
    const float g = 0.52f;
    const float b = 0.53f;
    lut->setValue(1, r, g, b);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::OpRcPtrVec ops;
    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), lut, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    auto constop = std::const_pointer_cast<const OCIO::Op>(ops[0]);
    OCIO_REQUIRE_ASSERT(constop);
    auto data = constop->data();
    auto lutdata = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DOpData>(data);
    OCIO_REQUIRE_ASSERT(lutdata);

    OCIO_CHECK_EQUAL(lutdata->getArray().getLength(), 3);
    OCIO_CHECK_EQUAL(lutdata->getArray()[3], r);
    OCIO_CHECK_EQUAL(lutdata->getArray()[4], g);
    OCIO_CHECK_EQUAL(lutdata->getArray()[5], b);
}

#endif // OCIO_UNIT_TEST

// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Blue
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Green
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Red
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_hue_adjust
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_identity_half
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_identity_half_to_integer
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_identity_integer_to_half
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_HALF_CODE
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_identity
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_increasing
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_decreasing_reversals
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_clamp_to_lut_range
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_flat_start_or_end
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_halfinput
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_identity
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_ctf
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_fclut
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_hueAdjust
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_hueAdjust
