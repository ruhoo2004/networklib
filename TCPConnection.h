#pragma once

#include <string>
#include <functional>
#include <set>

struct evbuffer;
struct bufferevent;
struct evconnlistener;
struct sockaddr;
struct event_base;

class TCPConnection {
public:
    TCPConnection(const std::string& addr, int port, bool server = true);
    ~TCPConnection();

    bool Init();
    void Start();
    void Stop();
    void Send(const std::string& message);
    void Send(evbuffer* buf);

    void SetEventCallback(std::function<void(short)> callback) 
        { m_event_callback = callback; }
    void SetMsgCallback(std::function<void(evbuffer*)> callback)
        { m_msg_callback = callback; }
    void AddTimeoutCallback(std::function<void()> callback, int ms);

private:
    struct TimerId {
        TCPConnection* conn;
        std::function<void()> callback;
        int ms;
    };
    
    static void OnConnect(evconnlistener* listener, int fd, 
    	sockaddr* address, int socklen, void *ctx);
    static void OnConnectError(evconnlistener* listener, void* ctx);
    static void OnEvent(bufferevent* bev, short events, void* ctx);
    static void OnMessage(bufferevent* bev, void* ctx);
    static void OnTimeout(int fd, short events, void* ctx);
    
    void SetupMsgAndEventCallback(int fd);
    void SetupTimeoutCallback(TimerId* timerid);

    bool m_server;
    const std::string m_addr;
    const int m_port;

    std::function<void(short)> m_event_callback;
    std::function<void(evbuffer*)> m_msg_callback;
    std::set<TimerId*> m_timer_ids;

    event_base* m_base;
    bufferevent* m_bev;
};
