package main

import (
	"flag"
	"fmt"
	"net"
	"os"
	"strconv"
	"time"

	"golang.org/x/net/ipv4"
)

func getLocalAddress(nic *net.Interface) string {
	addrs, err := nic.Addrs()
	if err != nil {
		fmt.Println("error: Could not get local IP address")
		os.Exit(1)
	}

	for _, addr := range addrs {
		switch v := addr.(type) {
		case *net.IPNet:
			return v.IP.String()
		case *net.IPAddr:
			return v.IP.String()
		}
	}

	return "none"
}

func getMulticastGroup(port int, address string) *net.UDPAddr {
	if (port < 1024) || (port > 65535) {
		fmt.Println("error: Port is out of range")
		os.Exit(1)
	}

	ip := net.ParseIP(address)
	if !ip.IsMulticast() {
		fmt.Println("error: Address is not a valid multicast address")
		os.Exit(1)
	}

	return &net.UDPAddr{IP: ip, Port: port}
}

func getInterfaceName(nic string) (*net.Interface, error) {
	localInterface, err := net.InterfaceByName(nic)
	return localInterface, err
}

func getConnection(localInterface *net.Interface, multicastGroup *net.UDPAddr) (*net.UDPConn, error) {
	conn, err := net.ListenMulticastUDP("udp", localInterface, multicastGroup)
	return conn, err
}

func checkTimeout(startTime int64, timeout int) bool {
	currentTime := time.Now().Unix()

	if int(currentTime-startTime) >= timeout {
		return true
	}

	return false
}

func showPackage(rawData int, source string, destination string, port string) {
	t := time.Now()

	timestamp := t.Format("2006-01-02 15:04:05.999999")
	data := make([]byte, rawData)
	size := strconv.Itoa(len(data))

	fmt.Println(timestamp + " source=" + source +
		" destination=" + destination + ":" + port + " size=" + size)
}

func main() {
	address := flag.String("address", "none", "Multicast group IP")
	port := flag.Int("port", 0, "Multicast group port")
	nic := flag.String("interface", "none", "Bind to a specific interface")
	timeout := flag.Int("timeout", 0, "Exit program after specified number of seconds")

	flag.Parse()
	multicastGroup := getMulticastGroup(*port, *address)

	localInterface, err := getInterfaceName(*nic)
	if err != nil {
		fmt.Println("error: Could not get IP address for interface " + *nic)
		os.Exit(1)
	}

	conn, err := getConnection(localInterface, multicastGroup)
	if err != nil {
		fmt.Println("error: Could not join group " + *nic)
		os.Exit(1)
	}
	defer conn.Close()

	localAddress := getLocalAddress(localInterface)
	fmt.Println("subscriber=" + localAddress + " destination=" +
		*address + ":" + strconv.Itoa(*port))

	packetConn := ipv4.NewPacketConn(conn)
	defer packetConn.Close()

	startTime := time.Now().Unix()
	messages := 0

	packetConn.SetControlMessage(ipv4.FlagSrc|ipv4.FlagDst, true)
	buf := make([]byte, 2048)

	for {
		if *timeout != 0 && checkTimeout(startTime, *timeout) {
			fmt.Println("\nExiting after " + strconv.Itoa(*timeout) +
				" seconds and " + strconv.Itoa(messages) + " messages")
			os.Exit(0)
		}

		rawData, cm, src, err := packetConn.ReadFrom(buf)
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}

		messages++
		showPackage(rawData, src.String(), cm.Dst.String(), strconv.Itoa(*port))
	}
}
