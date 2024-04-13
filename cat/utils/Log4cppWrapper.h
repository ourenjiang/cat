#pragma once
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/Category.hh>

namespace log4cpp_wrapper
{
using namespace std;
using namespace log4cpp;
using LoggerList = vector<Category*>;

/**
 * 从字符串向具体的PriorityLevel类型的转换。
 * 这是为了方便基于yaml配置文件导入日志参数。
*/
Priority::PriorityLevel convertPriorityLevelFromString(const string& level);

class Object
{
public:
    Object() = delete;

    /**
     * categoryNamelist：日志名称列表
     * path：日志磁盘文件的统一存放路径
     * level：日志列表的统一级别
    */
    static void init(const vector<string>& categoryNamelist, const string& path, const Priority::PriorityLevel level);

    /**
     * 基于日志名称列表的下标获取日志对象。
     * 这个下标，即是init()接口中的categoryNamelist的元素下标。
    */
    static Category& getLogger(const int index);
private:

    /**
     * 创建自定义格式的Layout对象，返回堆对象指针。
     * 其生命期依赖于使用它的Appender对象。
    */
    static Layout* createLayout();

    /**
     * 创建滚动日志文件，
     * 由于实际只给Category对象绑定了一个唯一的Appender，
     * 因此直接将Category名称用作与其绑定的Appender的日志文件名称；
     * 至于Appender的名称，在内部是固定的，暂时配置的必要。
    */
    static Appender* createAppender(const string& categoryName);

    static LoggerList logs_;
    static string path_;
    static Priority::PriorityLevel level_;
};

}//namespace log4cpp_wrapper
