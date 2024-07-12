#include "SyncRespond.h"

using namespace ems;
using namespace boost::asio;

SyncRespond::SyncRespond(const string port, function< vector<uint8_t> (const vector<uint8_t>&)> matchCallback)
    : acceptor_(io_context_, ip::tcp::endpoint(ip::tcp::v4(), std::stoi(port)))
    , work_(io_context_)
    // , socket_(io_context_)
    , respondMatchCallback_(matchCallback)
    , ioContextThread_([this]{ io_context_.run(); })
    , connectionNum_(0)
{
    asyncAccept();
}

SyncRespond::~SyncRespond()
{
    if(ioContextThread_.joinable())
        ioContextThread_.join();
}

void SyncRespond::asyncAccept()
{
    auto [newItem, isInserted] = sockets_.emplace(connectionNum_++,
        make_pair(ip::tcp::socket(io_context_), string()));
    // auto& [sock, rcvBuffer] = sockets_[connectionNum_];
    auto& [index, sockInfo] = *newItem;
    auto& [sock, rcvBuffer] = sockInfo;
    acceptor_.async_accept(sock,
                            std::bind(&SyncRespond::handle_accept, this,
                                        std::placeholders::_1,
                                        std::ref(acceptor_),
                                        index,
                                        std::ref(io_context_)));
}

void SyncRespond::handle_accept(const boost::system::error_code& ec, 
                    ip::tcp::acceptor& acceptor,
                    const int index,
                    io_context& io_context) {
    if (!ec) {
        cout << "New connection" << endl;
        // auto new_socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor.get_executor().context());

        // 开始接收消息
        asyncRead(index);

        // 继续接收新连接
        asyncAccept();
    }
}

void SyncRespond::asyncRead(const int index)
{
    auto& [sock, rcvBuffer] = sockets_.at(index);

    rcvBuffer.clear();
    rcvBuffer.resize(1024);
    sock.async_read_some(boost::asio::buffer(rcvBuffer), 
                            std::bind(&SyncRespond::handle_read,
                                        this, std::placeholders::_1, std::placeholders::_2,
                                        index));
}

bool SyncRespond::asyncWrite(const int index, const std::string& msg)
{
    auto& [sock, rcvBuffer] = sockets_.at(index);
    sock.async_write_some(boost::asio::buffer(msg),
                            std::bind(&SyncRespond::handle_write,
                                        this, std::placeholders::_1, std::placeholders::_2,
                                        index));
    return true;
}

void SyncRespond::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred,
                                const int index)
{
    auto& [sock, rcvBuffer] = sockets_.at(index);

    if(error){
        cout << "async read error: " << error.message() << endl;
        sock.close();
        sockets_.erase(index);
        return;
    }
    rcvBuffer.resize(bytes_transferred);
    vector<uint8_t> requestFrame(rcvBuffer.begin(), rcvBuffer.end());

    // 处理外部消息
    vector<uint8_t> respondFrame{ 'h', 'e', 'l', 'l', 'o' };
    if(respondMatchCallback_){
        respondFrame = respondMatchCallback_(requestFrame);
    }

    // 响应
    const string respondString(respondFrame.begin(), respondFrame.end());
    asyncWrite(index, respondString);

    // 响应完成后，继续接收消息
    asyncRead(index);
}

void SyncRespond::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred, const int index)
{
    auto& [sock, rcvBuffer] = sockets_.at(index);

    if(error){
        cout << "async write error: " << error.message() << endl;
        sock.close();
        sockets_.erase(index);
        return;
    }
    // std::cout << "write " << bytes_transferred << " bytes" << std::endl;

}
