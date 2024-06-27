#include "UserManager.h"
#include "utils/YamlcppWrapper.h"
#include "sqlite_modern_cpp.h"
#include "utils/Miscellaneous.h"
#include "ems/station/OperationRecord.h"
#include "utils/AuthException.h"

using namespace ems;

UserManager::UserManager()
    : log_(ems::Log4cppWrapper::getLogger(5))
{
    insertDefault();
    registerCallbacks();
}

bool UserManager::insertDefault()
{
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    // 创建表
    // 用户名 - 密码 - 用户等级 - 电话 - 邮箱
    db_users << "CREATE TABLE IF NOT EXISTS USER("
        "NAME TEXT PRIMARY KEY,"
        "PASSWORD TEXT,"
        "GRADE TEXT,"
        "PHONE TEXT,"
        "EMAIL TEXT);";

    //清空表记录
    db_users << "DELETE FROM USER;";

    // 插入默认记录
    // 0 - 超级管理员   1 - 管理员   2 - 普通用户
    db_users << "INSERT INTO USER (NAME, PASSWORD, GRADE, PHONE, EMAIL) VALUES (?, ?, ?, ?, ?);"
        << "root"  << "123" << "0" << "18513627947" << "ourenjiang@163.com";
    db_users << "INSERT INTO USER (NAME, PASSWORD, GRADE, PHONE, EMAIL) VALUES (?, ?, ?, ?, ?);"
        << "admin" << "456" << "1" << "15070165514" << "rejia@163.com";
    db_users << "INSERT INTO USER (NAME, PASSWORD, GRADE, PHONE, EMAIL) VALUES (?, ?, ?, ?, ?);"
        << "test"  << "789" << "2" << "15070165514" << "1527474379@163.com";
    return true;
}

void UserManager::registerCallbacks()
{
    using namespace std::placeholders;
    using httplib::Server;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/user/namelist", bind(&UserManager::getNameListCallback, this, _1, _2));
    serv.Post("/user/info/query", Server::Handler(bind(&UserManager::getCallback, this, _1, _2)));
    serv.Post("/user/info/queryAll", Server::Handler(bind(&UserManager::getAllCallback, this, _1, _2)));
    serv.Post("/user/info/add", Server::Handler(bind(&UserManager::postCallback, this, _1, _2)));
    serv.Post("/user/info/modify", Server::Handler(bind(&UserManager::putCallback, this, _1, _2)));
    serv.Post("/user/info/delete", Server::Handler(bind(&UserManager::deleteCallback, this, _1, _2)));
}

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

