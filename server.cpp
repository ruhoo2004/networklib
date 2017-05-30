#include "TCPConnection.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>

int main(int argc, char **argv)
{
    TCPConnection conn("0.0.0.0", 8000);

    conn.SetEventCallback([](short events) {
        if (events & BEV_EVENT_ERROR)
            std::cout << "main: error from bufferevent" << std::endl;
    });

    conn.SetMsgCallback([&](evbuffer* buf) {
        conn.Send(buf);
    });

    if (conn.Init())
        conn.Start();

    return 0;
}
