#!/usr/bin/env python
"""
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Receive raw data I/Q from Pavel's SDR Transceiver app... currently unbuffered and single threaded.
(http://pavel-demin.github.io/red-pitaya-notes/sdr-transceiver/)

2016, Frank Werner-Krippendorf, HB9FXQ (twitter @hb9fxq)

Sample to write raw data into file:

./rx.py  --address 192.168.188.21 --rate 500000 --freq=14100000 > fillSDA1.raw

Sample use with baudline:
./rx.py  --address 192.168.188.21 --rate 500000 --freq=14100000 | baudline -channels 2 -samplerate 500000 -stdin -quadrature -fftsize 32768 -reset -basefrequency 14100000 -format le32f -scaleby 32767 -flipcomplex


"""
import socket, sys, struct, argparse, signal

"Allowed sample rates"
rates = {20000:0, 50000:1, 100000:2, 250000:3, 500000:4, 1250000:5}

"Parse arguments"
parser = argparse.ArgumentParser(description='Receive raw data I/Q from Pavel''s SDR Transceiver app (http://pavel-demin.github.io/red-pitaya-notes/sdr-transceiver/)')

parser.add_argument("--address", help="IP address Red Pitaya", default="192.168.188.21")
parser.add_argument("--freq", help="Center Frequency, default 14100000 Hz", type=int, default=14100000)
parser.add_argument("--rate", help="Sample rate, allowed values: 20000 50000 100000 250000 500000 1250000", type=int, default=500000)
parser.add_argument("--corr", help="Correction in ppm, dafault 0", type=int, default=0)

args = parser.parse_args()

control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
data_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
run=True

def aquire(freq, rateindex, ipaddr, corr):
    """
    Connects to the Red Pitaya Transeiver server and writes the data to stdout
    :param freq: Frequency in Hz
    :param rateindex: Sample rate index
    :param ipaddr: Red Pitaya IP address
    :return:
    """

    "Setup control socket"
    control_socket.connect((ipaddr, 1001))
    control_socket.send(struct.pack('<I', 0))

    "Setup data socket"
    data_socket.connect((ipaddr, 1001))
    data_socket.send(struct.pack('<I', 1))

    "Set frequency and rate"
    control_socket.send(struct.pack('<I', 0<<28 | int((1.0 + 1e-6 * corr ) * freq)))
    control_socket.send(struct.pack('<I', 1<<28 | rateindex))

    "write aquisition parameters to console"
    sys.stderr.write("...receiving at " + str(freq) + " Hz" + " sample rate " + str(rateindex) + " correction(ppm) " + str(corr) + "\n")

    "fetch data"

    while run:
        sys.stdout.write(data_socket.recv(1024))


def signal_term_handler(signal, frame):
        global run
        data_socket.close()
        control_socket.close()
        sys.stderr.write("Bye\n")
        run=False;
        sys.stdout.flush()
        sys.exit(0)

if __name__ == "__main__":

    signal.signal(signal.SIGTERM, signal_term_handler)

    if args.rate in rates:
        aquire(args.freq, rates[args.rate], args.address, args.corr)
    else:
        raise Exception('Invalid sample rate!')



