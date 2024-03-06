# Copyright (c) 2024 GP Orcullo
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD milkv_duo)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
list(APPEND SOC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../drivers)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(blinky)

target_sources(app PRIVATE src/main.c)
