// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/noop/NoOps.cpp"

#include "ops/lut1d/Lut1DOp.h"
#include "ops/matrix/MatrixOp.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{

void CreateGenericAllocationOp(OCIO::OpRcPtrVec & ops)
{
    OCIO::AllocationData srcAllocation;
    srcAllocation.allocation = OCIO::ALLOCATION_LG2;
    srcAllocation.vars.push_back(-8.0f);
    srcAllocation.vars.push_back(8.0f);
    OCIO::CreateGpuAllocationNoOp(ops, srcAllocation);
}

void CreateGenericScaleOp(OCIO::OpRcPtrVec & ops)
{
    const double scale4[4] = { 1.04, 1.05, 1.06, 1.0 };
    OCIO::CreateScaleOp(ops, scale4, OCIO::TRANSFORM_DIR_FORWARD);
}

void CreateGenericLutOp(OCIO::OpRcPtrVec & ops)
{
    // Make a LUT that squares the input.
    const unsigned long size = 256;
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(size);
    auto & lutArray = lut->getArray();

    for(unsigned long i = 0; i < size; ++i)
    {
        const float x = (float)i / (float)(size-1);
        const float x2 = x*x;

        for(int c=0; c<3; ++c)
        {
            lutArray[3 * i +  c] = x2;
        }
    }

    OCIO::CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD);
}

void AssertPartitionIntegrity(OCIO::OpRcPtrVec & gpuPreOps,
                              OCIO::OpRcPtrVec & gpuLatticeOps,
                              OCIO::OpRcPtrVec & gpuPostOps)
{
    // All gpu pre ops must support analytical gpu shader generation.
    for(unsigned int i=0; i<gpuPreOps.size(); ++i)
    {
        if(!gpuPreOps[i]->supportedByLegacyShader())
        {
            throw OCIO::Exception("Partition failed check. One gpuPreOps op does not support GPU.");
        }
    }

    // If there are any lattice ops, at lease one must NOT support GPU
    // shaders (otherwise this block isnt necessary!).
    if(gpuLatticeOps.size()>0)
    {
        bool requiresLattice = false;
        for(unsigned int i=0; i<gpuLatticeOps.size() && !requiresLattice; ++i)
        {
            if (!gpuLatticeOps[i]->supportedByLegacyShader())
            {
                requiresLattice = true;
            }
        }

        if(!requiresLattice)
        {
            throw OCIO::Exception("Partition failed check. All gpuLatticeOps ops do support GPU.");
        }
    }

    // All gpu post ops must support analytical gpu shader generation.
    for(unsigned int i=0; i<gpuPostOps.size(); ++i)
    {
        if(!gpuPostOps[i]->supportedByLegacyShader())
        {
            throw OCIO::Exception("Partition failed check. One gpuPostOps op does not support GPU.");
        }
    }
}

} // anon.

OCIO_ADD_TEST(NoOps, partition_gpu_ops)
{
    {
    OCIO::OpRcPtrVec ops;

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        OCIO::PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_CHECK_EQUAL(gpuPreOps.size(), 0);
    OCIO_CHECK_EQUAL(gpuLatticeOps.size(), 0);
    OCIO_CHECK_EQUAL(gpuPostOps.size(), 0);

    OCIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }

    {
    OCIO::OpRcPtrVec ops;
    CreateGenericAllocationOp(ops);
    OCIO_CHECK_EQUAL(ops.size(), 1);

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        OCIO::PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_REQUIRE_EQUAL(gpuPreOps.size(), 1);
    OCIO_CHECK_EQUAL(gpuLatticeOps.size(), 0);
    OCIO_CHECK_EQUAL(gpuPostOps.size(), 0);

    OCIO::ConstOpRcPtr op0 = gpuPreOps[0];

    OCIO_CHECK_EQUAL(ops[0]->isSameType(op0), true);

    OCIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }

    {
    OCIO::OpRcPtrVec ops;

    CreateGenericAllocationOp(ops);
    CreateGenericScaleOp(ops);
    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        OCIO::PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_CHECK_EQUAL(gpuPreOps.size(), 2);
    OCIO_CHECK_EQUAL(gpuLatticeOps.size(), 0);
    OCIO_CHECK_EQUAL(gpuPostOps.size(), 0);

    OCIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }

    {
    OCIO::OpRcPtrVec ops;

    CreateGenericAllocationOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    OCIO_CHECK_EQUAL(ops.size(), 3);

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        OCIO::PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_CHECK_EQUAL(gpuPreOps.size(), 2);
    OCIO_CHECK_EQUAL(gpuLatticeOps.size(), 4);
    OCIO_CHECK_EQUAL(gpuPostOps.size(), 1);

    OCIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }

    {
    OCIO::OpRcPtrVec ops;

    CreateGenericLutOp(ops);
    OCIO_CHECK_EQUAL(ops.size(), 1);

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        OCIO::PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_CHECK_EQUAL(gpuPreOps.size(), 0);
    OCIO_CHECK_EQUAL(gpuLatticeOps.size(), 1);
    OCIO_CHECK_EQUAL(gpuPostOps.size(), 0);

    OCIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }

    {
    OCIO::OpRcPtrVec ops;

    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    OCIO_CHECK_EQUAL(ops.size(), 6);

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        OCIO::PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_CHECK_EQUAL(gpuPreOps.size(), 0);
    OCIO_CHECK_EQUAL(gpuLatticeOps.size(), 4);
    OCIO_CHECK_EQUAL(gpuPostOps.size(), 2);

    OCIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }

    {
    OCIO::OpRcPtrVec ops;

    CreateGenericAllocationOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    OCIO_CHECK_EQUAL(ops.size(), 8);

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        OCIO::PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_CHECK_EQUAL(gpuPreOps.size(), 2);
    OCIO_CHECK_EQUAL(gpuLatticeOps.size(), 8);
    OCIO_CHECK_EQUAL(gpuPostOps.size(), 2);

    OCIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    /*
    std::cerr << "gpuPreOps" << std::endl;
    std::cerr << SerializeOpVec(gpuPreOps, 4) << std::endl;
    std::cerr << "gpuLatticeOps" << std::endl;
    std::cerr << SerializeOpVec(gpuLatticeOps, 4) << std::endl;
    std::cerr << "gpuPostOps" << std::endl;
    std::cerr << SerializeOpVec(gpuPostOps, 4) << std::endl;
    */
    }
} // PartitionGPUOps

