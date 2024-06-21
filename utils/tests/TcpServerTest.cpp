#include <iostream>
#include "boost/asio.hpp"
#include <functional>

using namespace std;

void handle_accept(const boost::system::error_code& ec, boost::asio::ip::tcp::acceptor& acceptor);

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    boost::asio::io_context io_context;
    cout << &io_context << endl;

    short port = std::atoi(argv[1]);
    boost::asio::ip::tcp::acceptor acceptor(io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

    // 开始异步等待连接
    boost::asio::ip::tcp::socket socket_(io_context);
    acceptor.async_accept(socket_, std::bind(handle_accept, std::placeholders::_1, std::ref(acceptor)));

    // 运行事件循环
    io_context.run();
    return 0;
}

void handle_accept(const boost::system::error_code& ec, boost::asio::ip::tcp::acceptor& acceptor) {
    if (!ec) {
        cout << "New connection" << endl;

        // 创建一个新的socket来接受下一个连接
        auto& io_context = static_cast<boost::asio::io_context&>(acceptor.get_executor().context());
        boost::asio::ip::tcp::socket new_socket(io_context);

        // 继续等待下一个连接
        acceptor.async_accept(new_socket, std::bind(handle_accept, std::placeholders::_1, std::ref(acceptor)));
    }
}
