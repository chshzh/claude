/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file sensor_example.c
 * @brief Sensor module using SMF + zbus pattern
 * 
 * This module demonstrates:
 * - Periodic sensor reading with SMF
 * - Message subscription (APP_EVENT channel)
 * - Publishing sensor data via zbus
 * - State machine for sampling control
 * 
 * Based on Nordic Asset Tracker Template environmental module
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/smf.h>

#include "sensor_example.h"

LOG_MODULE_REGISTER(sensor, CONFIG_APP_SENSOR_LOG_LEVEL);

BUILD_ASSERT(CONFIG_APP_SENSOR_WATCHDOG_TIMEOUT_SECONDS >
	     CONFIG_APP_SENSOR_MSG_PROCESSING_TIMEOUT_SECONDS,
	     "Watchdog timeout must be greater than message processing time");

/* Define zbus channel provided by this module */
ZBUS_CHAN_DEFINE(SENSOR_CHAN,
		 struct sensor_msg,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.type = SENSOR_IDLE)
);

/* Register as zbus subscriber */
ZBUS_MSG_SUBSCRIBER_DEFINE(sensor);

/* Observe sensor channel and APP_EVENT channel */
ZBUS_CHAN_ADD_OBS(SENSOR_CHAN, sensor, 0);
/* ZBUS_CHAN_ADD_OBS(APP_EVENT_CHAN, sensor, 0);  // Uncomment when APP_EVENT exists */

/*******************************************************************************
 * State Machine States
 ******************************************************************************/

enum sensor_state {
	STATE_INIT,
	STATE_IDLE,
	STATE_SAMPLING,
	STATE_DATA_READY,
};

struct sensor_state_object {
	struct smf_ctx ctx;
	const struct zbus_channel *chan;
	uint8_t msg_buf[sizeof(struct sensor_msg)];
	float temperature;
	float humidity;
	uint32_t sample_count;
	int wdt_id;
};

/* Forward declarations */
static void state_init_entry(void *obj);
static enum smf_state_result state_init_run(void *obj);
static void state_idle_entry(void *obj);
static enum smf_state_result state_idle_run(void *obj);
static void state_sampling_entry(void *obj);
static enum smf_state_result state_sampling_run(void *obj);
static void state_data_ready_entry(void *obj);
static enum smf_state_result state_data_ready_run(void *obj);

static const struct smf_state states[] = {
	[STATE_INIT] = SMF_CREATE_STATE(
		state_init_entry,
		state_init_run,
		NULL,
		NULL,
		&states[STATE_IDLE]
	),
	[STATE_IDLE] = SMF_CREATE_STATE(
		state_idle_entry,
		state_idle_run,
		NULL,
		NULL,
		NULL
	),
	[STATE_SAMPLING] = SMF_CREATE_STATE(
		state_sampling_entry,
		state_sampling_run,
		NULL,
		NULL,
		&states[STATE_DATA_READY]
	),
	[STATE_DATA_READY] = SMF_CREATE_STATE(
		state_data_ready_entry,
		state_data_ready_run,
		NULL,
		NULL,
		NULL
	),
};

static struct sensor_state_object state_obj;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static void publish_sensor_data(float temperature, float humidity)
{
	int err;
	struct sensor_msg msg = {
		.type = SENSOR_DATA_READY,
		.temperature = temperature,
		.humidity = humidity,
		.timestamp = k_uptime_get_32(),
	};

	LOG_DBG("Publishing sensor data: temp=%.1f, hum=%.1f", temperature, humidity);

	err = zbus_chan_pub(&SENSOR_CHAN, &msg, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub failed: %d", err);
	}
}

static int read_sensor_data(float *temperature, float *humidity)
{
	/* Simulate sensor reading */
	/* In real implementation, use actual sensor drivers */
	
	*temperature = 22.5f + (float)(sys_rand32_get() % 100) / 10.0f;
	*humidity = 50.0f + (float)(sys_rand32_get() % 300) / 10.0f;

	LOG_DBG("Sensor read: temp=%.1f, hum=%.1f", *temperature, *humidity);
	
	return 0;
}

/*******************************************************************************
 * State Handlers
 ******************************************************************************/

