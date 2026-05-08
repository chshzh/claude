/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file module_template_simple.c
 * @brief Simple Multi-Threaded Module Template
 * 
 * This template demonstrates a traditional multi-threaded approach using
 * Zephyr's message queues, semaphores, and mutexes for communication.
 * 
 * ARCHITECTURE: Simple Multi-Threaded
 * - Direct message queue communication between threads
 * - Semaphores for synchronization
 * - Mutexes for shared resource protection
 * - Traditional procedural design
 * 
 * USE THIS PATTERN WHEN:
 * - Building simple applications (1-3 threads)
 * - Need straightforward control flow
 * - Team familiar with traditional RTOS patterns
 * - Quick prototyping
 * 
 * For complex applications with 4+ modules, consider SMF+zbus pattern.
 * See module_template_smf.c and guides/ARCHITECTURE_PATTERNS.md
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(module_template_simple, CONFIG_MODULE_TEMPLATE_LOG_LEVEL);

/*******************************************************************************
 * Message Definitions
 ******************************************************************************/

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
 * @brief Message structure for message queue
 */
struct module_message {
	enum message_type type;
	void *data;             /* Pointer to message-specific data */
	size_t data_len;        /* Length of data */
	uint32_t timestamp;     /* Message timestamp */
};

/*******************************************************************************
 * Module State & Configuration
 ******************************************************************************/

/**
 * @brief Module operational state
 */
enum module_state {
	STATE_UNINITIALIZED,
	STATE_INITIALIZED,
	STATE_RUNNING,
	STATE_STOPPED,
	STATE_ERROR,
};

/**
 * @brief Module context structure
 * 
 * Contains all module state and shared resources.
 * Access to this structure must be protected by the state_mutex when
 * shared between threads.
 */
struct module_context {
	atomic_t state;         /* Current module state (atomic for lock-free reads) */
	struct k_mutex mutex;   /* Protects shared resources */
	uint32_t run_count;     /* Number of operations performed */
	uint32_t error_count;   /* Number of errors encountered */
};

/* Global module context */
static struct module_context ctx = {
	.state = ATOMIC_INIT(STATE_UNINITIALIZED),
};

/*******************************************************************************
 * Message Queue
 ******************************************************************************/

/* Message queue for receiving commands */
K_MSGQ_DEFINE(module_msgq, sizeof(struct module_message), 10, 4);

/*******************************************************************************
 * Synchronization Primitives
 ******************************************************************************/

/* Semaphore for signaling operation completion */
static K_SEM_DEFINE(operation_complete_sem, 0, 1);

/* Semaphore for signaling data ready */
static K_SEM_DEFINE(data_ready_sem, 0, 1);

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

/**
 * @brief Thread-safe state transition
 * 
 * @param new_state New state to transition to
 */
static void set_state(enum module_state new_state)
{
	enum module_state old_state = atomic_set(&ctx.state, new_state);
	LOG_DBG("State transition: %d -> %d", old_state, new_state);
}

/**
 * @brief Get current module state
 * 
 * @return Current state (thread-safe atomic read)
 */
static enum module_state get_state(void)
{
	return (enum module_state)atomic_get(&ctx.state);
}

/**
 * @brief Initialize module resources
 * 
 * @return 0 on success, negative errno on failure
 */
static int module_init(void)
{
	int ret;

	LOG_INF("Initializing module...");

	/* Initialize mutex for protecting shared resources */
	ret = k_mutex_init(&ctx.mutex);
	if (ret != 0) {
		LOG_ERR("Failed to initialize mutex: %d", ret);
		return ret;
	}

	/* Initialize module-specific resources here */
	/* Example:
	 * - Initialize hardware peripherals
	 * - Allocate memory
	 * - Set default configuration
	 */

	k_mutex_lock(&ctx.mutex, K_FOREVER);
	ctx.run_count = 0;
	ctx.error_count = 0;
	k_mutex_unlock(&ctx.mutex);

	set_state(STATE_INITIALIZED);
	LOG_INF("Module initialized successfully");
	
	return 0;
}

/**
 * @brief Start module operation
 * 
 * @return 0 on success, negative errno on failure
 */
