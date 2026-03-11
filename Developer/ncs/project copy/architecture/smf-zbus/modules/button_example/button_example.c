/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file button_example.c
 * @brief Button module using SMF + zbus pattern
 * 
 * This module demonstrates:
 * - Button event detection (short/long press)
 * - SMF state machine for button handling
 * - zbus channel publishing
 * - Task watchdog integration
 * 
 * Based on Nordic Asset Tracker Template pattern
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/smf.h>
#include <dk_buttons_and_leds.h>

#include "button_example.h"

LOG_MODULE_REGISTER(button_example, CONFIG_APP_BUTTON_LOG_LEVEL);

BUILD_ASSERT(CONFIG_APP_BUTTON_WATCHDOG_TIMEOUT_SECONDS >
	     CONFIG_APP_BUTTON_MSG_PROCESSING_TIMEOUT_SECONDS,
	     "Watchdog timeout must be greater than message processing time");

/* Long press timeout */
#define LONG_PRESS_TIMEOUT_MS CONFIG_APP_BUTTON_LONG_PRESS_TIMEOUT_MS

/* Define zbus channel provided by this module */
ZBUS_CHAN_DEFINE(BUTTON_CHAN,
		 struct button_msg,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.type = BUTTON_IDLE)
);

/* Register as zbus subscriber */
ZBUS_MSG_SUBSCRIBER_DEFINE(button);

/* Observe button channel for internal messages */
ZBUS_CHAN_ADD_OBS(BUTTON_CHAN, button, 0);

/*******************************************************************************
 * State Machine States
 ******************************************************************************/

enum button_state {
	STATE_INIT,
	STATE_IDLE,
	STATE_PRESSED,
	STATE_LONG_PRESS_PENDING,
};

/* State object with SMF context */
struct button_state_object {
	struct smf_ctx ctx;
	const struct zbus_channel *chan;
	uint8_t msg_buf[sizeof(struct button_msg)];
	uint32_t pressed_buttons;
	int wdt_id;
};

/* Forward declarations */
static void state_init_entry(void *obj);
static enum smf_state_result state_init_run(void *obj);
static void state_idle_entry(void *obj);
static enum smf_state_result state_idle_run(void *obj);
static void state_pressed_entry(void *obj);
static enum smf_state_result state_pressed_run(void *obj);
static void state_long_press_pending_entry(void *obj);
static enum smf_state_result state_long_press_pending_run(void *obj);

/* State definitions */
static const struct smf_state states[] = {
	[STATE_INIT] = SMF_CREATE_STATE(
		state_init_entry,
		state_init_run,
		NULL,  /* No exit */
		NULL,  /* No parent */
		&states[STATE_IDLE]  /* Initial transition to IDLE */
	),
	[STATE_IDLE] = SMF_CREATE_STATE(
		state_idle_entry,
		state_idle_run,
		NULL,
		NULL,
		NULL  /* No initial transition */
	),
	[STATE_PRESSED] = SMF_CREATE_STATE(
		state_pressed_entry,
		state_pressed_run,
		NULL,
		NULL,
		&states[STATE_LONG_PRESS_PENDING]  /* Transition to long press check */
	),
	[STATE_LONG_PRESS_PENDING] = SMF_CREATE_STATE(
		state_long_press_pending_entry,
		state_long_press_pending_run,
		NULL,
		NULL,
		NULL
	),
};

static struct button_state_object state_obj;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static void publish_button_msg(enum button_msg_type type, uint8_t button_number)
{
	int err;
	struct button_msg msg = {
		.type = type,
		.button_number = button_number,
	};

	LOG_DBG("Publishing button event: type=%d, button=%d", type, button_number);

	err = zbus_chan_pub(&BUTTON_CHAN, &msg, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub failed: %d", err);
	}
}

/*******************************************************************************
 * State Handlers
 ******************************************************************************/

