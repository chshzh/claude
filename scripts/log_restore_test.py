#!/usr/bin/env python3
"""
Log Restore Verification Test
Monitors two Nordic boards, triggers WiFi AP disconnect, and verifies
=== [LOG RESTORE] === marker appears after board reconnects.
"""

import serial
import threading
import subprocess
import time
import sys
from datetime import datetime
from collections import deque

# ── Board config ───────────────────────────────────────────────────────────
BOARDS = {
    "nRF54LM20DK": {
        "port":   "/dev/tty.usbmodem0010518067141",  # VCOM0, UART30
        "baud":   115200,
        "rtscts": True,
        "sn":     "1051806714",
    },
    "nRF7002DK": {
        "port":   "/dev/tty.usbmodem0010507879623",  # VCOM1
        "baud":   115200,
        "rtscts": False,
        "sn":     "1050787962",
    },
}

# ── Router config ──────────────────────────────────────────────────────────
ROUTER_INFO_PATH = Path.home() / ".claude" / "skills" / "chsh-sk-router-control" / "config.json"
ROUTER_IFACE = "wl1.1"


def load_router_info():
    return json.loads(ROUTER_INFO_PATH.read_text())

# ── Detection markers ──────────────────────────────────────────────────────
# Memfault upload: match any of these (case-insensitive)
UPLOAD_MARKERS = [
    "uploading queued data",
    "upload complete",
    "chunk(s) sent",
    "memfault_http",
    "posting data",
    "posted data",
    "mflt",          # common short prefix in NCS memfault logs
]
# We count a "cycle" when we see a line matching a cycle-start or cycle-end heuristic
UPLOAD_CYCLE_MARKERS = [
    "uploading",
    "posted",
    "chunk(s) sent",
    "upload complete",
    "http_post",
]
DISCONNECT_MARKERS = [
    "disconnected",
    "is down",
    "link down",
    "wifi_mgmt_event_handler",
    "wifi_state",
    "wlan0",
    "reconnect",
]
LOG_RESTORE_MARKER = "=== [LOG RESTORE]"

# ── Shared state (guarded by state_lock) ──────────────────────────────────
state_lock = threading.Lock()
state = {
    name: {
        "upload_cycles":      0,
        "last_upload_line":   "",
        "disconnect_seen":    False,
        "restore_seen":       False,
        "restore_ts":         None,
        "restore_snippet":    [],   # ~20 lines around the marker
        "log":                [],   # full captured log
        "recent":             deque(maxlen=30),  # sliding window for snippet
    }
    for name in BOARDS
}

stop_event = threading.Event()


# ── Helpers ────────────────────────────────────────────────────────────────
def ts():
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]


def print_banner(msg):
    print(f"\n{'─'*60}")
    print(f"  {msg}")
    print(f"{'─'*60}")


def wait_for(cond_fn, timeout, poll=0.5, label="condition"):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if cond_fn():
            return True
        time.sleep(poll)
    print(f"[{ts()}] TIMEOUT waiting for: {label}")
    return False


