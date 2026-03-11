# SMF + zbus Module Examples

**Production-ready module templates organized like Nordic Asset Tracker Template**

These modules are ready to copy into your Nordic NCS project's `src/modules/` directory.

---

## 📦 Available Modules

### button_example/
Button handling with short/long press detection
- **Pattern**: Hardware event → SM → zbus publish
- **States**: INIT → IDLE → PRESSED → LONG_PRESS_PENDING
- **Publishes**: `BUTTON_CHAN`

### sensor_example/
Periodic sensor reading and data publishing
- **Pattern**: Periodic sampling → data processing → zbus publish
- **States**: INIT → IDLE → SAMPLING → DATA_READY
- **Publishes**: `SENSOR_CHAN`
- **Subscribes**: `SENSOR_CHAN` (for start/stop commands)

### data_processor_example/
Multi-channel subscriber and data processing
- **Pattern**: Subscribe to multiple channels → process → publish results
- **States**: INIT → IDLE → PROCESSING → PUBLISHING
- **Publishes**: `DATA_CHAN`
- **Subscribes**: `SENSOR_CHAN`, `BUTTON_CHAN`

---

## 🚀 How to Use

### Step 1: Copy Module to Your Project

```bash
# Navigate to your NCS project
cd my_ncs_project

# Create modules directory if it doesn't exist
mkdir -p src/modules

# Copy desired module (example: button_example)
cp -r ~/.claude/skills/Developer/ncs/project/architecture/smf-zbus/modules/button_example \
     src/modules/button
```

### Step 2: Add Module to CMakeLists.txt

In your project's `src/CMakeLists.txt`:

```cmake
# Add modules subdirectory
add_subdirectory_ifdef(CONFIG_APP_BUTTON modules/button)
add_subdirectory_ifdef(CONFIG_APP_SENSOR modules/sensor)
# Add more modules as needed
```

### Step 3: Add Module to Kconfig

In your project's `src/Kconfig` (create if doesn't exist):

```kconfig
menu "Application modules"

rsource "modules/button/Kconfig.button"
rsource "modules/sensor/Kconfig.sensor"
# Add more modules as needed

endmenu
```

### Step 4: Enable Module in prj.conf

```properties
# Enable button module
CONFIG_APP_BUTTON=y
CONFIG_APP_BUTTON_LOG_LEVEL_DBG=y

# Enable sensor module
CONFIG_APP_SENSOR=y
CONFIG_APP_SENSOR_SAMPLE_INTERVAL_SECONDS=10
```

### Step 5: Use Module in Your Application

In `src/main.c` or other modules:

```c
#include <zephyr/zbus/zbus.h>
#include "modules/button/button_example.h"

/* Subscribe to button events */
static void button_callback(const struct zbus_channel *chan)
{
    const struct button_msg *msg = zbus_chan_const_msg(chan);
    
    if (msg->type == BUTTON_PRESS_SHORT) {
        printk("Button %d short press\n", msg->button_number);
    } else if (msg->type == BUTTON_PRESS_LONG) {
        printk("Button %d long press\n", msg->button_number);
    }
}

ZBUS_LISTENER_DEFINE(my_button_listener, button_callback);
ZBUS_CHAN_ADD_OBS(BUTTON_CHAN, my_button_listener, 0);
```

---

## 📋 Module Structure

Each module follows Nordic Asset Tracker Template pattern:

```
module_name/
├── module_name.c              # Implementation with SMF state machine
├── module_name.h              # Public API and message definitions
├── CMakeLists.txt             # Build configuration
└── Kconfig.module_name        # Configuration options
```

### Required Components

**1. Header File (.h)**:
- Message type enum
- Message structure
- `ZBUS_CHAN_DECLARE()` for channels

**2. Implementation (.c)**:
- `ZBUS_CHAN_DEFINE()` - Define channels
- `ZBUS_MSG_SUBSCRIBER_DEFINE()` - Register subscriber
- `ZBUS_CHAN_ADD_OBS()` - Observe channels
- SMF state machine definitions
- State handler functions
- Module thread with `K_THREAD_DEFINE()`

