# TC-WIFI-THROUGHPUT — Wi-Fi UDP Throughput Test

**When to run**: when PRD includes throughput acceptance criteria, or to
benchmark sQSPI vs SPI performance.

## Setup

1. Flash both devices with `overlay-zperf.conf` + `overlay-high-performance.conf`
2. Start `iperf -s -u` on Mac (must be on wired Ethernet `192.168.92.178`)

## Run

```bash
SCRIPT=~/.claude/skills/chsh-sk-ncs-tc-wifi-throughput/scripts/run_throughput_test.py
python3 -u $SCRIPT /dev/tty.usbmodem<VCOM1> "label"
```

Results: stdout + `/tmp/results_<label>.json`. ~6–7 min per device.

## Manual Spot-Check (UDP TX)

```
[Mac]  iperf -s -u
[nRF]  wifi connect -s <SSID> -k 1 -p <wifi-password>
[nRF]  zperf udp upload 192.168.92.178 5001 5 1K 50M
```

WiFi credentials: check `chsh-sk-router-control/config.json` → `used_ssid` / `password`.

## Success Criteria

Match or exceed PRD throughput target.
SPI@8MHz baseline: 2.4 GHz TX ~4.9 Mbps, RX ~4.7 Mbps.

## Gotchas

- Always use `python3 -u` (unbuffered) — plain `python3` drops output lines
- Close all serial monitors before running the script
- Start `iperf -s -u` on Mac **before** launching the script
