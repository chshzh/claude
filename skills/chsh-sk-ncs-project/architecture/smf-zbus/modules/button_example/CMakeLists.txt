#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add button module source files to app target
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/button_example.c)

# Add module directory to include path
target_include_directories(app PRIVATE .)
