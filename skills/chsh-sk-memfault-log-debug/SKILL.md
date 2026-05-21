---
name: chsh-sk-memfault-log-debug
description: Debug Memfault log upload issues on NCS/Zephyr: diagnose why logs appear on UART but not in the Memfault cloud, fix ring-buffer CBOR corruption at wrap boundaries, fix stale triggered-state bugs, and verify end-to-end with the cloud API. Use when a Memfault log file is missing expected entries, truncated, or when LOG RESTORE markers don't appear in the cloud after a disconnect/reconnect cycle.
---

# chsh-sk-memfault-log-debug

Debugging guide for Memfault log-upload issues in NCS/Zephyr firmware.
Covers the full cycle: UART vs cloud discrepancy diagnosis → root-cause
identification → SDK patch → automated reconnect test → cloud verification.

---

## The core diagnostic question

> **"Message appears on UART but not in the Memfault cloud log file."**

This symptom means the message was written via the **Zephyr UART backend** but
was **silently dropped from the Memfault ring-buffer backend**.  There are two
known root causes in NCS v3.3.0.

---

## Known Bug 1 — Ring-buffer wrap-boundary CBOR corruption

### File
`modules/lib/memfault-firmware-sdk/components/core/src/memfault_log_data_source.c`

### Symptom
A log entry that spans the 4096-byte wrap boundary of the Memfault circular
ring buffer arrives in the cloud truncated (only the first contiguous chunk
appears, e.g. 29 chars of a 109-char message).

### Root cause
`prv_serialize_msg_callback()` is invoked once **per contiguous region** by
`memfault_circular_buffer_read_with_callback()`.  The original code ignored the
`offset` parameter, so the second call (continuation bytes) was treated as a
fresh entry: it re-parsed the continuation bytes as a timestamp header and
opened a new, wrong-length CBOR string, corrupting the encoding.

### Fix
```c
// When offset > 0 this is a continuation call — just append bytes.
if (offset > 0) {
    const eMemfaultLogRecordType type = memfault_log_get_type_from_hdr(iter->entry.hdr);
    if (type == kMemfaultLogRecordType_Preformatted) {
        return memfault_cbor_join(&ctx->encoder, buf, buf_len);
    }
    return false;  // compact logs can't be split; abort
}
```

And in the first-call path, use `iter->entry.len` (the **full** ring-buffer
entry length) rather than `buf_len` (only the first contiguous region) when
opening the CBOR string:

```c
const size_t ts_size = MEMFAULT_LOG_TIMESTAMPS_ENABLE
    ? (memfault_log_hdr_is_timestamped(iter->entry.hdr) ? sizeof(uint32_t) : 0)
    : 0;
const size_t total_msg_len = iter->entry.len - ts_size;
bool success = memfault_cbor_encode_string_begin(&ctx->encoder, total_msg_len);
return success && memfault_cbor_join(&ctx->encoder, buf, buf_len);
```

---

## Known Bug 2 — Stale `triggered=true` silently drops ring-buffer writes

### File
`nordic-wifi-memfault/src/modules/app_memfault/core/memfault_log_state_restore.c`

### Symptom
After restoring the ring buffer from flash (full at 4096 B), a `LOG_INF()`
call succeeds on UART but the message never appears in the cloud.  The periodic
Memfault upload had fired just before disconnect; DNS failed (`-11 ENETUNREACH`
or `-101`), but `s_memfault_log_data_source_ctx.triggered` was left as `true`.

### Root cause
`prv_try_free_space()` in the SDK refuses to expire old entries while
`triggered=true`.  When the ring buffer is full (4096 B), any new write that
requires expiry is silently rejected.  The Zephyr UART backend is unaffected
(it has its own independent path) — this is why UART shows the message but
cloud does not.

### Fix
Call `memfault_log_data_source_reset()` just before writing the LOG RESTORE
marker.  This zeros `s_memfault_log_data_source_ctx`, clearing `triggered`.
`memfault_log_trigger_collection()` in the on-connect path immediately
re-establishes the trigger with the correct entry count.

```c
/* At top of file: */
extern void memfault_log_data_source_reset(void);

/* In memfault_log_state_restore_on_connect(), just before LOG_INF(): */
memfault_log_data_source_reset();
LOG_INF("=== [LOG RESTORE] pre-disconnect logs above | live session below ===");
log_flush();
```

> **Also note**: use `LOG_INF()` + `log_flush()` rather than
> `memfault_log_save_preformatted()` for the LOG RESTORE marker itself.
> `log_flush()` feeds the message through the Zephyr backend in small internal
> pieces that never hit the wrap-boundary condition (Bug 1).

---

## Memfault Cloud API — Correct URL format

The log-file **download** URL requires the **device serial** in the path.
Using the wrong URL gives a 404 with no useful error message.

