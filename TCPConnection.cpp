#include "TCPConnection.h"
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <iostream>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>

TCPConnection::TCPConnection(const std::string& addr, int port, bool server):
    m_server(server),
    m_addr(addr),
    m_port(port),
    m_event_callback(nullptr),
    m_msg_callback(nullptr),
    m_bev(nullptr)
{
}

TCPConnection::~TCPConnection()
{    
    if (m_bev != nullptr)
        bufferevent_free(m_bev);
        
    if (m_base != nullptr)
        event_base_free(m_base);
        
    for (auto timerid: m_timer_ids) {
        delete timerid;
    }
}

bool TCPConnection::Init()
{
    m_base = event_base_new();
    if (!m_base) {
	    std::cerr << "couldn't open event base" << std::endl;
	    return false;
    } else {
        sockaddr_in sin;
	    sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = inet_addr(m_addr.c_str());
	    sin.sin_port = htons(m_port);

        if (m_server) {
    	    evconnlistener* listener = evconnlistener_new_bind(
    	        m_base, &TCPConnection::OnConnect, this,
                LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,
                -1, (sockaddr*)&sin, sizeof(sin));
            if (!listener) {
                std::cerr << "couldn't create listener" << std::endl;
                return false;
            } else {
	            evconnlistener_set_error_cb(listener, &TCPConnection::OnConnectError);
            }
        } else  {
            std::cout << m_addr << ","<< sin.sin_addr.s_addr << std::endl;
            SetupMsgAndEventCallback(-1);
            if (bufferevent_socket_connect(m_bev, (sockaddr*)&sin, sizeof(sin)) < 0) {
                std::cerr << "couldn't open socket" << std::endl;
                return false;
            }
        }
    }
    return true;
}

void TCPConnection::Start()
{
    event_base_dispatch(m_base);
}

void TCPConnection::Stop()
{
    event_base_loopexit(m_base, NULL);
}

void TCPConnection::Send(const std::string& message)
{
    evbuffer* buf = bufferevent_get_output(m_bev);
    evbuffer_add(buf, message.c_str(), message.length());
}

void TCPConnection::Send(evbuffer* buf)
{
    evbuffer* output = bufferevent_get_output(m_bev);
    evbuffer_add_buffer(output, buf);
}

void TCPConnection::OnConnect(evconnlistener* listener, int fd, 
	sockaddr* address, int socklen, void *ctx)
{
    std::cout << "connection established" << std::endl;

    TCPConnection* conn = static_cast<TCPConnection*>(ctx);
    conn->SetupMsgAndEventCallback(fd);
}

void TCPConnection::SetupMsgAndEventCallback(int fd)
{
    m_bev = bufferevent_socket_new(m_base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(m_bev, &TCPConnection::OnMessage, NULL, 
			&TCPConnection::OnEvent, this);
    bufferevent_enable(m_bev, EV_READ|EV_WRITE);
    
    for (auto timerid: m_timer_ids) {
        SetupTimeoutCallback(timerid);
    }
}

void TCPConnection::OnConnectError(evconnlistener* listener, void* ctx)
{
    event_base* base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    std::cerr << "Got an error " << err <<  " " << 
	    evutil_socket_error_to_string(err) << 
	    " on the listener. Shutting down." << std::endl;
    event_base_loopexit(base, NULL);
}

void TCPConnection::OnEvent(bufferevent* bev, short events, void* ctx)
{
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        std::cerr << "eof or error from bufferevent" << std::endl;
        bufferevent_free(bev);
    }
    if (events & BEV_EVENT_CONNECTED) {
        std::cout << "bufferevent connected" << std::endl;
        evutil_socket_t fd = bufferevent_getfd(bev);
        int yes = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(int));
    }

    TCPConnection* conn = static_cast<TCPConnection*>(ctx);
    if (conn->m_event_callback != nullptr)
	conn->m_event_callback(events);
}

void TCPConnection::OnMessage(bufferevent* bev, void* ctx)
{
    TCPConnection* conn = static_cast<TCPConnection*>(ctx);
    if (conn->m_msg_callback != nullptr) {
        evbuffer* input = bufferevent_get_input(bev);
        conn->m_msg_callback(input);
    }
}

void TCPConnection::OnTimeout(int fd, short what, void* ctx)
{
    TimerId* timerid = static_cast<TimerId*>(ctx);
    timerid->callback();
    timerid->conn->m_timer_ids.erase(timerid);
    delete timerid;
}

void TCPConnection::AddTimeoutCallback(std::function<void()> callback, int ms)
{
    TimerId* timerid = new TimerId{this, callback, ms};
    m_timer_ids.insert(timerid);
    
    // if timeout is added before the base get initialized, wait
    if (m_base != nullptr) {
        SetupTimeoutCallback(timerid);
    }
}

void TCPConnection::SetupTimeoutCallback(TimerId* timerid)
{
    timeval tv;
    tv.tv_sec = timerid->ms / 1000;
    tv.tv_usec = (timerid->ms % 1000) * 1000;
    event* evt = evtimer_new(m_base, &TCPConnection::OnTimeout, timerid);
    event_add(evt, &tv);
}