OCIO_ADD_TEST(NoOps, throw)
{
    // PartitionGPUOps might throw, but could not find how

    OCIO::OpRcPtrVec ops;

    CreateGenericAllocationOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);

    OCIO::OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    OCIO_CHECK_NO_THROW(
        PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops));

    OCIO_CHECK_THROW_WHAT(AssertPartitionIntegrity(
        gpuLatticeOps, gpuLatticeOps, gpuPostOps),
        OCIO::Exception, "One gpuPreOps op does not support GPU");

    OCIO_CHECK_THROW_WHAT(AssertPartitionIntegrity(
        gpuPreOps, gpuPreOps, gpuPostOps),
        OCIO::Exception, "All gpuLatticeOps ops do support GPU");

    OCIO_CHECK_THROW_WHAT(AssertPartitionIntegrity(
        gpuPreOps, gpuLatticeOps, gpuLatticeOps),
        OCIO::Exception, "One gpuPostOps op does not support GPU");

}

OCIO_ADD_TEST(NoOps, allocation_op)
{
    OCIO::OpRcPtrVec ops;
    CreateGenericAllocationOp(ops);
    CreateGenericScaleOp(ops);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::OpRcPtr clonedOp = ops[0]->clone();

    OCIO_CHECK_EQUAL(clonedOp->isSameType(op0), true);
    OCIO_CHECK_EQUAL(clonedOp->isSameType(op1), false);
    OCIO_CHECK_EQUAL(clonedOp->isInverse(op0), true);
    OCIO_CHECK_EQUAL(clonedOp->isInverse(op1), false);

    OCIO_CHECK_EQUAL(clonedOp->isNoOp(), true);
    OCIO_CHECK_EQUAL(clonedOp->hasChannelCrosstalk(), false);
    OCIO_CHECK_EQUAL(clonedOp->supportedByLegacyShader(), true);
}

OCIO_ADD_TEST(NoOps, file_op)
{
    OCIO::OpRcPtrVec ops;
    CreateFileNoOp(ops,"");
    CreateGenericAllocationOp(ops);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::OpRcPtr clonedOp = ops[0]->clone();

    OCIO_CHECK_EQUAL(clonedOp->isSameType(op0), true);
    OCIO_CHECK_EQUAL(clonedOp->isSameType(op1), false);
    OCIO_CHECK_EQUAL(clonedOp->isInverse(op0), true);
    OCIO_CHECK_EQUAL(clonedOp->isInverse(op1), false);

    OCIO_CHECK_EQUAL(clonedOp->isNoOp(), true);
    OCIO_CHECK_EQUAL(clonedOp->hasChannelCrosstalk(), false);
    OCIO_CHECK_EQUAL(clonedOp->supportedByLegacyShader(), true);
}

OCIO_ADD_TEST(NoOps, look_op)
{
    OCIO::OpRcPtrVec ops;
    CreateLookNoOp(ops, "");
    CreateGenericAllocationOp(ops);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::OpRcPtr clonedOp = ops[0]->clone();

    OCIO_CHECK_EQUAL(clonedOp->isSameType(op0), true);
    OCIO_CHECK_EQUAL(clonedOp->isSameType(op1), false);
    OCIO_CHECK_EQUAL(clonedOp->isInverse(op0), true);
    OCIO_CHECK_EQUAL(clonedOp->isInverse(op1), false);

    OCIO_CHECK_EQUAL(clonedOp->isNoOp(), true);
    OCIO_CHECK_EQUAL(clonedOp->hasChannelCrosstalk(), false);
    OCIO_CHECK_EQUAL(clonedOp->supportedByLegacyShader(), true);
}

