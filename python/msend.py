#!/usr/bin/env python2.7

import sys
import time
import socket
import datetime
import argparse
from multicast import Multicast


class MSend(Multicast):
    def __init__(self, group_ip, group_port, interface):
        super(MSend, self).__init__(group_ip, group_port, interface)

        self.local_port = 0
        self.ttl = 10
        self.interval = 0.5
        self.payload = 64

    def bind_to_socket(self):
        self.connection.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, self.ttl)
        self.connection.bind((self.local_ip, self.local_port))
        self.local_port = self.connection.getsockname()[1]

    def send_loop(self):
        while True:
            if self.timeout:
                self.process_timeout()

            message = '{0}{1}'.format(
                '0' * (self.payload - len(str(self.message_count))), 
                self.message_count
            )
            self.connection.sendto(message, self.group)
            print("{0} source={1}:{2} destination={3}:{4} seq={5} size={6}"
                    .format(datetime.datetime.now(), 
                            self.local_ip, self.local_port, 
                            self.group_ip, self.group_port,
                            self.message_count, len(message)))

            self.message_count += 1
            time.sleep(self.interval)

    def send(self, timeout=None, payload=64, interval=0.5, ttl=10):
        self.ttl = ttl
        self.interval = interval
        self.payload = payload
        self.timeout = timeout
        self.start_time = datetime.datetime.now()

        self.bind_to_socket()

        try:
            self.send_loop()

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
    parser.add_argument('--ttl', type=int,
                        dest='ttl', default=10, required=False,
                        help='Set the packet TTL. Default: 10')
    parser.add_argument('--payload', type=int,
                        dest='payload', default=64, required=False,
                        help='Set the size of the payload. Default: 64')
    parser.add_argument('--interval', type=float,
                        dest='interval', default=0.5, required=False,
                        help='Send packet in intervals (in seconds). Default: 0.5 ')


    args = parser.parse_args()
    return args


if __name__ == '__main__':
    arguments = get_arguments()

    mcast = MSend(arguments.address,
                  arguments.port,
                  arguments.interface)

    mcast.send(timeout=arguments.timeout,
               payload=arguments.payload,
               interval=arguments.interval,
               ttl=arguments.ttl)
