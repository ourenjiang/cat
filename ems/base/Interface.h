#include <vector>

namespace interface
{
using namespace std;

/*
// 1, 构造消息结构 pair
// 2, 序列化为msgpack字节流
// 3, 转换为标准的vector<byte>字节流
*/
vector<byte> respondMessage(const bool status, const vector<byte>& content);

}//namespace interface