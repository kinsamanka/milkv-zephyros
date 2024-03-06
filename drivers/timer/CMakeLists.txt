# Copyright (c) 2024 GP Orcullo
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SOPHGO_CV1800B_CLINT riscv_machine_timer.c)
