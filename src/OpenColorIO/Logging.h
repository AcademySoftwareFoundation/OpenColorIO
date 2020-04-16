// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOGGING_H
#define INCLUDED_OCIO_LOGGING_H

#include <OpenColorIO/OpenColorIO.h>

#include <string>

namespace OCIO_NAMESPACE
{
void LogWarning(const std::string & text);
void LogInfo(const std::string & text);
void LogDebug(const std::string & text);

bool IsDebugLoggingEnabled();

} // namespace OCIO_NAMESPACE

#endif
