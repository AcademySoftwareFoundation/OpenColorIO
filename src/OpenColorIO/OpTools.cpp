/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "OpTools.h"

OCIO_NAMESPACE_ENTER
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

        // NB: We assume finalize will set the bit-depth at each op interface
        // to 32f so there is never any quantization to integer.
        // Furthermore, to avoid improper recursion we must never call
        // the renderer with an integer input depth.
        // TODO: Currently the OCIO finalize is hard-coded to 32f.
        //       When that changes we need to update this to request 32f input
        //       and output bit depths.
        FinalizeOpVec(ops);

        for (OpRcPtrVec::size_type i = 0, size = ops.size(); i<size; ++i)
        {
            ops[i]->apply(&tmp[0], numPixels);
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

    const char * GetInvQualityName(LutInversionQuality invStyle)
    {
        switch (invStyle)
        {
        case LUT_INVERSION_EXACT:
        {
            return "exact";
            break;
        }
        case LUT_INVERSION_FAST:
        {
            return "fast";
            break;
        }
        case LUT_INVERSION_DEFAULT:
        {
            return "default";
            break;
        }
        case LUT_INVERSION_BEST:
        {
            return "best";
            break;
        }
        }

        throw Exception("The LUT has an unrecognized inversion quality setting.");
    }
}
OCIO_NAMESPACE_EXIT
