# Static Web Server Guide

**Complete guide for building HTTP servers with static resources, REST APIs, and WebSockets in Nordic nRF Connect SDK**

Based on Nordic Thingy91x Suitcase Demo - Production-proven implementation

---

## 📋 Overview

This guide shows how to create a full-featured web server on Nordic devices with:
- **Static Resources**: HTML, CSS, JS, images, 3D models (gzip-compressed)
- **Dynamic REST APIs**: JSON endpoints for device control and status
- **WebSockets**: Real-time bidirectional communication
- **mDNS Discovery**: Access via `device.local` hostname

### Architecture

```
┌─────────────┐         HTTP/WS          ┌────────────────┐
│   Browser   │ ◄──────────────────────► │  nRF Device    │
│             │                          │                │
│  - HTML/CSS │                          │ - HTTP Server  │
│  - JavaScript                          │ - Static Files │
│  - WebSocket │                          │ - REST API     │
│  Client     │                          │ - WebSocket    │
└─────────────┘                          └────────────────┘
```

**Use Cases**:
- Device configuration portals
- Real-time sensor dashboards
- OTA firmware update interfaces
- Debug/diagnostic consoles
- Smart home control panels

---

## 🚀 Quick Start

### 1. Project Structure

```
my_webserver_app/
├── CMakeLists.txt                # Build configuration
├── prj.conf                      # Project configuration  
├── sections-rom.ld              # Linker script (required!)
├── src/
│   ├── main.c                   # Application logic
│   ├── http_resources.c         # Resource definitions
│   ├── http_resources.h         # Public interface
│   └── static_web_resources/    # Web files
│       ├── index.html
│       ├── main.js
│       ├── styles.css
│       └── logo.svg
└── build/
```

### 2. Copy Templates

```bash
# Copy webserver templates
cp ~/.claude/skills/Developer/ncs/project/protocols/webserver/templates/* src/

# Copy configuration overlay
cp ~/.claude/skills/Developer/ncs/project/protocols/webserver/overlay-static-webserver.conf .

# Copy Wi-Fi config
cp ~/.claude/skills/Developer/ncs/project/wifi/configs/wifi-sta.conf .
```

### 3. Build & Run

```bash
# Build with web server
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="wifi-sta.conf;overlay-static-webserver.conf"

# Flash
west flash

# Connect to Wi-Fi (via shell)
uart:~$ wifi_cred add "<SSID>" WPA2-PSK "<password>"
uart:~$ wifi connect

# Access web interface
# http://192.168.1.99/  (or your device's IP)
# http://thingy91x.local/  (if mDNS enabled)
```

---

## 📦 Component Details

### Static Resources (HTML, CSS, JS, Images)

Static files are embedded into firmware as gzip-compressed binary data.

#### Step 1: Create Web Files

```bash
mkdir -p src/static_web_resources
cd src/static_web_resources
```

Create `index.html`:
```html
<!DOCTYPE html>
<html>
<head>
    <title>Device Control</title>
    <link rel="stylesheet" href="styles.css">
</head>
<body>
    <h1>Nordic Device Control Panel</h1>
    <div id="status"></div>
    <script src="main.js"></script>
</body>
</html>
```

Create `styles.css`:
```css
body {
    font-family: Arial, sans-serif;
    margin: 20px;
    background-color: #f0f0f0;
}
h1 {
    color: #00a9ce;  /* Nordic blue */
}
```

Create `main.js`:
```javascript
// Fetch device status on load
async function getStatus() {
    const response = await fetch('/api/status');
    const data = await response.json();
    document.getElementById('status').innerHTML = 
        `Uptime: ${data.uptime}s, Temp: ${data.temperature}°C`;
}
getStatus();
```

#### Step 2: Configure CMakeLists.txt

Add to your `CMakeLists.txt`:

```cmake
# Generate gzip-compressed .inc files
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(web_resource
    index.html
    main.js
    styles.css
    )
    generate_inc_file_for_target(
        app
        src/static_web_resources/${web_resource}
        ${gen_dir}/${web_resource}.gz.inc
        --gzip
    )
endforeach()
```

**What this does**:
- Reads each file from `src/static_web_resources/`
- Gzip compresses it (reduces size by ~70%)
- Generates a `.inc` file with C array data
- Places in `build/zephyr/include/generated/`

