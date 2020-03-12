import serial
import logging

SERIAL_DEVICE = '/dev/ttyAMA0'
SERIAL_BAUD = 9600
SERIAL_TIMEOUT = 2.0

FRAME_HEADER = b'\x42\x4d'


def int16bit(b):
    return (b[0] << 8) + b[1]


class PMS5003:

    def __init__(self, port):
        self._port = port

    def read(self):
        logging.debug("scanning..")

        self._port.read_until(FRAME_HEADER)
        ret = FRAME_HEADER + self._port.read(30)

        logging.debug("found frame!")
        return self._decode_frame(ret)

    @staticmethod
    def _decode_frame(rcv):
        if (rcv == None) or len(rcv) != 32:
            return None

        s = sum(rcv[:30])
        checksum = int16bit(rcv[30:])
        if s != checksum:
            logging.error("Invalid frame checksum %d != %d" % (s, checksum))
            return None

        # Got a valid data-frame.
        return {
            'data1':     int16bit(rcv[4:]),
            'data2':     int16bit(rcv[6:]),
            'data3':     int16bit(rcv[8:]),
            'data4':     int16bit(rcv[10:]),
            'data5':     int16bit(rcv[12:]),
            'data6':     int16bit(rcv[14:]),
            'data7':     int16bit(rcv[16:]),
            'data8':     int16bit(rcv[18:]),
            'data9':     int16bit(rcv[20:]),
            'data10':    int16bit(rcv[22:]),
            'data11':    int16bit(rcv[24:]),
            'data12':    int16bit(rcv[26:]),
            'reserved':  int16bit(rcv[28:]),
            'checksum':  int16bit(rcv[30:])
        }

    @staticmethod
    def verbose(f):
        return (' PM1.0 (CF=1) μg/m³: {};\n'
                ' PM2.5 (CF=1) μg/m³: {};\n'
                ' PM10  (CF=1) μg/m³: {};\n'
                ' PM1.0 (STD)  μg/m³: {};\n'
                ' PM2.5 (STD)  μg/m³: {};\n'
                ' PM10  (STD)  μg/m³: {};\n'
                ' Particles >  0.3 μm count: {};\n'
                ' Particles >  0.5 μm count: {};\n'
                ' Particles >  1.0 μm count: {};\n'
                ' Particles >  2.5 μm count: {};\n'
                ' Particles >  5.0 μm count: {};\n'
                ' Particles > 10.0 μm count: {};\n'
                ' Reserved: {};\n'
                ' Checksum: {};'.format(
                    f['data1'],  f['data2'],  f['data3'],
                    f['data4'],  f['data5'],  f['data6'],
                    f['data7'],  f['data8'],  f['data9'],
                    f['data10'], f['data11'], f['data12'],
                    f['reserved'], f['checksum']))

if __name__ == '__main__':
    log = logging.getLogger()
    log.setLevel('INFO')

    port = serial.Serial(SERIAL_DEVICE, baudrate=SERIAL_BAUD, timeout=SERIAL_TIMEOUT)
    pms = PMS5003(port)

    while True:
        data = pms.read()
        if data:
            print(pms.verbose(data))
            break

