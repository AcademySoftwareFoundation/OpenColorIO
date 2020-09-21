// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>
#include <string>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "SystemMonitor.h"


#ifdef OCIO_HEADLESS_ENABLED

namespace OCIO_NAMESPACE
{

void SystemMonitorsImpl::getAllMonitors()
{
    // A headless machine does not have any monitors.
}


} // namespace OCIO_NAMESPACE

#elif __APPLE__

#include "SystemMonitor_macos.cpp"

#elif _WIN32

#include "SystemMonitor_windows.cpp"

#else // i.e. Linux

namespace OCIO_NAMESPACE
{

void SystemMonitorsImpl::getAllMonitors()
{
    // There are no uniform way to retrieve the monitor information.
    // So, the default list of active monitors is empty.
}

} // namespace OCIO_NAMESPACE

#endif


namespace OCIO_NAMESPACE
{

ConstSystemMonitorsRcPtr SystemMonitors::Get() noexcept
{
    static ConstSystemMonitorsRcPtr monitors;
    static Mutex mutex;

    AutoMutex guard(mutex);
    
    if (!monitors)
    {
        SystemMonitorsRcPtr m = std::make_shared<SystemMonitorsImpl>();
        DynamicPtrCast<SystemMonitorsImpl>(m)->getAllMonitors();
        monitors = m;
    }

    return monitors;
}

bool SystemMonitorsImpl::isSupported() const noexcept
{
    return m_monitors.size() != 0;
}

size_t SystemMonitorsImpl::getNumMonitors() const noexcept
{
    return m_monitors.size();
}

const char * SystemMonitorsImpl::getMonitorName(size_t index) const
{
    if (index >= m_monitors.size())
    {
        std::ostringstream oss;
        oss << "Invalid index for the monitor name " << index
            << " where the number of monitors is " << m_monitors.size() << ".";
        throw Exception(oss.str().c_str());
    }

    return m_monitors[index].m_monitorName.c_str();
}

const char * SystemMonitorsImpl::getProfileFilepath(size_t index) const
{
    if (index >= m_monitors.size())
    {
        std::ostringstream oss;
        oss << "Invalid index for the monitor name " << index
            << " where the number of monitors is " << m_monitors.size() << ".";
        throw Exception(oss.str().c_str());
    }

    return m_monitors[index].m_ICCFilepath.c_str();
}

std::string SystemMonitorsImpl::GetICCProfileFromMonitorName(const char * monitorName)
{
    for (size_t idx = 0; idx < SystemMonitors::Get()->getNumMonitors(); ++idx)
    {
        if (0 == strcmp(SystemMonitors::Get()->getMonitorName(idx), monitorName))
        {
            return SystemMonitors::Get()->getProfileFilepath(idx);
        }
    }

    std::ostringstream oss;
    oss << "The monitor name '" << monitorName << "' does not exist.";
    throw Exception(oss.str().c_str());
}

} // namespace OCIO_NAMESPACE
