# Wi-Fi Reconnection and Network State Management Patterns

**Best practices for robust network connectivity in Nordic NCS Wi-Fi applications**

## 🎯 Overview

Reliable network applications must handle:
- WiFi disconnections (router reboot, signal loss, interference)
- Network stack startup delays (DNS, DHCP stabilization)
- Application protocol reconnections (MQTT, HTTPS, WebSockets)
- State synchronization across multiple threads

**Key Principle**: Only attempt operations when the network layer below is ready.

## 📋 Network Layer Dependencies

```
┌─────────────────────────────────────┐
│   Application (MQTT, HTTPS, etc)    │ ← Only operate when connected
├─────────────────────────────────────┤
│   Network Stack (DNS, TCP/IP)       │ ← Needs stabilization time
├─────────────────────────────────────┤
│   WiFi L2 (Association, WPA)        │ ← Auto-retry on disconnect
├─────────────────────────────────────┤
│   WiFi Driver (nRF70)                │
└─────────────────────────────────────┘
```

Each layer must wait for the layer below to be ready.

## 🔄 WiFi Layer Reconnection

### Problem: WiFi doesn't auto-reconnect after router reboot

**Symptoms**:
- Device connects at boot
- Router power-cycled → device stays disconnected forever
- Manual reconnect works, but automatic doesn't

**Root Cause**: Connection logic only runs once at startup or requires user intervention.

### Solution: Periodic Reconnection with Stored Credentials

```c
#define WIFI_RECONNECT_DELAY_SEC 5    // Initial retry
#define WIFI_RECONNECT_RETRY_SEC 30   // Periodic retry

static struct k_work_delayable wifi_connect_work;
static bool wifi_reconnect_pending = false;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                    uint64_t mgmt_event, struct net_if *iface)
{
    if (!wifi_prov_state_get()) {
        return;  // No stored credentials
    }

    switch (mgmt_event) {
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        if (!wifi_reconnect_pending) {
            wifi_reconnect_pending = true;
            k_work_reschedule(&wifi_connect_work, 
                            K_SECONDS(WIFI_RECONNECT_DELAY_SEC));
            LOG_INF("WiFi disconnected, scheduling reconnect");
        }
        break;
    case NET_EVENT_WIFI_CONNECT_RESULT:
        wifi_reconnect_pending = false;
        k_work_cancel_delayable(&wifi_connect_work);
        break;
    }
}

static void wifi_connect_work_handler(struct k_work *work)
{
    int err;
    struct net_if *iface = net_if_get_default();
    struct wifi_iface_status status = {0};
    
    int rc = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(status));
    bool wifi_is_connected = (rc == 0 && status.state >= WIFI_STATE_ASSOCIATED);
    bool wifi_is_connecting = (rc == 0 && status.state > WIFI_STATE_DISCONNECTED &&
                               status.state < WIFI_STATE_ASSOCIATED);
    
    if (wifi_is_connected) {
        wifi_reconnect_pending = false;
        return;
    }
    
    if (wifi_credentials_is_empty()) {
        LOG_WRN("No stored WiFi credentials");
        wifi_reconnect_pending = false;
        return;
    }
    
    if (!wifi_is_connecting) {
        err = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);
        if (err) {
            LOG_WRN("WiFi connection request failed: %d", err);
        } else {
            LOG_INF("WiFi connection request sent");
        }
    }
    
    // Keep retrying periodically while disconnected
    if (wifi_reconnect_pending) {
        k_work_reschedule(&wifi_connect_work, K_SECONDS(WIFI_RECONNECT_RETRY_SEC));
        LOG_INF("WiFi still disconnected, retrying in %d seconds",
                WIFI_RECONNECT_RETRY_SEC);
    }
}
```

**Key Points**:
- ✅ Trigger reconnect on `NET_EVENT_WIFI_DISCONNECT_RESULT`
- ✅ Keep retrying every 30 seconds until connected
- ✅ Check current WiFi state before attempting connection
- ✅ Cancel pending work when connection succeeds
- ✅ Don't block BLE provisioning from triggering manual reconnects

## 🌐 Network Stack Stabilization

### Problem: DNS/TCP failures immediately after WiFi connects

**Symptoms**:
```
[00:00:07.685] Network connectivity established
[00:00:07.709] Looking up example.com
[00:00:07.710] getaddrinfo() failed, err 11  ← DNS not ready yet
```

**Root Cause**: `NET_EVENT_L4_CONNECTED` fires when IP is assigned, but DNS resolver/routing tables need additional time.

### Solution: Stabilization Delays

