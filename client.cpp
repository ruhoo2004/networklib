#include "TCPConnection.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>

int main(int argc, char **argv)
{
    std::string ip_addr = "0.0.0.0";
    if (argc >= 2) {
        ip_addr = argv[1];
    }

    int packet_size = 1024;
    if (argc >= 3) {
        packet_size = std::stoi(argv[2]);
    }
    
    TCPConnection conn(ip_addr, 8000, false);

    conn.SetEventCallback([](short events) {
        if (events & BEV_EVENT_ERROR)
            std::cout << "client: error from bufferevent" << std::endl;
    });

    int total_bytes_read = 0;
    int total_messages_read = 0;
    conn.SetMsgCallback([&](evbuffer* buf) {
        total_bytes_read += evbuffer_get_length(buf);
        total_messages_read++;
        
        conn.Send(buf);
    });
    
    conn.AddTimeoutCallback([&]() {
        conn.Stop();
        
        std::cout << total_bytes_read << " total bytes read" << std::endl;
            std::cout << total_messages_read << " total messages read" << std::endl;
            std::cout << (double)total_bytes_read/total_messages_read 
                << " average message size" << std::endl;
            std::cout << (double)total_bytes_read/(1024 * 1024 * 10) 
                << " MiB/s throughput" << std::endl;
    }, 10000);
    
    if (conn.Init()) {
        std::string message;
        for (int i=0; i<packet_size; ++i) {
            message += (i % 26) + 'a';
        }
        conn.Send(message);
        conn.Start();
    }

    return 0;
}