static int module_start(void)
{
	enum module_state state = get_state();
	
	if (state != STATE_INITIALIZED && state != STATE_STOPPED) {
		LOG_ERR("Cannot start from state %d", state);
		return -EINVAL;
	}

	LOG_INF("Starting module...");

	/* Start module operation here */
	/* Example:
	 * - Enable hardware
	 * - Start timers
	 * - Begin data acquisition
	 */

	set_state(STATE_RUNNING);
	LOG_INF("Module started");
	
	return 0;
}

/**
 * @brief Stop module operation
 * 
 * @return 0 on success, negative errno on failure
 */
static int module_stop(void)
{
	enum module_state state = get_state();
	
	if (state != STATE_RUNNING) {
		LOG_WRN("Module not running (state %d)", state);
		return -EALREADY;
	}

	LOG_INF("Stopping module...");

	/* Stop module operation here */
	/* Example:
	 * - Disable hardware
	 * - Stop timers
	 * - Save state if needed
	 */

	set_state(STATE_STOPPED);
	LOG_INF("Module stopped");
	
	return 0;
}

/**
 * @brief Process data
 * 
 * @param data Pointer to data buffer
 * @param len Length of data
 * @return 0 on success, negative errno on failure
 */
static int process_data(const void *data, size_t len)
{
	enum module_state state = get_state();
	
	if (state != STATE_RUNNING) {
		LOG_ERR("Module not running (state %d)", state);
		return -EINVAL;
	}

	if (data == NULL || len == 0) {
		LOG_ERR("Invalid data parameters");
		return -EINVAL;
	}

	LOG_DBG("Processing %zu bytes", len);

	/* Process data here */
	/* Example:
	 * - Parse incoming data
	 * - Perform calculations
	 * - Update internal state
	 */

	k_mutex_lock(&ctx.mutex, K_FOREVER);
	ctx.run_count++;
	k_mutex_unlock(&ctx.mutex);

	/* Signal data processing complete */
	k_sem_give(&data_ready_sem);
	
	return 0;
}

/**
 * @brief Handle error condition
 * 
 * @param error_code Error code
 */
static void handle_error(int error_code)
{
	LOG_ERR("Error occurred: %d", error_code);

	k_mutex_lock(&ctx.mutex, K_FOREVER);
	ctx.error_count++;
	k_mutex_unlock(&ctx.mutex);

	/* Handle error here */
	/* Example:
	 * - Attempt recovery
	 * - Reset hardware
	 * - Notify other modules
	 */

	/* For critical errors, transition to error state */
	if (ctx.error_count >= 5) {
		LOG_ERR("Too many errors, entering error state");
		set_state(STATE_ERROR);
	}
}

/**
 * @brief Get module status
 * 
 * @param run_count Pointer to store run count (can be NULL)
 * @param error_count Pointer to store error count (can be NULL)
 * @return Current module state
 */
static enum module_state get_status(uint32_t *run_count, uint32_t *error_count)
{
	k_mutex_lock(&ctx.mutex, K_FOREVER);
	
	if (run_count != NULL) {
		*run_count = ctx.run_count;
	}
	
	if (error_count != NULL) {
		*error_count = ctx.error_count;
	}
	
	k_mutex_unlock(&ctx.mutex);

	return get_state();
}

/*******************************************************************************
 * Message Processing
 ******************************************************************************/

/**
 * @brief Process received message
 * 
 * @param msg Pointer to received message
 */
static void process_message(const struct module_message *msg)
{
	int ret;

	switch (msg->type) {
	case MSG_TYPE_INIT:
		LOG_DBG("Received INIT message");
		ret = module_init();
		if (ret != 0) {
			handle_error(ret);
		}
		k_sem_give(&operation_complete_sem);
		break;

	case MSG_TYPE_START:
		LOG_DBG("Received START message");
		ret = module_start();
		if (ret != 0) {
			handle_error(ret);
		}
		k_sem_give(&operation_complete_sem);
		break;

	case MSG_TYPE_STOP:
		LOG_DBG("Received STOP message");
		ret = module_stop();
		if (ret != 0) {
			handle_error(ret);
		}
		k_sem_give(&operation_complete_sem);
		break;

	case MSG_TYPE_DATA:
		LOG_DBG("Received DATA message");
		ret = process_data(msg->data, msg->data_len);
		if (ret != 0) {
			handle_error(ret);
		}
		break;

	case MSG_TYPE_STATUS_REQ:
		LOG_DBG("Received STATUS_REQ message");
		{
			uint32_t runs, errors;
			enum module_state state = get_status(&runs, &errors);
			LOG_INF("Status: state=%d, runs=%u, errors=%u", 
				state, runs, errors);
		}
		break;

	case MSG_TYPE_ERROR:
		LOG_DBG("Received ERROR message");
		handle_error(-EFAULT);
		break;

	default:
		LOG_WRN("Unknown message type: %d", msg->type);
		break;
	}
}

