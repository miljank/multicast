#include <signal.h>
#include "common.cpp"
#include "multicast_group.cpp"


int main(int argc, char **argv)
{
    process_command_line_arguments(argc, argv);

    MulticastGroup mg;
    mg.local_address = local_address;
    mg.group_address = group_address;
    mg.group_port = group_port;
    mg.timeout = timeout;
    mg.start_time = &start_time;
    mg.msg_counter = &msg_counter;

    signal(SIGINT, signal_handler);

    mg.start();
    show_exit_statistics();
    return 0;
}
