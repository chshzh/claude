/*
 * Copyright (c) 2026 [Your Company]
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file module_template.c
 * @brief Template for creating SMF+zbus modules
 * 
 * This template demonstrates the recommended pattern for creating
 * a module using State Machine Framework (SMF) and zbus.
 * 
 * Usage:
 * 1. Copy this file to your modules directory
 * 2. Rename to your module name (e.g., sensors.c)
 * 3. Replace MODULE_TEMPLATE with your module name
 * 4. Define your states and message handlers
 * 5. Update CMakeLists.txt to include this file
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/smf.h>
#include <zephyr/task_wdt/task_wdt.h>

#include "common/messages.h"
#include "MODULE_TEMPLATE.h"

/* Register log module */
LOG_MODULE_REGISTER(MODULE_TEMPLATE, CONFIG_APP_MODULE_TEMPLATE_LOG_LEVEL);

/* Build-time assertions for configuration */
BUILD_ASSERT(CONFIG_APP_MODULE_TEMPLATE_WATCHDOG_TIMEOUT_SECONDS >
             CONFIG_APP_MODULE_TEMPLATE_MSG_PROCESSING_TIMEOUT_SECONDS,
             "Watchdog timeout must be greater than message processing timeout");

/* ============================================================================
 * ZBUS CHANNEL DEFINITIONS
 * ============================================================================ */

/**
 * Define the channel(s) that this module provides.
 * Other modules can subscribe to this channel to receive messages.
 */
ZBUS_CHAN_DEFINE(MODULE_TEMPLATE_CHAN,
                 struct module_template_msg,
                 NULL,   /* No validator */
                 NULL,   /* No user data */
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.type = MODULE_TEMPLATE_IDLE)
);

/**
 * Register as a message subscriber.
 * This allows the module to queue messages from other channels.
 */
ZBUS_MSG_SUBSCRIBER_DEFINE(module_template_sub);

/**
 * Subscribe to channels that this module needs to observe.
 * Add one line for each channel you want to subscribe to.
 */
ZBUS_CHAN_ADD_OBS(BUTTON_CHAN, module_template_sub, 0);
/* Add more subscriptions as needed */
/* ZBUS_CHAN_ADD_OBS(OTHER_CHAN, module_template_sub, 0); */

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

#define MAX_MSG_SIZE 128  /* Maximum message size this module handles */

/* ============================================================================
 * STATE MACHINE DEFINITION
 * ============================================================================ */

/**
 * Module states.
 * 
 * State naming convention:
 * - Use nouns that describe the system state, not actions
 * - Order from general to specific
 * - Use STATE_ prefix
 * 
 * Example state hierarchy:
 *   STATE_INIT (initialization)
 *   STATE_RUNNING (parent state for normal operation)
 *     STATE_IDLE (child: waiting for events)
 *     STATE_ACTIVE (child: processing)
 *     STATE_ERROR (child: error handling)
 */
enum module_template_state {
    STATE_INIT,
    STATE_RUNNING,
        STATE_IDLE,
        STATE_ACTIVE,
    STATE_ERROR,
};

/**
 * State object.
 * 
 * Holds all context data for the state machine.
 * This is passed to every state handler.
 */
struct module_template_state_obj {
    /* MUST be first - SMF context */
    struct smf_ctx ctx;
    
    /* Channel that sent the last message */
    const struct zbus_channel *chan;
    
    /* Buffer for last received message */
    uint8_t msg_buf[MAX_MSG_SIZE];
    
    /* Module-specific working data */
    uint32_t event_count;
    int32_t last_value;
    bool error_flag;
    
    /* Add your module's data here */
};

/* Forward declarations of state handlers */
static void state_init_entry(void *obj);
static enum smf_state_result state_init_run(void *obj);
static void state_running_entry(void *obj);
static void state_idle_entry(void *obj);
static enum smf_state_result state_idle_run(void *obj);
static void state_active_entry(void *obj);
static enum smf_state_result state_active_run(void *obj);
static void state_active_exit(void *obj);
static void state_error_entry(void *obj);
static enum smf_state_result state_error_run(void *obj);

/**
 * State machine definition.
 * 
 * Each state is defined with:
 * - Entry action (optional): Called when entering the state
 * - Run action (optional): Called repeatedly while in the state
 * - Exit action (optional): Called when leaving the state
 * - Parent state (optional): Creates state hierarchy
 * - Initial transition (optional): Child state to enter automatically
 * 
 * SMF_CREATE_STATE(entry, run, exit, parent, initial)
 */
