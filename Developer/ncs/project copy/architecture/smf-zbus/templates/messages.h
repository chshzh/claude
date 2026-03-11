/*
 * Copyright (c) 2026 [Your Company]
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file messages.h
 * @brief Common message definitions for all modules
 * 
 * This file defines all message structures used for zbus communication.
 * All modules should include this file and use these message definitions.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * BUTTON MESSAGES
 * ============================================================================ */

/**
 * @brief Button message types
 */
enum button_msg_type {
    BUTTON_PRESS_SHORT,     /**< Short button press */
    BUTTON_PRESS_LONG,      /**< Long button press */
};

/**
 * @brief Button message structure
 */
struct button_msg {
    enum button_msg_type type;
    uint8_t button_number;
};

/* ============================================================================
 * WIFI MESSAGES
 * ============================================================================ */

/**
 * @brief WiFi message types
 */
enum wifi_msg_type {
    WIFI_DISCONNECTED,      /**< WiFi disconnected */
    WIFI_CONNECTING,        /**< WiFi connecting */
    WIFI_CONNECTED,         /**< WiFi connected */
    WIFI_ERROR,             /**< WiFi error occurred */
};

/**
 * @brief WiFi message structure
 */
struct wifi_msg {
    enum wifi_msg_type type;
    char ssid[32];
    int8_t rssi;
    uint8_t channel;
};

/* ============================================================================
 * SENSOR MESSAGES
 * ============================================================================ */

/**
 * @brief Sensor message structure
 */
struct sensor_msg {
    float temperature;
    float humidity;
    float pressure;
    uint32_t timestamp;
};

/* ============================================================================
 * NETWORK MESSAGES
 * ============================================================================ */

/**
 * @brief Network message types
 */
enum network_msg_type {
    NETWORK_DISCONNECTED,   /**< Network disconnected */
    NETWORK_CONNECTED,      /**< Network connected */
    NETWORK_SEND_DATA,      /**< Request to send data */
    NETWORK_DATA_SENT,      /**< Data successfully sent */
    NETWORK_ERROR,          /**< Network error */
};

/**
 * @brief Network message structure
 */
struct network_msg {
    enum network_msg_type type;
    uint8_t *data;
    size_t len;
    int error_code;
};

/* ============================================================================
 * CLOUD MESSAGES
 * ============================================================================ */

/**
 * @brief Cloud message types
 */
enum cloud_msg_type {
    CLOUD_DISCONNECTED,     /**< Disconnected from cloud */
    CLOUD_CONNECTING,       /**< Connecting to cloud */
    CLOUD_CONNECTED,        /**< Connected to cloud */
    CLOUD_CONFIG_UPDATE,    /**< Configuration update received */
    CLOUD_FOTA_AVAILABLE,   /**< FOTA update available */
};

/**
 * @brief Cloud message structure
 */
struct cloud_msg {
    enum cloud_msg_type type;
    char *payload;
    size_t payload_len;
};

/* ============================================================================
 * LOCATION MESSAGES
 * ============================================================================ */

/**
 * @brief Location message types
 */
enum location_msg_type {
    LOCATION_SEARCHING,     /**< Searching for location */
    LOCATION_FOUND,         /**< Location found */
    LOCATION_TIMEOUT,       /**< Location search timed out */
    LOCATION_ERROR,         /**< Location error */
};

/**
 * @brief Location message structure
 */
struct location_msg {
    enum location_msg_type type;
    double latitude;
    double longitude;
    double accuracy;
    uint32_t timestamp;
};

/* ============================================================================
 * APPLICATION MESSAGES
 * ============================================================================ */

/**
 * @brief Application message types
 */
enum app_msg_type {
    APP_START,              /**< Application started */
    APP_SAMPLE_DATA,        /**< Trigger data sampling */
    APP_UPLOAD_DATA,        /**< Trigger data upload */
    APP_CONFIG_CHANGED,     /**< Configuration changed */
};

/**
 * @brief Application message structure
 */
struct app_msg {
    enum app_msg_type type;
    uint32_t interval_seconds;
};

#endif /* MESSAGES_H */
