#include <boost/asio.hpp>
#include <iostream>
#include <memory>

using namespace std;

// class session : public std::enable_shared_from_this<session> {
// public:
//     session(boost::asio::io_context& io_context)
//         : socket_(io_context) {}

//     std::shared_ptr<session> shared_from_this() {
//         return std::static_pointer_cast<session>(std::enable_shared_from_this<session>::shared_from_this());
//     }

//     void start() {
//         do_read();
//     }

// private:
//     boost::asio::ip::tcp::socket socket_;

//     void do_read() {
//         auto self(shared_from_this());
//         socket_.async_read_some(boost::asio::buffer(data_, max_length),
//             [this, self](boost::system::error_code ec, std::size_t length) {
//                 if (!ec) {
//                     do_write(length);
//                 }
//             });
//     }

//     void do_write(std::size_t length) {
//         auto self(shared_from_this());
//         boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
//             [this, self](boost::system::error_code ec, std::size_t /* length */) {
//                 if (!ec) {
//                     do_read();
//                 }
//             });
//     }
// };

// class server {
// public:
//     server(boost::asio::io_context& io_context, short port)
//         : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {}

//     void start() {
//         do_accept();
//     }

// private:
//     boost::asio::ip::tcp::acceptor acceptor_;
//     enum { max_length = 1024 };
//     char data_[max_length];

//     void do_accept() {
//         auto new_session = std::make_shared<session>(acceptor_.get_executor().context());
//         acceptor_.async_accept(new_session->socket_,
//             [this, new_session](boost::system::error_code ec) {
//                 if (!ec) {
//                     new_session->start();
//                 }
//                 do_accept();
//             });
//     }
// };

int main(int argc, char **argv) {
    boost::asio::io_context io_context;

    short port = std::atoi(argv[1]);
    boost::asio::ip::tcp::acceptor acceptor(io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

    auto handle_accept = [&acceptor, &io_context](boost::system::error_code ec) {
                                if (!ec) {
                                    cout << "new connection" << endl;
                                    
                                    boost::asio::ip::tcp::socket socket_(io_context);
                                    acceptor.async_accept(socket_, handle_accept);
                                }
                            };

    boost::asio::ip::tcp::socket socket_(io_context);
    acceptor.async_accept(socket_, [](boost::system::error_code ec) {
                                        if (!ec) {
                                            cout << "new connection" << endl;
                                        }
                                    });

    io_context.run();
    return 0;
}