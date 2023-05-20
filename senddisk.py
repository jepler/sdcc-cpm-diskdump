#!/usr/bin/python3
import time
import sys
import serial

port = serial.serial_for_url('spy:////dev/ttyUSB0',
19200
)

NUM_TRACKS = 77
NUM_SECTORS = 26
SECTOR_SIZE = 128
TLEN = NUM_SECTORS * SECTOR_SIZE
XLEN = NUM_TRACKS * TLEN

print(f"track len {TLEN}")
with open(sys.argv[1], 'rb') as f:
    content = f.read()

if len(content) != XLEN:
    raise SystemExit(f"Expected {XLEN} bytes but read {len(content)}")

print("start rw.exe on xerox")
for i in range(NUM_TRACKS):
    trackdata = content[i*TLEN:(i+1)*TLEN]
    print(len(trackdata))
    c = port.read(1)
    if c != b'T':
        raise SystemExit(f"Expected b'T' got {c}")
    print(f"sending track {i}")
    port.write(trackdata)
