# Copyright (c) 2024 GP Orcullo
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(resource_table.c)

zephyr_linker_sources(SECTIONS resource_table.ld)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
