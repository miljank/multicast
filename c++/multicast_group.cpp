#include <cstring>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#include <arpa/inet.h>


#define MSGBUFSIZE 1500


class MulticastGroup {
    public:
        int timeout;
        int group_port;
        int* msg_counter;
        time_t* start_time;
        const char* group_address;
        const char* local_address;
        int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);


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
                if (timeout != -1 && timeout <= get_run_time(*start_time))
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