static void state_init_entry(void *obj)
{
	struct sensor_state_object *state = obj;

	LOG_INF("Sensor module initializing");

	/* Initialize sensor hardware here */
	/* Example: sensor_init(); */

#ifdef CONFIG_TASK_WDT
	state->wdt_id = task_wdt_add(
		CONFIG_APP_SENSOR_WATCHDOG_TIMEOUT_SECONDS * 1000,
		NULL,
		NULL
	);
	if (state->wdt_id < 0) {
		LOG_ERR("task_wdt_add failed: %d", state->wdt_id);
	}
#endif

	state->sample_count = 0;
}

static enum smf_state_result state_init_run(void *obj)
{
	return SMF_STATE_HANDLED();
}

static void state_idle_entry(void *obj)
{
	LOG_DBG("Sensor idle");
	
	struct sensor_msg msg = {
		.type = SENSOR_IDLE,
	};
	zbus_chan_pub(&SENSOR_CHAN, &msg, K_NO_WAIT);
}

static enum smf_state_result state_idle_run(void *obj)
{
	struct sensor_state_object *state = obj;

#ifdef CONFIG_TASK_WDT
	if (state->wdt_id >= 0) {
		task_wdt_feed(state->wdt_id);
	}
#endif

	/* Check for start command */
	if (state->chan == &SENSOR_CHAN) {
		struct sensor_msg *msg = (struct sensor_msg *)state->msg_buf;
		if (msg->type == SENSOR_START) {
			smf_set_state(SMF_CTX(state), &states[STATE_SAMPLING]);
			return SMF_STATE_TRANSITION();
		}
	}

	return SMF_STATE_HANDLED();
}

static void state_sampling_entry(void *obj)
{
	LOG_DBG("Starting sensor sampling");
}

static enum smf_state_result state_sampling_run(void *obj)
{
	struct sensor_state_object *state = obj;
	int err;

	/* Read sensor */
	err = read_sensor_data(&state->temperature, &state->humidity);
	if (err) {
		LOG_ERR("Sensor read failed: %d", err);
		smf_set_state(SMF_CTX(state), &states[STATE_IDLE]);
		return SMF_STATE_TRANSITION();
	}

	state->sample_count++;

	/* Transition to data ready */
	return SMF_STATE_HANDLED();
}

static void state_data_ready_entry(void *obj)
{
	struct sensor_state_object *state = obj;

	LOG_DBG("Sensor data ready (sample #%u)", state->sample_count);

	/* Publish data */
	publish_sensor_data(state->temperature, state->humidity);
}

static enum smf_state_result state_data_ready_run(void *obj)
{
	struct sensor_state_object *state = obj;

	/* Check for stop command */
	if (state->chan == &SENSOR_CHAN) {
		struct sensor_msg *msg = (struct sensor_msg *)state->msg_buf;
		if (msg->type == SENSOR_STOP) {
			smf_set_state(SMF_CTX(state), &states[STATE_IDLE]);
			return SMF_STATE_TRANSITION();
		}
	}

	/* Wait for sampling interval */
	k_sleep(K_SECONDS(CONFIG_APP_SENSOR_SAMPLE_INTERVAL_SECONDS));

	/* Continue sampling */
	smf_set_state(SMF_CTX(state), &states[STATE_SAMPLING]);
	return SMF_STATE_TRANSITION();
}

/*******************************************************************************
 * Module Thread
 ******************************************************************************/

static void sensor_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Sensor module thread started");

	/* Initialize state machine */
	smf_set_initial(SMF_CTX(&state_obj), &states[STATE_INIT]);

	while (1) {
		/* Wait for zbus messages */
		int err = zbus_sub_wait_msg(&sensor, &state_obj.chan,
					    state_obj.msg_buf,
					    K_MSEC(CONFIG_APP_SENSOR_MSG_PROCESSING_TIMEOUT_SECONDS * 1000));

		if (err == -EAGAIN || err == -ENOMSG) {
			/* Timeout - run state machine anyway */
		} else if (err) {
			LOG_ERR("zbus_sub_wait_msg failed: %d", err);
			continue;
		}

		/* Run state machine */
		int32_t ret = smf_run_state(SMF_CTX(&state_obj));
		if (ret) {
			LOG_ERR("smf_run_state failed: %d", ret);
		}
	}
}

K_THREAD_DEFINE(sensor_module,
		CONFIG_APP_SENSOR_STACK_SIZE,
		sensor_thread,
		NULL, NULL, NULL,
		CONFIG_APP_SENSOR_PRIORITY,
		0, 0);