#### Step 3: Define HTTP Resources

In `http_resources.c`:

```c
#include "http_resources.h"

// Include the generated .inc file
static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

// Define resource metadata
struct http_resource_detail_static index_html_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_STATIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .content_encoding = "gzip",
        .content_type = "text/html",
    },
    .static_data = index_html_gz,
    .static_data_len = sizeof(index_html_gz),
};

// Register at "/" URL
HTTP_RESOURCE_DEFINE(index_html, web_service, "/", &index_html_detail);
```

**Repeat for each file** (main.js, styles.css, images, etc.)

#### Supported Content Types

| File Type | Content-Type | Example |
|-----------|--------------|---------|
| HTML | `text/html` | index.html |
| CSS | `text/css` | styles.css |
| JavaScript | `application/javascript` | main.js |
| JSON | `application/json` | config.json |
| PNG | `image/png` | logo.png |
| JPEG | `image/jpeg` | photo.jpg |
| SVG | `image/svg+xml` | icon.svg |
| WOFF2 Font | `font/woff2` | font.woff2 |
| GLTF/GLB | `model/gltf-binary` | model.glb |

---

### Dynamic Resources (REST API)

Dynamic resources handle runtime data with callbacks.

#### Example: GET /api/status

Returns device status as JSON.

**In http_resources.c:**

```c
static uint8_t status_buf[512];

struct http_resource_detail_dynamic status_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_DYNAMIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .content_type = "application/json",
    },
    .cb = NULL,  // Set later
    .data_buffer = status_buf,
    .data_buffer_len = sizeof(status_buf),
};

HTTP_RESOURCE_DEFINE(status_res, web_service, "/api/status", &status_detail);

void http_resources_set_status_handler(http_resource_dynamic_cb_t handler) {
    status_detail.cb = handler;
}
```

**In main.c:**

```c
int status_handler(struct http_client_ctx *client, 
                   enum http_data_status status,
                   uint8_t *data, size_t len, void *user_data)
{
    if (status == HTTP_SERVER_DATA_FINAL) {
        // Build JSON response
        struct device_status st = {
            .uptime = k_uptime_get_32() / 1000,
            .temperature = read_temperature(),
            .humidity = read_humidity(),
        };
        
        char json[256];
        snprintf(json, sizeof(json),
                 "{\"uptime\":%u,\"temperature\":%.1f,\"humidity\":%.1f}",
                 st.uptime, st.temperature, st.humidity);
        
        return http_server_response_send(client, 
                                         HTTP_SERVER_STATUS_200_OK,
                                         json, strlen(json));
    }
    return 0;
}

void main(void) {
    http_resources_init();
    http_resources_set_status_handler(status_handler);
    // ... start Wi-Fi
}
```

#### Example: POST /api/control

Receives JSON to control device (e.g., LED color).

**Define JSON structure:**

```c
struct led_command {
    int r, g, b;
};

static const struct json_obj_descr led_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct led_command, r, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct led_command, g, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct led_command, b, JSON_TOK_NUMBER),
};
```

**Handler:**

```c
int control_handler(struct http_client_ctx *client,
                    enum http_data_status status,
                    uint8_t *data, size_t len, void *user_data)
{
    if (status == HTTP_SERVER_DATA_FINAL) {
        struct led_command cmd;
        
        // Parse JSON: {"r":255,"g":0,"b":0}
        int ret = json_obj_parse((char *)data, len, 
                                 led_descr, ARRAY_SIZE(led_descr), &cmd);
        if (ret < 0) {
            return http_server_response_send(client, 
                                             HTTP_SERVER_STATUS_400_BAD_REQUEST,
                                             NULL, 0);
        }
        
        // Control hardware
        set_led_rgb(cmd.r, cmd.g, cmd.b);
        
        // Send success response
        const char *resp = "{\"status\":\"ok\"}";
        return http_server_response_send(client,
                                         HTTP_SERVER_STATUS_200_OK,
                                         resp, strlen(resp));
    }
    return 0;
}
```

**JavaScript client:**

```javascript
async function setLED(r, g, b) {
    await fetch('/api/control', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({r, g, b})
    });
}

setLED(255, 0, 0);  // Set red
```

---

### WebSocket (Real-Time Communication)

