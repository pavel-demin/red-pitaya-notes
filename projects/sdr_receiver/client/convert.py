from argparse import ArgumentParser
import numpy as np
import scipy.io.wavfile as wf

rates = [48000, 96000, 192000, 384000]

parser = ArgumentParser()

parser.add_argument("--file", help="input file", required=True)
parser.add_argument("--rate", help="sample rate %s" % rates, type=int, required=True)

args = parser.parse_args()

if args.rate not in rates:
    parser.print_help()
    exit(1)

data = np.fromfile(args.file, dtype=np.complex64)

for i in range(8):
    channel = data[i::8].view("(2,)float32")
    wf.write("%d.wav" % i, args.rate, channel)
