# NCS Wi-Fi Project Development Guide

## Overview

This guide provides comprehensive information for developing Wi-Fi applications using Nordic nRF70 series (nRF7002/nRF7001) with nRF Connect SDK.

---

## Wi-Fi Hardware Platforms

### Supported Configurations

| Platform | Description | Use Case |
|----------|-------------|----------|
| **nRF7002 DK** | Standalone Wi-Fi development kit (nRF5340 + nRF7002) | Primary Wi-Fi development |
| **nRF54L15 DK + nRF7002EB** | Next-gen low-power MCU with external Wi-Fi board | Low-power Wi-Fi applications |
| **nRF54LM20 DK + nRF7002EB** | Next-gen ultra-low-power MCU with external Wi-Fi board | Ultra-low-power Wi-Fi applications |
| **nRF7002 EK** | Expansion kit shield for other boards | Add Wi-Fi to existing platforms |
| **nRF7001 EK** | Low-power Wi-Fi expansion kit | Power-optimized applications |

### Build Configurations

**nRF7002 DK (Built-in Wi-Fi)**:
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp
```

**nRF54L15 DK + nRF7002EB (External Board)**:
```bash
west build -p -b nrf54l15dk/nrf54l15/cpuapp
```

**nRF54LM20 DK + nRF7002EB (External Board)**:
```bash
west build -p -b nrf54lm20dk/nrf54lm20/cpuapp
```

**nRF5340 Audio DK + nRF7002 EK Shield**:
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek
```

**nRF52840 DK + nRF7002 EK Shield**:
```bash
west build -p -b nrf52840dk/nrf52840 -- -DSHIELD=nrf7002ek
```

---

## Wi-Fi Operating Modes

### 1. Station (STA) Mode

**Description**: Device connects to an existing Wi-Fi access point (router).

**Use Cases**:
- IoT devices connecting to home/office networks
- Cloud connectivity applications
- Standard Wi-Fi client operations

**Key Configurations**:
```properties
CONFIG_WIFI=y
CONFIG_WIFI_NRF70=y
CONFIG_L2_WIFI_CONNECTIVITY=y
CONFIG_L2_WIFI_CONNECTIVITY_AUTO_CONNECT=y
CONFIG_WIFI_CREDENTIALS=y
CONFIG_NET_DHCPV4=y
```

**Code Pattern**:
```c
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_credentials.h>

/* Add credentials via shell */
uart:~$ wifi cred add -s "MySSID" -p "MyPassword" -k 1
uart:~$ wifi cred auto_connect

/* Or programmatically */
struct wifi_credentials_personal creds = {
    .header = {
        .ssid = "MySSID",
        .ssid_len = strlen("MySSID"),
        .type = WIFI_SECURITY_TYPE_PSK,
    },
    .password = "MyPassword",
    .password_len = strlen("MyPassword"),
};
wifi_credentials_set_personal_struct(&creds);
```

---

### 2. Access Point (SoftAP) Mode

**Description**: Device creates its own Wi-Fi network that others can join.

**Use Cases**:
- Device provisioning and configuration
- Local device-to-device communication
- Gateway applications
- Captive portal setups

**Key Configurations**:
```properties
CONFIG_NRF70_AP_MODE=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_AP=y
CONFIG_NET_DHCPV4_SERVER=y
```

**Code Pattern**:
```c
/* Configure SoftAP */
struct wifi_connect_req_params params = {
    .ssid = "MyDeviceAP",
    .ssid_length = strlen("MyDeviceAP"),
    .psk = "password123",
    .psk_length = strlen("password123"),
    .channel = WIFI_CHANNEL_ANY,
    .security = WIFI_SECURITY_TYPE_PSK,
    .band = WIFI_FREQ_BAND_2_4_GHZ,
};

net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params, sizeof(params));

/* Configure static IP: 192.168.1.1 */
/* Start DHCP server: 192.168.1.10-192.168.1.100 */
```

**Network Configuration**:
- AP IP: 192.168.1.1 (typical)
- DHCP range: 192.168.1.10 - 192.168.1.100
- Netmask: 255.255.255.0

---

