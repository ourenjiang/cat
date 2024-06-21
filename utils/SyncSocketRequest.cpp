#include "SyncSocketRequest.h"

using namespace ems;
using namespace boost::asio;

SyncSocketRequest::SyncSocketRequest(const string ip, const string port)
    : connected_(false)
    , work_(io_context_)
    , socket_(io_context_)
    , dataReady_(false)
    , ip_(ip)
    , port_(port)
    , ioContextThread_([this]{ io_context_.run(); })
{
    asyncConnect();

    // socket_.set_option(ip::tcp::no_delay(true));
    // socket_.set_option(socket_base::keep_alive(true));
}

SyncSocketRequest::~SyncSocketRequest()
{
    if(ioContextThread_.joinable())
        ioContextThread_.join();
}

void SyncSocketRequest::asyncConnect()
{
    {
        int fd = socket_.native_handle();
        tcp_info info;
        int len = sizeof(tcp_info);
        ::getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len);
        if(info.tcpi_state == TCP_ESTABLISHED){
            cout << "connection is well" << endl;
            connected_ = true;
            cout << "port: " << socket_.local_endpoint().port() << endl;
            return;
        }
        else{
            cout << "connection has disconnected" << endl;
            connected_ = false;
        }
    }

    if(connected_){
        // cout << "have connected, do not connect again" << endl;
        return;
    }

    tcp::resolver resolver(io_context_);
    auto endpoint = resolver.resolve(ip_, port_);
    boost::asio::async_connect(socket_, endpoint,
        [&](const boost::system::error_code& ec, const tcp::endpoint& endpoint){
        if(ec){
            cout << "connect errmsg: " << ec.message() << endl;
            return;
        }
        connected_ = true;
    });
}

bool SyncSocketRequest::syncReadConditionVariable()
{
    if(!connected_){
        asyncConnect();
        return false;
    }

    recvBuffer_.clear();
    recvBuffer_.resize(1024);
    socket_.async_read_some(boost::asio::buffer(recvBuffer_), 
                            std::bind(&SyncSocketRequest::handle_read,
                                        this, std::placeholders::_1, std::placeholders::_2));

    std::unique_lock<mutex> lock(mtx_);
    auto result = cv_.wait_for(lock, std::chrono::seconds(1), [this]{ return dataReady_; });
    if(result){

        // cout << "Recv: " << recvBuffer_ << endl;
        // for(auto& item: recvBuffer_){
        //     auto ubyte = static_cast<uint8_t>(item);
        //     printf("%02x ", ubyte);
        // }
        // cout << endl;
    }else{
        cout << "async read timeout" << endl;
        return false;
    }

    dataReady_ = false;//reset
    return true;
}

bool SyncSocketRequest::asyncWrite(const std::string& msg)
{
    if(!connected_){
        asyncConnect();
        return false;
    }
    socket_.async_write_some(boost::asio::buffer(msg),
                            std::bind(&SyncSocketRequest::handle_write,
                                        this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

void SyncSocketRequest::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if(error){
        socket_.close();
        asyncConnect();
        cout << "async read error: " << error.message() << endl;
        return;
    }

    // std::cout << "recv " << bytes_transferred << " bytes" << std::endl;
    recvBuffer_.resize(bytes_transferred);

    // 设置条件变量为'读取成功'
    std::unique_lock<mutex> lock(mtx_);
    dataReady_ = true;
    cv_.notify_one();
}

void SyncSocketRequest::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if(error){
        socket_.close();
        asyncConnect();
        cout << "async write error: " << error.message() << endl;
        return;
    }
    // std::cout << "write " << bytes_transferred << " bytes" << std::endl;
}
