// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GAMMAOPUTILS_H
#define INCLUDED_OCIO_GAMMAOPUTILS_H


#include <OpenColorIO/OpenColorIO.h>

#include "ops/gamma/GammaOpData.h"


namespace OCIO_NAMESPACE
{

struct RendererParams
{
    RendererParams()
        :   gamma(1.0f)
        ,   offset(0.0f)
        ,   breakPnt(0.0f)
        ,   slope(1.0f)
        ,   scale(1.0f)
    { }

    float gamma;
    float offset;
    float breakPnt;
    float slope;
    float scale;
};


// Utility function to compute the rendering coefficients for the forward 
// monitor curve gamma op.
void ComputeParamsFwd(const GammaOpData::Params & gParams,
                      RendererParams & rParams);

// Utility function to compute the rendering coefficients for the reverse 
// monitor curve gamma op.
void ComputeParamsRev(const GammaOpData::Params & gParams,
                      RendererParams & rParams);

} // namespace OCIO_NAMESPACE


#endif