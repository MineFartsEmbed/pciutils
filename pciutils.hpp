#pragma once

#ifdef _WIN32

    #pragma comment(lib, __FILE__ "\\..\\pci.lib")

    #pragma comment(lib, "cfgmgr32.lib")
    #pragma comment(lib, "advapi32.lib")

#endif

#include "pci.h"

namespace pciutils {

    // --- Enums ---
    using ::pci_access_type;
    using ::pci_lookup_mode;

    // --- Structures ---
    using ::pci_access;
    using ::pci_param;
    using ::pci_dev;
    using ::pci_msi_routing;
    using ::pci_cap;
    using ::pci_filter;

    // --- Functions ---
    using ::pci_alloc;
    using ::pci_init;
    using ::pci_cleanup;
    using ::pci_scan_bus;
    using ::pci_get_dev;
    using ::pci_free_dev;
    using ::pci_lookup_method;
    using ::pci_get_method_name;
    using ::pci_get_param;
    using ::pci_set_param;
    using ::pci_walk_params;
    using ::pci_read_byte;
    using ::pci_read_word;
    using ::pci_read_long;
    using ::pci_read_vpd;
    using ::pci_write_byte;
    using ::pci_write_word;
    using ::pci_write_long;
    using ::pci_read_block;
    using ::pci_write_block;
    using ::pci_fill_info;
    using ::pci_get_string_property;
    using ::pci_setup_cache;
    using ::pci_find_cap;
    using ::pci_find_cap_nr;
    using ::pci_filter_init;
    using ::pci_filter_parse_slot;
    using ::pci_filter_parse_id;
    using ::pci_filter_match;
    using ::pci_filter_has_slot;
    using ::pci_filter_has_id;
    using ::pci_lookup_name;
    using ::pci_load_name_list;
    using ::pci_free_name_list;
    using ::pci_set_name_list_path;
    using ::pci_id_cache_flush;

    using pci_devs = std::vector<pci_dev*>;

    pci_devs get_devices() {
        pci_devs devices;

        struct pci_access *pacc = pci_alloc();
        pci_init(pacc);
        pci_scan_bus(pacc);

        for (struct pci_dev *dev = pacc->devices; dev; dev = dev->next) {
            if (dev->func != 0) continue;

            pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_PHYS_SLOT);

            dev->slot = dev->phy_slot ? std::atoi(dev->phy_slot) : -1;

            devices.push_back(dev);
        }

        pci_cleanup(pacc);

        return devices;
    }

}

