#!/usr/bin/env node

'use strict';

var os = require('os');
var net = require('net');
var dgram = require('dgram');
var args = require('commander');

class Multicast {
    constructor(groupAddress, groupPort, localInterface, timeout) {
        this.groupAddress = groupAddress;
        this.groupPort = groupPort;
        this.localInterface = localInterface;
        this.timeout = timeout;
        this.localAddress;
        this.startTime;
        this.messageCounter = 0;
        this.socket;
    }

    isMulticastAddress(groupAddress) {
        if (! net.isIPv4(groupAddress))
            return false;
        
        var firstOctet = parseInt(groupAddress.split('.')[0]);
        if (firstOctet < 224 || firstOctet > 239)
            return false;

        return true;
    }

    getLocalIpAddress() {
        const interfaces = os.networkInterfaces();

        var localAddress = Object.keys(interfaces)
                .filter(name => name === this.localInterface)
                .reduce((results, name) => interfaces[name], [])
                .filter((address) => address.family === 'IPv4')
                .map((address) => address.address)[0];

        if (! net.isIPv4(localAddress))
            return false;
        
        return localAddress;
    }

    validateParameters() {
        if (! this.groupAddress) {
            console.log('error: Address is a required field');
            process.exit(1);
        }

        if (! this.isMulticastAddress(this.groupAddress)) {
            console.log('error: Address is not a valid multicast address');
            process.exit(1);
        }

        if (! this.groupPort) {
            console.log('error: Port is a required field');
            process.exit(1);
        }

        if (this.groupPort <= 1024 || this.groupPort > 65535) {
            console.log('error: Port is out of range');
            process.exit(1);
        }

        if (! this.localInterface) {
            console.log('error: Interface is a required field');
            process.exit(1);
        }

        this.localAddress = this.getLocalIpAddress();
        if ( ! this.localAddress) {
            console.log('error: Interface not found or interface has no IP address');
            process.exit(1);
        }
    }

    setSignalHandler(self) {
        process.on('SIGINT', function() {
            self.socket.close(() => {
                self.showExitStatistics();
                process.exit();
            });
        });
    }

    showMessage(message, remote) {
        var time_now = new Date().toISOString();

        console.log(
            time_now +
            " source="+ remote.address +":"+ remote.port +
            " destination="+ this.groupAddress +":"+ this.groupPort +
            " size="+ remote.size
        );

        this.messageCounter++;
    }

    timeSinceStart() {
        var currentTime = new Date();
        return Math.floor((currentTime - this.startTime) / 1000);
    }

    showExitStatistics() {
        console.log("\nExiting after "+ this.timeSinceStart() +" seconds and "+ this.messageCounter +" messages.")
    }

    checkForTimeout() {
        if (! this.timeout)
            return;

        if (this.timeSinceStart() >= this.timeout) {
            this.showExitStatistics();
            process.exit(0);
        }
    }

    subscribeToFeed() {
        this.startTime = new Date();
        console.log(
            "subscriber="+ this.localAddress +
            " destination="+ this.groupAddress +":"+ this.groupPort
        );

        this.socket = dgram.createSocket({ type: 'udp4', reuseAddr: true });
        this.socket.bind(this.groupPort, () => {
            this.socket.addMembership(this.groupAddress, this.localAddress);
        });
    }

    receiveData(self) {
        this.socket.on('message', function(message, remote) {
            self.showMessage(message, remote);
        });
    }

    startTimeoutCheck(self) {
        setInterval(function() {
            self.checkForTimeout()
        }, 500);
    }

    start() {
        var self = this;

        this.validateParameters();
        this.setSignalHandler(self);
        this.subscribeToFeed();
        this.receiveData(self);
        
        if (this.timeout)
            this.startTimeoutCheck(self);
    }
}

args.version('0.0.1')
    .option('--address <value>', 'Multicast group IP')
    .option('--port <number>', 'Multicast group port', parseInt)
    .option('--interface <value>', 'Bind to a specific interface')
    .option('--timeout [value]', 'Exit program after specified number of seconds', parseInt)
    .parse(process.argv);

let mreceiver = new Multicast(
    args.address,
    args.port,
    args.interface,
    args.timeout
);

mreceiver.start();
