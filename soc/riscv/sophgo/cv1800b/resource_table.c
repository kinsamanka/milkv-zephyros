/*
 * Copyright (c) 2024 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <autoconf.h>


struct __attribute__((packed)) resource_table {
    uint32_t ver;
    uint32_t num;
    uint32_t reserved[2];
    uint32_t offset[0];
};

struct __attribute__((packed)) fw_rsc_hdr {
    uint32_t type;
    uint8_t data[0];
};

struct __attribute__((packed)) fw_rsc_carveout {
    uint32_t da;
    uint32_t pa;
    uint32_t len;
    uint32_t flags;
    uint32_t reserved;
    uint8_t name[32];
};

enum fw_resource_type {
    RSC_CARVEOUT        = 0,
    RSC_DEVMEM          = 1,
    RSC_TRACE           = 2,
    RSC_VDEV            = 3,
    RSC_LAST            = 4,
    RSC_VENDOR_START    = 128,
    RSC_VENDOR_END      = 512,
};

struct __attribute__((packed)) remote_resource_table {
    struct resource_table resource_table;
    uint32_t offset[1];
    struct fw_rsc_hdr carve_out;
    struct fw_rsc_carveout carve_out_data;
};

__attribute__ ((section(".resource_table")))
struct remote_resource_table resources = {
    .resource_table = { 1, 1 },
    .offset = { offsetof(struct remote_resource_table, carve_out) },
    .carve_out = { RSC_CARVEOUT },
    .carve_out_data = {
        CONFIG_SRAM_BASE_ADDRESS,
        CONFIG_SRAM_BASE_ADDRESS,
        CONFIG_SRAM_SIZE * 1024,
        0, 0,
        "zephyr"
    },
};

