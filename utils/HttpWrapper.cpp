#include "HttpWrapper.h"

httplib::Server& ems::utils::getHttpServerSingleton()
{
    static httplib::Server instance;
    return instance;
}

void ems::utils::httpRespond(Response &respond, const Json::Value data)
{
    // 3, 响应数据
    Json::StreamWriterBuilder builder;
    builder["emitUTF8"] = true;//输出编码格式
    builder["indentation"] = "  ";//两个空格缩进
    builder["precisionType"] = "decimal";//浮点数表示类型(decimal: 小数，significant: 科学计数法)
    builder["precision"] = 3;//浮点数保留两位小数点
    respond.set_content(Json::writeString(builder, data), "text/json");
}
