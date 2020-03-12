import argparse, serial, logging

from pinode import PiNode
from pms5003 import PMS5003


def main():
    log = logging.getLogger()
    log.setLevel('INFO')

    parser = argparse.ArgumentParser(description='Pi Sensor Node')
    parser.add_argument('-s', '--server', type=str, help='mqtt server')
    args = parser.parse_args()
    server = args.server

    pi = PiNode(server)

    port = serial.Serial('/dev/ttyAMA0', baudrate=9600, timeout=2.0)
    pms = PMS5003(port)
    pi.add_sensor('pms5003', pms, 10.0)


if __name__ == '__main__':
    main()