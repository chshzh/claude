/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HTTP_RESOURCES_TEMPLATE_H__
#define HTTP_RESOURCES_TEMPLATE_H__

/**
 * @file http_resources_template.h
 * @brief Static Web Server HTTP Resources - Public Interface
 */

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/data/json.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Configuration (Set via Kconfig or prj.conf)
 ******************************************************************************/

#ifndef CONFIG_HTTP_SERVER_SERVICE_PORT
#define CONFIG_HTTP_SERVER_SERVICE_PORT 80
#endif

#ifndef CONFIG_HTTP_RESOURCES_LOG_LEVEL
#define CONFIG_HTTP_RESOURCES_LOG_LEVEL LOG_LEVEL_INF
#endif

/*******************************************************************************
 * JSON Descriptor Examples
 ******************************************************************************/

/**
 * Example: LED control command structure
 * JSON: {"r": 255, "g": 128, "b": 0}
 */
struct led_command {
	int r;
	int g;
	int b;
};

static const struct json_obj_descr led_command_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct led_command, r, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, g, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, b, JSON_TOK_NUMBER),
};

/**
 * Example: Device status structure
 * JSON: {"uptime": 12345, "temperature": 25.5, "humidity": 60.2}
 */
struct device_status {
	uint32_t uptime;
	float temperature;
	float humidity;
};

static const struct json_obj_descr device_status_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct device_status, uptime, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct device_status, temperature, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct device_status, humidity, JSON_TOK_FLOAT),
};

/*******************************************************************************
 * WebSocket Context
 ******************************************************************************/

/**
 * @brief WebSocket connection context
 * 
 * Maintains state for each active WebSocket connection.
 * Use work queue for periodic data transmission.
 */
struct ws_data_ctx {
	int sock;                       /* Socket descriptor */
	struct k_work_delayable work;   /* Delayed work for periodic updates */
	bool active;                    /* Connection active flag */
};

/*******************************************************************************
 * Public API
 ******************************************************************************/

/**
 * @brief Initialize HTTP resources
 * 
 * Call this during system initialization before starting network.
 */
void http_resources_init(void);

/**
 * @brief Set handler for POST /api/control endpoint
 * 
 * @param handler Callback function to process control requests
 */
void http_resources_set_control_handler(http_resource_dynamic_cb_t handler);

/**
 * @brief Set handler for GET /api/status endpoint
 * 
 * @param handler Callback function to provide status data
 */
void http_resources_set_status_handler(http_resource_dynamic_cb_t handler);

/**
 * @brief Set handler for WebSocket /ws/data endpoint
 * 
 * @param handler Callback function for WebSocket events
 */
void http_resources_set_ws_handler(http_resource_websocket_cb_t handler);

/**
 * @brief Get WebSocket context array
 * 
 * @param ctx Pointer to receive context array
 */
void http_resources_get_ws_contexts(struct ws_data_ctx **ctx);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_RESOURCES_TEMPLATE_H__ */
