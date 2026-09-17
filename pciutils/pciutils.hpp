#pragma once

#include <iostream>

extern "C" {
    #include "pciutils/pci.h"
}

// 1. Tell Visual Studio to link your custom static library relative to this header file
#pragma comment(lib, __FILE__ "\\..\\pci.lib")

// 2. Force link the missing Windows OS modules needed by pciutils 
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "advapi32.lib")
