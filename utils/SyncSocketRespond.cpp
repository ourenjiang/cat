#include "SyncSocketRespond.h"

using namespace ems;
using namespace boost::asio;

SyncSocketRespond::SyncSocketRespond(const string port, function< vector<uint8_t> (const vector<uint8_t>&)> matchCallback)
    : acceptor_(io_context_, ip::tcp::endpoint(ip::tcp::v4(), std::stoi(port)))
    , work_(io_context_)
    , socket_(io_context_)
    , respondMatchCallback_(matchCallback)
    , ioContextThread_([this]{ io_context_.run(); })
{
    asyncAccept();
}

SyncSocketRespond::~SyncSocketRespond()
{
    if(ioContextThread_.joinable())
        ioContextThread_.join();
}

void SyncSocketRespond::handle_accept(const boost::system::error_code& ec, 
                    ip::tcp::acceptor& acceptor,
                    ip::tcp::socket& connected_socket,
                    io_context& io_context) {
    if (!ec) {
        cout << "New connection" << endl;
        // auto new_socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor.get_executor().context());

        // 开始接收消息
        asyncRead();
    }
}

void SyncSocketRespond::asyncAccept()
{
    acceptor_.async_accept(socket_,
                            std::bind(&SyncSocketRespond::handle_accept, this,
                                        std::placeholders::_1,
                                        std::ref(acceptor_),
                                        std::ref(socket_),
                                        std::ref(io_context_)));
}

void SyncSocketRespond::asyncRead()
{
    recvBuffer_.clear();
    recvBuffer_.resize(1024);
    socket_.async_read_some(boost::asio::buffer(recvBuffer_), 
                            std::bind(&SyncSocketRespond::handle_read,
                                        this, std::placeholders::_1, std::placeholders::_2));
}

bool SyncSocketRespond::asyncWrite(const std::string& msg)
{
    socket_.async_write_some(boost::asio::buffer(msg),
                            std::bind(&SyncSocketRespond::handle_write,
                                        this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

void SyncSocketRespond::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if(error){
        socket_.close();
        cout << "async read error: " << error.message() << endl;
        return;
    }
    recvBuffer_.resize(bytes_transferred);
    vector<uint8_t> requestFrame(recvBuffer_.begin(), recvBuffer_.end());

    // 处理外部消息
    vector<uint8_t> respondFrame{ 'h', 'e', 'l', 'l', 'o' };
    if(respondMatchCallback_){
        respondFrame = respondMatchCallback_(requestFrame);
    }

    // 响应
    const string respondString(respondFrame.begin(), respondFrame.end());
    asyncWrite(respondString);

    // 响应完成后，继续接收消息
    asyncRead();
}

void SyncSocketRespond::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if(error){
        socket_.close();
        cout << "async write error: " << error.message() << endl;
        return;
    }
    // std::cout << "write " << bytes_transferred << " bytes" << std::endl;

}
