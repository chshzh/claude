# EEDP — Physical Test Automation via Zephyr GPIO Shell

## The Insight

When testing embedded firmware that involves buttons and LEDs, **don't build mechanical actuators (solenoids/servos)**. Instead, use another Zephyr board's GPIO pins to directly simulate button presses and read LED states. This is a zero-firmware, zero-mechanical approach.

## How It Works

A "controller board" (e.g., nRF7002DK) runs Zephyr with `CONFIG_GPIO_SHELL=y`. Its GPIO pins are wired to the target board's button and LED pins via the pin headers.

```
gpio set P1.00   → drives target board BTN1 pin HIGH  → simulates button press
gpio get P0.00   → reads target board LED1 pin level  → returns 0 or 1
```

The controller board doesn't need any custom firmware — the built-in `gpio` shell command is sufficient.

## Prerequisites

The target board must expose button and LED pins on its pin headers. For nRF54LM20DK, this is already configured in the board definition — see the board's DTS overlay or pinout documentation.

## Controller Board Selection

Any Zephyr board with enough GPIO pins and `CONFIG_GPIO_SHELL=y` works. Requirements per target board: 4 output (buttons) + 4 input (LEDs) = 8 GPIO pins. For 3 target boards: 24 GPIO pins needed. nRF5340 (on nRF7002DK) has ~40 usable GPIO — sufficient.

## Wiring

```
Controller Board GPIO          Target Board Pin Header
─────────────────────────      ───────────────────────
P1.00 (OUT, 3.3V logic) ────→  BTN1 pin
P1.01 (OUT)              ────→  BTN2 pin
P1.02 (OUT)              ────→  BTN3 pin
P1.03 (OUT)              ────→  BTN4 pin
P0.00 (IN, 3.3V logic)   ←──── LED1 pin
P0.01 (IN)               ←──── LED2 pin
P0.02 (IN)               ←──── LED3 pin
P0.03 (IN)               ←──── LED4 pin
GND                      ───── GND (common ground)
```

All signals are 3.3V — level matching is not an issue. Use a transistor (2N2222 + 10k base resistor) between GPIO output and button pin if the button requires higher current or is active-low with pull-up.

## From Python (EEDP Controller)

```python
import serial

class GPIOShell:
    """Control buttons and read LEDs via Zephyr GPIO shell over UART."""
    def __init__(self, port, baud=115200):
        self.ser = serial.Serial(port, baud, timeout=1)
        self._sync()

    def _sync(self):
        """Wait for shell prompt and ensure GPIO module is loaded."""
        self.ser.write(b"\r\n")
        self.ser.read_until(b"uart:~$")

    def press(self, pin):
        self.ser.write(f"gpio set {pin}\r\n".encode())
        self.ser.read_until(b"uart:~$")

    def release(self, pin):
        self.ser.write(f"gpio clear {pin}\r\n".encode())
        self.ser.read_until(b"uart:~$")

    def read(self, pin):
        self.ser.write(f"gpio get {pin}\r\n".encode())
        response = self.ser.read_until(b"uart:~$").decode()
        return "1" in response  # True if HIGH

    def tap(self, pin, hold_ms=200):
        """Press and release a button."""
        self.press(pin)
        import time; time.sleep(hold_ms / 1000)
        self.release(pin)

    def close(self):
        self.ser.close()
```

## Advantages Over Mechanical Approaches

| | GPIO Shell | Mechanical (solenoid/servo) |
|---|---|---|
| Hardware | Another dev board you already have | Custom 3D-printed fixture + actuators |
| Firmware | Zero (Zephyr built-in) | Custom firmware for ESP32/Pi |
| Precision | Exact — electrical connection | Physical alignment needed (±0.5mm) |
| Speed | Instant (us level) | 10-200ms mechanical delay |
| Cost | ¥0 (existing board) | ¥50-200/board |
| Reliability | No mechanical wear | Spring fatigue, alignment drift |
| Replaceability | Any Zephyr board | Must rebuild fixture |
