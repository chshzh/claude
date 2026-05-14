#!/usr/bin/env python3
"""
WCS-121 automated zperf throughput test — improved v3.

Key design decisions:
  - TX: reuses any existing `iperf -s -u` running in user's terminal (don't kill it).
        Starts one only if none is found. Parses rate from zperf nRF output.
  - RX: starts `zperf udp download 5001` once per band, then loops 3x iperf -c.
        Parses rate from nRF output after stripping ANSI escape codes.
  - Warmup: 1 extra zperf run before the 3 measured runs (stabilises sQSPI bus).

Usage: python3 -u ~/.claude/skills/chsh-sk-ncs-wifi-throughput/scripts/run_throughput_test.py <serial_port> <device_label>
Prereq: have `iperf -s -u` already running in a terminal (for TX measurement).
"""
import serial, time, re, sys, subprocess, json

PORT     = sys.argv[1]
LABEL    = sys.argv[2]
BAUD     = 115200
SSID_24  = "EX75_2.4G"
SSID_5   = "EX75_5G"
PASS     = "@BillionWIFI"
MAC_IP   = "192.168.75.216"
RUNS     = 3
DURATION = 5   # 5 s per run — sufficient for UDP steady state; halves test time vs 10 s

# ── ANSI stripping ────────────────────────────────────────────────────────────

def strip_ansi(text):
    """Remove ANSI/CSI escape codes and shell prompt interleaving from nRF serial output."""
    # Standard ESC-prefixed sequences: ESC[params...letter
    text = re.sub(r'\x1b\[[0-9;]*[A-Za-z]', '', text)
    # Bare CSI sequences without ESC (as nRF shell sometimes emits): [params...letter
    # * not + so it also catches [J (zero digits), [m, [8D, [1;32m, etc.
    text = re.sub(r'\[[0-9;]*[A-Za-z]', '', text)
    # Shell prompt text that gets interleaved with async zperf stats output
    text = re.sub(r'uart:~\$\s*', ' ', text)
    return text

# ── Serial helpers ────────────────────────────────────────────────────────────

def open_port(port):
    s = serial.Serial(port, BAUD, timeout=1, rtscts=False, dsrdtr=False)
    time.sleep(1)
    s.reset_input_buffer()
    return s

def send(ser, cmd):
    ser.write((cmd + "\r\n").encode())

def read_until(ser, markers, timeout=60):
    if isinstance(markers, str):
        markers = [markers]
    buf = ""
    deadline = time.time() + timeout
    while time.time() < deadline:
        data = ser.read(ser.in_waiting or 1)
        buf += data.decode(errors="replace")
        clean = strip_ansi(buf)
        if any(m in clean for m in markers):
            break
        time.sleep(0.05)
    return buf

def read_for(ser, seconds):
    """Collect serial output for a fixed duration."""
    buf = ""
    deadline = time.time() + seconds
    while time.time() < deadline:
        data = ser.read(ser.in_waiting or 1)
        buf += data.decode(errors="replace")
        time.sleep(0.05)
    return buf

# ── Wi-Fi ─────────────────────────────────────────────────────────────────────

def wait_for_ready(ser, timeout=30):
    """Wait for shell prompt after possible reboot."""
    out = read_until(ser, ["uart:~$", "wpa_supplicant initialized"], timeout=timeout)
    if "wpa_supplicant" in strip_ansi(out):
        # Device just booted — wait for full init
        read_until(ser, "uart:~$", timeout=15)
    time.sleep(1)
    ser.reset_input_buffer()

def disconnect(ser):
    send(ser, "wifi disconnect")
    time.sleep(3)
    ser.reset_input_buffer()

def connect_wifi(ser, ssid, timeout=60):
    print(f"    Connecting to {ssid}...")
    wait_for_ready(ser, timeout=5)
    ser.reset_input_buffer()
    send(ser, f"wifi connect -s {ssid} -k 1 -p {PASS}")
    out = read_until(ser, ["IPv4 address:", "FAIL"], timeout=timeout)
    m = re.search(r"IPv4 address:\s*([\d.]+)", strip_ansi(out))
    if not m:
        print(f"    FAIL: no IP. Tail: {strip_ansi(out)[-80:].strip()}")
        return None
    ip = m.group(1)
    print(f"    IP: {ip}")
    time.sleep(10)   # let PHY rate settle (critical for sQSPI)
    ser.reset_input_buffer()
    return ip

# ── iperf server management ───────────────────────────────────────────────────

_iperf_started_by_us = False
_iperf_proc = None