### 3. Wi-Fi Direct (P2P) Mode

**Description**: Direct device-to-device connection without infrastructure.

**Use Cases**:
- File sharing between devices
- Ad-hoc device pairing
- Direct streaming applications
- Industrial M2M communication

**Key Configurations**:
```properties
CONFIG_NRF70_P2P_MODE=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_WPS=y
CONFIG_NRF_WIFI_LOW_POWER=n  # Disable for P2P
CONFIG_NET_DHCPV4_SERVER=y   # For GO role
CONFIG_NET_DHCPV4=y          # For CLI role
```

**Code Pattern**:
```c
/* Start P2P discovery */
net_mgmt(NET_REQUEST_WIFI_P2P_FIND, iface, NULL, 0);

/* Connect to peer (after discovery) */
struct wifi_p2p_connect_params p2p_params = {
    .peer_address = {0xf4, 0xce, 0x36, 0x00, 0xae, 0xec},
    .go_intent = 15,  /* 15=always GO, 0=always CLI */
    .wps_method = WIFI_WPS_METHOD_PBC,
};

net_mgmt(NET_REQUEST_WIFI_P2P_CONNECT, iface, &p2p_params, sizeof(p2p_params));
```

**Roles**:
- **GO (Group Owner)**: Acts as access point, provides DHCP
- **CLI (Client)**: Connects to GO, receives IP via DHCP

**GO Intent**:
- 0-15 range (higher = more likely to be GO)
- 15: Always become GO
- 0: Always become Client
- 1-14: Negotiated based on both devices' values

---

### 4. Monitor (Promiscuous) Mode

**Description**: Capture all Wi-Fi packets on a channel.

**Use Cases**:
- Network analysis and debugging
- Latency measurements
- Security auditing
- Protocol analysis

**Key Configurations**:
```properties
CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON=y
CONFIG_NRF_WIFI_IFACE_MTU=1500
CONFIG_NRF_WIFI_LOW_POWER=n
```

**Code Pattern**:
```c
/* Enable monitor mode */
struct wifi_mode_info mode_info = {
    .mode = WIFI_MODE_MONITOR,
};
net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));

/* Set channel */
struct wifi_channel_info channel_info = {
    .channel = 2,
};
net_mgmt(NET_REQUEST_WIFI_CHANNEL, iface, &channel_info, sizeof(channel_info));

/* Receive raw packets */
void packet_handler(struct net_if *iface, struct net_pkt *pkt) {
    /* Process raw 802.11 frame */
}
```

---

### 5. Raw Packet Injection

**Description**: Send custom 802.11 frames without association.

**Use Cases**:
- Latency benchmarking
- Custom protocol implementation
- Beacon frame transmission
- Test equipment

**Key Configurations**:
```properties
CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON=y
CONFIG_NRF70_RAW_DATA_TX=y
CONFIG_NRF_WIFI_LOW_POWER=n
```

**Code Pattern**:
```c
/* Send raw beacon frame */
struct wifi_raw_tx_params tx_params = {
    .channel = 11,
    .rate = WIFI_DATA_RATE_1M,
};

uint8_t beacon_frame[] = {
    /* 802.11 beacon frame */
    0x80, 0x00,  /* Frame control */
    /* ... MAC header ... */
    /* ... Beacon body ... */
};

net_mgmt(NET_REQUEST_WIFI_RAW_TX, iface, &tx_params, sizeof(tx_params));
```

---

## Common Wi-Fi Application Patterns

### Pattern 1: UDP Echo Server/Client

**Use Case**: Simple data exchange, testing, latency measurement

**Server Code**:
```c
int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(5001),
    .sin_addr.s_addr = INADDR_ANY,
};

bind(sock, (struct sockaddr *)&addr, sizeof(addr));

while (1) {
    char buf[1024];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int len = recvfrom(sock, buf, sizeof(buf), 0,
                      (struct sockaddr *)&client_addr, &client_len);
    
    /* Echo back */
    sendto(sock, buf, len, 0,
          (struct sockaddr *)&client_addr, client_len);
}
```

