---
name: chsh-sk-ncs-wifi-throughput
description: >-
  Run automated UDP throughput tests on nRF54LM20DK + nRF7002EB2 hardware using
  zperf (on-device) and iperf (Mac), compare sQSPI vs SPI interfaces, and update
  Jira results. Use when testing Wi-Fi throughput on nRF devices, running
  zperf/iperf benchmarks, debugging sQSPI or SPI bus performance, or re-running
  the WCS-121 test suite.
---

# nRF Wi-Fi Throughput Test

Automated UDP TX/RX benchmark comparing sQSPI@32MHz vs SPI@8MHz on
nRF54LM20DK + nRF7002EB2, using zperf (nRF side) and iperf v2 (Mac side).

**Script:** `~/.claude/skills/chsh-sk-ncs-wifi-throughput/scripts/run_throughput_test.py`

---

## Pre-flight Checklist

Before running the script:

- [ ] Both devices are flashed (see [Build & Flash](#build--flash))
- [ ] Both UART ports are free (no other serial terminals open on them)
- [ ] Wi-Fi router is on and both SSIDs are reachable (`BE92U_2.4G`, `BE92U_5G`)
- [ ] Mac is connected to router via **wired Ethernet** (`192.168.92.178`)
- [ ] `iperf -s -u` is running in a **separate terminal** on the Mac (for TX runs)

```bash
# Keep this running in a separate terminal throughout the test:
iperf -s -u
```

---

## Devices

| Label | Interface | J-Link SN | UART port | Shield |
|-------|-----------|-----------|-----------|--------|
| `sQSPI@32MHz` | sQSPI | 1051869687 | `/dev/tty.usbmodem0010518696873` (VCOM1) | `nrf7002eb2_mspi` |
| `SPI@8MHz` | SPI | 1051851328 | `/dev/tty.usbmodem0010518513281` (VCOM0) | `nrf7002eb2` |

---

## Build & Flash

Overlays required: `overlay-zperf.conf` + `overlay-high-performance.conf`
(copy both from `nrf/samples/wifi/throughput/` if not already in the project).

```bash
# Device 1 — sQSPI (nordic-wifi-shell-sqspi project)
west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp \
  -d nordic-wifi-shell-sqspi/build_zperf nordic-wifi-shell-sqspi \
  -- -DSHIELD=nrf7002eb2_mspi \
     -DEXTRA_CONF_FILE="overlay-zperf.conf;overlay-high-performance.conf"
west flash -d nordic-wifi-shell-sqspi/build_zperf --recover --dev-id 1051869687

# Device 2 — SPI (upstream wifi/shell sample)
west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp \
  -d nrf/samples/wifi/shell/build_zperf nrf/samples/wifi/shell \
  -- -DSHIELD=nrf7002eb2 \
     -DEXTRA_CONF_FILE="overlay-zperf.conf;overlay-high-performance.conf"
west flash -d nrf/samples/wifi/shell/build_zperf --recover --dev-id 1051851328
```

Wrap each `west` command with `nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 --` when running outside the managed shell.

---

## Running the Test

Run one device at a time. Always use `python3 -u` (unbuffered — otherwise output is hidden until script exits).

```bash
SCRIPT=~/.claude/skills/chsh-sk-ncs-wifi-throughput/scripts/run_throughput_test.py

# Device 1
python3 -u $SCRIPT /dev/tty.usbmodem0010518696873 "sQSPI@32MHz"

# Device 2
python3 -u $SCRIPT /dev/tty.usbmodem0010518513281 "SPI@8MHz"
```

Results are printed to stdout **and** saved to `/tmp/results_<label>.json`.

Expected total time: **~6–7 min per device** (2 SSIDs × 2 directions × 3 runs × 5 s + overhead).

---

## Configurable Constants (top of script)

| Constant | Default | Notes |
|----------|---------|-------|
| `SSID_24` | `BE92U_2.4G` | 2.4 GHz SSID |
| `SSID_5` | `BE92U_5G` | 5 GHz SSID |
| `PASS` | `@BillionWIFI` | Wi-Fi password |
| `MAC_IP` | `192.168.92.178` | Mac iperf endpoint |
| `DURATION` | `5` | Seconds per run |
| `RUNS` | `3` | Measured runs per direction |

---

## Manual Commands (for spot-checks or re-runs)

**Server command first**, then client.

```
# UDP TX (nRF → Mac)
[Mac]  iperf -s -u
[nRF]  zperf udp upload 192.168.92.178 5001 5 1K 50M

# UDP RX (Mac → nRF)
[nRF]  zperf udp download 5001
[Mac]  iperf -c <nRF_IP> -u -p 5001 -t 5 -b 50M -l 1000
```

Connect to Wi-Fi first:
```
wifi connect -s BE92U_2.4G -k 1 -p @BillionWIFI
wifi status   # confirm IP + PHY TX rate
```

---

## Known Gotchas

| Problem | Cause | Fix |
|---------|-------|-----|
| Script hangs, no output | Python stdout buffering | Always use `python3 -u` |
| Serial port busy | Another terminal has the port open | Close all other serial monitors |
| `RX run 3: parse failed` | zperf stats printed during sleep gap, flushed before read | Script restarts `zperf udp download` before each run — fixed in v3 |
| `UDP upload failed (-1)` | iperf server not running on Mac | Start `iperf -s -u` before running script |
| Low sQSPI throughput + `xfer timeout: cmd=0xeb` | FLPR VPR sQSPI read deadline missed at 32 MHz | Known issue — see WCS-121 conclusion for next debug steps |
| IP not assigned after connect | DHCP slow on first connect | Script waits up to 60 s; retry connect manually if it fails |
| PHY rate settles slowly (sQSPI) | 802.11 rate adaptation after association | Script waits 10 s after IP before starting traffic |

---

## Updating Jira Results

After the run, copy values from stdout into the WCS-121 results table using the Atlassian MCP (`editJiraIssue`):

- Cloud ID: `1dd50090-232c-480b-8bbe-6d89df728f94`
- Issue: `WCS-121`
- Format: `run1/run2/run3` Mbps per cell
- Include test execution time from the `Total test time:` line

---

## Result Interpretation

| sQSPI@32MHz | SPI@8MHz | Assessment |
|-------------|----------|------------|
| ~0.4–0.8 Mbps | ~4.3–5.1 Mbps | sQSPI underperforming — xfer timeouts suspected |
| Matches SPI | Matches SPI | Bus issue resolved |
| >> SPI@32MHz baseline (~10–12 Mbps) | — | sQSPI working as designed |

SPI@8MHz reference baseline from WCS-78 (NCS v3.2.0-preview2):
- 2.4 GHz: TX 4.85–4.92, RX 4.66–4.71 Mbps
- 5 GHz: TX 4.74–4.86, RX 4.63–4.65 Mbps
