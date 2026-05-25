#!/usr/bin/env python3
"""
NCS WiFi Loop Test Script
=========================
Automates: reset → boot → wifi connect → verify → pass/fail per iteration.

Usage:
    python3 loop_test.py [iterations]

Edit the constants below to match your hardware.

Requirements:
    pip install pyserial
"""

import serial
import subprocess
import time
import sys
import re
import argparse

# ============================================================
# Configuration — edit these for your setup
# ============================================================
SERIAL_PORT   = "/dev/cu.usbmodem..."   # VCOM port (use 'nrfutil device list')
BAUD          = 115200
BOARD_SERIAL  = "0000000000"            # nrfutil serial number
SSID          = "YOUR_SSID"
PSK           = "YOUR_PASSWORD"
KEY_MGMT      = 1                       # 1=WPA2-PSK, 2=WPA3-SAE, 0=open
ITERATIONS    = 10

BOOT_TIMEOUT    = 30   # seconds to wait for uart:~$ after reset
CONNECT_TIMEOUT = 30   # seconds to wait for 'Connected'
STATUS_DELAY    = 3    # seconds after Connected before checking status
# ============================================================


def reset_board(serial_number: str) -> None:
    subprocess.run(
        ["nrfutil", "device", "reset", "--serial-number", serial_number],
        capture_output=True, timeout=10
    )


def drain(ser: serial.Serial, timeout: float = 0.5) -> str:
    ser.timeout = timeout
    data = b""
    while True:
        chunk = ser.read(4096)
        if not chunk:
            break
        data += chunk
    return data.decode("utf-8", errors="replace")


def read_until(ser: serial.Serial, pattern: str, timeout: float):
    """Read serial until regex pattern found or timeout. Returns (matched, all_output)."""
    end = time.time() + timeout
    buf = ""
    ser.timeout = 0.5
    while time.time() < end:
        chunk = ser.read(4096)
        if chunk:
            buf += chunk.decode("utf-8", errors="replace")
        if re.search(pattern, buf):
            return True, buf
    return False, buf


def send_cmd(ser: serial.Serial, cmd: str) -> None:
    ser.write((cmd + "\r\n").encode())
    time.sleep(0.1)


def run_iteration(i: int, port: str, baud: int, sn: str,
                  ssid: str, psk: str, key_mgmt: int) -> tuple[bool, str]:
    print(f"\n{'='*60}")
    print(f"  Iteration {i}")
    print(f"{'='*60}")

    print("  [1/4] Resetting board...")
    reset_board(sn)
    time.sleep(2)

    try:
        ser = serial.Serial(port, baud, rtscts=True, timeout=1)
    except Exception as e:
        return False, f"Serial open failed: {e}"

    try:
        print("  [2/4] Waiting for boot...")
        found, output = read_until(ser, r"uart:~\$", BOOT_TIMEOUT)
        if not found:
            send_cmd(ser, "")
            time.sleep(1)
            found2, output2 = read_until(ser, r"uart:~\$", 5)
            output += output2
            if not found2:
                return False, "Boot timeout — no shell prompt"

        if "UMAC init timed out" in output or "nrf_wifi: FMAC init failed" in output:
            return False, "WiFi init failed during boot"

        time.sleep(1)
        drain(ser)

        print("  [3/4] Connecting WiFi...")
        cmd = f"wifi connect -s {ssid} -k {key_mgmt}"
        if key_mgmt > 0:
            cmd += f" -p {psk}"
        send_cmd(ser, cmd)

        found, output = read_until(ser, r"Connected", CONNECT_TIMEOUT)
        if not found:
            if "Disconnect" in output or "disconnect" in output:
                return False, "Disconnected before Connected"
            if "timeout" in output.lower():
                return False, "Transfer timeout during connect"
            return False, "Connect timeout — no 'Connected' in output"

        print("  [4/4] Checking WiFi status...")
        time.sleep(STATUS_DELAY)
        drain(ser)
        send_cmd(ser, "wifi status")
        time.sleep(2)
        status_output = drain(ser)

        if "connected" in status_output.lower() or "COMPLETED" in status_output:
            send_cmd(ser, "net iface")
            time.sleep(2)
            net_output = drain(ser)
            ip_match = re.search(r"IPv4.*?(\d+\.\d+\.\d+\.\d+)", net_output)
            ip = ip_match.group(1) if ip_match else "no-ip"
            return True, f"Connected, IP={ip}"
        else:
            return False, "Status not connected after association"

    finally:
        ser.close()


def main() -> int:
    parser = argparse.ArgumentParser(description="NCS WiFi loop test")
    parser.add_argument("iterations", nargs="?", type=int, default=ITERATIONS,
                        help=f"Number of iterations (default: {ITERATIONS})")
    parser.add_argument("--port", default=SERIAL_PORT)
    parser.add_argument("--baud", type=int, default=BAUD)
    parser.add_argument("--serial", default=BOARD_SERIAL)
    parser.add_argument("--ssid", default=SSID)
    parser.add_argument("--psk", default=PSK)
    parser.add_argument("--key-mgmt", type=int, default=KEY_MGMT)
    args = parser.parse_args()

    n = args.iterations
    results = []

    print(f"Loop test: {n} iterations")
    print(f"Board: {args.serial}, Port: {args.port}")
    print(f"Network: {args.ssid}")

    for i in range(1, n + 1):
        try:
            ok, detail = run_iteration(i, args.port, args.baud, args.serial,
                                       args.ssid, args.psk, args.key_mgmt)
        except Exception as e:
            ok, detail = False, f"Exception: {e}"

        results.append((ok, detail))
        status = "PASS" if ok else "FAIL"
        print(f"  => {status}: {detail}")

    passes = sum(1 for ok, _ in results if ok)
    print(f"\n{'='*60}")
    print(f"  RESULTS: {passes}/{n} passed")
    print(f"{'='*60}")
    for i, (ok, detail) in enumerate(results, 1):
        status = "PASS" if ok else "FAIL"
        print(f"  [{i:2d}] {status}: {detail}")

    return 0 if passes == n else 1


if __name__ == "__main__":
    sys.exit(main())
