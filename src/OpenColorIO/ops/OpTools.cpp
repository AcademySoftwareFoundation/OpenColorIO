// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ops/OpTools.h"

namespace OCIO_NAMESPACE
{
void EvalTransform(const float * in,
                    float * out,
                    long numPixels,
                    OpRcPtrVec & ops)
{
    std::vector<float> tmp(numPixels * 4);

    // Render the LUT entries (domain) through the ops.
    const float * values = in;
    for (long idx = 0; idx<numPixels; ++idx)
    {
        tmp[4 * idx + 0] = values[0];
        tmp[4 * idx + 1] = values[1];
        tmp[4 * idx + 2] = values[2];
        tmp[4 * idx + 3] = 1.0f;

        values += 3;
    }

    ops.finalize(OPTIMIZATION_NONE);

    for (OpRcPtrVec::size_type i = 0, size = ops.size(); i<size; ++i)
    {
        ops[i]->apply(&tmp[0], &tmp[0], numPixels);
    }

    float * result = out;
    for (long idx = 0; idx<numPixels; ++idx)
    {
        result[0] = tmp[4 * idx + 0];
        result[1] = tmp[4 * idx + 1];
        result[2] = tmp[4 * idx + 2];

        result += 3;
    }
}
} // namespace OCIO_NAMESPACE
