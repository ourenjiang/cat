#pragma once
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class Setting
{
public:
    Setting();
    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const;
    static optional<string> getRecord(const string& branchIndex);
    vector<byte> identity();
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);

    string createTable();
    void insertIntoDefaultRecord();

    zmq::socket_t dealer_;
    const string identity_;
};

}//namespace xftg
}//namespace ems
