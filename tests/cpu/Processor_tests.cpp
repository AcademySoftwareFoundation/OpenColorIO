// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "Processor.cpp"

#include "ops/exposurecontrast/ExposureContrastOp.h"
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestOptimFlags.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Processor, basic)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    auto processorEmptyGroup = config->getProcessor(group);
    OCIO_CHECK_EQUAL(processorEmptyGroup->getNumTransforms(), 0);
    OCIO_CHECK_EQUAL(std::string(processorEmptyGroup->getCacheID()), "<NOOP>");

    auto mat = OCIO::MatrixTransform::Create();
    double matrix[16]{
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };
    double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    mat->setMatrix(matrix);
    mat->setOffset(offset);

    auto processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(processorMat->getNumTransforms(), 1);
    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "1b1880136f7669351adb0dcae0f4f9fd");

    // Check behaviour of the cacheID

    offset[0] = 0.0;
    mat->setOffset(offset);
    processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "675ca29c0f7d28fbdc865818c8cf5c4c");

    matrix[0] = 2.0;
    mat->setMatrix(matrix);
    processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "1ebac7d1c2d833943e1d1d3c26a7eb18");

    offset[0] = 0.1;
    matrix[0] = 1.0;
    mat->setOffset(offset);
    mat->setMatrix(matrix);
    processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "1b1880136f7669351adb0dcae0f4f9fd");
}

OCIO_ADD_TEST(Processor, unique_dynamic_properties)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data = std::make_shared<OCIO::ExposureContrastOpData>();

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

    OCIO::DynamicPropertyDoubleImplRcPtr dp0 = data0->getExposureProperty();
    OCIO::DynamicPropertyDoubleImplRcPtr dp1 = data1->getExposureProperty();

    OCIO_CHECK_NE(dp0->getValue(), dp1->getValue());

    OCIO::LogGuard log;
    OCIO_CHECK_NO_THROW(ops.validateDynamicProperties());
    OCIO_CHECK_EQUAL(log.output(), "[OpenColorIO Warning]: Exposure dynamic property can "
                                   "only be there once.\n");
}

OCIO_ADD_TEST(Processor, dynamic_properties)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setExposure(1.2);
    ec->setPivot(0.5);
    ec->makeContrastDynamic();

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    auto proc = config->getProcessor(ec);
    OCIO_CHECK_ASSERT(proc->isDynamic());
    OCIO_CHECK_ASSERT(proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(!proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OCIO::DynamicPropertyRcPtr dpc;
    OCIO_CHECK_NO_THROW(dpc = proc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(dpc);
    OCIO_CHECK_THROW_WHAT(proc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE),
                          OCIO::Exception, "Cannot find dynamic property");
}

OCIO_ADD_TEST(Processor, optimized_processor)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
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

    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, "144457667");
    OCIO_CHECK_EQUAL(OCIO::OPTIMIZATION_LOSSLESS, OCIO::EnvironmentOverride(testFlag));

    OCIO::SetEnvVariable(OCIO::OCIO_OPTIMIZATION_FLAGS_ENVVAR, "0xFFC3FC3");
    OCIO_CHECK_EQUAL(OCIO::OPTIMIZATION_GOOD, OCIO::EnvironmentOverride(testFlag));
}

