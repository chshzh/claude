# NCS Architecture Patterns Guide

Comprehensive guide for choosing and implementing architectural patterns in NCS projects.

---

## Overview

This guide covers two primary architectural patterns for NCS applications:

1. **Simple Multi-Threaded Architecture** - Traditional approach with threads and direct communication
2. **SMF + zbus Modular Architecture** - Advanced pattern with state machines and message bus (Nordic's recommended approach for complex system)

---

## Pattern Comparison

| Aspect | Simple Multi-Threaded | SMF + zbus Modular |
|--------|----------------------|-------------------|
| **Complexity** | Low - easier to learn | Medium - requires SMF and zbus knowledge |
| **Scalability** | Limited - becomes complex with many threads | Excellent - scales well with many modules |
| **Testability** | Moderate - some coupling between thread | Excellent - modules are independent |
| **Debugging** | Harder - race conditions, deadlocks | Easier - predictable state transitions |
| **Code Reuse** | Moderate - threads are application-specific | Excellent - modules are reusable |
| **Communication** | Direct (queues, semaphores, mutexes) | Message-based (zbus channels) |
| **State Management** | Manual - if/else or switch statements | Structured - State Machine Framework |
| **Best For** | Simple apps, prototypes, 1-3 threads | Complex apps, products, 4+ modules |
| **Memory Overhead** | Lower - no SMF/zbus overhead | Higher - SMF/zbus infrastructure |
| **Development Time** | Faster for simple apps | Faster for complex apps (less debugging) |
| **Maintenance** | Harder for complex apps | Easier - modular, well-defined interfaces |

---

## Pattern 1: Simple Multi-Threaded Architecture

### When to Use

✅ **Good for:**
- Simple applications (1-3 concurrent tasks)
- Quick prototypes
- Learning Zephyr basics
- Resource-constrained devices
- Linear workflows

❌ **Not recommended for:**
- Complex state machines (4+ states)
- Many concurrent modules (4+)
- Applications requiring high testability
- Long-term maintainable products

### Architecture Overview

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ Thread 1    │────>│   Queue     │────>│  Thread 2   │
│  (WiFi)     │     │             │     │  (Network)  │
└─────────────┘     └─────────────┘     └─────────────┘
       │                                        │
       │            ┌─────────────┐             │
       └──────────> │ Semaphore   │ <──────────┘
                    └─────────────┘
```

**Key Components:**
- **Threads**: Independent execution contexts
- **Queues**: FIFO message passing between threads
- **Semaphores**: Synchronization primitives
- **Mutexes**: Protect shared resources

### Implementation Example

#### prj.conf Configuration

```conf
# Threading
CONFIG_MULTITHREADING=y
CONFIG_NUM_PREEMPT_PRIORITIES=15
CONFIG_MAIN_STACK_SIZE=2048

# Synchronization
CONFIG_THREAD_STACK_INFO=y
CONFIG_THREAD_NAME=y

# Logging
CONFIG_LOG=y
CONFIG_LOG_MODE_DEFERRED=y
```

#### Simple Multi-Threaded Code

```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

/* Message structure */
struct sensor_msg {
    float temperature;
uint32_t timestamp;
};

/* Message queue */
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_msg), 10, 4);

/* Semaphore for synchronization */
K_SEM_DEFINE(wifi_ready_sem, 0, 1);

/* Mutex for shared resource */
K_MUTEX_DEFINE(data_mutex);

/* Shared data */
static float latest_temperature = 0.0f;

/* WiFi thread */
void wifi_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    LOG_INF("WiFi thread started");
    
    /* Initialize WiFi */
    // ... WiFi initialization code ...
    
    /* Signal WiFi is ready */
    k_sem_give(&wifi_ready_sem);
    
    while (1) {
        /* WiFi connection management */
        if (!wifi_is_connected()) {
            wifi_connect("SSID", "password");
        }
        
        k_sleep(K_SECONDS(1));
    }
}

