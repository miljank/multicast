import sys
import fcntl
import socket
import struct
import datetime


class Multicast(object):
    def __init__(self, group_ip, group_port, interface):
        self.group_ip = group_ip
        self.group_port = group_port
        self.group = (self.group_ip, self.group_port)

        self.interface = interface
        self.local_ip = self.__get_interface_ip_address()

        self.connection = self.__create_socket()

        self.timeout = None
        self.message_count = 0
        self.start_time = None

    def __create_socket(self):
        connection = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        connection.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        connection.setblocking(0)

        return connection

    def __get_interface_ip_address(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        try:
            return socket.inet_ntoa(fcntl.ioctl(
                s.fileno(),
                0x8915,  # SIOCGIFADDR
                struct.pack('256s', self.interface[:15])
            )[20:24])

        except IOError as error:
            print('error: {0}'.format(error))
            sys.exit(1)

    def show_process_stats(self, run_time):
        print('\nExiting after {0} seconds and {1} messages.'
              .format(run_time, self.message_count))

    def process_timeout(self):
        run_time = (datetime.datetime.now() - self.start_time).seconds

        if run_time < self.timeout:
            return True

        self.show_process_stats(run_time)

        if self.message_count == 0:
            sys.exit(1)

        sys.exit(0)