def ensure_iperf_server():
    """Reuse existing iperf -s if running; start one if not. Never kills user's server."""
    global _iperf_started_by_us, _iperf_proc
    result = subprocess.run(["lsof", "-i", "UDP:5001"], capture_output=True, text=True)
    if "iperf" in result.stdout.lower():
        print("    iperf -s server already running (reusing).")
        _iperf_started_by_us = False
        return True
    # Start our own
    log = open("/tmp/iperf_server.log", "w")
    _iperf_proc = subprocess.Popen(
        ["iperf", "-s", "-u", "-p", "5001"],
        stdout=log, stderr=log)
    time.sleep(2)
    if _iperf_proc.poll() is not None:
        print("    WARNING: iperf server exited on start (port conflict?).")
        return False
    _iperf_started_by_us = True
    print(f"    Started iperf server (pid={_iperf_proc.pid})")
    return True

def stop_our_iperf():
    global _iperf_proc, _iperf_started_by_us
    if _iperf_started_by_us and _iperf_proc:
        try:
            _iperf_proc.kill(); _iperf_proc.wait()
        except Exception:
            pass
        _iperf_proc = None
        _iperf_started_by_us = False

# ── Parse helpers ─────────────────────────────────────────────────────────────

def to_mbps(val_str, unit_str):
    val = float(val_str)
    u = unit_str.strip().lower()
    if u in ("kbps", "kbit/s", "kbits/sec"):
        return round(val / 1000, 3)
    if u in ("mbps", "mbit/s", "mbits/sec"):
        return round(val, 3)
    if u in ("gbps",):
        return round(val * 1000, 3)
    return round(val, 3)

def parse_zperf_tx(raw):
    """Parse server-measured rate from zperf udp upload statistics block.

    zperf prints two 'Rate:' lines:
      1. Before upload (config):  'Rate:    50.00 Mbps'   ← no parenthesis, no stats block
      2. After upload (result):   'Rate:     8.29 Mbps       (7.60 Mbps)'

    Strategy: find the Statistics block that appears after "Upload completed!"
    and extract the first Rate line within it.  Falls back to any Rate with
    parenthesised second value if the block anchor is missing.
    """
    text = strip_ansi(raw)
    # Remove interleaved shell prompts
    text = re.sub(r'uart:~\$\s*', ' ', text)

    # Primary: parse from the Statistics block (appears after "Upload completed!")
    stats_m = re.search(r"Upload completed!(.*)", text, re.DOTALL | re.IGNORECASE)
    if stats_m:
        stats_text = stats_m.group(1)
        # First Rate: line in the stats block is the server-measured rate
        m = re.search(r"Rate:\s+([\d.]+)\s+(Kbps|kbps|Mbps|mbps)", stats_text)
        if m:
            return to_mbps(m.group(1), m.group(2))

    # Fallback: Rate line followed by parenthesised client rate
    m = re.search(r"Rate:\s+([\d.]+)\s+(Kbps|kbps|Mbps|mbps)\s+\(", text)
    if m:
        return to_mbps(m.group(1), m.group(2))
    return None

def parse_zperf_rx(raw):
    """Parse receive rate from zperf udp download output (ANSI-stripped)."""
    text = strip_ansi(raw)
    # zperf download prints: "rate:  740 Kbps" or "rate:  5.07 Mbps"
    for pat in [
        r"rate:\s+([\d.]+)\s+(Kbps|kbps|Mbps|mbps)",
        r"throughput:\s+([\d.]+)\s+(Kbps|kbps|Mbps|mbps)",
    ]:
        m = re.search(pat, text, re.IGNORECASE)
        if m:
            return to_mbps(m.group(1), m.group(2))
    return None

# ── Single zperf TX run ───────────────────────────────────────────────────────

def zperf_tx_once(ser, label=""):
    ser.reset_input_buffer()
    send(ser, f"zperf udp upload {MAC_IP} 5001 {DURATION} 1K 50M")
    # Wait for "Upload completed!" first, then capture the Statistics block
    # that follows asynchronously after the shell prompt.
    nrf_out = read_until(ser, ["Upload completed!", "uart:~$"], timeout=DURATION + 30)
    nrf_out += read_for(ser, 5)   # stats block appears after prompt — drain 5s
    mbps = parse_zperf_tx(nrf_out)
    return mbps, nrf_out

# ── TX band ───────────────────────────────────────────────────────────────────

def run_tx_band(ser):
    print(f"\n  UDP TX (nRF \u2192 Mac)  [server=iperf -s -u, client=zperf udp upload]:")
    if not ensure_iperf_server():
        return [None] * RUNS

    # Warmup run (not recorded)
    print("    warmup run... ", end="", flush=True)
    mbps, _ = zperf_tx_once(ser, "warmup")
    print(f"{mbps} Mbps" if mbps else "failed (ok, continuing)")
    time.sleep(1)

    vals = []
    for i in range(RUNS):
        mbps, nrf_out = zperf_tx_once(ser, f"run {i+1}")
        if mbps is None:
            print(f"    TX run {i+1}: ? [parse failed]")
            print(f"      tail: {strip_ansi(nrf_out)[-200:].strip()}")
        else:
            print(f"    TX run {i+1}: {mbps} Mbps")
        vals.append(mbps)
        time.sleep(1)

    stop_our_iperf()
    return vals

