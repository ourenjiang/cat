#pragma once
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/Priority.hh>
#include <tuple>
#include <initializer_list>

namespace log4cpp_wrapper
{
using namespace std;
using namespace log4cpp;
using LogAttr = tuple<string,  // 日志名称
                      string>; // 级别

enum class LogId
{
    Logger_1,
    Logger_2
};

using LoggerMap = map<LogId, Category&>;

class Object
{
public:
    static void init(initializer_list<LogAttr> _list, const string& path);
    static Category& getLogger(LogId);
private:
    static Category& init(initializer_list<LogAttr>::const_iterator itr);
    static LoggerMap loggerMap_;
    static string path_;
};

}//namespace log4cpp_wrapper