WebSockets provide bidirectional real-time data streaming.

**Use cases**: Sensor data streaming, live charts, notifications

#### Server-Side (C)

```c
/* WebSocket context for each connection */
struct ws_sensor_ctx {
    int sock;
    struct k_work_delayable work;
    bool active;
};

static struct ws_sensor_ctx ws_contexts[10];

/* Send sensor data periodically */
void ws_sensor_work(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct ws_sensor_ctx *ctx = CONTAINER_OF(dwork, struct ws_sensor_ctx, work);
    
    if (!ctx->active) return;
    
    // Read sensors
    char json[128];
    snprintf(json, sizeof(json), 
             "{\"temp\":%.1f,\"hum\":%.1f,\"time\":%u}",
             read_temp(), read_humidity(), k_uptime_get_32());
    
    // Send via WebSocket
    websocket_send_msg(ctx->sock, json, strlen(json),
                       WEBSOCKET_OPCODE_DATA_TEXT,
                       true, true, K_FOREVER);
    
    // Schedule next update (every 100ms)
    k_work_schedule(&ctx->work, K_MSEC(100));
}

/* WebSocket event handler */
int ws_handler(struct http_client_ctx *client,
               enum http_server_websocket_event event,
               void *user_data)
{
    switch (event) {
    case HTTP_SERVER_WEBSOCKET_CONNECTED:
        /* Find free context slot */
        for (int i = 0; i < ARRAY_SIZE(ws_contexts); i++) {
            if (!ws_contexts[i].active) {
                ws_contexts[i].sock = client->fd;
                ws_contexts[i].active = true;
                k_work_init_delayable(&ws_contexts[i].work, ws_sensor_work);
                k_work_schedule(&ws_contexts[i].work, K_NO_WAIT);
                break;
            }
        }
        break;
        
    case HTTP_SERVER_WEBSOCKET_DISCONNECTED:
        /* Clean up context */
        for (int i = 0; i < ARRAY_SIZE(ws_contexts); i++) {
            if (ws_contexts[i].sock == client->fd) {
                ws_contexts[i].active = false;
                k_work_cancel_delayable(&ws_contexts[i].work);
                break;
            }
        }
        break;
    }
    return 0;
}
```

#### Client-Side (JavaScript)

```javascript
const ws = new WebSocket('ws://192.168.1.99/ws/sensors');

ws.onopen = () => {
    console.log('WebSocket connected');
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log(`Temp: ${data.temp}°C, Humidity: ${data.hum}%`);
    updateChart(data);  // Update live chart
};

ws.onerror = (error) => {
    console.error('WebSocket error:', error);
};

ws.onclose = () => {
    console.log('WebSocket disconnected');
};
```

---

## 🔧 Configuration

### Network Configuration

In `prj.conf`:

```properties
# Static IP (or use DHCP)
CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.168.1.99"
CONFIG_NET_CONFIG_MY_IPV4_NETMASK="255.255.255.0"
CONFIG_NET_CONFIG_MY_IPV4_GW="192.168.1.1"

# HTTP Server Port
CONFIG_HTTP_SERVER_SERVICE_PORT=80

# mDNS Hostname
CONFIG_NET_HOSTNAME="mydevice"
```

Access via:
- `http://192.168.1.99/` (static IP)
- `http://mydevice.local/` (mDNS)

### Memory Requirements

| Component | Flash | RAM | Heap |
|-----------|-------|-----|------|
| HTTP Server Base | 40KB | 30KB | 20KB |
| Static Files (per 10KB) | 3KB | - | - |
| WebSocket | +10KB | +10KB | +20KB |
| JSON Parsing | +5KB | +2KB | +5KB |
| **Total (Typical)** | **100KB** | **80KB** | **90KB** |

Adjust `CONFIG_HEAP_MEM_POOL_SIZE` based on:
- Number of concurrent connections
- Size of static resources
- WebSocket buffer requirements

### Optimization Tips

**Reduce memory usage**:
1. **Gzip**: Already saves ~70% (enabled by default)
2. **Minify**: Minify HTML/CSS/JS before building
3. **CDN**: Load large libraries (Bootstrap, jQuery) from CDN
4. **Lazy Loading**: Load resources on-demand

