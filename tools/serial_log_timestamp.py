#!/usr/bin/env python3
"""
serial_log_timestamp.py

Simple timestamping serial logger using pyserial. Reads lines from the serial
port and writes timestamp-prefixed lines to an output file and stdout.

Usage:
  python tools\serial_log_timestamp.py --port COM3 --baud 115200 --out m1128_log.txt

Install dependency if needed:
  pip install pyserial

"""
import argparse
import serial
import time
import sys

parser = argparse.ArgumentParser(description='Timestamped serial logger')
parser.add_argument('--port', '-p', required=True, help='Serial port, e.g. COM3')
parser.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate')
parser.add_argument('--out', '-o', default='serial_log.txt', help='Output file')
parser.add_argument('--timeout', '-t', type=float, default=1.0, help='Read timeout (s)')
args = parser.parse_args()

try:
    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
except Exception as e:
    print(f"Failed to open serial port {args.port}: {e}")
    sys.exit(1)

print(f"Opened {args.port} @ {args.baud}, writing to {args.out}")
with open(args.out, 'a', encoding='utf-8', errors='replace') as fout:
    try:
        while True:
            try:
                line = ser.readline()
            except KeyboardInterrupt:
                print('\nInterrupted; exiting')
                break
            if not line:
                continue
            try:
                text = line.decode('utf-8', errors='replace').rstrip('\r\n')
            except Exception:
                text = repr(line)
            ts = time.strftime('%Y-%m-%d %H:%M:%S')
            out = f"{ts} | {text}"
            print(out)
            fout.write(out + '\n')
            fout.flush()
    finally:
        ser.close()