# ── RX band ───────────────────────────────────────────────────────────────────

def _start_zperf_download(ser):
    """Start (or restart) zperf udp download server and wait for it to bind."""
    ser.reset_input_buffer()
    send(ser, "zperf udp download 5001")
    out = read_until(ser, ["Listening on port", "already started", "uart:~$"], timeout=10)
    return "already started" not in strip_ansi(out)

def _run_iperf_rx(ser, nrf_ip):
    """Run one iperf -c RX measurement and return the parsed nRF rate in Mbps."""
    iperf = subprocess.Popen(
        ["iperf", "-c", nrf_ip, "-u", "-p", "5001",
         "-t", str(DURATION), "-b", "50M", "-l", "1000"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    iperf.wait()                    # blocks ~DURATION seconds
    # Wait for zperf to print "rate:" after receiving the iperf FIN
    nrf_out = read_until(ser, ["rate:", "End of session!"], timeout=DURATION + 8)
    nrf_out += read_for(ser, 2)     # capture the value that follows "rate:"
    return parse_zperf_rx(nrf_out), nrf_out

def run_rx_band(ser, nrf_ip):
    print(f"\n  UDP RX (Mac \u2192 nRF)  [server=zperf udp download per run, client=iperf -c]:")

    # Warmup: fresh zperf server + one iperf -c before measured runs
    print("    warmup run... ", end="", flush=True)
    _start_zperf_download(ser)
    w_mbps, _ = _run_iperf_rx(ser, nrf_ip)
    print(f"{w_mbps} Mbps" if w_mbps else "no parse (ok, continuing)")
    time.sleep(1)

    vals = []
    for i in range(RUNS):
        # Restart zperf server for each run — eliminates run-3 stats-loss issue
        ser.reset_input_buffer()
        bound = _start_zperf_download(ser)
        if not bound:
            print(f"    RX run {i+1}: zperf server restart failed")
            vals.append(None)
            continue

        mbps, nrf_out = _run_iperf_rx(ser, nrf_ip)
        if mbps is None:
            print(f"    RX run {i+1}: ? [parse failed]")
            print(f"      tail: {strip_ansi(nrf_out)[-250:].strip()}")
        else:
            print(f"    RX run {i+1}: {mbps} Mbps")
        vals.append(mbps)
        time.sleep(1)

    return vals

# ── Band test ─────────────────────────────────────────────────────────────────

def test_band(ser, ssid, band):
    print(f"\n{'='*60}\n  {LABEL} | {band}\n{'='*60}")

    # One connection for the entire band (TX + RX) — matches user's manual approach
    disconnect(ser)
    ip = connect_wifi(ser, ssid)
    if not ip:
        return {"band": band, "error": "WiFi connect failed",
                "udp_tx": [None]*RUNS, "udp_rx": [None]*RUNS}

    tx_vals = run_tx_band(ser)
    rx_vals = run_rx_band(ser, ip)

    def fmt(v): return "/".join(str(x) if x is not None else "?" for x in v)
    print(f"\n  Summary: TX={fmt(tx_vals)} | RX={fmt(rx_vals)} Mbps")
    return {"band": band, "ip": ip, "udp_tx": tx_vals, "udp_rx": rx_vals}

# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import datetime
    test_start = time.time()

    print(f"\n{'#'*60}")
    print(f"  Device: {LABEL}")
    print(f"  Port:   {PORT}")
    print(f"  PREREQ: keep 'iperf -s -u' running in a terminal for TX!")
    print(f"  Start:  {datetime.datetime.now().strftime('%H:%M:%S')}")
    print(f"{'#'*60}")
    ser = open_port(PORT)
    send(ser, ""); time.sleep(1); ser.reset_input_buffer()

    results = []
    for ssid, band in [(SSID_24, "2.4GHz"), (SSID_5, "5GHz")]:
        r = test_band(ser, ssid, band)
        results.append(r)
        time.sleep(5)
    ser.close()
    stop_our_iperf()

    elapsed_s = int(time.time() - test_start)
    elapsed_str = f"{elapsed_s // 60} min {elapsed_s % 60} s"

    print(f"\n{'#'*60}\n  FINAL RESULTS — {LABEL}\n{'#'*60}")
    for r in results:
        tx = "/".join(str(v) if v is not None else "?" for v in r["udp_tx"])
        rx = "/".join(str(v) if v is not None else "?" for v in r["udp_rx"])
        err = f" [{r['error']}]" if r.get("error") else ""
        print(f"  {r['band']}: TX={tx} | RX={rx}{err}")
    print(f"\n  Total test time: {elapsed_str}")

    label_safe = LABEL.replace(" ","_").replace("@","at").replace("/","_")
    fname = f"/tmp/results_{label_safe}.json"
    with open(fname, "w") as f:
        json.dump({"device": LABEL, "results": results, "elapsed": elapsed_str}, f, indent=2)
    print(f"  Saved: {fname}")
