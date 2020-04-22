// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "Processor.cpp"

#include "ops/exposurecontrast/ExposureContrastOp.h"
#include "testutils/UnitTest.h"
#include "UnitTestOptimFlags.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Processor, basic)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    auto processorEmptyGroup = config->getProcessor(group);
    OCIO_CHECK_EQUAL(processorEmptyGroup->getNumTransforms(), 0);
    OCIO_CHECK_EQUAL(std::string(processorEmptyGroup->getCacheID()), "<NOOP>");

    auto mat = OCIO::MatrixTransform::Create();
    double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    mat->setOffset(offset);

    auto processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(processorMat->getNumTransforms(), 1);

    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "$b8861d4acd905af0e84ddf10c860b220");
}

OCIO_ADD_TEST(Processor, shared_dynamic_properties)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();

    data->setExposure(1.2);
    data->setPivot(0.5);
    data->getExposureProperty()->makeDynamic();

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    data = data->clone();
    data->setExposure(2.2);

    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    auto data0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastOpData>(op0->data());
    auto data1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastOpData>(op1->data());

    OCIO::DynamicPropertyImplRcPtr dp0 = data0->getExposureProperty();
    OCIO::DynamicPropertyImplRcPtr dp1 = data1->getExposureProperty();

    OCIO_CHECK_NE(dp0->getDoubleValue(), dp1->getDoubleValue());

    OCIO_CHECK_NO_THROW(ops.unifyDynamicProperties());

    OCIO::DynamicPropertyImplRcPtr dp0_post = data0->getExposureProperty();
    OCIO::DynamicPropertyImplRcPtr dp1_post = data1->getExposureProperty();

    OCIO_CHECK_EQUAL(dp0_post->getDoubleValue(), dp1_post->getDoubleValue());

    // Both share the same pointer.
    OCIO_CHECK_EQUAL(dp0_post.get(), dp1_post.get());

    // The first pointer is the one that gets shared.
    OCIO_CHECK_EQUAL(dp0.get(), dp0_post.get());
}

namespace
{
void GetFormatName(const std::string & extension, std::string & name)
{
    std::string requestedExt{ extension };
    std::transform(requestedExt.begin(), requestedExt.end(), requestedExt.begin(), ::tolower);
    for (int i = 0; i < OCIO::Processor::getNumWriteFormats(); ++i)
    {
        std::string formatExt(OCIO::Processor::getFormatExtensionByIndex(i));
        if (requestedExt == formatExt)
        {
            name = OCIO::Processor::getFormatNameByIndex(i);
            break;
        }
    }
}
}

OCIO_ADD_TEST(Processor, write_formats)
{
    std::string clfFileFormat;
    GetFormatName("CLF", clfFileFormat);
    OCIO_CHECK_EQUAL(clfFileFormat, OCIO::FILEFORMAT_CLF);

    std::string ctfFileFormat;
    GetFormatName("CTF", ctfFileFormat);
    OCIO_CHECK_EQUAL(ctfFileFormat, OCIO::FILEFORMAT_CTF);

    std::string noFileFormat;
    GetFormatName("XXX", noFileFormat);
    OCIO_CHECK_ASSERT(noFileFormat.empty());
}

OCIO_ADD_TEST(Processor, optimized_processor)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    auto mat = OCIO::MatrixTransform::Create();
    double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    mat->setOffset(offset);

    group->appendTransform(mat);
    group->appendTransform(mat);
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");

    auto processorGroup = config->getProcessor(group);
    OCIO_CHECK_EQUAL(processorGroup->getNumTransforms(), 2);

    auto processorOpt1 = processorGroup->getOptimizedProcessor(OCIO::OPTIMIZATION_DEFAULT);
    OCIO_CHECK_EQUAL(processorOpt1->getNumTransforms(), 1);
    OCIO_REQUIRE_EQUAL(processorOpt1->getFormatMetadata().getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(processorOpt1->getFormatMetadata().getAttributeName(0)), OCIO::METADATA_ID);
    OCIO_CHECK_EQUAL(std::string(processorOpt1->getFormatMetadata().getAttributeValue(0)), "UID42");

    auto processorOpt2 = processorGroup->getOptimizedProcessor(OCIO::OPTIMIZATION_NONE);
    OCIO_CHECK_EQUAL(processorOpt2->getNumTransforms(), 2);
    OCIO_REQUIRE_EQUAL(processorOpt2->getFormatMetadata().getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(processorOpt2->getFormatMetadata().getAttributeName(0)), OCIO::METADATA_ID);
    OCIO_CHECK_EQUAL(std::string(processorOpt2->getFormatMetadata().getAttributeValue(0)), "UID42");

    // Use an optimization flags environment variable.
    {
        OCIOOptimizationFlagsEnvGuard flagsGuard("0"); // OPTIMIZATION_NONE.
        auto processorOpt3 = processorGroup->getOptimizedProcessor(OCIO::OPTIMIZATION_DEFAULT);
        OCIO_CHECK_EQUAL(processorOpt3->getNumTransforms(), 2);
    }
}

