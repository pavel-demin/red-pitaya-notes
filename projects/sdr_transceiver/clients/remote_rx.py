#!/usr/bin/env python

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
# AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Receive / transmit raw data I/Q from Pavel's SDR Transceiver app... currently unbuffered and only RX
# (http://pavel-demin.github.io/red-pitaya-notes/sdr-transceiver/)
#
# 2016, Frank Werner-Krippendorf, HB9FXQ (twitter @hb9fxq)
#
# Sample to write raw data into file:
#
# ./remote_rx.py  --address 192.168.188.21 --rate 500000 --freq=14100000 > fillSDA1.raw
#
# Sample use with baudline:
# ./remote_rx.py  --address 192.168.188.21 --rate 500000 --freq=14100000 | baudline -channels 2 -samplerate 500000 -stdin -quadrature -fftsize 65536 -reset -basefrequency 14100000 -format le32f -scaleby 32767 -flipcomplex#
#
#
#
import socket, sys, struct, argparse, signal

rates = {20000:0, 50000:1, 100000:2, 250000:3, 500000:4, 1250000:5}
activeReceiver = None


class Transceiver(object):

    def __init__(self, ipaddr):
        self.p_ip_address = ipaddr
        self.run = True;
        self.control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.data_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def shutdown(self):
        sys.stderr.write("shutdown\n")
        self.run = False

        self.data_socket.close()
        self.control_socket.close()
        sys.stdout.flush()

        sys.stderr.write("bye\n")
        sys.exit(0)

    def start_rx(self, freq, rate_index, corr):
        """
        Connects to the Red Pitaya Transeiver server and writes the data to stdout
        :param freq: Frequency in Hz
        :param rate_index: Sample rate index
        :param corr: correction in ppm
        :return:
        """
        # Setup control socket
        self.control_socket.connect((self.p_ip_address, 1001))
        self.control_socket.send(struct.pack('<I', 0))

        # Setup data socket
        self.data_socket.connect((self.p_ip_address, 1001))
        self.data_socket.send(struct.pack('<I', 1))

        # Set frequency and rate
        self.control_socket.send(struct.pack('<I', 0<<28 | int((1.0 + 1e-6 * corr ) * freq)))
        self.control_socket.send(struct.pack('<I', 1<<28 | rate_index))

        # write aquisition parameters to console
        sys.stderr.write(
            "...receiving at: {0} Hz sample sample rate: {1}  / correction (ppm): {2}\n".format(freq, list(rates.keys())[list(rates.values()).index(rate_index)],
                                                                                  corr))

        "fetch data"
        try:
            while self.run:
                sys.stdout.write(self.data_socket.recv(1024))
        except Exception:
            sys.stderr.write(">>> rx force shutdown\n")
            self.shutdown()


def signal_term_handler(signal, frame):
    activeReceiver.shutdown()

if __name__ == "__main__":

    # Parse arguments
    parser = argparse.ArgumentParser(description='Receive raw data I/Q from Pavel''s SDR Transceiver app (http://pavel-demin.github.io/red-pitaya-notes/sdr-transceiver/)')

    parser.add_argument("--address", help="IP address Red Pitaya", default="192.168.188.21")
    parser.add_argument("--freq", help="Center Frequency, default 14100000 Hz", type=int, default=14100000)
    parser.add_argument("--rate", help="Sample rate, allowed values: 20000 50000 100000 250000 500000 1250000", type=int, default=500000)
    parser.add_argument("--corr", help="Correction in ppm, dafault 0", type=int, default=0)

    args = parser.parse_args()

    # Register SIGTERM handler
    signal.signal(signal.SIGINT, signal_term_handler)

    trx = Transceiver(args.address)

    activeReceiver = trx;
    trx.start_rx(args.freq, rates[args.rate], args.corr)