static const struct smf_state states[] = {
    [STATE_INIT] = SMF_CREATE_STATE(
        state_init_entry,
        state_init_run,
        NULL,                       /* No exit action */
        NULL,                       /* No parent state */
        NULL                        /* No initial transition */
    ),
    [STATE_RUNNING] = SMF_CREATE_STATE(
        state_running_entry,
        NULL,                       /* No run action */
        NULL,                       /* No exit action */
        NULL,                       /* No parent state */
        &states[STATE_IDLE]         /* Initial transition to IDLE */
    ),
    [STATE_IDLE] = SMF_CREATE_STATE(
        state_idle_entry,
        state_idle_run,
        NULL,                       /* No exit action */
        &states[STATE_RUNNING],     /* Parent state */
        NULL                        /* No initial transition */
    ),
    [STATE_ACTIVE] = SMF_CREATE_STATE(
        state_active_entry,
        state_active_run,
        state_active_exit,
        &states[STATE_RUNNING],     /* Parent state */
        NULL                        /* No initial transition */
    ),
    [STATE_ERROR] = SMF_CREATE_STATE(
        state_error_entry,
        state_error_run,
        NULL,                       /* No exit action */
        NULL,                       /* No parent state */
        NULL                        /* No initial transition */
    ),
};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Publish a message on this module's channel
 * 
 * @param msg Pointer to the message to publish
 */
static void module_template_publish(const struct module_template_msg *msg)
{
    int err;
    
    err = zbus_chan_pub(&MODULE_TEMPLATE_CHAN, msg, K_SECONDS(1));
    if (err) {
        LOG_ERR("Failed to publish message: %d", err);
        /* In production, consider error handling strategy */
    }
}

/**
 * @brief Handle messages from BUTTON_CHAN
 * 
 * @param state Pointer to state object
 * @param msg Pointer to button message
 * @return State transition result
 */
static enum smf_state_result handle_button_msg(
    struct module_template_state_obj *state,
    const struct button_msg *msg)
{
    if (msg->type == BUTTON_PRESS_SHORT) {
        LOG_DBG("Button %d short press", msg->button_number);
        
        /* Example: Transition to ACTIVE state on button press */
        smf_set_state(SMF_CTX(state), &states[STATE_ACTIVE]);
        return SMF_STATE_TRANSITION_HANDLED;
    }
    
    return SMF_STATE_WAIT_FOR_EVENT;
}

/* ============================================================================
 * STATE HANDLERS
 * ============================================================================ */

/**
 * @brief Entry action for INIT state
 * 
 * Called once when entering INIT state.
 * Use for module initialization.
 */
static void state_init_entry(void *obj)
{
    struct module_template_state_obj *state = obj;
    
    LOG_INF("Module initializing");
    
    /* Initialize module-specific data */
    state->event_count = 0;
    state->last_value = 0;
    state->error_flag = false;
    
    /* Initialize hardware/drivers if needed */
    /* int err = driver_init(); */
    /* if (err) { */
    /*     state->error_flag = true; */
    /*     return; */
    /* } */
}

/**
 * @brief Run action for INIT state
 * 
 * Called to process the INIT state.
 * Typically transitions to RUNNING when initialization is complete.
 */
static enum smf_state_result state_init_run(void *obj)
{
    struct module_template_state_obj *state = obj;
    
    if (state->error_flag) {
        LOG_ERR("Initialization failed");
        smf_set_state(SMF_CTX(state), &states[STATE_ERROR]);
        return SMF_STATE_TRANSITION_HANDLED;
    }
    
    LOG_INF("Initialization complete, entering RUNNING state");
    smf_set_state(SMF_CTX(state), &states[STATE_RUNNING]);
    return SMF_STATE_TRANSITION_HANDLED;
}

/**
 * @brief Entry action for RUNNING state
 */
static void state_running_entry(void *obj)
{
    struct module_template_msg msg = {
        .type = MODULE_TEMPLATE_IDLE,
    };
    
    LOG_INF("Module running");
    
    /* Publish status */
    module_template_publish(&msg);
}

/**
 * @brief Entry action for IDLE state
 */
static void state_idle_entry(void *obj)
{
    LOG_DBG("Module idle, waiting for events");
}

/**
 * @brief Run action for IDLE state
 * 
 * Process messages received while in IDLE state.
 */
static enum smf_state_result state_idle_run(void *obj)
{
    struct module_template_state_obj *state = obj;
    
    /* Check which channel sent the message */
    if (state->chan == &BUTTON_CHAN) {
        return handle_button_msg(state, (struct button_msg *)state->msg_buf);
    }
    
