#include "Log4cppWrapper.h"
#include "boost/assert.hpp"

using namespace ems;

void Log4cppWrapper::init(vector<LogAttr> _list, const string& path)
{
    std::lock_guard<mutex> lock(mtx_);
    path_ = path;// 日志文件路径
    for(auto itr : _list)
    {
        loggers_.emplace_back(&init(itr));
    }
}

Category& Log4cppWrapper::init(const LogAttr& attr)
{
    auto logName = std::get<0>(attr);// 日志名称

    // 格式
    auto layout = new PatternLayout();
    layout->setConversionPattern("%d [%c] [%p] %m %n");
    // 输出目标(磁盘文件)
    const size_t MBytes = 1024 * 1024;// MB
    const size_t maxFileSize = 10 * MBytes;// 单个日志文件容量上限
    const unsigned maxBackupIndex = 3;// 日志文件数量上限
    const string fileName = path_ + '/' + logName + ".log";
    auto appender = new RollingFileAppender("DefaultAppender",
                                            fileName,
                                            maxFileSize,
                                            maxBackupIndex);
    appender->setLayout(layout);

    auto& logInstance = Category::getInstance(logName);
    logInstance.addAppender(appender);

    // 设置日志级别
    auto logPriority = std::get<1>(attr);
    Priority::Value logPriorityValue;
    if(logPriority == "ERROR") logPriorityValue = Priority::ERROR;
    else if(logPriority == "WARN") logPriorityValue = Priority::WARN;
    else if(logPriority == "INFO") logPriorityValue = Priority::INFO;
    else if(logPriority == "DEBUG") logPriorityValue = Priority::DEBUG;
    else logPriorityValue = Priority::DEBUG;
    logInstance.setPriority(logPriorityValue);
    return logInstance;
}

Category& Log4cppWrapper::getLogger(const size_t index)
{
    BOOST_ASSERT(index < loggers_.size());
    // return loggers_.at(id);// 下标不行，at()可以?
    return *(loggers_[index]);// 下标不行，at()可以?
}
