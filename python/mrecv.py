#!/usr/bin/env python2.7

import sys
import fcntl
import socket
import struct
import datetime
import argparse


class MRecv(object):
    def __init__(self, group_ip, group_port, interface):
        self.group_ip = group_ip
        self.group_port = group_port
        self.interface = interface

        self.group = (self.group_ip, self.group_port)
        self.local_ip = self.__get_interface_ip_address()
        self.connection = self.__create_socket()

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

        except IOError, error:
            print('error: {0}'.format(error))
            sys.exit(1)

    def show_process_stats(self, run_time):
        print('\nExiting after {0} seconds and {1} messages.'
              .format(run_time, self.message_count))

    def process_timeout(self, timeout):
        run_time = (datetime.datetime.now() - self.start_time).seconds

        if run_time < timeout:
            return True

        self.show_process_stats(run_time)

        if self.message_count == 0:
            sys.exit(1)

        sys.exit(0)

    def subscribe_to_feed(self):
        self.connection.setsockopt(socket.IPPROTO_IP,
                                   socket.IP_ADD_MEMBERSHIP,
                                   socket.inet_aton(self.group_ip) +
                                   socket.inet_aton(self.local_ip))
        self.connection.bind((self.group_ip, self.group_port))

    def process_loop(self, timeout):
        while True:
            if timeout:
                self.process_timeout(timeout)

            try:
                message, sender = self.connection.recvfrom(2048)

            except socket.error:
                pass

            else:
                sourceaddress, sourceport = sender
                self.message_count += 1

                print("{0} source={1}:{2} destination={3}:{4} size={5}"
                      .format(datetime.datetime.now(), sourceaddress,
                              sourceport, self.group_ip, self.group_port,
                              len(message)))

    def show(self, timeout=None):
        """Method is used when the program is running in show mode.
        It prints information about every packet received on the feed.
        """
        self.start_time = datetime.datetime.now()
        self.subscribe_to_feed()

        print("subscriber={0} destination={1}:{2}".format(self.local_ip,
                                                          self.group_ip,
                                                          self.group_port))

        try:
            self.process_loop(timeout)

        except KeyboardInterrupt:
            run_time = (datetime.datetime.now() - self.start_time).seconds
            self.show_process_stats(run_time)
            sys.exit(0)


def get_arguments():
    parser = argparse.ArgumentParser(description='Subscribe and listen on a '
                                     'specified multicast feed.')

    parser.add_argument('--address', type=str, required=True,
                        dest='address', default=None, help='Multicast group IP')
    parser.add_argument('--port', type=int, required=True,
                        dest='port', default=None, help='Multicast group port')
    parser.add_argument('--interface', type=str, required=True,
                        dest='interface', default=None, help='Bind to a specific interface')
    parser.add_argument('--timeout', type=int,
                        dest='timeout', default=None, required=False,
                        help='Exit program after specified number of seconds')

    args = parser.parse_args()
    return args


if __name__ == '__main__':
    arguments = get_arguments()

    mcast = MRecv(arguments.address,
                  arguments.port,
                  arguments.interface)

    mcast.show(timeout=arguments.timeout)