```c
static void https_client_thread(void)
{
    while (running) {
        k_sem_take(&network_sem, K_FOREVER);
        
        if (!network_ready) {  // Race condition check
            continue;
        }
        
        // Wait for DNS/network stack to stabilize
        k_sleep(K_SECONDS(3));  // HTTPS: lightweight
        
        // Verify still connected after delay
        if (!network_ready) {
            LOG_WRN("Network disconnected during stabilization");
            continue;
        }
        
        // Now safe to make requests
        while (network_ready) {
            send_http_request();
            k_sleep(K_SECONDS(interval));
        }
    }
}

static void mqtt_client_thread(void)
{
    while (running) {
        k_sem_take(&network_sem, K_FOREVER);
        
        if (!network_ready) {
            continue;
        }
        
        // MQTT needs longer due to TLS + DNS + broker handshake
        k_sleep(K_SECONDS(20));
        
        if (!network_ready) {
            continue;
        }
        
        // Progressive retry logic for initial connection
        int retry_count = 0;
        while (network_ready && state != CONNECTED) {
            err = mqtt_connect();
            if (err) {
                retry_count++;
                if (retry_count <= 3) {
                    // Quick retries for transient startup issues
                    k_sleep(K_SECONDS(5));
                } else {
                    // Longer backoff after initial attempts
                    k_sleep(K_SECONDS(60));
                }
            } else {
                retry_count = 0;
                k_sleep(K_MSEC(500));  // Wait for callback
            }
        }
    }
}
```

**Recommended Delays**:
- **HTTP/HTTPS**: 3-5 seconds
- **MQTT/TLS**: 15-20 seconds
- **CoAP/UDP**: 2-3 seconds
- **WebSocket**: 5-10 seconds

## 🔐 Application Protocol State Management

### Problem: Publishing when not connected

**Symptoms**:
```
[00:00:22.709] Connecting to MQTT broker: test.mosquitto.org
[00:00:23.386] Disconnected from MQTT broker, result: -128
[00:00:23.877] Connecting to MQTT broker: test.mosquitto.org  ← Tight loop
```

**Root Cause**: Retry logic doesn't distinguish between "connecting" and "disconnected" states.

### Solution: Explicit State Machine

```c
enum app_mqtt_state {
    APP_MQTT_STATE_DISCONNECTED,
    APP_MQTT_STATE_CONNECTING,
    APP_MQTT_STATE_CONNECTED,
};

static enum app_mqtt_state mqtt_state = APP_MQTT_STATE_DISCONNECTED;
static bool network_ready = false;

static int mqtt_connect(void)
{
    if (mqtt_state == APP_MQTT_STATE_CONNECTED) {
        return 0;  // Already connected
    }
    
    if (mqtt_state == APP_MQTT_STATE_CONNECTING) {
        return -EINPROGRESS;  // Connection in progress
    }
    
    mqtt_state = APP_MQTT_STATE_CONNECTING;
    
    int err = mqtt_helper_connect(&params);
    if (err) {
        mqtt_state = APP_MQTT_STATE_DISCONNECTED;
        return err;
    }
    
    return 0;
}

static int mqtt_publish(const char *payload)
{
    // Guard against invalid state
    if (!network_ready) {
        LOG_WRN("Network not ready");
        return -ENETDOWN;
    }
    
    if (mqtt_state != APP_MQTT_STATE_CONNECTED) {
        LOG_WRN("MQTT not connected");
        return -ENOTCONN;
    }
    
    return mqtt_helper_publish(&param);
}

static void on_mqtt_connack(enum mqtt_conn_return_code return_code, ...)
{
    if (return_code != MQTT_CONNECTION_ACCEPTED) {
        mqtt_state = APP_MQTT_STATE_DISCONNECTED;
        return;
    }
    
    mqtt_state = APP_MQTT_STATE_CONNECTED;
}

static void on_mqtt_disconnect(int result)
{
    mqtt_state = APP_MQTT_STATE_DISCONNECTED;
    
    if (network_ready) {
        // Unexpected disconnect - will retry in main loop
        LOG_WRN("Unexpected disconnect, will reconnect");
    }
}
```

**State Checks Required**:
- ✅ Before attempting connection → check if already connected/connecting
- ✅ Before publishing → check network_ready AND protocol connected
- ✅ After sleep/delay → re-verify state (could have changed)
- ✅ In callbacks → update state atomically

## 🚦 Idempotent Network Notifications

### Problem: Duplicate semaphore signals cause hangs

**Symptoms**:
- Multiple "Network connected" log messages
- Thread wakes multiple times for single connection event
- Missed disconnection events

**Root Cause**: `notify_connected()` called multiple times without checking current state.

### Solution: Idempotent Notifications

```c
static bool network_ready = false;

void app_mqtt_notify_connected(void)
{
    if (mqtt_running && !network_ready) {
        LOG_INF("Network connected, notifying MQTT client");
        network_ready = true;
        k_sem_give(&mqtt_sem);
    } else if (network_ready) {
        LOG_DBG("Already marked as ready, skipping duplicate");
    }
}

void app_mqtt_notify_disconnected(void)
{
    if (!network_ready) {
        return;  // Already disconnected
    }
    
    LOG_INF("Network disconnected, stopping MQTT client");
    network_ready = false;
    
    // Clean up current connection
    if (mqtt_state == APP_MQTT_STATE_CONNECTED) {
        mqtt_helper_disconnect();
        mqtt_state = APP_MQTT_STATE_DISCONNECTED;
    }
}
```

