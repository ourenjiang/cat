#include "SyncRequest.h"
#include "Miscellaneous.h"

using namespace ems;
using namespace boost::asio;

SyncRequest::SyncRequest()
    : ioContext_(make_shared<io_context>())
    , ioWork_(make_shared<io_context::work>(*ioContext_))
    , ioSocket_(make_shared<ip::tcp::socket>(*ioContext_))
    , dataReady_(false)
    , ioStarted_(false)
    , connected_(false)
{
}

SyncRequest::SyncRequest(const string& address)
    : ioContext_(make_shared<io_context>())
    , ioWork_(make_shared<io_context::work>(*ioContext_))
    , ioSocket_(make_shared<ip::tcp::socket>(*ioContext_))
    , dataReady_(false)
    , address_(address)
    , ioStarted_(false)
    , connected_(false)
{
    // socket_.set_option(ip::tcp::no_delay(true));
    // socket_.set_option(socket_base::keep_alive(true));
}

void SyncRequest::start()
{
    if(ioStarted_) return;
    ioContextThread_ = make_shared<std::thread>(
        [this]{
            ioContext_->run();
        }
    );
    ioStarted_ = true;
}

SyncRequest::~SyncRequest()
{
    if(ioContextThread_->joinable())
        ioContextThread_->join();
}

void SyncRequest::tryAsyncConnect()
{
    // {
    //     int fd = ioSocket_->native_handle();
    //     tcp_info info;
    //     int len = sizeof(tcp_info);
    //     ::getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len);
    //     if(info.tcpi_state == TCP_ESTABLISHED){
    //         cout << "connection is well" << endl;
    //         connected_ = true;
    //         cout << "port: " << ioSocket_->local_endpoint().port() << endl;
    //         return;
    //     }
    //     else{
    //         cout << "connection has disconnected" << endl;
    //         connected_ = false;
    //     }
    // }


    if(connected_){
        // cout << "have connected, do not connect again" << endl;
        return;
    }
    if(!ioStarted_){
        cerr << "please start io before connect" << endl;
        quick_exit(0);
    }

    ip::tcp::resolver resolver(*ioContext_);
    auto [ip, port] = miscellaneous::parseTcpAddress(address_).value();
    auto endpoint = resolver.resolve(ip, port);
    boost::asio::async_connect(*ioSocket_, endpoint,
        [&](const boost::system::error_code& ec, const ip::tcp::endpoint& endpoint){

            if(ec){
                cout << "connect errmsg: " << ec.message() << endl;
                return;
            }

            std::unique_lock<mutex> lock(mtx_);
            dataReady_ = true;
            cv_.notify_one();
        }
    );

    std::unique_lock<mutex> lock(mtx_);
    auto result = cv_.wait_for(lock, std::chrono::seconds(1), [this]{ return dataReady_; });
    if(result){
        dataReady_ = false;//reset
        connected_ = true;
    }
}

optional<vector<byte>> SyncRequest::syncRead(const int64_t timeout)
{
    tryAsyncConnect();
    if(!connected_) return {};

    recvBuffer_.clear();
    recvBuffer_.resize(1024);
    ioSocket_->async_read_some(boost::asio::buffer(recvBuffer_), 
        [this](const boost::system::error_code& error, std::size_t bytes_transferred){

            if(error){
                ioSocket_->close();
                connected_ = false;
                cout << "async read error: " << error.message() << endl;
                return;
            }

            // std::cout << "recv " << bytes_transferred << " bytes" << std::endl;
            // recvBuffer_.resize(bytes_transferred);
            recvBytes_ = bytes_transferred;

            // 设置条件变量为'读取成功'
            std::unique_lock<mutex> lock(mtx_);
            dataReady_ = true;
            cv_.notify_one();
        }
    );

    std::unique_lock<mutex> lock(mtx_);
    auto result = cv_.wait_for(lock,
                                std::chrono::milliseconds(timeout), 
                                [this]{ return dataReady_; });
    if(result){
        dataReady_ = false;//reset

        // cout << "Recv: " << recvBuffer_ << endl;
        // for(auto& item: recvBuffer_){
        //     auto ubyte = static_cast<uint8_t>(item);
        //     printf("%02x ", ubyte);
        // }
        // cout << endl;
    }else{
        // cout << "async read timeout" << endl;
        return {};
    }

    return vector<byte>(reinterpret_cast<byte*>(recvBuffer_.data()),
        reinterpret_cast<byte*>(recvBuffer_.data()) + recvBytes_);
}

void SyncRequest::syncWrite(const vector<byte>& data, const int64_t timeout)
{
    tryAsyncConnect();
    if(!connected_) return;

    ioSocket_->async_write_some(boost::asio::buffer(data),
        [this, &data](const boost::system::error_code& error, std::size_t bytes_transferred){

            if(error){
                std::cerr << error.message() << std::endl;
                ioSocket_->close();
                connected_ = false;
                return;
            }
            assert(bytes_transferred == data.size());

            std::unique_lock<mutex> lock(mtx_);
            dataReady_ = true;
            cv_.notify_one();
        }
    );
    // cout << "try write over" << endl;

    std::unique_lock<mutex> lock(mtx_);
    auto result = cv_.wait_for(lock,
                                std::chrono::milliseconds(timeout), 
                                [this]{ return dataReady_; });
    if(result){
        dataReady_ = false;// reset
    }
}
