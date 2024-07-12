#include "jsonWrapper.h"

using namespace std;

Json::Value json_wrapper::deserialize(const string& str)
{
    if(str.empty())
        throw std::runtime_error("request body empty");
    
    JSONCPP_STRING err;
    Json::Value root;
    Json::CharReaderBuilder builder;
    const unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(str.data(), str.data() + str.length(), &root, &err)){
        throw std::runtime_error("json string parse failed");
    }
    return root;
}

string json_wrapper::serialize(const Json::Value& root)
{
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}
