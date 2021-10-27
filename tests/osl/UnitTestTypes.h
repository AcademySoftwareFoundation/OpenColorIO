// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef OPENCOLORIO_OSL_UNITTEST_TYPES_H
#define OPENCOLORIO_OSL_UNITTEST_TYPES_H

#include <array>
#include <string>
#include <vector>

// Utility type to mimic the OSL::Vec4
using Vec4 = std::array<float, 4>;

// Utility to hold an image to process.
using Image = std::vector<Vec4>;


#endif /* OPENCOLORIO_OSL_UNITTEST_TYPES_H */
