#!/usr/bin/env python3
"""
Serial → OSC bridge (Raspberry Pi / Linux)

Goal:
- Read two sensor values from ESP32 over serial at high baud (e.g., 460800)
- Accept ESP32 text lines in these formats:
    123a456b
    Na31b
    120aNb
    NaNb
  (also tolerates stray fragments -> treated as "no hand")
- Send OSC continuously as TWO INTS only: [A, B]
- If a sensor is missing/invalid -> send NO_HAND (default 1000)
- Low-latency: minimal work in loop, no printing, steady send rate

Run:
  cd ~/serialbridge
  source .venv/bin/activate
  python serial_to_osc.py

Test OSC without SC: ( THIS STEP IS OPTIONAL TO CHECK IF THE SCRIPT IS WORKING FINE )
  python osc_dump.py
"""

import re
import time
import serial
from pythonosc.udp_client import SimpleUDPClient

# -------------------- USER CONFIG --------------------
SERIAL_DEV = "/dev/ttyUSB0"
BAUD = 460800

OSC_HOST = "127.0.0.1"
OSC_PORT = 57120
OSC_ADDR = "/sensors"

NO_HAND = 1000      # value to output when a sensor has no reading
SEND_HZ = 200       # constant output rate (100–200 is a good range)
# -----------------------------------------------------

# Accepts:
#  "123a456b"
#  "Na31b"     (A missing)
#  "120aNb"    (B missing)
#  "NaNb"      (both missing)
# Notes:
# - 'a' may or may not appear between the two fields in your stream.
# - trailing 'b' is expected in your stream; tolerate whitespace.
_RX = re.compile(
    rb"^\s*(Na|\d+)\s*(?:a\s*)?(Nb|\d+)\s*b\s*$",
    re.IGNORECASE
)

def _parse_two(line: bytes) -> tuple[int, int]:
    """
    Always returns (a, b) as ints.
    Missing/invalid -> NO_HAND
    Unparseable fragments -> (NO_HAND, NO_HAND)
    """
    if not line:
        return (NO_HAND, NO_HAND)

    line = line.strip()
    if not line:
        return (NO_HAND, NO_HAND)

    m = _RX.match(line)
    if not m:
        return (NO_HAND, NO_HAND)

    a_raw = m.group(1).lower()
    b_raw = m.group(2).lower()

    a = NO_HAND if a_raw == b"na" else int(a_raw)
    b = NO_HAND if b_raw == b"nb" else int(b_raw)
    return (a, b)

def main():
    # Serial
    ser = serial.Serial(
        SERIAL_DEV,
        BAUD,
        timeout=0.02,                # small timeout keeps loop responsive
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    )

    # Start clean (avoid old buffered bytes)
    try:
        ser.reset_input_buffer()
    except Exception:
        pass

    # OSC
    client = SimpleUDPClient(OSC_HOST, OSC_PORT)

    # Timing (steady output)
    period = 1.0 / float(SEND_HZ)
    next_send = time.monotonic()

    # Last values (keep sending even if serial temporarily stops)
    a, b = (NO_HAND, NO_HAND)

    while True:
        # Read latest line if available
        line = ser.readline()
        if line:
            a, b = _parse_two(line)

        # Send at constant rate (low jitter, consistent control update)
        now = time.monotonic()
        if now >= next_send:
            client.send_message(OSC_ADDR, [a, b])
            next_send = now + period

if __name__ == "__main__":
    main()
