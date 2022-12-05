// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#if !defined(_WIN32)

#error The file is for the Windows platform only.

#endif

#include <windows.h>

#include <Logging.h>

#include "Platform.h"
#include "utils/StringUtils.h"

#include <iostream>

namespace OCIO_NAMESPACE
{


static constexpr char ErrorMsg[] { "Problem obtaining monitor profile information from operating system." };

void getAllMonitorsWithQueryDisplayConfig(std::vector<std::tstring> & monitorsName)
{
    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
    LONG result = ERROR_SUCCESS;

    do
    {
        // Determine how many path and mode structures to allocate
        UINT32 pathCount, modeCount;
        result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);
        
        // Allocate the path and mode arrays
        paths.resize(pathCount);
        modes.resize(modeCount);

        // Get all active paths and their modes
        result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        // The function may have returned fewer paths/modes than estimated
        paths.resize(pathCount);
        modes.resize(modeCount);

        // It's possible that between the call to GetDisplayConfigBufferSizes and QueryDisplayConfig
        // that the display state changed, so loop on the case of ERROR_INSUFFICIENT_BUFFER.
    } while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result == ERROR_SUCCESS)
    {
        // For each active path
        for (auto& path : paths)
        {
            // Find the target (monitor) friendly name
            DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
            targetName.header.adapterId = path.targetInfo.adapterId;
            targetName.header.id = path.targetInfo.id;
            targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
            targetName.header.size = sizeof(targetName);
            result = DisplayConfigGetDeviceInfo(&targetName.header);

            if (result == ERROR_SUCCESS)
            {
                monitorsName.push_back(
                    (result == ERROR_SUCCESS && targetName.flags.friendlyNameFromEdid) ? 
                    targetName.monitorFriendlyDeviceName : L""
                );
            }
        }
    }
}

void SystemMonitorsImpl::getAllMonitors()
{
    m_monitors.clear();

    std::vector<std::tstring> friendlyMonitorNames;
    getAllMonitorsWithQueryDisplayConfig(friendlyMonitorNames);

    // Initialize the structure.
    DISPLAY_DEVICE dispDevice;
    ZeroMemory(&dispDevice, sizeof(dispDevice));
    dispDevice.cb = sizeof(dispDevice);

    // Iterate over all the monitors.
    DWORD dispNum = 0;
    while (EnumDisplayDevices(nullptr, dispNum, &dispDevice, 0))
    {
        const std::tstring deviceName = dispDevice.DeviceName;

        // Only select active monitors. 
        // NOTE: Currently the two DISPLAY enums are equivalent, but we check both in case one may 
        // change in the future.
        if ((dispDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)
            && (dispDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            HDC hDC = CreateDC(nullptr, deviceName.c_str(), nullptr, nullptr);
            if (hDC)
            {
                ZeroMemory(&dispDevice, sizeof(dispDevice));
                dispDevice.cb = sizeof(dispDevice);

                // After a second call, dispDev.DeviceString contains the  
                // monitor name for that device.
                EnumDisplayDevices(deviceName.c_str(), dispNum, &dispDevice, 0);   

                TCHAR icmPath[MAX_PATH + 1];
                DWORD pathLength = MAX_PATH;

                // TODO: Is a monitor without ICM profile possible?

                // TODO: Several ICM profiles could be associated to a single device.

                const std::tstring extra = 
                    (friendlyMonitorNames.size() >= dispNum+1 && 
                    !friendlyMonitorNames.at(dispNum).empty()) ? 
                        friendlyMonitorNames.at(dispNum) : std::tstring(dispDevice.DeviceString);
                        
                const std::tstring displayName = deviceName + TEXT(", ") + extra;

                // Get the associated ICM profile path.
                if (GetICMProfile(hDC, &pathLength, icmPath))
                {
#ifdef _UNICODE
                    m_monitors.push_back({Platform::Utf16ToUtf8(displayName), Platform::Utf16ToUtf8(icmPath)});
#else
                    m_monitors.push_back({displayName, icmPath});
#endif
                }
                else
                {
                    std::tostringstream oss;
                    oss << TEXT("Unable to access the ICM profile for the monitor '")
                        << displayName << TEXT("'.");

                    LogDebugT(oss.str());
                }

                DeleteDC(hDC);
            }
            else
            {
                std::tostringstream oss;
                oss << TEXT("Unable to access the monitor '") << deviceName << TEXT("'.");
                LogDebugT(oss.str());
            }
        }

        ZeroMemory(&dispDevice, sizeof(dispDevice));
        dispDevice.cb = sizeof(dispDevice);

        ++dispNum;
    }
}

} // namespace OCIO_NAMESPACE
