#include "HttpServer.h"
#include "utils/YamlcppWrapper.h"
#include "utils/HttpWrapper.h"

using namespace ems;

void HttpServer::start()
{
    loopThread_ = std::thread([this]{
        auto& cfgRoot = YamlcppWrapper::getRoot();
        const auto& cfgInterface = cfgRoot["dataInterface"];
        const int servPort = cfgInterface["httpPort"].as<int>();

        auto& serv = utils::getHttpServerSingleton();
        /**
         * Fix: 如果当前端口被占用，执行listen()并不会直接返回，
         * 而是两个http接口随机地被访问，这在客户端这一侧是无感知的，
         * 但接收到的数据却是随机地来自于两个不同的服务。
         * TODO: 使用外部工具，检查当前端口是否被占用
        */
        serv.listen("0.0.0.0", servPort); // 重复绑定并没有失败？存在调试漏洞。
    });
}
