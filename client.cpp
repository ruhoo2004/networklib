#include "TCPConnection.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>

int main(int argc, char **argv)
{
    int packet_size = 1024;
    if (argc >= 2) {
        packet_size = std::stoi(argv[1]);
    }
    
    TCPConnection conn("127.0.0.1", 8000, false);

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
            std::cout << (double)total_bytes_read/(1024 * 1024 * 10000 / 1000) 
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
