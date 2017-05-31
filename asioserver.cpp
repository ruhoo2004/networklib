#include <boost/asio.hpp>
#include <memory>
#include <iostream>

using boost::asio::ip::tcp;

const int max_length = 2048;
char data_[max_length];

void DoRead(tcp::socket* socket_);

void DoWrite(tcp::socket* socket_, std::size_t length)
{
    boost::asio::async_write(*socket_, boost::asio::buffer(data_, length),
        [socket_](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                DoRead(socket_);
            }
        });
}

void DoRead(tcp::socket* socket_)
{
    socket_->async_read_some(boost::asio::buffer(data_, max_length),
        [socket_] (boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                DoWrite(socket_, length);
            }
        });
}

void Accept(tcp::acceptor* acceptor_, tcp::socket* socket_)
{
    acceptor_->async_accept(*socket_, [socket_, acceptor_](boost::system::error_code ec) {
        if (!ec) {
            DoRead(socket_);
            Accept(acceptor_, socket_);
        }
    });
}

int main(int argc, char** argv)
{
    try {
        boost::asio::io_service io_service;

        tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), 8000));
        tcp::socket socket_(io_service);

        Accept(&acceptor_, &socket_);

        io_service.run();
    } catch (std::exception& e)
    {
        std::cerr << "Exception " << e.what() << std::endl;
    }

    return 0;
}