**Client Code**:
```c
int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

struct sockaddr_in server_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(5001),
};
inet_pton(AF_INET, "192.168.1.1", &server_addr.sin_addr);

char buf[64] = "Hello";
sendto(sock, buf, strlen(buf), 0,
      (struct sockaddr *)&server_addr, sizeof(server_addr));

recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
```

---

### Pattern 2: TCP Streaming

**Use Case**: Reliable data transfer, file transfer, audio streaming

**Server Code**:
```c
int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(8080),
    .sin_addr.s_addr = INADDR_ANY,
};

bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
listen(listen_sock, 1);

int client_sock = accept(listen_sock, NULL, NULL);

/* Stream data */
while (1) {
    send(client_sock, data, sizeof(data), 0);
    k_msleep(10);
}
```

**Client Code**:
```c
int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

struct sockaddr_in server_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(8080),
};
inet_pton(AF_INET, "192.168.1.1", &server_addr.sin_addr);

connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

/* Receive stream */
while (1) {
    char buf[1024];
    recv(sock, buf, sizeof(buf), 0);
    /* Process data */
}
```

---

### Pattern 3: MQTT Client

**Use Case**: IoT telemetry, cloud connectivity

**Configuration**:
```properties
CONFIG_MQTT_LIB=y
CONFIG_MQTT_LIB_TLS=y
CONFIG_NET_SOCKETS_SOCKOPT_TLS=y
CONFIG_MBEDTLS=y
```

**Code**:
```c
#include <net/mqtt.h>

struct mqtt_client client;
uint8_t rx_buffer[256];
uint8_t tx_buffer[256];

mqtt_client_init(&client);

struct sockaddr_storage broker;
struct mqtt_utf8 client_id = {
    .utf8 = "nrf_device",
    .size = strlen("nrf_device"),
};

client.broker = &broker;
client.client_id = client_id;
client.rx_buf = rx_buffer;
client.rx_buf_size = sizeof(rx_buffer);
client.tx_buf = tx_buffer;
client.tx_buf_size = sizeof(tx_buffer);

mqtt_connect(&client);

/* Publish */
struct mqtt_publish_param param = {
    .message.topic.utf8 = "sensors/temp",
    .message.payload.data = "25.5",
};
mqtt_publish(&client, &param);
```

---

### Pattern 4: DNS-SD Service Discovery

**Use Case**: Automatic device discovery on local network

**Configuration**:
```properties
CONFIG_DNS_SD=y
CONFIG_MDNS_RESPONDER=y
```

**Code**:
```c
#include <zephyr/net/dns_sd.h>

/* Advertise service */
DNS_SD_REGISTER_SERVICE(audio_service,
    "_audiostream._tcp.local",
    "local",
    DNS_SD_EMPTY_TXT,
    8080  /* port */
);

/* Discover services */
dns_sd_query("_audiostream._tcp.local", callback);
```

---

## Memory Management for Wi-Fi

### Typical Memory Requirements

| Configuration | Heap | Stack (Main) | Stack (NET_RX) | Stack (NET_TX) |
|---------------|------|--------------|----------------|----------------|
| STA Mode | 80 KB | 4 KB | 4 KB | 4 KB |
| SoftAP Mode | 80 KB | 4 KB | 4 KB | 4 KB |
| P2P Mode | 100 KB | 5 KB | 4 KB | 4 KB |
| Monitor Mode | 80 KB | 4 KB | 4 KB | 4 KB |

### Wi-Fi Heap Configuration

```properties
# Total heap pool
CONFIG_HEAP_MEM_POOL_SIZE=80000
CONFIG_HEAP_MEM_POOL_IGNORE_MIN=y

# Wi-Fi specific heaps
CONFIG_NRF_WIFI_CTRL_HEAP_SIZE=40000
CONFIG_NRF_WIFI_DATA_HEAP_SIZE=100000
```

### Buffer Configuration

```properties
# Network buffers
CONFIG_NET_PKT_RX_COUNT=16
CONFIG_NET_PKT_TX_COUNT=16
CONFIG_NET_BUF_RX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=32

# Wi-Fi RX buffers
CONFIG_NRF70_RX_NUM_BUFS=16
```

