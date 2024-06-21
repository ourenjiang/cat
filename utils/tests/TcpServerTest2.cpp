#include <iostream>
#include "boost/asio.hpp"
#include <functional>
#include <vector>
#include <memory>

using namespace std;
using namespace boost::asio;

// vector<shared_ptr<ip::tcp::socket>> g_sockets;
vector<pair<ip::tcp::socket, string>> g_sockets;

void handle_accept(const boost::system::error_code& ec,
                    ip::tcp::acceptor& acceptor,
                    // shared_ptr<ip::tcp::socket> connected_socket,
                    ip::tcp::socket& connected_socket,
                    io_context& io_context);

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
    // auto& new_socket = g_sockets.emplace_back(make_shared<ip::tcp::socket>(io_context));
    auto& new_socket = g_sockets.emplace_back(make_pair(ip::tcp::socket(io_context), string()));

    // acceptor.async_accept(*new_socket.get(),
    //                         std::bind(handle_accept, 
    //                                     std::placeholders::_1,
    //                                     std::ref(acceptor),
    //                                     new_socket,
    //                                     std::ref(io_context)));
    // acceptor.async_accept(new_socket,
    //                         std::bind(handle_accept, 
    //                                     std::placeholders::_1,
    //                                     std::ref(acceptor),
    //                                     std::ref(new_socket),
    //                                     std::ref(io_context)));

    // 运行事件循环
    io_context.run();
    return 0;
}

void handle_accept(const boost::system::error_code& ec, 
                    ip::tcp::acceptor& acceptor,
                    // shared_ptr<ip::tcp::socket> connected_socket,
                    ip::tcp::socket& connected_socket,
                    io_context& io_context) {
    if (!ec) {
        cout << "New connection" << endl;
        // auto new_socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor.get_executor().context());

        {
            // 创建一个新的socket来接受下一个连接
            // auto& new_socket = g_sockets.emplace_back(make_shared<ip::tcp::socket>(io_context));
            auto& new_socket = g_sockets.emplace_back(io_context);
            // 继续等待下一个连接
            // acceptor.async_accept(*new_socket.get(),
            //                         std::bind(handle_accept,
            //                                     std::placeholders::_1,
            //                                     std::ref(acceptor),
            //                                     new_socket,
            //                                     std::ref(io_context)));
            acceptor.async_accept(new_socket,
                                    std::bind(handle_accept,
                                                std::placeholders::_1,
                                                std::ref(acceptor),
                                                std::ref(new_socket),
                                                std::ref(io_context)));
        }
    }
}
