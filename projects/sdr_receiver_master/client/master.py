import struct
import socket

addr = "192.168.1.100"
port = 1001

rate = 500000

rx_freq1 = 10000000
rx_freq2 = 10000000

tx_freq1 = 10000010
tx_freq2 = 10000010

tx_level1 = 32766
tx_level2 = 32766

size = 1000000 * 16

# connect
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((addr, port))

# open output file
output = open("master.dat", "wb")

# set rate
value = 62500000 // rate
sock.send(struct.pack("<I", 0 << 28 | value))

# set rx frequencies
sock.send(struct.pack("<I", 1 << 28 | rx_freq1))
sock.send(struct.pack("<I", 2 << 28 | rx_freq2))

# set tx frequencies
sock.send(struct.pack("<I", 3 << 28 | tx_freq1))
sock.send(struct.pack("<I", 4 << 28 | tx_freq2))

# set tx levels
sock.send(struct.pack("<I", 5 << 28 | tx_level1))
sock.send(struct.pack("<I", 6 << 28 | tx_level2))

# start
sock.send(struct.pack("<I", 7 << 28))

# read samples
while size > 0:
    l = min(size, 2**20)
    data = sock.recv(l, socket.MSG_WAITALL)
    output.write(data)
    size -= len(data)

output.close()
