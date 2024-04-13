#include "Log4cppWrapper.h"
#include <algorithm>
#include <assert.h>
using namespace std;
using namespace log4cpp_wrapper;

Priority::PriorityLevel log4cpp_wrapper::convertPriorityLevelFromString(const string& level)
{
    if(level == "FATAL")  return Priority::FATAL;
    if(level == "ALERT")  return Priority::ALERT;
    if(level == "CRIT")   return Priority::CRIT;
    if(level == "ERROR")  return Priority::ERROR;
    if(level == "WARN")   return Priority::WARN;
    if(level == "NOTICE") return Priority::NOTICE;
    if(level == "INFO")   return Priority::INFO;
    if(level == "DEBUG")  return Priority::DEBUG;
    if(level == "NOTSET") return Priority::NOTSET;
    return Priority::NOTSET;
}

LoggerList Object::logs_;
string Object::path_;
Priority::PriorityLevel Object::level_;

void Object::init(const vector<string>& categoryNamelist, const string& path, const Priority::PriorityLevel level)
{
    path_ = path;
    level_ = level;

    std::transform(categoryNamelist.begin(), categoryNamelist.end(), back_inserter(logs_), [](const string& item){
        // 获取日志实例
        auto& category = Category::getInstance(item);
        // 添加输出源(通常，只需要一个滚动日志文件)
        auto appender = Object::createAppender(item);
        category.addAppender(appender);
        // 设置日志级别
        category.setPriority(level_);
        return &category;
    });
}

/**日志格式说明符列表：
 * %d --date     创建时间
 * %p --priority 级别
 * %t --thread   线程名称
 * %m --message  日志内容
 * %n            换行符
*/
Layout* Object::createLayout()
{
    auto layout = new PatternLayout();
    const string pattern("[%d] [%p] %m %n");
    layout->setConversionPattern(pattern);
    return layout;
}

Appender* Object::createAppender(const string& categoryName)
{
    const string appenderName("DefaultAppender");
    const string filename = path_ + categoryName + ".log";// 所在目录 + 文件名 + 文件后缀
    auto appender = new RollingFileAppender(appenderName, filename);
    const unsigned maxBackupIndex = 3;
    appender->setMaxBackupIndex(maxBackupIndex);
    appender->setLayout(createLayout());
    return appender;
}

Category& Object::getLogger(const int index)
{
    assert(index < logs_.size());
    return *logs_[index];
}
