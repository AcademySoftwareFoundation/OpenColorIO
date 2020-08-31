// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_SYSTEM_MONITOR_H
#define INCLUDED_OCIO_SYSTEM_MONITOR_H


#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class SystemMonitorsImpl : public SystemMonitors
{
public:
    SystemMonitorsImpl() = default;
    SystemMonitorsImpl(const SystemMonitorsImpl &) = delete;
    SystemMonitorsImpl & operator= (const SystemMonitorsImpl &) = delete;
    virtual ~SystemMonitorsImpl() = default;

    bool isSupported() const noexcept override;

    size_t getNumMonitors() const noexcept override;
    const char * getMonitorName(size_t idx) const override;
    const char * getProfileFilepath(size_t idx) const override;

    static std::string GetICCProfileFromMonitorName(const char * monitorName);

    void getAllMonitors();

private:
    struct MonitorInfo
    {
        MonitorInfo() = default;
        MonitorInfo(const std::string & monitorName, const std::string & ICCFilepath)
            :   m_monitorName(monitorName)
            ,   m_ICCFilepath(ICCFilepath)
        {}
        ~MonitorInfo() = default;

        std::string m_monitorName; // Name built using the vendor information from the monitor if accessible.
        std::string m_ICCFilepath; // The ICC profile path.
    };

    std::vector<MonitorInfo> m_monitors;
};


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_SYSTEM_MONITOR_H