/* Sensor thread */
void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    struct sensor_msg msg;
    
    LOG_INF("Sensor thread started");
    
    while (1) {
        /* Read sensor */
        float temp = read_temperature();
        
        /* Update shared data (with mutex protection) */
        k_mutex_lock(&data_mutex, K_FOREVER);
        latest_temperature = temp;
        k_mutex_unlock(&data_mutex);
        
        /* Send to network thread */
        msg.temperature = temp;
        msg.timestamp = k_uptime_get_32();
        
        if (k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT) != 0) {
            LOG_WRN("Sensor queue full");
        }
        
        k_sleep(K_SECONDS(60));
    }
}

/* Network thread */
void network_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    struct sensor_msg msg;
    
    LOG_INF("Network thread started");
    
    /* Wait for WiFi to be ready */
    k_sem_take(&wifi_ready_sem, K_FOREVER);
    LOG_INF("WiFi ready, network thread proceeding");
    
    while (1) {
        /* Wait for sensor data */
        if (k_msgq_get(&sensor_msgq, &msg, K_FOREVER) == 0) {
            LOG_INF("Received temp: %.2f at %u", 
                    msg.temperature, msg.timestamp);
            
            /* Send to cloud */
            if (wifi_is_connected()) {
                send_to_cloud(&msg);
            }
        }
    }
}

