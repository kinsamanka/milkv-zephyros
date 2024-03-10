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

struct __attribute__((packed)) remote_resource_table {
	struct resource_table resource_table;
	uint32_t offset[1];
};

__attribute__ ((section(".resource_table")))
struct remote_resource_table resources = {
	.resource_table = { 1, 0 },
	.offset = { 0 },
};

