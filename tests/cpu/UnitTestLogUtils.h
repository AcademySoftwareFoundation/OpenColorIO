// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_UNITTEST_LOGUTILS_H
#define INCLUDED_OCIO_UNITTEST_LOGUTILS_H

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

// Trap any log message while preserving the original logging settings.
// Note that the mechanism is not thread-safe.
class LogGuard
{
public:
    LogGuard();
    LogGuard(const LogGuard &) = delete;
    LogGuard & operator=(const LogGuard &) = delete;
    ~LogGuard();

    // Return the output message or null.
    const std::string & output() const;

    void clear();
    bool empty() const;

private:
    LoggingLevel m_logLevel;
};

// Utility to mute the logging mechanism so the unit test output is clean.
class MuteLogging
{
public:
    MuteLogging();
    ~MuteLogging();
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_UNITTEST_LOGUTILS_H
