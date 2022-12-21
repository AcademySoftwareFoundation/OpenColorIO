// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#if !defined(_WIN32)

#error The file is for the Windows platform only.

#endif

#include <windows.h>

#include <Logging.h>

#include "Platform.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{


static constexpr char ErrorMsg[] { "Problem obtaining monitor profile information from operating system." };

// List all active display paths using QueryDisplayConfig and GetDisplayConfigBufferSizes.
// Get the data from each path using DisplayConfigGetDeviceInfo.
void getAllMonitorsWithQueryDisplayConfig(std::vector<std::tstring> & monitorsName)
{
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_path_info
    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_mode_info
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;

    UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
    LONG result = ERROR_SUCCESS;

    do
    {
        // Determine how many path and mode structures to allocate.
        UINT32 pathCount, modeCount;
        // The GetDisplayConfigBufferSizes function retrieves the size of the buffers that are 
        // required to call the QueryDisplayConfig function.
        result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);
        
        // Allocate the path and mode arrays.
        paths.resize(pathCount);
        modes.resize(modeCount);

        // The QueryDisplayConfig function retrieves information about all possible display paths 
        // for all display devices, or views, in the current setting.
        result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        // The function may have returned fewer paths/modes than estimated.
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
            // The DISPLAYCONFIG_TARGET_DEVICE_NAME structure contains information about the target.
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

/**
 * Populate the internal structure with monitors name and ICC profiles name.
 * 
 * Expected monitor display name: 
 * 
 * DISPLAYn, <monitorFriendlyDeviceName | DeviceString>
 * 
 * where n is a positive integer starting at 1.
 * where monitorFriendlyDeviceName comes from DISPLAYCONFIG_TARGET_DEVICE_NAME structure.
 * where DeviceString comes from DISPLAY_DEVICE structure.
 * 
 */
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
    // After the first call to EnumDisplayDevices, dispDevice.DeviceString is the adapter name.
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

                // After second call, dispDevice.DeviceString is the monitor name for that device.
                // Second parameters must be 0 to get the monitor name.
                // See https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdisplaydevicesw
                EnumDisplayDevices(deviceName.c_str(), 0, &dispDevice, 0);   

                TCHAR icmPath[MAX_PATH + 1];
                DWORD pathLength = MAX_PATH;

                // TODO: Is a monitor without ICM profile possible?

                // TODO: Several ICM profiles could be associated to a single device.

                bool idxExists = friendlyMonitorNames.size() >= dispNum+1;
                bool friendlyNameExists = idxExists && !friendlyMonitorNames.at(dispNum).empty();

                // Check if the distNum index exists in friendlyMonitorNames vector and check if
                // there is a corresponding friendly name.
                const std::tstring extra = friendlyNameExists ? 
                        friendlyMonitorNames.at(dispNum) : std::tstring(dispDevice.DeviceString);

                std::tstring strippedDeviceName = deviceName;
                if(StringUtils::StartsWith(Platform::Utf16ToUtf8(deviceName), "\\\\.\\DISPLAY"))
                {
                    // Remove the slashes.
                    std::string prefix = "\\\\.\\";
                    strippedDeviceName = deviceName.substr(prefix.length());
                }
                        
                const std::tstring displayName = strippedDeviceName + TEXT(", ") + extra;

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