---

## Power Management

### Low Power Mode

**Enable** (for production):
```properties
CONFIG_NRF_WIFI_LOW_POWER=y
CONFIG_NRF_WIFI_PS_MODE=y
```

**Disable** (for development/debugging):
```properties
CONFIG_NRF_WIFI_LOW_POWER=n
```

### Power Save Modes

| Mode | Description | Use Case |
|------|-------------|----------|
| **Active** | Full power, no sleep | High throughput |
| **PS-Poll** | Legacy power save | Low data rate |
| **U-APSD** | Unscheduled APSD | VoIP applications |

---

## Debugging Wi-Fi Applications

### Enable Comprehensive Logging

```properties
CONFIG_LOG=y
CONFIG_LOG_MODE_IMMEDIATE=y
CONFIG_LOG_BUFFER_SIZE=8192

# Wi-Fi specific logging
CONFIG_WIFI_NM_WPA_SUPPLICANT_LOG_LEVEL_DBG=y
CONFIG_NET_LOG=y

# Network shell
CONFIG_SHELL=y
CONFIG_NET_SHELL=y
```

### Useful Shell Commands

```bash
# Wi-Fi status
uart:~$ wifi status

# Scan networks
uart:~$ wifi scan

# Statistics
uart:~$ wifi statistics

# Network interface info
uart:~$ net iface

# Connection status
uart:~$ net conn

# Ping test
uart:~$ net ping 192.168.1.1
```

### Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| Connection timeout | Wrong credentials | Check SSID/password |
| Low memory error | Insufficient heap | Increase HEAP_MEM_POOL_SIZE |
| DHCP failure | Network issue | Check router DHCP settings |
| Packet loss | Low power mode | Disable NRF_WIFI_LOW_POWER |
| Build errors | Missing configs | Check overlay files |

---

## Performance Optimization

### Throughput Optimization

```properties
# Increase buffers
CONFIG_NET_BUF_TX_COUNT=64
CONFIG_NET_BUF_RX_COUNT=64
CONFIG_NRF70_RX_NUM_BUFS=32

# Optimize TCP
CONFIG_NET_TCP_BACKLOG_SIZE=4
CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE=4096

# Disable low power
CONFIG_NRF_WIFI_LOW_POWER=n
```

### Latency Optimization

```properties
# Immediate processing
CONFIG_NET_TC_TX_COUNT=1
CONFIG_NET_RX_STACK_SIZE=4096

# Disable power save
CONFIG_NRF_WIFI_LOW_POWER=n

# Use UDP instead of TCP for real-time
```

---

## Testing and Validation

### Connectivity Tests

1. **Basic Connectivity**: `net ping`
2. **Speed Test**: iperf3 UDP/TCP
3. **Stability Test**: Long-duration connection
4. **Roaming Test**: Multiple APs
5. **Range Test**: Various distances

### Performance Metrics

- **Throughput**: UDP/TCP bandwidth
- **Latency**: RTT measurements
- **Packet Loss**: Error rate
- **Power Consumption**: Active/sleep current
- **Connection Time**: Association duration

---

## Best Practices

1. **Always use WIFI_READY_LIB**: Ensures safe Wi-Fi initialization
2. **Disable low power during development**: Easier debugging
3. **Use overlays for configurations**: Keep prj.conf minimal
4. **Handle network events properly**: Subscribe to all relevant events
5. **Implement timeouts**: Prevent indefinite waits
6. **Use credentials library**: Secure credential storage
7. **Test with various APs**: Compatibility verification
8. **Monitor memory usage**: Prevent heap exhaustion
9. **Log strategically**: Balance verbosity and performance
10. **Version control firmware**: Track configuration changes

---

## Additional Resources

- [nRF7002 Product Specification](https://www.nordicsemi.com/Products/nRF7002)
- [NCS Wi-Fi Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/protocols/wifi/index.html)
- [Zephyr Networking](https://docs.zephyrproject.org/latest/connectivity/networking/index.html)
- [Nordic DevZone Wi-Fi Forum](https://devzone.nordicsemi.com/search?q=wifi)
