/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_EXAMPLE_H_
#define _SENSOR_EXAMPLE_H_

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_msg_type {
	SENSOR_IDLE = 0x1,
	SENSOR_START,
	SENSOR_STOP,
	SENSOR_DATA_READY,
};

struct sensor_msg {
	enum sensor_msg_type type;
	float temperature;
	float humidity;
	uint32_t timestamp;
};

#define MSG_TO_SENSOR_MSG(_msg) (*(const struct sensor_msg *)_msg)

ZBUS_CHAN_DECLARE(SENSOR_CHAN);

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_EXAMPLE_H_ */
