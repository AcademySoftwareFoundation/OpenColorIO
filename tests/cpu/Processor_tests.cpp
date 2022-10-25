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
    double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    mat->setOffset(offset);

    auto processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(processorMat->getNumTransforms(), 1);

    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "$096c01a0daf07446874bd91f7c2abdea");
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

OCIO_ADD_TEST(Processor, processor_to_known_colorspace)
{
    constexpr const char * CONFIG { R"(
ocio_profile_version: 2

roles:
  default: raw
  scene_linear: ref_cs

colorspaces:
  - !<ColorSpace>
    name: raw
    description: A data colorspace.
    isdata: true

  - !<ColorSpace>
    name: ref_cs
    description: The reference colorspace.
    isdata: false

  - !<ColorSpace>
    name: not sRGB
    description: 
    isdata: false
    to_scene_reference: !<BuiltinTransform> {style: ACEScct_to_ACES2065-1}

  - !<ColorSpace>
    name: ACES cg
    description: 
    isdata: false
    to_scene_reference: !<BuiltinTransform> {style: ACEScg_to_ACES2065-1}

  - !<ColorSpace>
    name: Linear ITU-R BT.709
    description: 
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to Linear Rec.709 (sRGB)
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}

  - !<ColorSpace>
    name: Texture -- sRGB
    description: 
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Rec.709
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: sRGB Encoded AP1 - Texture
    description: 
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Encoded AP1 - Texture
      children:
        - !<MatrixTransform> {matrix: [1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}
)" };

    auto checkProcessor = [](OCIO::ConstProcessorRcPtr & proc)
    {
        OCIO::GroupTransformRcPtr gt = proc->createGroupTransform();
        OCIO_CHECK_EQUAL(gt->getNumTransforms(), 4);

        {
            OCIO::ConstLogCameraTransformRcPtr logAfTf0 = 
            OCIO::DynamicPtrCast<const OCIO::LogCameraTransform>(gt->getTransform(0));

            std::cout << "Transform type = " << gt->getTransform(0)->getTransformType() << std::endl;

            OCIO_REQUIRE_ASSERT(logAfTf0);

            double values[3];
            logAfTf0->getLogSideSlopeValue(values);
            OCIO_CHECK_CLOSE(values[0], 0.0570776, 0.000001);
        }

        {
            auto mt1 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gt->getTransform(1));
            double mat[] = { 0.0, 0.0, 0.0, 0.0,
                             0.0, 0.0, 0.0, 0.0, 
                             0.0, 0.0, 0.0, 0.0, 
                             0.0, 0.0, 0.0, 0.0 };
            mt1->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 0.6954522413574519);
        }

        {
            auto mt2 = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(gt->getTransform(2));
            double mat[] = { 0.0, 0.0, 0.0, 0.0,
                             0.0, 0.0, 0.0, 0.0, 
                             0.0, 0.0, 0.0, 0.0, 
                             0.0, 0.0, 0.0, 0.0 };
            mt2->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 1.45143931607166);
        }

        {
            double vals[4];
            auto mt3 = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(gt->getTransform(3));
            mt3->getValue(vals);
            OCIO_CHECK_EQUAL(vals[0], 2.2);
        }
    };

    std::istringstream is;
    is.str(CONFIG);
    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));

    OCIO::ConfigRcPtr editableCfg = cfg->createEditableCopy();

    // Make all color spaces suitable for the heuristics inactive.
    // The heuristics don't look at inactive color spaces.
    editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, Texture -- sRGB");

    std::string srcColorSpaceName = "not sRGB";
    std::string biColorSpaceName  = "Utility - Gamma 2.2 - AP1 - Texture";

    {
        // Test throw if no suitable spaces are present.
        std::cout << "Test 1" << std::endl;
        OCIO_CHECK_THROW(auto proc = OCIO::Config::getProcessorToBuiltinColorSpace(
            editableCfg,
            srcColorSpaceName.c_str(),
            biColorSpaceName.c_str()),

            OCIO::Exception
        );
    }

    {
        // Test sRGB Texture space.
        std::cout << "Test 2" << std::endl;
        editableCfg->setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709");
        auto proc = OCIO::Config::getProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  biColorSpaceName.c_str());
        checkProcessor(proc);
    }

    {
        // Test linear color space from_ref direction.
        std::cout << "Test 3" << std::endl;
        editableCfg->setInactiveColorSpaces("ACES cg, Texture -- sRGB");
        auto proc = OCIO::Config::getProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  biColorSpaceName.c_str());
        checkProcessor(proc);
    }

    {
        // Test linear color space to_ref direction.
        std::cout << "Test 4" << std::endl;
        editableCfg->setInactiveColorSpaces("Linear ITU-R BT.709, Texture -- sRGB");
        auto proc = OCIO::Config::getProcessorToBuiltinColorSpace(editableCfg,
                                                                  srcColorSpaceName.c_str(),
                                                                  biColorSpaceName.c_str());
        checkProcessor(proc);
    }
}
