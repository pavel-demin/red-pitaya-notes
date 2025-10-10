#!/usr/bin/python3

# copied from https://github.com/topic-embedded-products/meta-topic/blob/master/recipes-bsp/fpga/fpga-bit-to-bin/fpga-bit-to-bin.py

import argparse
import struct


def flip32(data):
    sl = struct.Struct("<I")
    sb = struct.Struct(">I")
    try:
        b = buffer(data)
    except NameError:
        # Python 3 does not have 'buffer'
        b = data
    d = bytearray(len(data))
    for offset in range(0, len(data), 4):
        sb.pack_into(d, offset, sl.unpack_from(b, offset)[0])
    return d


parser = argparse.ArgumentParser(description="Convert FPGA bit files to raw bin format suitable for flashing")
parser.add_argument("-f", "--flip", dest="flip", action="store_true", default=False, help="Flip 32-bit endianess (needed for Zynq)")
parser.add_argument("bitfile", help="Input bit file name")
parser.add_argument("binfile", help="Output bin file name")
args = parser.parse_args()

short = struct.Struct(">H")
ulong = struct.Struct(">I")

bitfile = open(args.bitfile, "rb")

l = short.unpack(bitfile.read(2))[0]
if l != 9:
    raise Exception("Missing <0009> header (0x%x), not a bit file" % l)
bitfile.read(l)
l = short.unpack(bitfile.read(2))[0]
d = bitfile.read(l)
if d != b"a":
    raise Exception("Missing <a> header, not a bit file")

l = short.unpack(bitfile.read(2))[0]
d = bitfile.read(l)
print("Design name: %s" % d)

# If bitstream is a partial bitstream, get some information from filename and header
if b"PARTIAL=TRUE" in d:
    print("Partial bitstream")
    partial = True

    # Get node_nr from filename (last (group of) digits)
    for i in range(len(args.bitfile) - 1, 0, -1):
        if args.bitfile[i].isdigit():
            pos_end = i + 1
            for j in range(i - 1, 0, -1):
                if not args.bitfile[j].isdigit():
                    pos_start = j + 1
                    break
            break
    if pos_end != 0 and pos_end != 0:
        node_nr = int(args.bitfile[pos_start:pos_end])
    else:
        node_nr = 0
    print("NodeID: %s" % node_nr)

    # Get 16 least significant bits of UserID in design name
    pos_start = d.find(b"UserID=")
    if pos_start != -1:
        pos_end = d.find(b";", pos_start)
        pos_start = pos_end - 4
        userid = int(d[pos_start:pos_end], 16)
        print("UserID: 0x%x" % userid)

else:
    print("Full bitstream")
    partial = False
    node_nr = 0

KEYNAMES = {b"b": "Partname", b"c": "Date", b"d": "Time"}

while 1:
    k = bitfile.read(1)
    if not k:
        bitfile.close()
        raise Exception("unexpected EOF")
    elif k == b"e":
        l = ulong.unpack(bitfile.read(4))[0]
        print("Found binary data: %s" % l)
        d = bitfile.read(l)
        if args.flip:
            print("Flipping data...")
            d = flip32(d)
        # Open bin file
        binfile = open(args.binfile, "wb")
        # Write header if it is a partial
        if partial:
            binfile.write(struct.pack("B", 0))
            binfile.write(struct.pack("B", node_nr))
            binfile.write(struct.pack(">H", userid))
        # Write the converted bit-2-bin data
        print("Writing data...")
        binfile.write(d)
        binfile.close()
        break
    elif k in KEYNAMES:
        l = short.unpack(bitfile.read(2))[0]
        d = bitfile.read(l)
        print(KEYNAMES[k], d)
    else:
        print("Unexpected key: %s" % k)
        l = short.unpack(bitfile.read(2))[0]
        d = bitfile.read(l)

bitfile.close()
