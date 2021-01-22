// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYSYSTEMMONITORS_H
#define INCLUDED_OCIO_PYSYSTEMMONITORS_H

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

// Wrapper to preserve the SystemMonitors singleton.
class OCIOHIDDEN PySystemMonitors
{
public:
    PySystemMonitors() = default;
    ~PySystemMonitors() = default;

    size_t getNumMonitors() const noexcept
    {
        return SystemMonitors::Get()->getNumMonitors();
    }

    const char * getMonitorName(size_t idx) const
    {
        return SystemMonitors::Get()->getMonitorName(idx);
    }

    const char * getProfileFilepath(size_t idx) const
    {
        return SystemMonitors::Get()->getProfileFilepath(idx);
    }
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYSYSTEMMONITORS_H