# ── Serial monitor thread ──────────────────────────────────────────────────
def monitor_board(name, cfg):
    port   = cfg["port"]
    baud   = cfg["baud"]
    rtscts = cfg["rtscts"]

    print(f"[{ts()}] [{name}] Connecting to {port} (rtscts={rtscts})…")
    try:
        ser = serial.Serial(port, baud, rtscts=rtscts, timeout=0.5)
    except Exception as e:
        print(f"[{ts()}] [{name}] ERROR opening {port}: {e}")
        return

    print(f"[{ts()}] [{name}] Connected.")

    # Track consecutive lines with upload marker to avoid over-counting
    last_upload_bucket = -1   # which 30-s bucket we last incremented in

    while not stop_event.is_set():
        try:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").rstrip()
            if not line:
                continue

            stamped = f"[{ts()}] {line}"
            print(f"  [{name}] {stamped}")

            ll = line.lower()

            with state_lock:
                st = state[name]
                st["log"].append(stamped)
                st["recent"].append(stamped)

                # ── Memfault upload cycle detection ──────────────────────
                if any(m in ll for m in UPLOAD_CYCLE_MARKERS):
                    # bucket by 30-second windows to avoid counting bursts
                    bucket = int(time.time() / 30)
                    if bucket != last_upload_bucket:
                        last_upload_bucket = bucket
                        st["upload_cycles"] += 1
                        st["last_upload_line"] = line
                        c = st["upload_cycles"]
                        print(f"[{ts()}] [{name}] *** Memfault upload cycle #{c} *** — {line[:80]}")

                # ── Disconnect detection ──────────────────────────────────
                if not st["disconnect_seen"] and any(m in ll for m in DISCONNECT_MARKERS):
                    st["disconnect_seen"] = True
                    print(f"[{ts()}] [{name}] *** DISCONNECT detected: {line[:80]} ***")

                # ── LOG RESTORE detection ─────────────────────────────────
                if not st["restore_seen"] and LOG_RESTORE_MARKER in line:
                    st["restore_seen"]   = True
                    st["restore_ts"]     = ts()
                    st["restore_snippet"] = list(st["recent"])  # last ≤30 lines
                    print(f"[{ts()}] [{name}] *** LOG RESTORE MARKER FOUND! ***")

        except serial.SerialException as e:
            print(f"[{ts()}] [{name}] Serial error: {e}")
            time.sleep(2)
        except Exception as e:
            print(f"[{ts()}] [{name}] Unexpected error: {e}")

    # Capture trailing lines after stop for snippet completion
    with state_lock:
        st = state[name]
        if st["restore_seen"] and len(st["restore_snippet"]) < 20:
            st["restore_snippet"] = list(st["recent"])

    try:
        ser.close()
    except Exception:
        pass
    print(f"[{ts()}] [{name}] Serial closed.")


# ── Router control ─────────────────────────────────────────────────────────
def router_cmd(cmd):
    import paramiko
    router_info = load_router_info()
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(router_info["host"], username=router_info["username"], password=router_info["password"], timeout=10)
    _, stdout, stderr = ssh.exec_command(cmd)
    out = stdout.read().decode().strip()
    err = stderr.read().decode().strip()
    ssh.close()
    return out, err


def ap_down():
    print(f"[{ts()}] Router: wl -i {ROUTER_IFACE} bss down")
    out, err = router_cmd(f"wl -i {ROUTER_IFACE} bss down")
    if out: print(f"[{ts()}]   stdout: {out}")
    if err: print(f"[{ts()}]   stderr: {err}")


def ap_up():
    print(f"[{ts()}] Router: wl -i {ROUTER_IFACE} bss up")
    out, err = router_cmd(f"wl -i {ROUTER_IFACE} bss up")
    if out: print(f"[{ts()}]   stdout: {out}")
    if err: print(f"[{ts()}]   stderr: {err}")


# ── Board reset ────────────────────────────────────────────────────────────
def reset_board(name, sn):
    print(f"[{ts()}] Resetting {name} (SN={sn})…")
    r = subprocess.run(
        ["nrfutil", "device", "reset", "--serial-number", sn],
        capture_output=True, text=True, timeout=30,
    )
    msg = r.stdout.strip() or r.stderr.strip()
    print(f"[{ts()}]   rc={r.returncode} {msg}")
    return r.returncode


