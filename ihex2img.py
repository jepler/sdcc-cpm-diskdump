#!/usr/bin/python3
import struct
import sys
import binascii 

NUM_TRACKS = 77
NUM_SECTORS = 26
content = bytearray(b'\xe5') * (128 * NUM_TRACKS * NUM_SECTORS)

for row in sys.stdin:
    row = row.strip()
    print(row)
    if not row.startswith(':'):
        continue
    row = row.removeprefix(':')
    values = binascii.a2b_hex(row)
    if sum(values) % 256 != 0:
        print("# Checksum error!")
    data_len, address, rectype = struct.unpack('>BHB', values[:4])
    data = values[4:-1]
    if rectype in (0x80, 0x81, 0xe5):
        track = data[0]
        sector = data[1]
        status = ("OK" if rectype == 0x80 else
                "EMPTY" if rectype == 0xe5 else "ERROR")
        linear_address = (NUM_SECTORS * track + sector - 1) * 128
        sector_data = memoryview(content)[linear_address:linear_address+128]
        print(f"T{track:02}S{sector:02d} {linear_address} {status}")
    elif rectype == 0:
        sector_data[address:address+data_len] = data
    else:
        print("???", rectype)

with open("disk.img", "wb") as f:
    f.write(content)