OCIO_ADD_TEST(Processor, is_noop)
{
    // Basic validation of the isNoOp() behavior.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    auto matrix = OCIO::MatrixTransform::Create();

    auto processor = config->getProcessor(matrix);

    OCIO_CHECK_ASSERT(processor->isNoOp());
    OCIO_CHECK_ASSERT(processor->getDefaultCPUProcessor()->isNoOp());
    OCIO_CHECK_ASSERT(processor->getDefaultGPUProcessor()->isNoOp());

    double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    matrix->setOffset(offset);

    processor = config->getProcessor(matrix);

    OCIO_CHECK_ASSERT(!processor->isNoOp());
    OCIO_CHECK_ASSERT(!processor->getDefaultCPUProcessor()->isNoOp());
    OCIO_CHECK_ASSERT(!processor->getDefaultGPUProcessor()->isNoOp());

    // Check with at least one dynamic property.

    auto ec = OCIO::ExposureContrastTransform::Create();

    processor = config->getProcessor(ec);

    OCIO_CHECK_ASSERT(processor->isNoOp());
    OCIO_CHECK_ASSERT(processor->getDefaultCPUProcessor()->isNoOp());
    OCIO_CHECK_ASSERT(processor->getDefaultGPUProcessor()->isNoOp());

    OCIO_CHECK_ASSERT(processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT)->isNoOp());
    OCIO_CHECK_ASSERT(processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DRAFT)->isNoOp());

    // Check with a bit-depth change.
    OCIO_CHECK_ASSERT(!processor->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F16,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::OPTIMIZATION_DEFAULT)->isNoOp());

    ec->makeExposureDynamic();

    processor = config->getProcessor(ec);

    OCIO_CHECK_ASSERT(!processor->isNoOp());
    OCIO_CHECK_ASSERT(!processor->getDefaultCPUProcessor()->isNoOp());
    OCIO_CHECK_ASSERT(!processor->getDefaultGPUProcessor()->isNoOp());
}

OCIO_ADD_TEST(Processor, channel_crosstalk)
{
    // Basic validation of the hasChannelCrosstalk() behavior.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    auto matrix = OCIO::MatrixTransform::Create();

    double mat[16]{ 1., 0., 0., 0.,
                    0., 1., 0., 0.,
                    0., 0., 2., 0.,
                    0., 0., 0., 1.  };

    matrix->setMatrix(mat);

    auto processor = config->getProcessor(matrix);

    OCIO_CHECK_ASSERT(!processor->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!processor->getDefaultCPUProcessor()->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!processor->getDefaultGPUProcessor()->hasChannelCrosstalk());
  
    mat[4] = 1.; // That's not anymore a diagonal matrix.
    matrix->setMatrix(mat);

    processor = config->getProcessor(matrix);

    OCIO_CHECK_ASSERT(processor->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(processor->getDefaultCPUProcessor()->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(processor->getDefaultGPUProcessor()->hasChannelCrosstalk());

    // Check with a bit-depth change i.e. no impact.
    OCIO_CHECK_ASSERT(
        processor->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F16,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT)->hasChannelCrosstalk());

    mat[4] = 0.; // It's now back to a diagonal matrix.
    matrix->setMatrix(mat);

    processor = config->getProcessor(matrix);

    OCIO_CHECK_ASSERT(!processor->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!processor->getDefaultCPUProcessor()->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!processor->getDefaultGPUProcessor()->hasChannelCrosstalk());

    // Check with a bit-depth change i.e. no impact.
    OCIO_CHECK_ASSERT(
        !processor->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F16,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::OPTIMIZATION_DEFAULT)->hasChannelCrosstalk());
}

OCIO_ADD_TEST(Processor, optimization_env_override_basic)
{
    OCIOOptimizationFlagsEnvGuard flagsGuard("");

    OCIO::OptimizationFlags testFlag{ OCIO::OPTIMIZATION_DEFAULT };
    OCIO_CHECK_EQUAL(testFlag, OCIO::EnvironmentOverride(testFlag));

    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, "0");
    OCIO_CHECK_EQUAL(OCIO::OPTIMIZATION_NONE, OCIO::EnvironmentOverride(testFlag));

    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, "0xFFFFFFFF");
    OCIO_CHECK_EQUAL(OCIO::OPTIMIZATION_ALL, OCIO::EnvironmentOverride(testFlag));

    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, "20479");
    OCIO_CHECK_EQUAL(OCIO::OPTIMIZATION_LOSSLESS, OCIO::EnvironmentOverride(testFlag));

    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, "0x1FFFF");
    OCIO_CHECK_EQUAL(OCIO::OPTIMIZATION_GOOD, OCIO::EnvironmentOverride(testFlag));
}

