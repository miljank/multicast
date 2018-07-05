#[macro_use]
extern crate clap;
extern crate time;
extern crate chrono;
extern crate socket2;
extern crate pnet_datalink;

use clap::App;
use std::process;
use std::str::FromStr;
use chrono::{DateTime, Local};
use socket2::{Domain, Protocol, Socket, SockAddr, Type};
use std::net::{IpAddr, Ipv4Addr, SocketAddr};

fn get_interface_ip_address(name: &str) -> Ipv4Addr {
    for interface in pnet_datalink::interfaces() {
        if (interface.name == name) && (!interface.ips.is_empty()) {
            let local_ip = interface.ips.first().unwrap().ip().to_string();
            return Ipv4Addr::from_str(&local_ip)
                .expect("error: Could not parse local IP address");
        }
    }

    println!("error: Could not find local interface address");
    process::exit(1);
}

fn get_socket(address: Ipv4Addr, port: u16, local_ip: Ipv4Addr) -> Socket {
    let socket = Socket::new(Domain::ipv4(), Type::dgram(), Some(Protocol::udp())).unwrap();
    let group_address = SocketAddr::new(IpAddr::V4(address), port);

    socket.set_nonblocking(true)
        .expect("error: Could not set non-blocking socket");
    socket.join_multicast_v4(&address, &local_ip)
        .expect("error: Could not subscribe to the group");
    socket.bind(&SockAddr::from(group_address))
        .expect("error: Could not bind to the group");

    return socket;
}

fn handle_exit(run_time: time::Duration, message_count: u64) {
    println!("\nExiting after {} seconds and {} messages.", run_time.num_seconds(), message_count);

    if message_count == 0 {
        process::exit(1);
    }

    process::exit(0);
}

fn process_timeout(timeout: i64, start_time: DateTime<Local>, message_count: u64, socket: &Socket) {
    if timeout > 0 {
        let current_time = chrono::Local::now();
        let run_time = current_time.signed_duration_since(start_time);
        println!("{}", run_time.num_seconds());

        if run_time.num_seconds() > timeout {
            handle_exit(run_time, message_count);
        }
    }
}

fn subscribe_and_show(address: Ipv4Addr, port: u16, local_ip: Ipv4Addr, timeout: i64) {
    let mut message_count = 0;
    let start_time = chrono::Local::now();

    let socket = get_socket(address, port, local_ip);
    let mut buffer = [0u8; 2048];

    println!("subscriber={} destination={}:{}", local_ip.to_string(), address.to_string(), port);

    loop {
        process_timeout(timeout, start_time, message_count, &socket);
        match socket.recv_from(&mut buffer) {
            Ok((size, sender)) => {
                message_count += 1;
                println!("{} source={:?} destination={}:{} size={}", chrono::Local::now(), sender.as_inet().unwrap(), address, port, size);
            },
            Err(err) => {}
        }
    }
}

fn main() {
    let yaml = load_yaml!("mrecv.yaml");
    let matches = App::from_yaml(yaml).get_matches();

    let address = value_t!(matches.value_of("address"), Ipv4Addr).unwrap_or_else(|e| e.exit());
    let port = value_t!(matches.value_of("port"), u16).unwrap_or_else(|e| e.exit());
    let interface = matches.value_of("interface").unwrap();
    let timeout = value_t!(matches.value_of("timeout"), i64).unwrap_or(0);

    if address.is_multicast() == false {
        println!("error: Address is not a valid multicast address");
        process::exit(1);
    }

    if port < 1025 {
        println!("error: Port is out of range");
        process::exit(1);
    }

    let local_ip = get_interface_ip_address(interface);
    subscribe_and_show(address, port, local_ip, timeout)
}
