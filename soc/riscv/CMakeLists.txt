# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${ZEPHYR_BASE}/soc/riscv/common ${CMAKE_CURRENT_BINARY_DIR}/_)

if(SOC_FAMILY)
  add_subdirectory(${SOC_FAMILY})
else()
  add_subdirectory(${SOC_NAME})
endif()
