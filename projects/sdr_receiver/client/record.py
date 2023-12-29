from argparse import ArgumentParser
from socket import socket, AF_INET, SOCK_STREAM
from signal import signal, SIGINT
from struct import pack
from sys import exit

rates = {48000: 0, 96000: 1, 192000: 2, 384000: 3}

parser = ArgumentParser()

parser.add_argument("--addr", help="IP address of the Red Pitaya board", required=True)
parser.add_argument("--freq", help="8 center frequencies in Hz [0 - 490000000]", nargs=8, type=int, required=True)
parser.add_argument("--rate", help="sample rate %s" % list(rates.keys()), type=int, required=True)
parser.add_argument("--corr", help="frequency correction in ppm [-100 - 100]", type=float, required=True)
parser.add_argument("--file", help="output file", required=True)

args = parser.parse_args()

for f in args.freq:
    if f < 0 or f > 490000000:
        parser.print_help()
        exit(1)

if args.rate not in rates:
    parser.print_help()
    exit(1)

if args.corr < -100 or args.corr > 100:
    parser.print_help()
    exit(1)

sock = socket(AF_INET, SOCK_STREAM)
sock.settimeout(5)
try:
    sock.connect((args.addr, 1001))
except:
    print("error: could not connect to", args.addr)
    exit(1)
sock.settimeout(None)

sock.send(pack("<10I", 0, rates[args.rate], *[int((1.0 + 1e-6 * args.corr) * f) for f in args.freq]))

interrupted = False


def signal_handler(signal, frame):
    global interrupted
    interrupted = True


signal(SIGINT, signal_handler)

try:
    f = open(args.file, "wb")
except:
    print("error: could not open", args.file)
    exit(1)

data = sock.recv(4096)
while data and not interrupted:
    f.write(data)
    data = sock.recv(4096)
