#include "Log4cppWrapper.h"
using namespace log4cpp_wrapper;

LoggerMap Object::loggerMap_;
string Object::path_;

void Object::init(initializer_list<LogAttr> _list, const string& path)
{
    path_ = path;// 日志文件路径

    for(auto itr = _list.begin(); itr != _list.end(); ++itr)
    {
        auto index = std::distance(_list.begin(), itr);
        loggerMap_.emplace(static_cast<LogId>(index), init(itr));
    }
}

Category& Object::init(initializer_list<LogAttr>::const_iterator itr)
{
    auto logName = std::get<0>(*itr);// 日志名称

    // 格式
    auto layout = new PatternLayout();
    layout->setConversionPattern("%d [%c] [%p] %m %n");
    // 输出目标(磁盘文件)
    const size_t MBytes = 1024 * 1024;// MB
    const size_t maxFileSize = 10 * MBytes;// 单个日志文件容量上限
    const unsigned maxBackupIndex = 3;// 日志文件数量上限
    auto appender = new RollingFileAppender("DefaultAppender",
                                            path_ + logName,
                                            maxFileSize,
                                            maxBackupIndex);
    appender->setLayout(layout);

    auto& logInstance = Category::getInstance(logName);
    logInstance.addAppender(appender);

    // 设置日志级别
    auto logPriority = std::get<1>(*itr);
    Priority::Value logPriorityValue;
    if(logPriority == "ERROR") logPriorityValue = Priority::ERROR;
    else if(logPriority == "WARN") logPriorityValue = Priority::WARN;
    else if(logPriority == "INFO") logPriorityValue = Priority::INFO;
    else if(logPriority == "DEBUG") logPriorityValue = Priority::DEBUG;
    else logPriorityValue = Priority::DEBUG;
    logInstance.setPriority(logPriorityValue);
    return logInstance;
}

Category& Object::getLogger(LogId id)
{
    return loggerMap_.at(id);// 下标不行，at()可以?
}
