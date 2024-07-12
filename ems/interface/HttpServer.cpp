#include "HttpServer.h"

using namespace ems;
using namespace httplib;

HttpServer::HttpServer(const int port)
    : port_(port)
    , server_(utils::getHttpServerSingleton())
{
}

void HttpServer::start()
{
    loopThread_ = std::thread([this]{

        /**
         * Fix: 如果当前端口被占用，执行listen()并不会直接返回，
         * 而是两个http接口随机地被访问，这在客户端这一侧是无感知的，
         * 但接收到的数据却是随机地来自于两个不同的服务。
         * TODO: 使用外部工具，检查当前端口是否被占用
        */
        server_.listen("0.0.0.0", port_); // 重复绑定并没有失败？存在调试漏洞。
    });
}

void HttpServer::setGetCallback(const string& url, const Server::Handler& callback)
{
    server_.Get(url, Server::Handler(callback));
}

void HttpServer::setPostCallback(const string& url, const Server::Handler& callback)
{
    server_.Post(url, Server::Handler(callback));
}

void HttpServer::setPutCallback(const string& url, const Server::Handler& callback)
{
    server_.Put(url, Server::Handler(callback));
}

void HttpServer::setDeleteCallback(const string& url, const Server::Handler& callback)
{
    server_.Delete(url, Server::Handler(callback));
}
