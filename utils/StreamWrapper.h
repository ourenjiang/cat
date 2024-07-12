#include <string>
#include <vector>

namespace stream_wrapper
{
using namespace std;

// 校验系统时间字符串
bool matchTimeHHMM(const string& timestr);

// 字符串转换字节流
vector<byte> toBytes(const string& data);
vector<byte> toBytes(const void* data, size_t len);

// 创建字符串
string toString(const void* data, size_t len);

// 按指定精度序列化浮点数
string serializeFloat(const double rawData, int precision);
// 构造定长字符串
string fixedSize(const string& content, const size_t len = 64);

}//namespace stream_wrapper