**Example minification**:
```bash
# Install minifier
npm install -g html-minifier clean-css-cli uglify-js

# Minify files
html-minifier --collapse-whitespace --remove-comments index.html -o index.min.html
cleancss -o styles.min.css styles.css
uglifyjs main.js -o main.min.js
```

---

## 🔒 Security Considerations

### HTTPS (TLS)

For production deployments, use HTTPS:

1. **Generate certificate**:
```bash
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes
```

2. **Enable TLS in prj.conf**:
```properties
CONFIG_NET_SOCKETS_SOCKOPT_TLS=y
CONFIG_MBEDTLS=y
CONFIG_MBEDTLS_TLS_LIBRARY=y
CONFIG_MBEDTLS_HEAP_SIZE=81920
```

3. **Provision certificates** (see `https_request.c` in Thingy91x demo)

### Authentication

Implement authentication for sensitive endpoints:

```c
int check_auth(struct http_client_ctx *client) {
    /* Check Authorization header */
    const char *auth = http_server_get_header(client, "Authorization");
    if (!auth || strcmp(auth, "Bearer secret-token") != 0) {
        return http_server_response_send(client,
                                         HTTP_SERVER_STATUS_401_UNAUTHORIZED,
                                         NULL, 0);
    }
    return 0;
}

int protected_endpoint(struct http_client_ctx *client, ...) {
    if (check_auth(client) != 0) return 0;
    /* Process request */
}
```

### Input Validation

Always validate incoming data:

```c
if (cmd.r < 0 || cmd.r > 255 || cmd.g < 0 || cmd.g > 255 || cmd.b < 0 || cmd.b > 255) {
    return http_server_response_send(client,
                                     HTTP_SERVER_STATUS_400_BAD_REQUEST,
                                     "{\"error\":\"Invalid RGB values\"}",
                                     strlen("{\"error\":\"Invalid RGB values\"}"));
}
```

---

## 📊 Complete Example

See Nordic Thingy91x Suitcase Demo for production reference:
- `/opt/nordic/ncs/myApps/Nordic_Thingy91x_Suitcase_Demo/`

**Features demonstrated**:
- Static HTML/CSS/JS with 3D model viewer
- PWM LED control via POST API
- WebSocket sensor streaming
- WiFi SSID location lookup (nRF Cloud integration)
- mDNS discovery (thingy91x.local)

---

## 🆘 Troubleshooting

### Build Issues

**Error: "sections-rom.ld not found"**
- Ensure `sections-rom.ld` is in project root
- Verify `zephyr_linker_sources(SECTIONS sections-rom.ld)` in CMakeLists.txt

**Error: "index.html.gz.inc not found"**
- Run `west build -p` to regenerate files
- Check `generate_inc_file_for_target()` in CMakeLists.txt

### Runtime Issues

**HTTP server not starting**
- Check `CONFIG_HTTP_SERVER=y`
- Verify network is up before server starts
- Check logs: `CONFIG_HTTP_SERVER_LOG_LEVEL_DBG=y`

**Static files not loading**
- Verify resource URLs match paths (case-sensitive)
- Check Content-Type headers
- Enable browser dev tools network tab

**WebSocket disconnects**
- Increase `CONFIG_NET_SOCKETS_POLL_MAX`
- Check work queue stack size
- Monitor heap usage

### Performance Issues

**Slow page loads**
- Ensure gzip is enabled (`--gzip` in CMake)
- Reduce image sizes
- Use CDN for large libraries

**High memory usage**
- Reduce `CONFIG_HTTP_SERVER_MAX_CLIENTS`
- Decrease buffer sizes
- Profile with `CONFIG_SYS_HEAP_RUNTIME_STATS=y`

---

## 📚 Reference

**Zephyr Documentation**:
- [HTTP Server API](https://docs.zephyrproject.org/latest/connectivity/networking/api/http_server.html)
- [Networking](https://docs.zephyrproject.org/latest/connectivity/networking/index.html)

**Nordic Resources**:
- [nRF7002 Wi-Fi](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/protocols/wifi/index.html)
- [Network Samples](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples.html#networking-samples)

**This Skill**:
- Templates: `protocols/webserver/templates/`
- Overlay: `protocols/webserver/overlay-static-webserver.conf`

---

**Summary**: Build production-grade web interfaces for Nordic devices with static resources, REST APIs, and WebSockets! 🚀
