#include "UserManager.h"
#include "sqlite_modern_cpp.h"
#include "utils/YamlcppWrapper.h"
#include "utils/Miscellaneous.h"

using namespace std;
using namespace ems;
using namespace ems::base;

bool UserManager::doAuth(const string& username, const string& password)
{
try
{
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    // 查询记录
    int recordCount{ 0 };
    db_users << "SELECT COUNT(*) FROM USER WHERE NAME = ? AND PASSWORD = ?;"
        << username << password >> recordCount;
    if(recordCount > 0) return true;
}
catch(const std::exception& e){
    return false;
}
    return false;
}

string UserManager::doAuth(const Json::Value& body)
{
    Json::Value auth;
    if(!body.isMember("auth"))
        throw std::runtime_error("request params err");
    auth = body["auth"];
    if(!auth.isMember("username") || !auth.isMember("password"))
        throw std::runtime_error("request params err");
    const string usernameAuth = auth["username"].asString();
    const string passwordAuth = auth["password"].asString();
    if(!base::UserManager::doAuth(usernameAuth, passwordAuth))
        throw std::runtime_error("auth failed");
    return usernameAuth;
}

string UserManager::getParam(const Json::Value& root, const string& key)
{
    if(!root.isMember(key))
        throw std::runtime_error("json key not exists");
    return root[key].asString();
}
