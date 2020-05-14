// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_TRANSFORM_HELPERS_H
#define INCLUDED_OCIO_TRANSFORM_HELPERS_H


#include <functional>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

// Linearly interpolate a single input value through a non-uniformly spaced LUT.
// The lutValues are ordered as: [in0, out0, in1, out1, in2, out2, ...].
// The lutSize is the number of in/out pairs, so there are 2*lutSize lutValues.
double Interpolate1D(unsigned lutSize, const double * lutValues, double in);

// Create a LUT 1D transform where values are the same for the three color components.
// The input values are linearly spaced on [0,1].  The output values from the functor
// should be nominally [0,1], although they may exceed that if needed.
void CreateLut(OpRcPtrVec & ops,
               unsigned long lutDimension,
               std::function<float(double)> lutValueGenerator);

// Create a LUT 1D transform where values may be different for the three color components.
// The first argument is an array of RGB inputs to the functor, the second argument is for
// the RGB outputs.
void CreateLut(OpRcPtrVec & ops,
               unsigned long lutDimension,
               std::function<void(const double *, double *)> lutValueGenerator);

// The input values to the functor are all possible values of a half-float.
// However NaNs are mapped to 0 and +/-Inf is mapped to +/-HALF_MAX.
void CreateHalfLut(OpRcPtrVec & ops,
                   std::function<float(double)> lutValueGenerator);

} // namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_TRANSFORM_HELPERS_H