/* Define threads */
K_THREAD_DEFINE(wifi_thread, 2048, wifi_thread_fn, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(sensor_thread, 2048, sensor_thread_fn, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(network_thread, 2048, network_thread_fn, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
    LOG_INF("Simple Multi-Threaded Application Started");
    
    /* Main thread can do other work or just sleep */
    while (1) {
        k_sleep(K_SECONDS(10));
    }
    
    return 0;
}
```

### Pros and Cons

**Advantages:**
- ✅ Simple and straightforward
- ✅ Direct thread communication
- ✅ Lower memory overhead
- ✅ Familiar to most developers
- ✅ Good for simple applications

**Disadvantages:**
- ❌ Risk of race conditions
- ❌ Potential for deadlocks
- ❌ Harder to test individual components
- ❌ Doesn't scale well
- ❌ Manual state management
- ❌ Tight coupling between threads

---

## Pattern 2: SMF + zbus Modular Architecture

### When to Use

✅ **Good for:**
- Complex applications (4+ concurrent modules)
- Applications with complex state machines
- Long-term maintainable products
- High testability requirements
- Scalable architectures
- Team development (modules can be developed independently)

❌ **Not recommended for:**
- Very simple applications
- Resource-extremely constrained devices
- Quick throwaway prototypes

### Architecture Overview

```
┌──────────────────────────────────────────────────┐
│                  zbus Message Bus                │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐   │
│  │ WIFI_CHAN │  │SENSOR_CHAN│  │ NET_CHAN  │   │
│  └───────────┘  └───────────┘  └───────────┘   │
└──────────────────────────────────────────────────┘
         │                │                │
    ┌────▼────┐      ┌────▼────┐      ┌────▼────┐
    │  WiFi   │      │ Sensor  │      │ Network │
    │ Module  │      │ Module  │      │ Module  │
    │  (SMF)  │      │  (SMF)  │      │  (SMF)  │
    └─────────┘      └─────────┘      └─────────┘
         │                │                │
  ┌──────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐
  │ State       │  │ State       │  │ State       │
  │ Machine     │  │ Machine     │  │ Machine     │
  │ (SMF)       │  │ (SMF)       │  │ (SMF)       │
  └─────────────┘  └─────────────┘  └─────────────┘
```

**Key Components:**
- **Modules**: Self-contained functional units
- **State Machines (SMF)**: Structured state management using Zephyr SMF
- **zbus Channels**: Publish/subscribe message passing
- **Message Subscribers**: Queue-based message receivers for blocking modules
- **Message Listeners**: Immediate callbacks for non-blocking modules

### Key Concepts

#### State Machine Framework (SMF)

SMF provides:
- **Run-to-completion model**: Each state handles messages completely before transitioning
- **Hierarchical states**: Parent/child state relationships
- **Entry/Run/Exit actions**: Structured state behavior
- **Predictable behavior**: No race conditions within a state machine

#### zbus Message Bus

zbus provides:
- **Publish/Subscribe**: Decoupled communication
- **Message Channels**: Typed message passing
- **Subscribers**: Queue messages for threaded processing
- **Listeners**: Immediate callbacks for fast processing
- **Observers**: Monitor channels without processing

### Implementation Example

#### prj.conf Configuration

```conf
# State Machine Framework
CONFIG_SMF=y
CONFIG_SMF_ANCESTOR_SUPPORT=y
CONFIG_SMF_INITIAL_TRANSITION=y

# zbus Message Bus
CONFIG_ZBUS=y
CONFIG_ZBUS_MSG_SUBSCRIBER=y
CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE=64
CONFIG_ZBUS_CHANNEL_NAME=y

# Threading
CONFIG_MULTITHREADING=y
CONFIG_NUM_PREEMPT_PRIORITIES=15

# Logging
CONFIG_LOG=y
CONFIG_LOG_MODE_DEFERRED=y
CONFIG_THREAD_NAME=y

# Task Watchdog (recommended for production)
CONFIG_TASK_WDT=y
CONFIG_TASK_WDT_CHANNELS=8
```

#### Message Definitions

**common/messages.h:**
```c
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

/* WiFi messages */
enum wifi_msg_type {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR,
};

struct wifi_msg {
    enum wifi_msg_type type;
    char ssid[32];
    int8_t rssi;
};

/* Sensor messages */
struct sensor_msg {
    float temperature;
    float humidity;
    uint32_t timestamp;
};

/* Network messages */
enum network_msg_type {
    NETWORK_SEND_DATA,
    NETWORK_DATA_SENT,
    NETWORK_ERROR,
};

struct network_msg {
    enum network_msg_type type;
    uint8_t *data;
    size_t len;
};

/* Button messages */
enum button_msg_type {
    BUTTON_PRESS_SHORT,
    BUTTON_PRESS_LONG,
};

struct button_msg {
    enum button_msg_type type;
    uint8_t button_number;
};

#endif /* MESSAGES_H */
```

#### Module Template: WiFi Module with SMF

**modules/wifi/wifi.c:**
```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/smf.h>
#include <zephyr/task_wdt/task_wdt.h>

#include "messages.h"

LOG_MODULE_REGISTER(wifi_module, LOG_LEVEL_DBG);

/* ============================================================================
 * ZBUS CHANNEL DEFINITIONS
 * ============================================================================ */

/* Define the channel this module provides */
ZBUS_CHAN_DEFINE(WIFI_CHAN,
                 struct wifi_msg,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.type = WIFI_DISCONNECTED)
);

/* Register as subscriber to receive messages */
ZBUS_MSG_SUBSCRIBER_DEFINE(wifi_sub);

/* Subscribe to channels this module needs */
ZBUS_CHAN_ADD_OBS(BUTTON_CHAN, wifi_sub, 0);

/* ============================================================================
 * STATE MACHINE DEFINITION
 * ============================================================================ */

/* WiFi module states */
enum wifi_state {
    STATE_INIT,
    STATE_RUNNING,
        STATE_DISCONNECTED,
        STATE_CONNECTING,
        STATE_CONNECTED,
    STATE_ERROR,
};

/* State object - holds context for state machine */
struct wifi_state_obj {
    /* Must be first */
    struct smf_ctx ctx;
    
    /* Channel that sent the last message */
    const struct zbus_channel *chan;
    
    /* Message buffer */
    uint8_t msg_buf[128];
    
    /* Module-specific data */
    char ssid[32];
    char password[64];
    int retry_count;
};

/* Forward declarations of state handlers */
static void state_init_entry(void *obj);
static enum smf_state_result state_init_run(void *obj);
static void state_disconnected_entry(void *obj);
static enum smf_state_result state_disconnected_run(void *obj);
static void state_connecting_entry(void *obj);
static enum smf_state_result state_connecting_run(void *obj);
static void state_connected_entry(void *obj);
static enum smf_state_result state_connected_run(void *obj);

/* State machine definition */
static const struct smf_state states[] = {
    [STATE_INIT] = SMF_CREATE_STATE(
        state_init_entry,
        state_init_run,
        NULL,  /* no exit action */
        NULL,  /* no parent */
        NULL   /* no initial transition */
    ),
    [STATE_RUNNING] = SMF_CREATE_STATE(
        NULL,  /* no entry */
        NULL,  /* no run */
        NULL,  /* no exit */
        NULL,  /* no parent */
        &states[STATE_DISCONNECTED]  /* initial transition to DISCONNECTED */
    ),
    [STATE_DISCONNECTED] = SMF_CREATE_STATE(
        state_disconnected_entry,
        state_disconnected_run,
        NULL,
        &states[STATE_RUNNING],  /* parent state */
        NULL
    ),
    [STATE_CONNECTING] = SMF_CREATE_STATE(
        state_connecting_entry,
        state_connecting_run,
        NULL,
        &states[STATE_RUNNING],
        NULL
    ),
    [STATE_CONNECTED] = SMF_CREATE_STATE(
        state_connected_entry,
        state_connected_run,
        NULL,
        &states[STATE_RUNNING],
        NULL
    ),
};

/* ============================================================================
 * STATE HANDLERS
 * ============================================================================ */

static void state_init_entry(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    
    LOG_INF("WiFi module initializing");
    
    /* Initialize WiFi hardware/driver */
    // wifi_driver_init();
    
    state->retry_count = 0;
}

static enum smf_state_result state_init_run(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    
    LOG_INF("WiFi initialized, transitioning to RUNNING");
    
    /* Transition to RUNNING state */
    smf_set_state(SMF_CTX(state), &states[STATE_RUNNING]);
    
    return SMF_STATE_TRANSITION_HANDLED;
}

static void state_disconnected_entry(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    int err;
    struct wifi_msg msg = {
        .type = WIFI_DISCONNECTED,
    };
    
    LOG_INF("WiFi disconnected");
    
    /* Publish disconnected status */
    err = zbus_chan_pub(&WIFI_CHAN, &msg, K_SECONDS(1));
    if (err) {
        LOG_ERR("Failed to publish WIFI_CHAN: %d", err);
    }
}

static enum smf_state_result state_disconnected_run(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    
    /* Check if we received a message */
    if (state->chan == &BUTTON_CHAN) {
        struct button_msg *btn_msg = (struct button_msg *)state->msg_buf;
        
        if (btn_msg->type == BUTTON_PRESS_SHORT) {
            LOG_INF("Button pressed, connecting to WiFi");
            
            /* Set credentials */
            strcpy(state->ssid, "MySSID");
            strcpy(state->password, "MyPassword");
            
            /* Transition to CONNECTING */
            smf_set_state(SMF_CTX(state), &states[STATE_CONNECTING]);
            return SMF_STATE_TRANSITION_HANDLED;
        }
    }
    
    return SMF_STATE_WAIT_FOR_EVENT;
}

static void state_connecting_entry(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    int err;
    struct wifi_msg msg = {
        .type = WIFI_CONNECTING,
    };
    
    LOG_INF("Connecting to WiFi: %s", state->ssid);
    
    strcpy(msg.ssid, state->ssid);
    
    /* Publish connecting status */
    err = zbus_chan_pub(&WIFI_CHAN, &msg, K_SECONDS(1));
    if (err) {
        LOG_ERR("Failed to publish WIFI_CHAN: %d", err);
    }
    
    /* Start connection process */
    // wifi_connect(state->ssid, state->password);
}

static enum smf_state_result state_connecting_run(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    
    /* Simulate connection check */
    k_sleep(K_SECONDS(2));
    
    bool connected = true;  // In real code: wifi_is_connected();
    
    if (connected) {
        LOG_INF("WiFi connected successfully");
        smf_set_state(SMF_CTX(state), &states[STATE_CONNECTED]);
        return SMF_STATE_TRANSITION_HANDLED;
    } else {
        state->retry_count++;
        if (state->retry_count >= 3) {
            LOG_WRN("Connection failed after %d retries", state->retry_count);
            smf_set_state(SMF_CTX(state), &states[STATE_DISCONNECTED]);
            return SMF_STATE_TRANSITION_HANDLED;
        }
    }
    
    return SMF_STATE_WAIT_FOR_EVENT;
}

static void state_connected_entry(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    int err;
    struct wifi_msg msg = {
        .type = WIFI_CONNECTED,
        .rssi = -45,  // Example RSSI
    };
    
    LOG_INF("WiFi connected!");
    
    strcpy(msg.ssid, state->ssid);
    state->retry_count = 0;
    
    /* Publish connected status */
    err = zbus_chan_pub(&WIFI_CHAN, &msg, K_SECONDS(1));
    if (err) {
        LOG_ERR("Failed to publish WIFI_CHAN: %d", err);
    }
}

static enum smf_state_result state_connected_run(void *obj)
{
    struct wifi_state_obj *state = (struct wifi_state_obj *)obj;
    
    /* Check connection status periodically */
    bool still_connected = true;  // In real code: wifi_is_connected();
    
    if (!still_connected) {
        LOG_WRN("WiFi connection lost");
        smf_set_state(SMF_CTX(state), &states[STATE_DISCONNECTED]);
        return SMF_STATE_TRANSITION_HANDLED;
    }
    
    return SMF_STATE_WAIT_FOR_EVENT;
}

/* ============================================================================
 * MODULE THREAD
 * ============================================================================ */

static void wifi_thread(void)
{
    int err;
    struct wifi_state_obj state_obj = {0};
    const struct zbus_channel *chan;
    int task_wdt_id;
    
    LOG_INF("WiFi module thread started");
    
    /* Register with watchdog */
    task_wdt_id = task_wdt_add(30000, NULL, NULL);
    
    /* Initialize state machine */
    smf_set_initial(SMF_CTX(&state_obj), &states[STATE_INIT]);
    
    while (1) {
        /* Feed watchdog */
        task_wdt_feed(task_wdt_id);
        
        /* Run state machine (process current state) */
        err = smf_run_state(SMF_CTX(&state_obj));
        if (err) {
            LOG_ERR("State machine error: %d", err);
        }
        
        /* Wait for message with timeout */
        err = zbus_sub_wait_msg(&wifi_sub, &chan, state_obj.msg_buf,
                                K_SECONDS(10));
        
        if (err == -EAGAIN) {
            /* Timeout - no message received */
            continue;
        } else if (err) {
            LOG_ERR("zbus_sub_wait_msg error: %d", err);
            continue;
        }
        
        /* Store channel that sent the message */
        state_obj.chan = chan;
        
        /* Message received - run state machine to process it */
        err = smf_run_state(SMF_CTX(&state_obj));
        if (err) {
            LOG_ERR("State machine error: %d", err);
        }
    }
}

K_THREAD_DEFINE(wifi_thread_id, 2048, wifi_thread, 
                NULL, NULL, NULL, 7, 0, 0);
```

#### Non-Blocking Module Example: Button Module

**modules/button/button.c:**
```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <dk_buttons_and_leds.h>

#include "messages.h"

LOG_MODULE_REGISTER(button_module, LOG_LEVEL_DBG);

/* Define the channel this module provides */
ZBUS_CHAN_DEFINE(BUTTON_CHAN,
                 struct button_msg,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(0)
);

/* Button handler - called from interrupt context */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
    int err;
    struct button_msg msg;
    
    /* Check if button 1 was pressed */
    if ((has_changed & DK_BTN1_MSK) && (button_states & DK_BTN1_MSK)) {
        msg.type = BUTTON_PRESS_SHORT;
        msg.button_number = 1;
        
        LOG_DBG("Button 1 pressed");
        
        /* Publish button event */
        err = zbus_chan_pub(&BUTTON_CHAN, &msg, K_NO_WAIT);
        if (err) {
            LOG_ERR("Failed to publish button event: %d", err);
        }
    }
}

