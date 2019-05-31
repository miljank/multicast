#!/usr/bin/env python2.7

import sys
import socket
import datetime
import argparse
from multicast import Multicast


class MRecv(Multicast):
    def __init__(self, group_ip, group_port, interface):
        super(MRecv, self).__init__(group_ip, group_port, interface)

    def subscribe_to_feed(self):
        self.connection.setsockopt(socket.IPPROTO_IP,
                                   socket.IP_ADD_MEMBERSHIP,
                                   socket.inet_aton(self.group_ip) +
                                   socket.inet_aton(self.local_ip))
        self.connection.bind((self.group_ip, self.group_port))

    def process_loop(self):
        while True:
            if self.timeout:
                self.process_timeout()

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
        self.timeout = timeout
        self.start_time = datetime.datetime.now()
        self.subscribe_to_feed()

        print("subscriber={0} destination={1}:{2}".format(self.local_ip,
                                                          self.group_ip,
                                                          self.group_port))

        try:
            self.process_loop()

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
