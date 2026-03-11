/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BUTTON_EXAMPLE_H_
#define _BUTTON_EXAMPLE_H_

/**
 * @file button_example.h
 * @brief Button module public interface
 */

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Button message types
 */
enum button_msg_type {
	/** Button is idle (no activity) */
	BUTTON_IDLE = 0x1,

	/** Short button press detected */
	BUTTON_PRESS_SHORT,

	/** Long button press detected */
	BUTTON_PRESS_LONG,
};

/**
 * @brief Button message structure
 */
struct button_msg {
	enum button_msg_type type;
	uint8_t button_number;
};

/**
 * @brief Cast message pointer to button message
 */
#define MSG_TO_BUTTON_MSG(_msg) (*(const struct button_msg *)_msg)

/**
 * @brief Button channel declaration
 * 
 * Other modules can observe this channel to receive button events.
 */
ZBUS_CHAN_DECLARE(BUTTON_CHAN);

#ifdef __cplusplus
}
#endif

#endif /* _BUTTON_EXAMPLE_H_ */
