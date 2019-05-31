#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <ifaddrs.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define MSGBUFSIZE 256

int timeout = -1;
int group_port = -1;
const char* group_address = "";
const char* interface = "";
const char* local_address = "";
int msg_counter = 0;
time_t start_time = time(0);

int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

int get_run_time()
{
    return difftime(time(0), start_time);
}

void show_exit_statistics() 
{
    std::cout << std::endl << "Exiting after "
                << get_run_time() << " seconds and "
                << msg_counter << " messages." << std::endl;
    exit(0);
}

void signal_handler(int s)
{
    show_exit_statistics();
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

    if ( std::strcmp(local_address, "") == 0 ) 
    {
        std::cout << "error: Could not get interface IP address: " 
                  << interface << std::endl;
        exit(1);
    }
}

class MulticastGroup {
    public:
        int timeout;
        int group_port;
        const char* group_address;
        const char* local_address;
        int* msg_counter;

        std::string get_current_time()
        {
            char format[64];
            char buffer[64];
            struct timeval tv;
            struct tm *tm;

            gettimeofday(&tv, NULL);
            tm = localtime(&tv.tv_sec);
            strftime(format, sizeof (format), "%Y-%m-%d %H:%M:%S.%%06u", tm);
            snprintf(buffer, sizeof (buffer), format, tv.tv_usec);
            
            return std::string(buffer);
        }

        void join()
        {
            int opt = 1; 
            
            struct ip_mreq mreq;
            mreq.imr_multiaddr.s_addr = inet_addr(group_address);
            mreq.imr_interface.s_addr = inet_addr(local_address);

            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
            {
                std::cout << "error: Failed to set socket options "
                          << "SO_REUSEADDR | SO_REUSEPORT: " << errno << std::endl;
                exit(-1);
            }

            if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) == -1) 
            {
                std::cout << "error: Failed to set socket option "
                          << "IP_ADD_MEMBERSHIP: " << errno << std::endl;
                exit(-1);
            }
        }

        void recieve()
        {
            socklen_t addrlen;
            int nbytes;
            char msgbuf[MSGBUFSIZE];
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));

            addr.sin_family = AF_INET;
            addr.sin_port = htons(group_port);
            addr.sin_addr.s_addr = inet_addr(group_address);

            if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                std::cout << "error: Failed to bind to " 
                          << group_address << ":" << group_port << std::endl;
                exit(1);
            }

            while (1)
            {
                memset(&msgbuf, 0, MSGBUFSIZE);

                addrlen = sizeof(addr);
                nbytes = recvfrom(sockfd, msgbuf, MSGBUFSIZE, 0, (struct sockaddr *)&addr, &addrlen);
                if (timeout != -1 && timeout <= get_run_time())
                    break;

                if (nbytes < 0)
                {
                    usleep(1000);
                    continue;
                }

                (*msg_counter)++;
                std::cout << get_current_time()
                          << " source=" << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port
                          << " destination=" << group_address << ":" << group_port 
                          << " size=" << nbytes << std::endl;
                std::cout.flush();
            }
        }

        void start()
        {
            join();
            std::cout << "subscriber=" << local_address
                      << " destination=" << group_address << ":" << group_port << std::endl;
            recieve();
        }
};

int main(int argc, char **argv)
{
    process_command_line_arguments(argc, argv);

    MulticastGroup mg;
    mg.local_address = local_address;
    mg.group_address = group_address;
    mg.group_port = group_port;
    mg.timeout = timeout;
    mg.msg_counter = &msg_counter;

    signal(SIGINT, signal_handler);

    mg.start();
    show_exit_statistics();
    return 0;
}
