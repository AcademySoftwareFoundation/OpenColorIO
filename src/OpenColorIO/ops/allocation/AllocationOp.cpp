// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/allocation/AllocationOp.h"
#include "ops/log/LogOp.h"
#include "ops/matrix/MatrixOp.h"

namespace OCIO_NAMESPACE
{

void CreateAllocationOps(OpRcPtrVec & ops,
                         const AllocationData & data,
                         TransformDirection dir)
{
    if(data.allocation == ALLOCATION_UNIFORM)
    {
        double oldmin[4] = { 0.0, 0.0, 0.0, 0.0 };
        double oldmax[4] = { 1.0, 1.0, 1.0, 1.0 };
        double newmin[4] = { 0.0, 0.0, 0.0, 0.0 };
        double newmax[4] = { 1.0, 1.0, 1.0, 1.0 };

        if(data.vars.size() >= 2)
        {
            for(int i=0; i<3; ++i)
            {
                oldmin[i] = data.vars[0];
                oldmax[i] = data.vars[1];
            }
        }

        CreateFitOp(ops,
                    oldmin, oldmax,
                    newmin, newmax,
                    dir);
    }
    else if(data.allocation == ALLOCATION_LG2)
    {
        double oldmin[4] = { -10.0, -10.0, -10.0, 0.0 };
        double oldmax[4] = {   6.0,   6.0,   6.0, 1.0 };
        double newmin[4] = {   0.0,   0.0,   0.0, 0.0 };
        double newmax[4] = {   1.0,   1.0,   1.0, 1.0 };

        if(data.vars.size() >= 2)
        {
            for(int i=0; i<3; ++i)
            {
                oldmin[i] = data.vars[0];
                oldmax[i] = data.vars[1];
            }
        }

        // Log Settings.
        // output = logSlope * log( linSlope * input + linOffset, base ) + logOffset

        double base = 2.0;
        double logSlope[3]  = { 1.0, 1.0, 1.0 };
        double linSlope[3]  = { 1.0, 1.0, 1.0 };
        double linOffset[3] = { 0.0, 0.0, 0.0 };
        double logOffset[3] = { 0.0, 0.0, 0.0 };

        if(data.vars.size() >= 3)
        {
            for(int i=0; i<3; ++i)
            {
                linOffset[i] = data.vars[2];
            }
        }

        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
            CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset, dir);
            CreateFitOp(ops, oldmin, oldmax, newmin, newmax, dir);
            break;
        case TRANSFORM_DIR_INVERSE:
            CreateFitOp(ops, oldmin, oldmax, newmin, newmax, dir);
            CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset, dir);
            break;
        }
    }
    else
    {
        throw Exception("Unsupported Allocation Type.");
    }
}

} // namespace OCIO_NAMESPACE