**3. CMakeLists.txt**:
- `target_sources(app PRIVATE ...)` - Add source files
- `target_include_directories(app PRIVATE .)` - Add includes

**4. Kconfig**:
- Menu for module settings
- Stack size configuration
- Thread priority
- Module-specific settings
- Logging configuration

---

## 🔧 Customization Guide

### Adding a New Module

1. **Copy existing example** closest to your needs:
```bash
cp -r sensor_example my_new_module
```

2. **Rename files**:
```bash
cd my_new_module
mv sensor_example.c my_new_module.c
mv sensor_example.h my_new_module.h
mv Kconfig.sensor Kconfig.my_new_module
```

3. **Update content**:
- Replace `sensor` with `my_new_module` throughout
- Update message types for your use case
- Modify state machine states
- Update Kconfig menu name

4. **Define your states**:
```c
enum my_module_state {
    STATE_INIT,
    STATE_YOUR_STATE_1,
    STATE_YOUR_STATE_2,
    // ... more states
};
```

5. **Implement state handlers**:
```c
static void state_your_state_1_entry(void *obj) {
    /* Entry actions */
}

static enum smf_state_result state_your_state_1_run(void *obj) {
    /* State logic */
    /* Transition: smf_set_state(SMF_CTX(state), &states[STATE_NEXT]); */
    return SMF_STATE_HANDLED();
}
```

---

## 📊 Module Communication Patterns

### Pattern 1: Publisher Only (button_example)
```
Hardware Event → State Machine → zbus_chan_pub() → Other Modules
```

### Pattern 2: Publisher + Subscriber (sensor_example)
```
Command(via zbus) → State Machine → Periodic Sampling → zbus_chan_pub()
                                          ↑
                                    Subscription to own channel for commands
```

### Pattern 3: Multi-Channel Subscriber (data_processor_example)
```
Channel A → \
Channel B → → State Machine → Process Data → zbus_chan_pub()
Channel C → /
```

---

## 🎯 Best Practices

### State Machine Design
- **Entry handlers**: Initialize resources, publish state change
- **Run handlers**: Main state logic, handle messages, transitions
- **Exit handlers**: Cleanup (optional, use sparingly)

### zbus Usage
- **One channel per module**: Module publishes to its own channel
- **Observe others' channels**: Subscribe to get updates
- **Timeout on publish**: Always use `K_SECONDS(1)` or similar
- **Check return values**: Log errors from `zbus_chan_pub()`

### Thread Configuration
- **Stack size**: Start with 2048, increase if needed
- **Priority**: 7 = default mid-priority
- **Watchdog**: Always enable and feed regularly

### Logging
- **Use LOG_DBG**: For state transitions, events
- **Use LOG_INF**: For important lifecycle events
- **Use LOG_ERR**: For errors, failed operations
- **Configure via Kconfig**: `CONFIG_APP_MODULE_LOG_LEVEL_DBG`

---

## 🔍 Debugging

###Enable detailed logging:
```properties
# In prj.conf
CONFIG_APP_BUTTON_LOG_LEVEL_DBG=y
CONFIG_ZBUS_LOG_LEVEL_DBG=y
CONFIG_SMF_LOG_LEVEL_DBG=y
```

### Monitor zbus traffic:
```properties
CONFIG_ZBUS_RUNTIME_OBSERVERS=y
```

### Check state machine execution:
```c
int32_t ret = smf_run_state(SMF_CTX(&state_obj));
if (ret) {
    LOG_ERR("State machine error: %d", ret);
}
```

---

## 📚 Reference

- **Nordic Asset Tracker Template**: `/opt/nordic/ncs/myApps/Asset-Tracker-Template/app/src/modules/`
- **SMF Documentation**: [Zephyr SMF](https://docs.zephyrproject.org/latest/services/smf/index.html)
- **zbus Documentation**: [Zephyr zbus](https://docs.zephyrproject.org/latest/services/zbus/index.html)
- **Architecture Guide**: `../../guides/ARCHITECTURE_PATTERNS.md`

---

**Ready to build professional Nordic NCS projects with modular SMF + zbus architecture!** 🚀
