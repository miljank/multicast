#!/usr/bin/env ruby

require 'socket'
require 'ipaddr'
require 'optparse'

SIOCGIFADDR    = 0x8915

def is_multicast_address(address)
    first_octet = /22[4-9]|23[0-9]/
    octet = /25[0-5]|2[0-4]\d|1[0-9]{2}|[0-9]{,2}/
    mc_ip_re = /\A#{first_octet}\.#{octet}\.#{octet}\.#{octet}\z/

    mc_ip_re.match(address).to_s == address
end

class MRecv
    def initialize(group_ip, group_port, interface)
        @group_ip = group_ip
        @group_port = group_port
        @interface = interface

        @local_ip = get_interface_ip_address
        @group = IPAddr.new(@group_ip).hton + IPAddr.new(@local_ip).hton
        @connection = UDPSocket.new

        @message_count = 0
        @start_time = nil
    end

    def get_interface_ip_address()
        socket = UDPSocket.new
        buffer = [@interface, ""].pack('a16h16')
        socket.ioctl(SIOCGIFADDR, buffer);
        socket.close

        buffer[20..24].unpack("CCCC").join(".")
    end

    def subscribe_to_feed
        @connection.setsockopt(Socket::IPPROTO_IP, Socket::IP_ADD_MEMBERSHIP, @group)
        @connection.setsockopt(Socket::SOL_SOCKET, Socket::SO_REUSEPORT, 1)
        @connection.bind(@local_ip, @group_port)
    end

    def show_process_stats(run_time)
        puts "\nExiting after #{run_time.to_i} seconds and #{@message_count} messages."
    end

    def process_timeout(timeout)
        run_time = Time.new - @start_time

        if run_time.to_i < timeout
            return true
        end

        @connection.close
        show_process_stats(run_time)

        if @message_count == 0
            exit 1
        end

        exit 0
    end

    def process_loop(timeout)
        loop do
            if timeout > 0
                process_timeout(timeout)
            end

            begin
                message, sender = @connection.recvfrom_nonblock(2048)

            rescue Errno::EAGAIN
                next

            else
                @message_count += 1
                time_now = Time.new
                puts "#{time_now} source=#{sender[3]}:#{sender[1]} destination=#{@group_ip}:#{group_port} size=#{message.size}"
            end
        end
    end

    def show(timeout=nil)
        @start_time = Time.new
        subscribe_to_feed

        puts "subscriber=#{@local_ip} destination=#{@group_ip}:#{@group_port}"
        begin
            process_loop(timeout)

        rescue Interrupt
            @connection.close
            run_time = Time.new - @start_time
            show_process_stats(run_time)
            exit 0
        end
    end
end

def get_arguments
    options = {}
    OptionParser.new do |opt|
        opt.on('--address ADDRESS', String,
            "Multicast group IP") { |o| options[:address] = o }
        opt.on('--port PORT', Integer, 
            "Multicast group port") { |o| options[:port] = o }
        opt.on('--interface INTERFACE', String, 
            "Bind to a specific interface") { |o| options[:interface] = o }
        opt.on('--timeout TIMEOUT', Integer, 
            "Exit program after specified number of seconds") { |o| options[:timeout] = o }
    end.parse!

    if options[:address] == nil
        puts "error: Address is a required field"
        exit!
    else
        if ! is_multicast_address(options[:address])
            puts "error: Address is not a valid multicast address"
            exit!
        end
    end

    if options[:port] == nil
        puts "error: Port is a required field"
        exit!
    else
        if (options[:port] < 1025) || (options[:port] > 65535)
            puts "error: Port is out of range"
            exit!
        end
    end

    if options[:interface] == nil
        puts "error: Interface is a required field"
        exit!
    end

    options
end

options = get_arguments
mcast = MRecv.new(options[:address], options[:port], options[:interface])
mcast.show(timeout=options[:timeout])