```
CORRECT:   GET {BASE}/devices/{serial}/log-files/{cid}/download
WRONG:     GET {BASE}/log-files/{cid}/download   ← returns 404
```

**List log files for a device:**
```
GET {BASE}/devices/{serial}/log-files?limit=N
```

**Python snippet for cloud verification:**
```python
import urllib.request, base64, json, gzip, io

KEY  = "oat_..."
AUTH = "Basic " + base64.b64encode(f":{KEY}".encode()).decode()
BASE = "https://api.memfault.com/api/v0/organizations/nordic/projects/nrf-test"
SERIAL = "F4CE3600AF12"

def api_get(url):
    req = urllib.request.Request(url, headers={"Authorization": AUTH})
    return json.loads(urllib.request.urlopen(req).read())

# List recent log files
files = api_get(f"{BASE}/devices/{SERIAL}/log-files?limit=5")["data"]

# Download and decompress a specific log file (gzip NDJSON)
cid = files[0]["cid"]
req = urllib.request.Request(
    f"{BASE}/devices/{SERIAL}/log-files/{cid}/download",
    headers={"Authorization": AUTH}
)
raw = urllib.request.urlopen(req).read()
lines = [json.loads(l) for l in gzip.decompress(raw).splitlines() if l.strip()]

for i, entry in enumerate(lines):
    if "LOG RESTORE" in entry.get("line", ""):
        print(f"Line {i}: {entry['line'][:80]}")
```

---

## Automated end-to-end test

Full workflow script lives in the project at `scripts/log_restore_test.py`.
Manual inline approach used in this session:

```
Phase 1: Monitor UART, wait for both boards to report MQTT Test OK #2
Phase 2: AP down via router SSH (wl -i wl1.1 bss down)
Phase 3: Wait 7 minutes (let boards attempt reconnect + fail)
Phase 4: AP up (wl -i wl1.1 bss up)
Phase 5: Watch UART for "LOG RESTORE" + "Memfault upload complete"
Phase 6: Poll cloud API every 30s for up to 5 minutes checking for LOG RESTORE
```

Router SSH: `paramiko` to `192.168.92.1`, user `nRF7x`, pwd `@BillionWIFI`.
AP interface: `wl1.1`.

**Success criteria:**
- UART shows `=== [LOG RESTORE] pre-disconnect logs above | live session below ===`
- Cloud log file (cid created after reconnect) contains the LOG RESTORE line
- Both UART and cloud must agree — UART-only means Bug 2 is still present

---

## Diagnosis checklist

When a log message is missing from the cloud:

1. **Check UART vs cloud**: Is it on UART but absent from the cloud?
   - Yes → ring-buffer backend drop (Bug 1 or Bug 2)
   - No → message was never logged at all (different issue)

2. **Was the ring buffer full at time of write?**
   - Check: `Disconnect log-state persisted to external flash (44 + 4096 B)` — 4096 = full
   - Full + stale `triggered` → Bug 2

3. **Was the message near or at the ring-buffer wrap boundary?**
   - Count bytes from the last `memfault_log_trigger_collection()` call
   - Truncated in cloud at a partial length → Bug 1

4. **Did a failed Memfault upload fire just before the disconnect?**
   - Look for `DNS lookup for chunks-nrf.memfault.com failed: -11` (or `-101`) in UART logs
   - Just before `=== WiFi DISCONNECTED` → stale `triggered=true` → Bug 2

5. **Verify cloud URL**: confirm you use `devices/{serial}/log-files/{cid}/download`

---

## Key constants (nordic-wifi-memfault project)

| Item | Value |
|------|-------|
| Ring buffer size | 4096 B |
| Persist trigger | 10 s after WiFi disconnect |
| Log state flash partition | `mflt_log_state_partition` (external NOR, 8 KB) |
| Log state header magic | `0x4d4c5352` (`MLSR`), version 2 |
| SDK function patched | `prv_serialize_msg_callback` in `memfault_log_data_source.c` |
| App function patched | `memfault_log_state_restore_on_connect` in `memfault_log_state_restore.c` |

---

## Verified fix — test result (2026-05-20, NCS v3.3.0)

Both Bug 1 and Bug 2 fixed and verified end-to-end on first test pass:

| Board | LOG RESTORE on UART | Upload complete | Cloud cid | Cloud timestamp |
|-------|---------------------|-----------------|-----------|-----------------|
| nRF7002DK (F4CE36006EC1) | 16:15:24 | 16:15:27 | `6701b79c` | 2026-05-20T14:15:26 |
| nRF54LM20DK+nRF7002EB2 (F4CE3600AF12) | 16:15:39 | 16:15:43 | `8a75e9c3` | 2026-05-20T14:15:42 |

Firmware version at time of fix: `3.2.0.11` (build IDs: `a7106434`, `5f168f30`).
