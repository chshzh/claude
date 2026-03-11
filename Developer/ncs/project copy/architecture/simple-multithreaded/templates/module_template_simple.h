/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MODULE_TEMPLATE_SIMPLE_H__
#define MODULE_TEMPLATE_SIMPLE_H__

/**
 * @file module_template_simple.h
 * @brief Simple Multi-Threaded Module Template - Public Interface
 */

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Module operational state
 */
enum module_state {
	STATE_UNINITIALIZED,    /* Module not yet initialized */
	STATE_INITIALIZED,      /* Module initialized but not running */
	STATE_RUNNING,          /* Module actively running */
	STATE_STOPPED,          /* Module stopped */
	STATE_ERROR,            /* Module in error state */
};

/**
 * @brief Message types for inter-thread communication
 */
enum message_type {
	MSG_TYPE_INIT,          /* Module initialization */
	MSG_TYPE_START,         /* Start operation */
	MSG_TYPE_STOP,          /* Stop operation */
	MSG_TYPE_DATA,          /* Data processing */
	MSG_TYPE_STATUS_REQ,    /* Status request */
	MSG_TYPE_ERROR,         /* Error notification */
};

/**
 * @brief Send message to module
 * 
 * Public function for other threads to send messages to this module.
 * 
 * @param type Message type
 * @param data Pointer to message data (can be NULL)
 * @param data_len Length of data
 * @param timeout Timeout for message queue insertion
 * @return 0 on success, negative errno on failure
 */
int module_template_send_message(enum message_type type, 
				  void *data, 
				  size_t data_len,
				  k_timeout_t timeout);

/**
 * @brief Initialize module (blocking)
 * 
 * Sends init message and waits for completion.
 * 
 * @param timeout Timeout for operation
 * @return 0 on success, negative errno on failure
 */
int module_template_init_blocking(k_timeout_t timeout);

/**
 * @brief Start module operation (blocking)
 * 
 * Sends start message and waits for completion.
 * 
 * @param timeout Timeout for operation
 * @return 0 on success, negative errno on failure
 */
int module_template_start_blocking(k_timeout_t timeout);

/**
 * @brief Get current module state
 * 
 * @return Current module state
 */
enum module_state module_template_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_TEMPLATE_SIMPLE_H__ */