void UserManager::getCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);

    //* 解析鉴权信息 */
    if(!reqbody.isMember("auth"))
        throw std::runtime_error("request params err");
    const auto& auth = reqbody["auth"];
    if(!auth.isMember("username") || !auth.isMember("password"))
        throw std::runtime_error("request params err");
    const string usernameAuth = auth["username"].asString();
    const string passwordAuth = auth["password"].asString();
    if(!doAuth(usernameAuth, passwordAuth))
        throw std::runtime_error("auth failed");
    const string username = reqbody["username"].asString();

    // 打开数据库
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    // 查询记录
    optional<tuple<string, string, string>> record;
    db_users << "SELECT GRADE, PHONE, EMAIL FROM USER WHERE NAME = ?;"
        << username >> [&](string grade, string phone, string email){
        record.emplace(grade, phone, email);
    };
    if(!record.has_value())
        throw AuthException("username or password err", username);
    const auto& [grade, phone, email] = record.value();

    //记录操作日志
    OperationRecord::insertRecord("成功", "查询用户信息", "用户管理", usernameAuth);

    // 成功响应
    Json::Value respondContent;
    auto& data = respondContent["data"];
    data["grade"] = grade;
    data["phone"] = phone;
    data["email"] = email;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){

    //记录操作日志
    auto authException = dynamic_cast<const AuthException*>(&e);
    if(authException){
        OperationRecord::insertRecord("失败", authException->what(), "用户管理", authException->username());
    }

    // 响应错误信息
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void UserManager::getNameListCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    // 打开数据库
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    Json::Value namelist(Json::arrayValue);
    db_users << "SELECT NAME FROM USER;"
                >> [&](string name){
                        namelist.append(name);
                    };
    
    Json::Value msg;
    msg["data"] = namelist;
    msg["errcode"] = 0;
    msg["errmsg"] = "success";
    utils::httpRespond(res, msg);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void UserManager::getAllCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);

    /* 解析鉴权信息 */
    if(!reqbody.isMember("auth"))
        throw std::runtime_error("request params err");
    const auto& auth = reqbody["auth"];
    if(!auth.isMember("username") || !auth.isMember("password"))
        throw std::runtime_error("request params err");
    const string username = auth["username"].asString();
    const string password = auth["password"].asString();
    if(!doAuth(username, password))
        throw std::runtime_error("auth failed");

    // 打开数据库
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    // 查询用户级别
    optional<string> grade;
    db_users << "SELECT GRADE FROM USER WHERE NAME = ? AND PASSWORD = ?;"
        << username << password >> [&](string columnGrade){
        grade = columnGrade;
    };
    BOOST_ASSERT(grade.has_value());

    using Record = tuple<string, string, string, string, string>;
    vector<Record> records;
    db_users << "SELECT NAME, PASSWORD, GRADE, PHONE, EMAIL FROM USER WHERE GRADE >= ?;"
        << grade 
        >> [&](string name, string password, string grade, string phone, string email){
       records.emplace_back(name, password, grade, phone, email);
    };

    Json::Value respondContent;
    auto& data = respondContent["data"];
    for(auto& ele: records){
        Json::Value info;
        info["username"] = get<0>(ele);
        info["password"] = get<1>(ele);
        info["grade"] = get<2>(ele);
        info["phone"] = get<3>(ele);
        info["email"] = get<4>(ele);
        data.append(info);
    }
    respondContent["message"] = "success";
    respondContent["errcode"] = 0;
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void UserManager::postCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);

    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    /* 解析鉴权信息 */
    if(!reqbody.isMember("auth"))
        throw std::runtime_error("request params err");
    const auto& auth = reqbody["auth"];
    if(!auth.isMember("username") || !auth.isMember("password"))
        throw std::runtime_error("request params err");
    const string usernameAuth = auth["username"].asString();
    const string passwordAuth = auth["password"].asString();
    
    /* 鉴权 */
    optional<int> authGrade;
    db_users << "SELECT GRADE FROM USER WHERE NAME = ? AND PASSWORD = ?;"
        << usernameAuth << passwordAuth >> [&](int columnGrade){
        authGrade = columnGrade;
    };
    if(!authGrade.has_value())
        throw std::runtime_error("auth failed");
    if(authGrade.value() > 0)
        throw AuthException("auth grade failed", usernameAuth);

    // username, password, grade
    if(!reqbody.isMember("username")
       || !reqbody.isMember("password") || !reqbody.isMember("grade"))
        throw AuthException("request params err", usernameAuth);
    const string username = reqbody["username"].asString();
    const string password = reqbody["password"].asString();
    const string grade = reqbody["grade"].asString();
 
    // phone, email
    string phone;
    if(reqbody.isMember("phone")) phone = reqbody["phone"].asString();
    string email;
    if(reqbody.isMember("email")) email = reqbody["email"].asString();

    // 查询记录
    int recordCount{ 0 };
    db_users << "SELECT COUNT(*) FROM USER WHERE NAME = ?;"
        << username >> recordCount;
    if(recordCount > 0)
        throw AuthException("用户已存在", usernameAuth);

    // exec sql
    db_users << "INSERT INTO USER (NAME, PASSWORD, GRADE, PHONE, EMAIL) VALUES (?, ?, ?, ?, ?);"
                << username << password << grade << phone << email;
    
    //记录正常操作日志
    OperationRecord::insertRecord("成功", "添加用户", "用户管理", usernameAuth);

    Json::Value respondContent;
    respondContent["message"] = "success";
    respondContent["errcode"] = 0;
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){

    //记录错误操作日志
    auto authException = dynamic_cast<const AuthException*>(&e);
    if(authException){
        OperationRecord::insertRecord("失败", authException->what(), "用户管理", authException->username());
    }
    
    // 响应错误信息
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void UserManager::putCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);

    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    /* 解析鉴权信息 */
    if(!reqbody.isMember("auth"))
        throw std::runtime_error("request params err");
    const auto& auth = reqbody["auth"];
    if(!auth.isMember("username") || !auth.isMember("password"))
        throw std::runtime_error("request params err");
    const string usernameAuth = auth["username"].asString();
    const string passwordAuth = auth["password"].asString();
    
    /* 鉴权 */
    optional<string> gradeAuth;
    db_users << "SELECT GRADE FROM USER WHERE NAME = ? AND PASSWORD = ?;"
        << usernameAuth << passwordAuth >> [&](string columnGrade){
        gradeAuth = columnGrade;
    };
    if(!gradeAuth.has_value())
        throw std::runtime_error("auth failed");

    // 必须根据用户名，定位需要修改的记录
    if(!reqbody.isMember("username"))
        throw std::runtime_error("request params err");
    const string username = reqbody["username"].asString();
    // 原记录参数
    optional<tuple<string, string, string, string>> recordOld;
    db_users << "SELECT PASSWORD, GRADE, PHONE, EMAIL FROM USER WHERE NAME = ?;" << username
    >> [&](string password, string grade, string phone, string email){
        recordOld.emplace(password, grade, phone, email);
    };
    if(!recordOld.has_value())
        throw AuthException("用户不存在", usernameAuth);
    auto& [passwordOld, gradeOld, phoneOld, emailOld] = recordOld.value();
    // 针对等级进行鉴权
    if(gradeOld < gradeAuth.value())
        throw AuthException("权限不足", usernameAuth);

    string passwordNew = passwordOld;
    string gradeNew = gradeOld;
    string phoneNew = phoneOld;
    string emailNew = emailOld;
    if(reqbody.isMember("passwordNew")) passwordNew = reqbody["passwordNew"].asString();
    if(reqbody.isMember("gradeNew")) gradeNew = reqbody["gradeNew"].asString();
    if(reqbody.isMember("phoneNew")) phoneNew = reqbody["phoneNew"].asString();
    if(reqbody.isMember("emailNew")) emailNew = reqbody["emailNew"].asString();

    // 修改
    db_users << "UPDATE USER SET PASSWORD=?, GRADE=?, PHONE=?, EMAIL=? WHERE NAME = ?;"
                << passwordNew << gradeNew << phoneNew << emailNew << username;

    //记录操作日志
    OperationRecord::insertRecord("成功", "修改用户信息", "用户管理", usernameAuth);

    Json::Value respondContent;
    respondContent["message"] = "success";
    respondContent["errcode"] = 0;
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){

    //记录操作日志
    auto authException = dynamic_cast<const AuthException*>(&e);
    if(authException){
        OperationRecord::insertRecord("失败", authException->what(), "用户管理", authException->username());
    }

    // 响应错误信息
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void UserManager::deleteCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);

    auto& cfgRoot = YamlcppWrapper::getRoot();
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string dbPath = cfgRoot["global"]["dbPath"].as<string>();
    const string dbname = rootPath + dbPath + "/Users.sqlite";
    sqlite::database db_users(dbname);

    /* 解析鉴权信息 */
    if(!reqbody.isMember("auth"))
        throw std::runtime_error("request params err");
    const auto& auth = reqbody["auth"];
    if(!auth.isMember("username") || !auth.isMember("password"))
        throw std::runtime_error("request params err");
    const string usernameAuth = auth["username"].asString();
    const string passwordAuth = auth["password"].asString();

    /* 鉴权 */
    optional<string> gradeAuth;
    db_users << "SELECT GRADE FROM USER WHERE NAME = ? AND PASSWORD = ?;"
        << usernameAuth << passwordAuth >> [&](string columnGrade){
        gradeAuth = columnGrade;
    };
    if(!gradeAuth.has_value())
        throw std::runtime_error("auth failed");
    
    // 必须根据用户名，定位需要删除的记录
    if(!reqbody.isMember("username"))
        throw std::runtime_error("request params err");
    const string usernameOld = reqbody["username"].asString();

    // 针对等级进行鉴权
    optional<string> gradeOld;
    db_users << "SELECT GRADE FROM USER WHERE NAME = ?;" << usernameOld
    >> [&](string grade){
        gradeOld = grade;
    };
    if(!gradeOld.has_value())
        throw std::runtime_error("user not exists");
    if(gradeOld < gradeAuth.value())
        throw std::runtime_error("auth failed");

    // exec sql
    db_users << "DELETE FROM USER WHERE NAME = ?;" << usernameOld;

    //记录操作日志
    OperationRecord::insertRecord("成功", "删除用户 " + usernameOld, "用户管理", usernameAuth);

    Json::Value respondContent;
    respondContent["message"] = "success";
    respondContent["errcode"] = 0;
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}
