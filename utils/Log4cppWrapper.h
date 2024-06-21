#pragma once
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/Priority.hh>
#include <tuple>
#include <vector>
#include <memory>
#include <mutex>

namespace ems
{
using namespace std;
using namespace log4cpp;
using LogAttr = tuple<string,  // 日志名称
                      string>; // 级别

class Log4cppWrapper
{
public:
    Log4cppWrapper() = delete;
    static void init(vector<LogAttr> _list, const string& path);
    static Category& getLogger(const size_t index);
private:
    static Category& init(const LogAttr& attr);
    inline static std::mutex mtx_;
    inline static string path_;
    inline static vector<std::shared_ptr<Category>> loggers_;
};

}//namespace ems