/* Initialize module at system init */
static int button_module_init(void)
{
    int err;
    
    LOG_INF("Button module initializing");
    
    err = dk_buttons_init(button_handler);
    if (err) {
        LOG_ERR("Failed to initialize buttons: %d", err);
        return err;
    }
    
    return 0;
}

SYS_INIT(button_module_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
```

### Directory Structure for SMF+zbus Project

```
my_project/
├── CMakeLists.txt
├── Kconfig
├── prj.conf
├── README.md
│
├── src/
│   ├── main.c                  # Main app and orchestration
│   │
│   ├── common/
│   │   ├── messages.h          # All message definitions
│   │   └── app_common.h        # Common macros and helpers
│   │
│   └── modules/
│       ├── wifi/
│       │   ├── wifi.c          # WiFi module implementation
│       │   └── wifi.h          # WiFi module interface
│       │
│       ├── sensor/
│       │   ├── sensor.c
│       │   └── sensor.h
│       │
│       ├── network/
│       │   ├── network.c
│       │   └── network.h
│       │
│       └── button/
│           ├── button.c
│           └── button.h
│
└── boards/
    └── nrf7002dk_nrf5340_cpuapp.conf
```

### Main Application with SMF+zbus

**src/main.c:**
```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "common/messages.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Declare external channels from modules */
ZBUS_CHAN_DECLARE(WIFI_CHAN);
ZBUS_CHAN_DECLARE(SENSOR_CHAN);
ZBUS_CHAN_DECLARE(NETWORK_CHAN);
ZBUS_CHAN_DECLARE(BUTTON_CHAN);

/* Register subscriber for main */
ZBUS_MSG_SUBSCRIBER_DEFINE(main_sub);

/* Subscribe to channels we want to monitor */
ZBUS_CHAN_ADD_OBS(WIFI_CHAN, main_sub, 0);
ZBUS_CHAN_ADD_OBS(NETWORK_CHAN, main_sub, 0);

int main(void)
{
    int err;
    const struct zbus_channel *chan;
    uint8_t msg_buf[128];
    
    LOG_INF("Application started");
    
    while (1) {
        /* Wait for messages */
        err = zbus_sub_wait_msg(&main_sub, &chan, msg_buf, K_FOREVER);
        if (err) {
            LOG_ERR("zbus_sub_wait_msg error: %d", err);
            continue;
        }
        
        /* Handle messages */
        if (chan == &WIFI_CHAN) {
            struct wifi_msg *msg = (struct wifi_msg *)msg_buf;
            LOG_INF("WiFi status: %d",msg->type);
            
        } else if (chan == &NETWORK_CHAN) {
            struct network_msg *msg = (struct network_msg *)msg_buf;
            LOG_INF("Network status: %d", msg->type);
        }
    }
    
    return 0;
}
```

### Best Practices for SMF+zbus Architecture

#### 1. Module Design

**Each module should:**
- ✅ Be self-contained with its own state machine
- ✅ Define its own zbus channel(s) for publishing
- ✅ Subscribe only to channels it needs
- ✅ Have a single responsibility
- ✅ Be testable in isolation

**Avoid:**
- ❌ Direct function calls between modules
- ❌ Shared global variables
- ❌ Circular dependencies
- ❌ Modules that know about other modules' internals

#### 2. Message Design

**Good message design:**
- ✅ Keep messages small and focused
- ✅ Use enums for message types
- ✅ Include timestamps when needed
- ✅ Version messages for long-term compatibility

**Message example:**
```c
struct sensor_msg {
    enum sensor_msg_type type;  /* Message type */
    uint32_t timestamp;         /* When measured */
    float temperature;          /* Sensor value */
    uint8_t battery_level;      /* Additional context */
};
```

#### 3. State Machine Design

**Good state design:**
- ✅ States represent system modes, not actions
- ✅ Use hierarchical states for common behavior
- ✅ Keep state handlers focused and simple
- ✅ Transition explicitly between states

**State transitions:**
```c
/* Good: Explicit transition */
smf_set_state(SMF_CTX(state), &states[STATE_CONNECTED]);

/* Bad: Implicit state change */
state->current_state = STATE_CONNECTED;  /* Don't do this! */
```

#### 4. Thread vs. Listener

**Use threaded subscriber (ZBUS_MSG_SUBSCRIBER) when:**
- Module performs blocking operations
- Module has complex state machine
- Module needs to process messages sequentially

**Use listener (ZBUS_LISTENER) when:**
- Module is non-blocking (e.g., button driver)
- Module just needs to be notified
- Processing is very fast (<100µs)

### Pros and Cons

**Advantages:**
- ✅ Excellent scalability
- ✅ Highly testable (modules are independent)
- ✅ Predictable behavior (run-to-completion)
- ✅ Easy to debug (state transitions are visible)
- ✅ Reusable modules
- ✅ No race conditions in state machines
- ✅ Decoupled communication
- ✅ Industry best practice for complex systems

**Disadvantages:**
- ❌ Steeper learning curve
- ❌ More boilerplate code
- ❌ Higher memory overhead
- ❌ Overkill for simple applications
- ❌ Requires understanding of SMF and zbus

---

## Migration Path

### From Simple to SMF+zbus

If your application outgrows the simple multi-threaded pattern:

**Step 1: Add zbus**
- Keep existing threads
- Replace queues with zbus channels
- Replace direct function calls with zbus messages

**Step 2: Add State Machines**
- Convert one thread to SMF module
- Test thoroughly
- Convert remaining threads one by one

**Step 3: Refactor into Modules**
- Extract related functionality into modules
- Define clear module boundaries
- Document module interfaces

---

## Comparison Table

| Feature | Simple Multi-Threaded | SMF + zbus |
|---------|----------------------|------------|
| **Learning Curve** | Low | Medium |
| **Code Complexity** | Low for simple apps | Medium  |
| **Maintenance** | Hard for complex apps | Easy |
| **Testability** | Moderate | Excellent |
| **Debugging** | Difficult | Easy |
| **Race Conditions** | Possible | Eliminated in SM |
| **Deadlocks** | Possible | Rare |
| **Scalability** | Poor | Excellent |
| **Code Reuse** | Low | High |
| **Memory Overhead** | ~5KB | ~15KB |
| **State Management** | Manual | Framework |
| **Message Passing** | Queues | zbus |
| **Best For** | 1-3 threads | 4+ modules |

---

## Decision Tree

```
┌─────────────────────────────┐
│ How many concurrent tasks?  │
└──────────┬──────────────────┘
           │
      ┌────▼────┐
      │ 1-2     │────> Simple Multi-Threaded
      └─────────┘
           │
      ┌────▼────┐
      │ 3-4     │────> Consider complexity
      └────┬────┘      │
           │           ├─> Simple states -> Multi-Threaded
           │           └─> Complex states -> SMF+zbus
      ┌────▼────┐
      │ 5+      │────> SMF+zbus
      └─────────┘
```

---

## Example Projects

### Simple Multi-Threaded Projects
- Basic sensor reader with WiFi upload
- Simple thermostat
- LED controller with network commands

### SMF+zbus Projects
- Asset Tracker (Nordic's official template)
- Smart home hub (many sensors + protocols)
- Industrial IoT gateway
- Complex wearable devices

---

## References

- [Zephyr State Machine Framework Documentation](https://docs.zephyrproject.org/latest/services/smf/index.html)
- [Zephyr zbus Documentation](https://docs.zephyrproject.org/latest/services/zbus/index.html)
- [Nordic Asset Tracker Template](https://docs.nordicsemi.com/bundle/asset-tracker-template-latest/page/index.html)
- [Zephyr Threading Documentation](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)

---

**Last Updated**: 2026-01-30
