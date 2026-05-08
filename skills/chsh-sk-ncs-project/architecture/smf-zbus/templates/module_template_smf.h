/*
 * Copyright (c) 2026 [Your Company]
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file module_template.h
 * @brief Module template header file
 */

#ifndef MODULE_TEMPLATE_H
#define MODULE_TEMPLATE_H

#include <zephyr/kernel.h>

/**
 * @brief Module message types
 */
enum module_template_msg_type {
    MODULE_TEMPLATE_IDLE,
    MODULE_TEMPLATE_ACTIVE,
    MODULE_TEMPLATE_ERROR,
};

/**
 * @brief Module message structure
 */
struct module_template_msg {
    enum module_template_msg_type type;
    uint32_t timestamp;
    int32_t value;
};

/* Declare the channel (defined in .c file) */
extern const struct zbus_channel MODULE_TEMPLATE_CHAN;

#endif /* MODULE_TEMPLATE_H */
