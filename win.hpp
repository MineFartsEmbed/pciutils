#pragma once

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>

namespace pciutils {

    inline int getUINumber(DEVINST currentInstance) {

        ULONG uiNumber = 0;
        ULONG dataType = 0;
        ULONG bufferSize = sizeof(uiNumber);

        CONFIGRET cr = CM_Get_DevNode_Registry_PropertyA(
            currentInstance, 
            CM_DRP_UI_NUMBER,
            &dataType,
            &uiNumber,
            &bufferSize,
            0
        );

        if (cr == CR_SUCCESS && uiNumber != 0xFFFFFFFF) {
            return static_cast<int>(uiNumber);
        } else return -1;

    }

    inline int getDevRegProp(
        HDEVINFO hDevInfo, 
        PSP_DEVINFO_DATA pDevInfoData, 
        DWORD propertyId
    ) {
        DWORD value = 0;
        BOOL prop = SetupDiGetDeviceRegistryPropertyA(
            hDevInfo, 
            pDevInfoData, 
            propertyId, 
            NULL, 
            reinterpret_cast<PBYTE>(&value), 
            sizeof(value), 
            NULL
        );
        return prop ? static_cast<int>(value) : -1;
    }

    inline int getBusNumber(
        HDEVINFO hDevInfo,
        PSP_DEVINFO_DATA pDevInfoData
    ) {
        return getDevRegProp(hDevInfo, pDevInfoData, SPDRP_BUSNUMBER);
    }

    inline int getDevNumber(
        HDEVINFO hDevInfo,
        PSP_DEVINFO_DATA pDevInfoData
    ) {
        int address = getDevRegProp(hDevInfo, pDevInfoData, SPDRP_ADDRESS);
        return (address == -1) 
            ? -1
            : (address >> 16) & 0xFFFF;
    }

    inline int GetWindowsPcieSlotInfo(int bus, int dev) {

        HDEVINFO hDevInfo = SetupDiGetClassDevsA(
            NULL, "PCI", NULL, 
            DIGCF_PRESENT | DIGCF_ALLCLASSES
        );

        int finalSlotNumber = -1;
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {

            if (bus != getBusNumber(hDevInfo, &devInfoData) ||
                dev != getDevNumber(hDevInfo, &devInfoData)
            ) continue;

            DEVINST currentInstance = devInfoData.DevInst;
            int depth = 0;

            while (depth < 4) {

                finalSlotNumber = getUINumber(currentInstance);
                if (finalSlotNumber != -1) goto end_func;

                if (CM_Get_Parent(&currentInstance, currentInstance, 0) != CR_SUCCESS)
                    break;
                
                depth++;
            }

        }

        end_func:
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return finalSlotNumber;
    }

}
