// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef OPENCOLORIO_GPU_HELPERS_H
#define OPENCOLORIO_GPU_HELPERS_H


#include <string>

// FIXME: Duplicate function implemented in `src/OpenColorIO/Platform.h and cpp`.
// Implement a function or class for temporary file creation useable by all tests.
std::string createTempFile(const std::string& fileExt, const std::string& fileContent);


#endif