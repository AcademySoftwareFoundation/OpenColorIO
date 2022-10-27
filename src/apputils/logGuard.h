// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_LOG_GUARD_H
#define INCLUDED_LOG_GUARD_H

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

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
    OCIO::LoggingLevel m_logLevel;
};

#endif