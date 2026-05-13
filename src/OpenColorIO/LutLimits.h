// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUTLIMITS_H
#define INCLUDED_OCIO_LUTLIMITS_H

namespace OCIO_NAMESPACE
{

// Maximum number of entries supported in a 1D LUT.
constexpr unsigned long Max1DLUTLength = 300000;

// Maximum grid size supported for a 3D LUT.
// 129 allows for a MESH dimension of 7 in the 3dl file format.
constexpr unsigned long Max3DLUTLength = 129;

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_LUTLIMITS_H