# ── Main test sequence ─────────────────────────────────────────────────────
def main():
    print_banner(f"LOG RESTORE VERIFICATION TEST  —  {datetime.now()}")

    # ── Step 1: Start monitoring ───────────────────────────────────────────
    print_banner("STEP 1 — Start serial monitors")
    threads = []
    for name, cfg in BOARDS.items():
        t = threading.Thread(target=monitor_board, args=(name, cfg), daemon=True)
        t.start()
        threads.append(t)
    time.sleep(2)

    # ── Step 2: Wait for 2 Memfault upload cycles on BOTH boards ──────────
    print_banner("STEP 2 — Wait for ≥2 Memfault upload cycles on BOTH boards (timeout 5 min)")

    def both_uploaded():
        with state_lock:
            return all(state[n]["upload_cycles"] >= 2 for n in BOARDS)

    upload_ok = wait_for(both_uploaded, timeout=300, label="2 upload cycles on both boards")

    with state_lock:
        for n in BOARDS:
            print(f"[{ts()}]   {n}: {state[n]['upload_cycles']} cycle(s)  — last: {state[n]['last_upload_line'][:60]}")

    if not upload_ok:
        print(f"[{ts()}] WARNING: Did not get 2 confirmed cycles — proceeding anyway.")

    # ── Step 3: Disable AP ─────────────────────────────────────────────────
    print_banner("STEP 3 — Disable BE92U_5G AP")
    try:
        ap_down()
    except Exception as e:
        print(f"[{ts()}] ERROR: {e}")

    # ── Step 4: Wait 15 s for disconnect handling ──────────────────────────
    print_banner("STEP 4 — Wait 15 s for disconnect / log-save")
    time.sleep(15)

    with state_lock:
        for n in BOARDS:
            print(f"[{ts()}]   {n}: disconnect_seen={state[n]['disconnect_seen']}")

    # ── Step 5: Reset both boards ──────────────────────────────────────────
    print_banner("STEP 5 — Reset both boards")
    for name, cfg in BOARDS.items():
        reset_board(name, cfg["sn"])
    time.sleep(1)

    # ── Step 6: Re-enable AP ───────────────────────────────────────────────
    print_banner("STEP 6 — Re-enable BE92U_5G AP")
    try:
        ap_up()
    except Exception as e:
        print(f"[{ts()}] ERROR: {e}")

    # ── Step 7: Wait for LOG RESTORE on both boards (90 s) ────────────────
    print_banner("STEP 7 — Wait for LOG RESTORE marker (timeout 90 s)")
    restore_start = time.time()

    def both_restored():
        with state_lock:
            return all(state[n]["restore_seen"] for n in BOARDS)

    restore_ok = wait_for(both_restored, timeout=90, label="LOG RESTORE on both boards")

    # Capture a few more lines after marker then stop
    time.sleep(5)
    stop_event.set()
    for t in threads:
        t.join(timeout=6)

    elapsed = time.time() - restore_start

    # ── Step 8: Report ─────────────────────────────────────────────────────
    print_banner(f"STEP 8 — RESULTS  (waited {elapsed:.1f}s for restore)")

    all_pass = True
    for name in BOARDS:
        with state_lock:
            st = state[name]
            restored        = st["restore_seen"]
            restore_ts_val  = st["restore_ts"]
            snippet         = st["restore_snippet"]
            uploads         = st["upload_cycles"]
            disconnected    = st["disconnect_seen"]

        pass_fail = "PASS" if restored else "FAIL"
        if not restored:
            all_pass = False

        print(f"\n{'='*50}")
        print(f"  Board:                {name}")
        print(f"  Memfault cycles:      {uploads}")
        print(f"  Disconnect seen:      {disconnected}")
        print(f"  LOG RESTORE seen:     {restored}  →  {pass_fail}")
        if restored:
            print(f"  LOG RESTORE ts:       {restore_ts_val}")
            print(f"\n  UART snippet (~20 lines around LOG RESTORE):")
            for l in snippet:
                print(f"    {l}")
        else:
            print(f"  LOG RESTORE:          NOT seen within timeout")
            # Show last 10 lines of log for debugging
            with state_lock:
                tail = state[name]["log"][-10:]
            print(f"\n  Last 10 UART lines:")
            for l in tail:
                print(f"    {l}")

    print(f"\n{'='*60}")
    print(f"  OVERALL: {'PASS ✓' if all_pass else 'FAIL ✗'}")
    print(f"{'='*60}\n")

    return 0 if all_pass else 1


if __name__ == "__main__":
    sys.exit(main())
