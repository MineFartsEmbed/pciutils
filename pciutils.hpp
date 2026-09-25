#pragma once

#include <vector>
#include <cstdlib>

#ifdef _WIN32

    #pragma comment(lib, __FILE__ "\\..\\pci.lib")

    #pragma comment(lib, "cfgmgr32.lib")
    #pragma comment(lib, "advapi32.lib")

    #include "win.hpp"

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

    template <typename... Ints>
    inline bool is_off(Ints... hexs) {
        return ((hexs == 0x0000 || hexs == 0xFFFF) && ...);
    }

    using pci_devs = std::vector<pci_dev>;

    inline pci_devs& get_devices() {

        static pci_devs _cached_result = []() {
            pci_devs local_list;

            struct pci_access *pacc = pci_alloc();
            pci_init(pacc);
            pci_scan_bus(pacc);

            for (struct pci_dev *src = pacc->devices; src; src = src->next) {

                pci_fill_info(src, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_PHYS_SLOT | PCI_FILL_IO_FLAGS);

                if (is_off(src->vendor_id, src->device_id)) continue;
                
                pci_dev dst{};
                dst.device_id = src->device_id;
                dst.vendor_id = src->vendor_id;
                dst.device_class = src->device_class;
                
                #ifdef _WIN32
                    dst.slot = GetWindowsPcieSlotInfo(src->bus, src->dev, src->vendor_id, src->device_id);
                #else
                    dst.slot = src->phy_slot ? std::atoi(src->phy_slot) : -1;
                #endif

                if (dst.slot != -1)
                    local_list.push_back(dst);
            }

            pci_cleanup(pacc);
            return local_list;
        }();

        return _cached_result;
    }
}