static void state_init_entry(void *obj)
{
	struct button_state_object *state = obj;

	LOG_INF("Button module initializing");

	/* Initialize button hardware */
	int err = dk_buttons_init(NULL);  /* Button handler in module thread */
	if (err) {
		LOG_ERR("dk_buttons_init failed: %d", err);
		return;
	}

#ifdef CONFIG_TASK_WDT
	/* Register with watchdog */
	state->wdt_id = task_wdt_add(
		CONFIG_APP_BUTTON_WATCHDOG_TIMEOUT_SECONDS * 1000,
		NULL,
		NULL
	);
	if (state->wdt_id < 0) {
		LOG_ERR("task_wdt_add failed: %d", state->wdt_id);
	}
#endif

	state->pressed_buttons = 0;
}

static enum smf_state_result state_init_run(void *obj)
{
	/* Transition to idle handled by initial_transition */
	return SMF_STATE_HANDLED();
}

static void state_idle_entry(void *obj)
{
	LOG_DBG("Button idle");
	publish_button_msg(BUTTON_IDLE, 0);
}

static enum smf_state_result state_idle_run(void *obj)
{
	struct button_state_object *state = obj;

	/* Feed watchdog */
#ifdef CONFIG_TASK_WDT
	if (state->wdt_id >= 0) {
		task_wdt_feed(state->wdt_id);
	}
#endif

	/* Wait for messages - handled by module thread */
	return SMF_STATE_HANDLED();
}

static void state_pressed_entry(void *obj)
{
	LOG_DBG("Button pressed");
}

static enum smf_state_result state_pressed_run(void *obj)
{
	struct button_state_object *state = obj;

	/* Check if button still pressed after delay */
	if (state->pressed_buttons == 0) {
		/* Button released - short press */
		publish_button_msg(BUTTON_PRESS_SHORT, 1);
		smf_set_state(SMF_CTX(state), &states[STATE_IDLE]);
		return SMF_STATE_TRANSITION();
	}

	/* Still pressed - check for long press */
	return SMF_STATE_HANDLED();
}

static void state_long_press_pending_entry(void *obj)
{
	LOG_DBG("Checking for long press");
}

static enum smf_state_result state_long_press_pending_run(void *obj)
{
	struct button_state_object *state = obj;

	/* Wait for long press timeout */
	k_sleep(K_MSEC(LONG_PRESS_TIMEOUT_MS));

	if (state->pressed_buttons != 0) {
		/* Still pressed after timeout - long press */
		publish_button_msg(BUTTON_PRESS_LONG, 1);
	}

	/* Wait for button release */
	while (state->pressed_buttons != 0) {
		k_sleep(K_MSEC(100));
	}

	smf_set_state(SMF_CTX(state), &states[STATE_IDLE]);
	return SMF_STATE_TRANSITION();
}

/*******************************************************************************
 * Button Hardware Handler (called from IRQ context)
 ******************************************************************************/

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (!(has_changed & DK_BTN1_MSK)) {
		return;
	}

	if (button_states & DK_BTN1_MSK) {
		/* Button pressed */
		state_obj.pressed_buttons = DK_BTN1_MSK;
		
		/* Trigger state machine transition */
		smf_set_state(SMF_CTX(&state_obj), &states[STATE_PRESSED]);
	} else {
		/* Button released */
		state_obj.pressed_buttons = 0;
	}
}

/*******************************************************************************
 * Module Thread
 ******************************************************************************/

static void button_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Button module thread started");

	/* Initialize state machine */
	smf_set_initial(SMF_CTX(&state_obj), &states[STATE_INIT]);

	/* Set up button handler */
	dk_buttons_init(button_handler);

	/* Run state machine */
	while (1) {
		/* Wait for zbus messages */
		int err = zbus_sub_wait_msg(&button, &state_obj.chan,
					    state_obj.msg_buf,
					    K_MSEC(CONFIG_APP_BUTTON_MSG_PROCESSING_TIMEOUT_SECONDS * 1000));

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

K_THREAD_DEFINE(button_module,
		CONFIG_APP_BUTTON_STACK_SIZE,
		button_thread,
		NULL, NULL, NULL,
		CONFIG_APP_BUTTON_PRIORITY,
		0, 0);