    /* Add handlers for other channels */
    /* else if (state->chan == &OTHER_CHAN) { */
    /*     return handle_other_msg(state, (struct other_msg *)state->msg_buf); */
    /* } */
    
    return SMF_STATE_WAIT_FOR_EVENT;
}

/**
 * @brief Entry action for ACTIVE state
 */
static void state_active_entry(void *obj)
{
    struct module_template_state_obj *state = obj;
    struct module_template_msg msg = {
        .type = MODULE_TEMPLATE_ACTIVE,
    };
    
    LOG_DBG("Module active");
    
    state->event_count++;
    
    /* Publish status */
    module_template_publish(&msg);
}

/**
 * @brief Run action for ACTIVE state
 */
static enum smf_state_result state_active_run(void *obj)
{
    struct module_template_state_obj *state = obj;
    
    /* Perform work */
    LOG_DBG("Processing... event count: %u", state->event_count);
    
    /* Simulate some work */
    k_sleep(K_SECONDS(2));
    
    /* Return to IDLE when done */
    smf_set_state(SMF_CTX(state), &states[STATE_IDLE]);
    return SMF_STATE_TRANSITION_HANDLED;
}

/**
 * @brief Exit action for ACTIVE state
 */
static void state_active_exit(void *obj)
{
    LOG_DBG("Leaving active state");
    
    /* Cleanup if needed */
}

/**
 * @brief Entry action for ERROR state
 */
static void state_error_entry(void *obj)
{
    struct module_template_msg msg = {
        .type = MODULE_TEMPLATE_ERROR,
    };
    
    LOG_ERR("Module entered error state");
    
    /* Publish error status */
    module_template_publish(&msg);
}

/**
 * @brief Run action for ERROR state
 *  * Could attempt recovery or wait for reset.
 */
static enum smf_state_result state_error_run(void *obj)
{
    /* Stay in error state */
    /* In production, might attempt recovery */
    k_sleep(K_SECONDS(10));
    
    return SMF_STATE_WAIT_FOR_EVENT;
}

/* ============================================================================
 * MODULE THREAD
 * ============================================================================ */

/**
 * @brief Main module thread
 * 
 * This thread:
 * 1. Initializes the state machine
 * 2. Runs the state machine
 * 3. Waits for messages
 * 4. Processes messages through the state machine
 * 5. Feeds the watchdog
 */
static void module_template_thread(void)
{
    int err;
    struct module_template_state_obj state_obj = {0};
    const struct zbus_channel *chan;
    int task_wdt_id;
    
    LOG_INF("Module thread started");
    
    /* Register with task watchdog */
    task_wdt_id = task_wdt_add(
        CONFIG_APP_MODULE_TEMPLATE_WATCHDOG_TIMEOUT_SECONDS * 1000,
        NULL,
        NULL
    );
    if (task_wdt_id < 0) {
        LOG_ERR("Failed to add task watchdog: %d", task_wdt_id);
    }
    
    /* Set initial state */
    smf_set_initial(SMF_CTX(&state_obj), &states[STATE_INIT]);
    
    /* Main loop */
    while (1) {
        /* Feed watchdog */
        if (task_wdt_id >= 0) {
            err = task_wdt_feed(task_wdt_id);
            if (err) {
                LOG_ERR("Failed to feed watchdog: %d", err);
            }
        }
        
        /* Run state machine (process current state) */
        err = smf_run_state(SMF_CTX(&state_obj));
        if (err) {
            LOG_ERR("State machine run error: %d", err);
        }
        
        /* Wait for message with timeout */
        err = zbus_sub_wait_msg(
            &module_template_sub,
            &chan,
            state_obj.msg_buf,
            K_SECONDS(CONFIG_APP_MODULE_TEMPLATE_MSG_PROCESSING_TIMEOUT_SECONDS)
        );
        
        if (err == -EAGAIN) {
            /* Timeout - no message received, continue */
            continue;
        } else if (err) {
            LOG_ERR("zbus_sub_wait_msg error: %d", err);
            continue;
        }
        
        /* Message received - store channel info */
        state_obj.chan = chan;
        
        LOG_DBG("Message received on channel: %s", zbus_chan_name(chan));
        
        /* Run state machine to process the message */
        err = smf_run_state(SMF_CTX(&state_obj));
        if (err) {
            LOG_ERR("State machine message processing error: %d", err);
        }
    }
}

/* Define the module thread */
K_THREAD_DEFINE(module_template_thread_id,
                CONFIG_APP_MODULE_TEMPLATE_STACK_SIZE,
                module_template_thread,
                NULL, NULL, NULL,
                CONFIG_APP_MODULE_TEMPLATE_THREAD_PRIORITY,
                0, 0);
