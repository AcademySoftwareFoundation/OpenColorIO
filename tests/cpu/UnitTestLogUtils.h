// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_UNITTEST_LOGUTILS_H
#define INCLUDED_OCIO_UNITTEST_LOGUTILS_H

#include <string>

#include "OpenColorABI.h"

namespace OCIO_NAMESPACE
{

// Trap any log message while preserving the original logging settings.
// Note that the mechanism is not thread-safe.
class LogGuard
{
public:
    LogGuard();     // Temporarily sets the level to LOGGING_LEVEL_DEBUG
    explicit LogGuard(LoggingLevel level);
    LogGuard(const LogGuard &) = delete;
    LogGuard & operator=(const LogGuard &) = delete;
    ~LogGuard();

    // Return the output message or null.
    [[nodiscard]] const std::string & output() const;

    void clear();
    [[nodiscard]] bool empty() const;

    [[nodiscard]] bool findAndRemove(const std::string & str) const;
    [[nodiscard]] bool findAllAndRemove(const std::string & sPattern) const;
    void print();
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

bool checkAndMuteSceneLinearRoleError(LogGuard & logGuard);
bool checkAndMuteCompositingLogRoleError(LogGuard & logGuard);
bool checkAndMuteColorTimingRoleError(LogGuard & logGuard);
bool checkAndMuteAcesInterchangeRoleError(LogGuard & logGuard);
bool checkAndMuteDisplayInterchangeRoleError(LogGuard & logGuard);

void muteInactiveColorspaceInfo(LogGuard & logGuard);

bool checkAndMuteWarning(LogGuard & logGuard, const std::string str);
bool checkAndMuteError(LogGuard & logGuard, const std::string str);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_UNITTEST_LOGUTILS_H
