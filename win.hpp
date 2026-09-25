#pragma once

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <string>
#include <cstdio>
#include <vector>
#include <algorithm>

namespace pciutils {

    inline DWORD GetDeviceRegistryPropertyDword(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD propertyId) {
        DWORD value = 0;
        if (SetupDiGetDeviceRegistryPropertyA(
            hDevInfo, 
            pDevInfoData, 
            propertyId, 
            NULL, 
            reinterpret_cast<PBYTE>(&value), 
            sizeof(value), 
            NULL)) {
            return value;
        }
        return 0xFFFFFFFF; // Error indicator
    }

    inline int GetDeviceBus(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData) {
        DWORD busNum = GetDeviceRegistryPropertyDword(hDevInfo, pDevInfoData, SPDRP_BUSNUMBER);
        return (busNum == 0xFFFFFFFF) ? -1 : static_cast<int>(busNum);
    }

    inline int GetDeviceAddress(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData) {
        DWORD address = GetDeviceRegistryPropertyDword(hDevInfo, pDevInfoData, SPDRP_ADDRESS);
        if (address == 0xFFFFFFFF) return -1;
        
        // The high word of SPDRP_ADDRESS contains the Device/Slot number
        return static_cast<int>((address >> 16) & 0x001F); 
    }

    // Algorithmic Normalizer: Discovers the motherboard configuration at runtime 
    inline int NormalizeBiosSlotIndex(int targetUiNumber) {
        if (targetUiNumber <= 0 || targetUiNumber == 0xFFFFFFFF) return -1;

        static const std::vector<int> activeUiNumbers = []() {
            std::vector<int> discoveredNumbers;
            HDEVINFO hDevInfo = SetupDiGetClassDevsA(NULL, "PCI", NULL, DIGCF_ALLCLASSES);
            if (hDevInfo == INVALID_HANDLE_VALUE) return discoveredNumbers;

            SP_DEVINFO_DATA devInfoData;
            devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
                DWORD uiNumber = 0;
                DWORD dataType = 0;
                DWORD bufferSize = sizeof(uiNumber);

                CONFIGRET cr = CM_Get_DevNode_Registry_PropertyA(
                    devInfoData.DevInst, 
                    CM_DRP_UI_NUMBER, 
                    &dataType, 
                    &uiNumber, 
                    &bufferSize, 
                    0
                );

                if (cr == CR_SUCCESS && uiNumber > 0 && uiNumber != 0xFFFFFFFF) {
                    int rawVal = static_cast<int>(uiNumber);
                    if (std::find(discoveredNumbers.begin(), discoveredNumbers.end(), rawVal) == discoveredNumbers.end()) {
                        discoveredNumbers.push_back(rawVal);
                    }
                }
            }
            SetupDiDestroyDeviceInfoList(hDevInfo);
            std::sort(discoveredNumbers.begin(), discoveredNumbers.end());
            return discoveredNumbers;
        }();

        if (activeUiNumbers.empty()) return -1;

        // Match against the master list to get an unshifted, true position index
        auto it = std::find(activeUiNumbers.begin(), activeUiNumbers.end(), targetUiNumber);
        if (it != activeUiNumbers.end()) {
            return static_cast<int>(std::distance(activeUiNumbers.begin(), it));
        }

        return -1;
    }

    inline int GetWindowsPcieSlotInfo(int bus, int dev, int vendor, int device) {
        HDEVINFO hDevInfo = SetupDiGetClassDevsA(NULL, "PCI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
        if (hDevInfo == INVALID_HANDLE_VALUE) return -1;

        int finalSlotNumber = -1;
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
            int currentBus = GetDeviceBus(hDevInfo, &devInfoData);
            int currentDev = GetDeviceAddress(hDevInfo, &devInfoData);

            if (currentBus == -1  ||
                currentBus != bus ||
                currentDev == -1  ||
                currentDev != dev
            ) continue;

            // Target matched. Bubble up to find the parent bridge's physical identifier
            DEVINST currentDevInst = devInfoData.DevInst;
            DWORD uiNumber = 0;
            DWORD dataType = 0;
            DWORD bufferSize = sizeof(uiNumber);

            while (currentDevInst != 0) {
                CONFIGRET cr = CM_Get_DevNode_Registry_PropertyA(
                    currentDevInst, 
                    CM_DRP_UI_NUMBER, 
                    &dataType, 
                    &uiNumber, 
                    &bufferSize, 
                    0
                );

                if (cr == CR_SUCCESS && uiNumber > 0 && uiNumber != 0xFFFFFFFF) {
                    finalSlotNumber = NormalizeBiosSlotIndex(static_cast<int>(uiNumber));
                    break;
                }

                DEVINST parentDevInst = 0;
                if (CM_Get_Parent(&parentDevInst, currentDevInst, 0) != CR_SUCCESS)
                    break; 
                currentDevInst = parentDevInst;
                bufferSize = sizeof(uiNumber);
            }
            break; 
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
        return finalSlotNumber;
    }

}
