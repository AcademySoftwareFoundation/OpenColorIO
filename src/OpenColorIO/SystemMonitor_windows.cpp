// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#if !defined(_WIN32)

#error The file is for the Windows platform only.

#endif

#include <windows.h>

#include <Logging.h>


namespace OCIO_NAMESPACE
{


static constexpr char ErrorMsg[] { "Problem obtaining monitor profile information from operating system." };


void SystemMonitorsImpl::getAllMonitors()
{
    m_monitors.clear();

    // Initialize the structure.
    DISPLAY_DEVICE dispDevice;
    ZeroMemory(&dispDevice, sizeof(dispDevice));
    dispDevice.cb = sizeof(dispDevice);

    // Iterate over all the monitors.
    DWORD dispNum = 0;
    while (EnumDisplayDevices(nullptr, dispNum, &dispDevice, 0))
    {
        const std::string deviceName = dispDevice.DeviceName;

        // Only select active monitors.
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

                char icmPath[MAX_PATH + 1];
                DWORD pathLength = MAX_PATH;

                // TODO: Is a monitor without ICM profile possible?

                // TODO: Several ICM profiles could be associated to a single device.

                const std::string displayName 
                    = deviceName + ", " + dispDevice.DeviceString;

                // Get the associated ICM profile path.
                if (GetICMProfile(hDC, &pathLength, icmPath))
                {
                    m_monitors.push_back({displayName, icmPath});
                }
                else
                {
                    std::ostringstream oss;
                    oss << "Unable to access the ICM profile for the monitor '" 
                        << displayName << "'.";

                    LogDebug(oss.str());
                }

                DeleteDC(hDC);
            }
            else
            {
                std::ostringstream oss;
                oss << "Unable to access the monitor '" << deviceName << "'.";
                LogDebug(oss.str());
            }
        }

        ZeroMemory(&dispDevice, sizeof(dispDevice));
        dispDevice.cb = sizeof(dispDevice);

        ++dispNum;
    }
}


} // namespace OCIO_NAMESPACE
