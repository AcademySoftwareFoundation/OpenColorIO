// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_MATHHALFUTILS_H
#define INCLUDED_OCIO_MATHHALFUTILS_H

#include <OpenColorABI.h>

#include <Imath/half.h>

namespace OCIO_NAMESPACE
{
// Compares half-floats as raw integers with a tolerance (essentially in ULPs).
// Returns true if the integer difference is strictly greater than the tolerance.
// If aimHalf is a NaN, valHalf must also be one of the NaNs.
// Inf is treated like any other value (diff from HALFMAX is 1).
bool HalfsDiffer(const half expected, const half actual, const int tolerance);

} // namespace OCIO_NAMESPACE

#endif
