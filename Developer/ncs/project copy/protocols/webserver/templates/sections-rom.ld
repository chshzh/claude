/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * sections-rom.ld - Linker script for HTTP service resources
 * 
 * This file creates a custom linker section to place HTTP service
 * resource definitions in the correct memory region.
 * 
 * REQUIRED for projects using Zephyr's HTTP server framework.
 * 
 * Place this file in your project root directory (same level as CMakeLists.txt)
 */

#include <zephyr/linker/iterable_sections.h>

/* 
 * Define a linkeable section for HTTP resources
 * This allows HTTP_RESOURCE_DEFINE() macro to place resources
 * in a contiguous memory region that can be iterated at runtime.
 */
ITERABLE_SECTION_ROM(http_resource_desc, 4)
