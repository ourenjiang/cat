#pragma once
#include "utils/HttpWrapper.h"
#include <optional>
#include <tuple>
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace electricity_price
{
using namespace std;

class TypeList
{
public:
    using Record = tuple<string, string, string, string, string, string, string, string>;
    using NameList = vector<string>;
    
    TypeList();
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const;
    vector<byte> identity();
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);

    std::string createTable();
    bool insertIntoDefaultRecord();

    Record getRecord(const string& name) const;

    zmq::socket_t dealer_;
    const string identity_;
    const string deleteSubtitle_;
    const string putSubtitle_;
    const string postSubtitle_;
};

}//namespace electorcity_price
}//namespace ems