/*******************************************************************************
 * Module Thread
 ******************************************************************************/

/**
 * @brief Module thread entry point
 * 
 * This thread runs continuously, waiting for messages on the message queue
 * and processing them.
 * 
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
static void module_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct module_message msg;

	LOG_INF("Module thread started");

	/* Main processing loop */
	while (1) {
		/* Wait for message on queue */
		int ret = k_msgq_get(&module_msgq, &msg, K_FOREVER);
		
		if (ret == 0) {
			/* Process received message */
			process_message(&msg);
		} else {
			LOG_ERR("Failed to receive message: %d", ret);
			k_sleep(K_MSEC(100));
		}
	}
}

/* Define module thread */
K_THREAD_DEFINE(module_thread, 
		CONFIG_MODULE_TEMPLATE_STACK_SIZE,
		module_thread_fn,
		NULL, NULL, NULL,
		CONFIG_MODULE_TEMPLATE_PRIORITY,
		0, 0);

/*******************************************************************************
 * Public API
 ******************************************************************************/

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
				  k_timeout_t timeout)
{
	struct module_message msg = {
		.type = type,
		.data = data,
		.data_len = data_len,
		.timestamp = k_uptime_get_32(),
	};

	int ret = k_msgq_put(&module_msgq, &msg, timeout);
	if (ret != 0) {
		LOG_ERR("Failed to send message type %d: %d", type, ret);
		return ret;
	}

	LOG_DBG("Sent message type %d", type);
	return 0;
}

/**
 * @brief Initialize module (blocking)
 * 
 * Sends init message and waits for completion.
 * 
 * @param timeout Timeout for operation
 * @return 0 on success, negative errno on failure
 */
int module_template_init_blocking(k_timeout_t timeout)
{
	int ret;

	ret = module_template_send_message(MSG_TYPE_INIT, NULL, 0, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	/* Wait for initialization to complete */
	ret = k_sem_take(&operation_complete_sem, timeout);
	if (ret != 0) {
		LOG_ERR("Init timeout");
		return -ETIMEDOUT;
	}

	return (get_state() == STATE_INITIALIZED) ? 0 : -EFAULT;
}

/**
 * @brief Start module operation (blocking)
 * 
 * Sends start message and waits for completion.
 * 
 * @param timeout Timeout for operation
 * @return 0 on success, negative errno on failure
 */
int module_template_start_blocking(k_timeout_t timeout)
{
	int ret;

	ret = module_template_send_message(MSG_TYPE_START, NULL, 0, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	/* Wait for start to complete */
	ret = k_sem_take(&operation_complete_sem, timeout);
	if (ret != 0) {
		LOG_ERR("Start timeout");
		return -ETIMEDOUT;
	}

	return (get_state() == STATE_RUNNING) ? 0 : -EFAULT;
}

/**
 * @brief Get current module state
 * 
 * @return Current module state
 */
enum module_state module_template_get_state(void)
{
	return get_state();
}

/*******************************************************************************
 * Usage Example
 ******************************************************************************/

#if 0
/* Example usage from another module or main.c:

#include "module_template_simple.h"

void application_main(void)
{
	int ret;

	// Initialize module
	ret = module_template_init_blocking(K_SECONDS(5));
	if (ret != 0) {
		LOG_ERR("Module init failed: %d", ret);
		return;
	}

	// Start module
	ret = module_template_start_blocking(K_SECONDS(5));
	if (ret != 0) {
		LOG_ERR("Module start failed: %d", ret);
		return;
	}

	// Send data to process
	uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
	ret = module_template_send_message(MSG_TYPE_DATA, 
					   data, 
					   sizeof(data),
					   K_SECONDS(1));
	if (ret != 0) {
		LOG_ERR("Send data failed: %d", ret);
	}

	// Request status
	ret = module_template_send_message(MSG_TYPE_STATUS_REQ,
					   NULL,
					   0,
					   K_NO_WAIT);

	// Check current state
	enum module_state state = module_template_get_state();
	LOG_INF("Module state: %d", state);
}

*/
#endif
