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

// MulticastClient ...
type MulticastClient struct {
	Interface      string
	Address        string
	Port           int
	LocalInterface *net.Interface
	LocalAddress   string
	Group          *net.UDPAddr
	Connection     *net.UDPConn
	Messages       int
	StartTime      int64
	Timeout        int
}

// GetInterfaceName ...
func (m *MulticastClient) GetInterfaceName() (*net.Interface, error) {
	localInterface, err := net.InterfaceByName(m.Interface)
	if err != nil {
		fmt.Println("error: Could not get IP address for interface " + m.Interface)
	}

	m.LocalInterface = localInterface
	return m.LocalInterface, err
}

// GetLocalAddress ...
func (m *MulticastClient) GetLocalAddress() bool {
	nic, err := m.GetInterfaceName()
	if err != nil {
		return false
	}

	addrs, err := nic.Addrs()
	if err != nil {
		return false
	}

	for _, addr := range addrs {
		switch v := addr.(type) {
		case *net.IPNet:
			if v.IP.To4() != nil {
				m.LocalAddress = v.IP.String()
				return true
			}
		case *net.IPAddr:
			if v.IP.To4() != nil {
				m.LocalAddress = v.IP.String()
				return true
			}
		}
	}

	return false
}

//IsValid ...
func (m *MulticastClient) IsValid() bool {
	if (m.Port < 1024) || (m.Port > 65535) {
		fmt.Println("error: Port is out of range")
		return false
	}

	ip := net.ParseIP(m.Address)
	if !ip.IsMulticast() {
		fmt.Println("error: Address is not a valid multicast address")
		return false
	}

	return true
}

// CreateGroup ...
func (m *MulticastClient) CreateGroup() {
	ip := net.ParseIP(m.Address)
	m.Group = &net.UDPAddr{IP: ip, Port: m.Port}
}

// CreateConnection ..
func (m *MulticastClient) CreateConnection() bool {
	conn, err := net.ListenMulticastUDP("udp", m.LocalInterface, m.Group)
	if err != nil {
		fmt.Println("error: Could not join group " + m.Interface)
		return false
	}

	m.Connection = conn
	return true
}

// Setup ...
func (m *MulticastClient) Setup() bool {
	if !m.IsValid() {
		return false
	}

	if !m.GetLocalAddress() {
		return false
	}

	m.CreateGroup()
	if !m.CreateConnection() {
		return false
	}
	return true
}

// SetTimeout ...
func (m *MulticastClient) SetTimeout(timeout int) {
	m.Timeout = timeout
}

// ShowDetails ...
func (m *MulticastClient) ShowDetails() {
	fmt.Println("subscriber="+m.LocalAddress, "destination="+m.Address+":"+strconv.Itoa(m.Port))
}

// HasTimedOut ...
func (m *MulticastClient) HasTimedOut() bool {
	if m.Timeout == 0 {
		return false
	}

	currentTime := time.Now().Unix()
	if int(currentTime-m.StartTime) >= m.Timeout {
		return true
	}

	return false
}

// ShowPacket ...
func (m *MulticastClient) ShowPacket(rawData int, source string, destination string) {
	t := time.Now()
	timestamp := t.Format("2006-01-02 15:04:05.999999")
	data := make([]byte, rawData)

	fmt.Println(timestamp, "source="+source, "destination="+destination+":"+strconv.Itoa(m.Port),
		"size="+strconv.Itoa(len(data)))
}

// Run ...
func (m *MulticastClient) Run() {
	packetConnection := ipv4.NewPacketConn(m.Connection)
	packetConnection.SetControlMessage(ipv4.FlagSrc|ipv4.FlagDst, true)
	defer packetConnection.Close()
	defer m.Connection.Close()

	m.StartTime = time.Now().Unix()
	buffer := make([]byte, 2048)

	m.ShowDetails()

	for {
		if m.HasTimedOut() {
			fmt.Println("\nExiting after", m.Timeout, "seconds and", m.Messages, "messages")
			break
		}

		rawData, cm, src, err := packetConnection.ReadFrom(buffer)
		if err != nil {
			fmt.Println(err)
			break
		}

		m.Messages++
		m.ShowPacket(rawData, src.String(), cm.Dst.String())
	}
}

func main() {
	address := flag.String("address", "none", "Multicast group IP")
	port := flag.Int("port", 0, "Multicast group port")
	nic := flag.String("interface", "none", "Bind to a specific interface")
	timeout := flag.Int("timeout", 0, "Exit program after specified number of seconds")

	flag.Parse()
	client := MulticastClient{Interface: *nic, Address: *address, Port: *port}
	if !client.Setup() {
		os.Exit(1)
	}

	client.SetTimeout(*timeout)
	client.Run()
}