OCIO_ADD_TEST(Processor, cache_optimized_processors)
{
    // Test the cache for the optimized processors.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    auto matrix = OCIO::MatrixTransform::Create();

    const double offset[16] { 0.1, 0.2, 0.3, 0. };
    matrix->setOffset(offset);

    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(matrix);
    group->appendTransform(matrix);

    auto proc1 = config->getProcessor(group);

    OCIO::ConstProcessorRcPtr optProc1;
    OCIO_CHECK_NO_THROW(optProc1 = proc1->getOptimizedProcessor(OCIO::BIT_DEPTH_F32,
                                                                OCIO::BIT_DEPTH_F32,
                                                                OCIO::OPTIMIZATION_DEFAULT));

    OCIO::ConstProcessorRcPtr optProc2;
    OCIO_CHECK_NO_THROW(optProc2 = proc1->getOptimizedProcessor(OCIO::BIT_DEPTH_F32,
                                                                OCIO::BIT_DEPTH_F32,
                                                                OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(optProc1.get(), optProc2.get());


    OCIO_CHECK_NO_THROW(optProc2 = proc1->getOptimizedProcessor(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(optProc1.get(), optProc2.get());

    // The input bit-depth is different. (The processor is actually the same in this case, but a 
    // new copy is made.)
    OCIO_CHECK_NO_THROW(optProc2 = proc1->getOptimizedProcessor(OCIO::BIT_DEPTH_F16,
                                                                OCIO::BIT_DEPTH_F32,
                                                                OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NE(optProc1.get(), optProc2.get());

    // The optimization flag is different.
    OCIO_CHECK_NO_THROW(optProc2 = proc1->getOptimizedProcessor(OCIO::BIT_DEPTH_F32,
                                                                OCIO::BIT_DEPTH_F32,
                                                                OCIO::OPTIMIZATION_NONE));
    OCIO_CHECK_NE(optProc1.get(), optProc2.get());

    OCIO_CHECK_NO_THROW(optProc1 = proc1->getOptimizedProcessor(OCIO::BIT_DEPTH_F32,
                                                                OCIO::BIT_DEPTH_F32,
                                                                OCIO::OPTIMIZATION_NONE));
    OCIO_CHECK_EQUAL(optProc1.get(), optProc2.get());

    // Even with a 'dynamic' transform (i.e. contains dynamic properties) the cache is still used.

    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setExposure(0.65);

    proc1 = config->getProcessor(ec);

    // The dynamic properties are not dynamic so the cache is still used.
    OCIO_CHECK_EQUAL(proc1->getOptimizedProcessor(OCIO::OPTIMIZATION_DEFAULT).get(),
                     proc1->getOptimizedProcessor(OCIO::OPTIMIZATION_DEFAULT).get());

    // Make Exposure dynamic.
    ec->makeExposureDynamic();

    proc1 = config->getProcessor(ec);

    // A dynamic property is now dynamic but the cache is still used.
    OCIO_CHECK_EQUAL(proc1->getOptimizedProcessor(OCIO::OPTIMIZATION_DEFAULT).get(),
                     proc1->getOptimizedProcessor(OCIO::OPTIMIZATION_DEFAULT).get());
}

OCIO_ADD_TEST(Processor, cache_cpu_processors)
{
    // Test the cache for the CPU processors.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    auto matrix = OCIO::MatrixTransform::Create();

    const double offset[16] { 0.1, 0.2, 0.3, 0. };

    matrix->setOffset(offset);


    // Step 1 - Test with default cache flags i.e. cache enabled and share processor instances
    // containing dynamic properties.

    auto proc1 = config->getProcessor(matrix);

    OCIO::ConstCPUProcessorRcPtr cpuProc1;
    OCIO_CHECK_NO_THROW(cpuProc1 = proc1->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                   OCIO::BIT_DEPTH_F32,
                                                                   OCIO::OPTIMIZATION_DEFAULT));

    OCIO::ConstCPUProcessorRcPtr cpuProc2;
    OCIO_CHECK_NO_THROW(cpuProc2 = proc1->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                   OCIO::BIT_DEPTH_F32,
                                                                   OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(cpuProc1.get(), cpuProc2.get());

    OCIO_CHECK_NO_THROW(cpuProc2 = proc1->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(cpuProc1.get(), cpuProc2.get());

    OCIO_CHECK_NO_THROW(cpuProc2 = proc1->getDefaultCPUProcessor());
    OCIO_CHECK_EQUAL(cpuProc1.get(), cpuProc2.get());


    // The input bit-depth is different.
    OCIO_CHECK_NO_THROW(cpuProc2 = proc1->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F16,
                                                                   OCIO::BIT_DEPTH_F32,
                                                                   OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NE(cpuProc1.get(), cpuProc2.get());

    // The optimization flag is different.
    OCIO_CHECK_NO_THROW(cpuProc2 = proc1->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                   OCIO::BIT_DEPTH_F32,
                                                                   OCIO::OPTIMIZATION_LOSSLESS));
    OCIO_CHECK_NE(cpuProc1.get(), cpuProc2.get());

    OCIO_CHECK_NO_THROW(cpuProc1 = proc1->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_LOSSLESS));
    OCIO_CHECK_EQUAL(cpuProc1.get(), cpuProc2.get());

    // If that's a 'dynamic' transform (i.e. contains dynamic properties) then the cache is used
    // or not depdending of the cache setting.

    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setExposure(0.65);
    
    auto proc2 = config->getProcessor(ec);

    // The dynamic properties are not dynamic so the cache is still used.
    OCIO_CHECK_EQUAL(proc2->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get(), 
                     proc2->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get());

    // Make Exposure dynamic.
    ec->makeExposureDynamic();

    proc2 = config->getProcessor(ec);

    // By default, the processor cache share processor instances containing dynamic properties.
    OCIO_CHECK_EQUAL(proc2->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get(), 
                     proc2->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get());


    // Step 2 - Change the default cache flags to disable the dyn. property share.

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    cfg->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_ENABLED); // Enabled but no dyn. property share

    OCIO_CHECK_EQUAL(cfg->getProcessor(matrix).get(), cfg->getProcessor(matrix).get());

    proc1 = cfg->getProcessor(ec);

    // Now the processor cache does not share processor instances containing dynamic properties.
    OCIO_CHECK_NE(proc1->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get(), 
                  proc1->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get());

    // Disable the caches.

    cfg->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_OFF); // Cache disabled

    OCIO_CHECK_NE(cfg->getProcessor(matrix).get(), cfg->getProcessor(matrix).get());

    proc1 = cfg->getProcessor(ec);

    OCIO_CHECK_NE(proc1->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get(), 
                  proc1->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get());
}

OCIO_ADD_TEST(Processor, cache_gpu_processors)
{
    // Test the cache for the GPU processors.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    auto matrix = OCIO::MatrixTransform::Create();

    const double offset[16] { 0.1, 0.2, 0.3, 0. };

    matrix->setOffset(offset);

    auto proc1 = config->getProcessor(matrix);

    OCIO::ConstGPUProcessorRcPtr gpuProc1;
    OCIO_CHECK_NO_THROW(gpuProc1 = proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_DEFAULT));

    OCIO::ConstGPUProcessorRcPtr gpuProc2;
    OCIO_CHECK_NO_THROW(gpuProc2 = proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(gpuProc1.get(), gpuProc2.get());

    // The optimization flag is different.
    OCIO_CHECK_NO_THROW(gpuProc2 = proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_LOSSLESS));
    OCIO_CHECK_NE(gpuProc1.get(), gpuProc2.get());

    OCIO_CHECK_NO_THROW(gpuProc1 = proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_LOSSLESS));
    OCIO_CHECK_EQUAL(gpuProc1.get(), gpuProc2.get());

    // Even with a 'dynamic' transform (i.e. contains dynamic properties) the cache is still used.

    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setExposure(0.65);
    
    proc1 = config->getProcessor(ec);

    // The dynamic properties are not dynamic so the cache is still used.
    OCIO_CHECK_EQUAL(proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get(),
                     proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get());

    // Make Exposure dynamic.
    ec->makeExposureDynamic();

    proc1 = config->getProcessor(ec);

    // A dynamic property is now dynamic but the cache is still used.
    OCIO_CHECK_EQUAL(proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get(),
                     proc1->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_DEFAULT).get());
}