**Key Points**:
- ✅ Only signal semaphore when transitioning from disconnected → connected
- ✅ Track state in both directions (connect AND disconnect)
- ✅ Clean up resources in disconnect handler
- ✅ Make functions safe to call multiple times

## 📊 Complete Integration Example

```c
// main.c - Network event coordination
static void l4_event_handler(struct net_mgmt_event_callback *cb,
                             uint64_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event) {
    case NET_EVENT_L4_CONNECTED:
        LOG_INF("Network connectivity established");
        
        // Notify all application protocols
        https_client_notify_connected();
        mqtt_client_notify_connected();
        websocket_notify_connected();
        break;
        
    case NET_EVENT_L4_DISCONNECTED:
        LOG_INF("Network connectivity lost");
        
        // Stop all application protocols
        https_client_notify_disconnected();
        mqtt_client_notify_disconnected();
        websocket_notify_disconnected();
        break;
    }
}

// ble_provisioning.c - WiFi layer reconnection
static void wifi_mgmt_event_handler(...)
{
    switch (mgmt_event) {
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        if (!wifi_reconnect_pending) {
            wifi_reconnect_pending = true;
            k_work_reschedule(&wifi_connect_work, K_SECONDS(5));
        }
        break;
    case NET_EVENT_WIFI_CONNECT_RESULT:
        wifi_reconnect_pending = false;
        k_work_cancel_delayable(&wifi_connect_work);
        break;
    }
}

// mqtt_client.c - Application protocol state machine
static void mqtt_client_thread(...)
{
    while (running) {
        // Wait for network
        k_sem_take(&mqtt_sem, K_FOREVER);
        
        if (!network_ready) continue;
        
        // Stabilize
        k_sleep(K_SECONDS(20));
        
        if (!network_ready) continue;
        
        // Connect with progressive retry
        while (network_ready && mqtt_state != CONNECTED) {
            if (mqtt_connect() == -EINPROGRESS) {
                k_sleep(K_MSEC(500));
            } else {
                k_sleep(K_SECONDS(5));
            }
        }
        
        // Publish loop
        while (network_ready && mqtt_state == CONNECTED) {
            mqtt_publish("data");
            k_sleep(K_SECONDS(60));
        }
    }
}
```

## ⚠️ Common Pitfalls

### Don't Skip State Checks
```c
// ❌ BAD: No state verification
void publish_handler(void) {
    mqtt_publish("data");  // May fail silently
}

// ✅ GOOD: Verify state first
void publish_handler(void) {
    if (!network_ready || mqtt_state != CONNECTED) {
        LOG_WRN("Cannot publish, not ready");
        return;
    }
    mqtt_publish("data");
}
```

### Don't Assume State After Sleep
```c
// ❌ BAD: Assume network still ready
k_sleep(K_SECONDS(20));
mqtt_connect();  // Network may have dropped!

// ✅ GOOD: Re-verify after delay
k_sleep(K_SECONDS(20));
if (!network_ready) {
    LOG_WRN("Network dropped during wait");
    return;
}
mqtt_connect();
```

### Don't Ignore Error Codes
```c
// ❌ BAD: Treat all errors the same
if (mqtt_connect() != 0) {
    k_sleep(K_SECONDS(60));
}

// ✅ GOOD: Handle -EINPROGRESS differently
err = mqtt_connect();
if (err == -EINPROGRESS) {
    k_sleep(K_MSEC(500));  // Just waiting for callback
} else if (err) {
    k_sleep(K_SECONDS(60));  // Real error, back off
}
```

### Don't Log Sensitive Info During Reconnect
```c
// ❌ BAD: Logs SSID/password on every retry
LOG_INF("Connecting to %s with password %s", ssid, password);

// ✅ GOOD: Log minimal info at DEBUG level
LOG_DBG("Attempting WiFi reconnection");
```

## 🧪 Testing Reconnection Logic

### Router Power Cycle Test
1. Device boots and connects to WiFi/MQTT/HTTPS successfully
2. Power off router completely
3. Wait 30 seconds (verify device logs periodic reconnect attempts)
4. Power on router
5. **Expected**: Device automatically reconnects within 60 seconds

### Network Congestion Test  
1. Device connected and publishing
2. Introduce latency/packet loss (tc qdisc, router QoS)
3. **Expected**: Graceful degradation, no crashes, eventual recovery

### DNS Failure Test
1. Connect with working router
2. Block DNS at router (firewall rule)
3. **Expected**: Connection attempts logged as failures, no hangs
4. Unblock DNS
5. **Expected**: Connections succeed within 2-3 retry cycles

## 📚 Related Resources

- [WIFI_GUIDE.md](WIFI_GUIDE.md) - WiFi configuration and modes
- [PRD_TEMPLATE.md](../../../../ProductManager/ncs/prd/PRD_TEMPLATE.md) - Test case TC-001: Router Power Cycle
- Nordic DevAcademy: Connection Manager Exercise
- NCS Connection Manager documentation
