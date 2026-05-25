# TC-MEMFAULT-LOG — Memfault Log Upload Verification

**When to run**: when PRD includes Memfault observability requirements (logs
appear in cloud after disconnect/reconnect).

## Test Sequence

1. Monitor UART — wait for `MQTT Test OK #2` on each board
2. Take AP down: `wl -i wl1.1 bss down` (router SSH via **chsh-sk-router-control**)
3. Wait ≥ 7 min (boards attempt reconnect and fail)
4. Bring AP up: `wl -i wl1.1 bss up`
5. Watch UART for `=== [LOG RESTORE] pre-disconnect logs above | live session below ===`
6. Poll Memfault cloud API every 30 s for up to 5 min — verify LOG RESTORE in latest log file

## Cloud API Verification

```python
import urllib.request, base64, json, gzip
KEY = "oat_..."
AUTH = "Basic " + base64.b64encode(f":{KEY}".encode()).decode()
BASE = "https://api.memfault.com/api/v0/organizations/nordic/projects/nrf-test"
SERIAL = "<device-serial>"
req = urllib.request.Request(f"{BASE}/devices/{SERIAL}/log-files?limit=5",
                             headers={"Authorization": AUTH})
files = json.loads(urllib.request.urlopen(req).read())["data"]
cid = files[0]["cid"]
req = urllib.request.Request(f"{BASE}/devices/{SERIAL}/log-files/{cid}/download",
                             headers={"Authorization": AUTH})
lines = [json.loads(l) for l in
         gzip.decompress(urllib.request.urlopen(req).read()).splitlines() if l.strip()]
for e in lines:
    if "LOG RESTORE" in e.get("line", ""):
        print(e["line"][:80])
```

## Success Criteria

UART shows LOG RESTORE marker **and** cloud log file contains it after reconnect.

## Known Root Causes (if log missing from cloud)

See **chsh-sk-ncs-tc-memfault-log-debug** for detailed diagnosis:

- **Bug 1**: CBOR corruption at ring-buffer wrap boundary — message truncated in cloud
- **Bug 2**: Stale `triggered=true` silently drops ring-buffer writes after failed upload
