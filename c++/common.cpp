#include <getopt.h>
#include <cstring>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <iostream>


int timeout = -1;
int group_port = -1;
const char* group_address = "";
const char* interface = "";
const char* local_address = "";
int msg_counter = 0;
time_t start_time = time(0);


int get_run_time(time_t start_time)
{
    return difftime(time(0), start_time);
}


void show_exit_statistics() 
{
    std::cout << std::endl << "Exiting after "
              << get_run_time(start_time) << " seconds and "
              << msg_counter << " messages." << std::endl;
    exit(0);
}


void signal_handler(int s)
{
    show_exit_statistics();
}


const char* get_interface_ip_address(const char* interface_name)
{
    const char* ip_address = NULL;
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_interface = NULL;
    struct in_addr temp_addr;

    int success = getifaddrs(&interfaces);

    if (success == 0) {
        temp_interface = interfaces;

        while(temp_interface != NULL) {
            if (temp_interface->ifa_addr && temp_interface->ifa_addr->sa_family == AF_INET)
            {
                if (std::strcmp(temp_interface->ifa_name, interface_name) == 0)
                {
                    temp_addr = ((struct sockaddr_in*)temp_interface->ifa_addr)->sin_addr;
                    ip_address = inet_ntoa(temp_addr);
                    break;
                }
            }

            temp_interface = temp_interface->ifa_next;
        }
    }

    freeifaddrs(interfaces);
    return ip_address;
}


void print_help()
{
    std::cout <<
        "--address <address>        Multicast group IP\n"
        "--port <port>              Multicast group port\n"
        "--interface <interface>    Bind to a specific interface\n"
        "--timeout <timeout>        Exit program after specified number of seconds\n"
        "--help                     Show this help message\n";
    exit(1);
}


void process_command_line_arguments(int argc, char** argv)
{
    const char* const short_opts = "a:p:i:t:h";
    const option long_opts[] = {
        {"address", required_argument, nullptr, 'a'},
        {"port", required_argument, nullptr, 'p'},
        {"interface", required_argument, nullptr, 'i'},
        {"timeout", required_argument, nullptr, 't'},
        {"help", no_argument, nullptr, 'h'}
    };

    while (true)
    {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (-1 == opt)
            break;

        switch (opt)
        {
            case 'a':
                group_address = optarg;
                break;

            case 'p':
                if (! isdigit(optarg[0]) )
                {
                    std::cout << "error: Port is not a number: " 
                              << optarg << std::endl;
                    exit(1);
                }

                group_port = std::stoi(optarg);
                break;

            case 'i':
                interface = optarg;
                local_address = get_interface_ip_address(interface);
                break;

            case 't':
                if ( ! isdigit(optarg[0]) )
                {
                    std::cout << "error: Timeout is out a number: " 
                              << optarg << std::endl;
                    exit(1);
                }

                timeout = std::stoi(optarg);
                break;

            case 'h':
            case '?':
            default:
                print_help();
                break;
        }
    }

    if (! IN_MULTICAST(ntohl(inet_addr(group_address))) ) 
    {
        std::cout << "error: Address is not a valid multicast address: "
                  << group_address << std::endl;
        exit(1);
    }

    if (group_port < 1025 || group_port > 65535) 
    {
        std::cout << "error: Port is out of range: "
                  << group_port << std::endl;
        exit(1);
    }

    if ( inet_aton(local_address, &addr) == 0 ) 
    {
        std::cout << "error: Could not get interface IP address: " 
                  << interface << std::endl;
        exit(1);
    }

}
