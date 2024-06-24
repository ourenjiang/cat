#include "Miscellaneous.h"
#include <regex>
#include <unistd.h>
#include <iomanip>
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems::miscellaneous;

// 解析IP&Port字符串
bool ems::miscellaneous::parseIpPortString(const std::string& ipPort, std::string& ip, int& port)
{
    regex pattern(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+))");
    smatch match;

    if(regex_search(ipPort, match, pattern))
    {
        ip = match[1].str();
        port = stoi(match[2].str());
        return true;
    }
    return false;
}

// 解析IP&Port字符串
bool ems::miscellaneous::parseIpPortString(const std::string& ipPort, std::string& ip, std::string& port)
{
    regex pattern(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+))");
    smatch match;

    if(regex_search(ipPort, match, pattern))
    {
        ip = match[1].str();
        port = match[2].str();
        return true;
    }
    return false;
}


uint16_t ems::miscellaneous::getCrc16Modbus(void *data, uint16_t number)
{
    auto ptr = static_cast<uint8_t *>(data);

    uint16_t m_Crc = 0;
    uint16_t m_inutCrc = 0xFFFF;
    for (uint16_t i = 0; i < number; i++)
    {
        m_inutCrc ^= *ptr;
        for (uint16_t j = 0; j < 8; j++)
        {
            m_Crc = m_inutCrc;
            m_inutCrc >>= 1;
            if ((m_Crc & 0x1) != 0)
            {
                m_inutCrc ^= 0xa001;
            }
        }
        ptr++;
    }
    return m_inutCrc;
}

/*
** brief@@ G(X) = x^16+x^15+x^2+1,
** IBM crc16 std, in/out Reflection.
*/
// uint16_t crc16_ibm(u16 crc_init, const u8 *src, u32 size)
uint16_t ems::miscellaneous::crc16_ibm(const uint8_t *src, uint32_t size)
{
    uint16_t crc_table[] =
    {
        /* polynomial0x8005, CRC 4bit Reflection remainder table
        */
        0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
        0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
    };

    uint16_t crc{ 0xffff };

    for (uint32_t i = 0; i < size; ++i)
    {
        crc = ((crc) >> 4) ^ crc_table[((crc & 0x0f) 
            ^ ((*src) &  0x0f)) & 0x0f];
            
        crc = ((crc) >> 4) ^ crc_table[((crc & 0x0f) 
            ^ ((*src) >> 0x04)) & 0x0f];
        src++;
    }

    return crc;
}

//
// 字节大小端反转
//
void ems::miscellaneous::reverseEndian(void *data, int len)
{
    auto addr = static_cast<uint8_t *>(data);
    for (int i = 0; i < len / 2; ++i)
    {
        static uint8_t tmp;
        tmp = addr[i];
        addr[i] = addr[len - i - 1];
        addr[len - i - 1] = tmp;
    }
}

void ems::miscellaneous::reverseByteArray(void *arr, const unsigned long len)
{
    auto byteArray = static_cast<char *>(arr);
    std::reverse(byteArray, byteArray + len);
}

// 获取POST请求参数
int ems::miscellaneous::getCgiPostPara(char *data)
{
    if (data == NULL)
        return 1;

    char *method = getenv("REQUEST_METHOD");
    if (method == NULL)
        return 2;
    if (strncmp(method, "POST", 4) != 0)
        return 3;

    char *pLength = getenv("CONTENT_LENGTH");
    if (pLength == NULL)
        return 4;

    int length = atoi(pLength);
    fread(data, length, 1, stdin);
    return 0;
}

void ems::miscellaneous::getMonthDayString(system_clock::time_point local_time, std::string &dateStr)
{
    auto local_time_t = system_clock::to_time_t(local_time);
    auto format_time = gmtime(&local_time_t);

    char buf[8] = {0};
    strftime(buf, sizeof(buf), "%m-%d", format_time);
    dateStr = std::move(buf);
}

//解析BCD码（压缩BCD码）
int32_t ems::miscellaneous::parseBCDGroupWithSymbol(void *bcd_mem, uint16_t byte_num, bool is_signed)
{
    if (byte_num < 1 || byte_num > 4)//一般不超过4字节
    {
        return -1;
    }

    //以字节为单位解析BCD码
    auto byte_list = static_cast<uint8_t *>(bcd_mem);

    int8_t result_symbol = 1;   //最终结果的符号位
    int32_t result_abs_val = 0; // 最终结果的绝对值
    vector<uint8_t> bcd_list;   //解析出来的BCD码列表

    {
        //先处理高位第一个字节（如果带符号位）
        auto &first_high_byte = byte_list[byte_num - 1];
        if (is_signed)
        {
            if (first_high_byte & 0x80)
            {
                //符号位 == 1
                result_symbol = -1;
            }

            uint8_t high_bcd = first_high_byte & 0x70; // 0111 0000
            high_bcd >>= 4;
            bcd_list.push_back(std::move(high_bcd)); //高位-BCD

            uint8_t low_bcd = first_high_byte & 0x0f; // 0000 1111
            bcd_list.push_back(std::move(low_bcd));   //低位-BCD
        }
    }

    {
        //再处理不带符号位的字节
        int begin_idx = byte_num - 1;
        if (is_signed)
        {
            --begin_idx; //从高位第二个字节继续处理
        }

        for (int idx = begin_idx; idx >= 0; --idx)
        {
            //第一部分-数值
            uint8_t high_bcd = byte_list[idx] & 0xf0; // 1111 0000
            high_bcd >>= 4;
            bcd_list.push_back(std::move(high_bcd));

            //第二部分-数值
            uint8_t low_bcd = byte_list[idx] & 0x0f; // 0000 1111
            bcd_list.push_back(std::move(low_bcd));
        }
    }

    {
        //将拆分出来的BCD码按十进制权值进行累加
        int dec_weight = 1; //十进制位-权值
        int dec_abs = 0;    //十进制位-绝对值
        auto r_itr = bcd_list.rbegin();
        for (; r_itr != bcd_list.rend(); ++r_itr)
        {
            dec_abs = *r_itr;

            dec_abs *= dec_weight;
            result_abs_val += dec_abs;

            dec_weight *= 10; //累加权值
        }
    }
    return (result_symbol * result_abs_val);
}

string ems::miscellaneous::formatedPrecision(const double rawData, int formatedPrecision)
{
    ostringstream oss;
    oss << std::fixed << std::setprecision(formatedPrecision) << rawData;
    return oss.str();
}

string ems::miscellaneous::getCurrentTimestamp()
{
    using Clock = std::chrono::system_clock;
    auto now = Clock::now();
    auto time_t_now = Clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&time_t_now), "%F %T");
    return ss.str();
}

string ems::miscellaneous::getCurrentYearMonthDay()
{
    using Clock = std::chrono::system_clock;
    auto now = Clock::now();
    auto time_t_now = Clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&time_t_now), "%F");
    return ss.str();
}

string ems::miscellaneous::getCurrentWeekDay()
{
    using Clock = std::chrono::system_clock;
    auto now = Clock::now();
    auto time_t_now = Clock::to_time_t(now);
    const tm* tmPtr = localtime(&time_t_now);
    return std::to_string(tmPtr->tm_wday);// [0, 6]
}

vector<uint8_t> ems::miscellaneous::createModbusRtuReadFrame(const uint8_t devAddress, const uint8_t funcCode, const uint16_t regAddress, const uint16_t regNum)
{
    vector<uint8_t> frame(8);
    frame[0] = devAddress;
    frame[1] = funcCode;

    const uint16_t regAddressBigEndian = htobe16(regAddress);
    ::memcpy(frame.data() + 2, &regAddressBigEndian, sizeof(uint16_t));
    const uint16_t regNumBigEndian = htobe16(regNum);
    ::memcpy(frame.data() + 4, &regNumBigEndian, sizeof(uint16_t));
    const uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(frame.data(), 6);
    ::memcpy(frame.data() + 6, &crc16Modbus, sizeof(uint16_t));
    return frame;
}

vector<uint8_t> ems::miscellaneous::createModbusRtuWriteFrame(const uint8_t devAddress, const uint8_t funcCode, const uint16_t regAddress, const uint16_t regData)
{
    vector<uint8_t> frame(8);
    frame[0] = devAddress;
    frame[1] = funcCode;

    const uint16_t regAddressBigEndian = htobe16(regAddress);
    ::memcpy(frame.data() + 2, &regAddressBigEndian, sizeof(uint16_t));
    const uint16_t regDataBigEndian = htobe16(regData);
    ::memcpy(frame.data() + 4, &regDataBigEndian, sizeof(uint16_t));
    const uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(frame.data(), 6);
    ::memcpy(frame.data() + 6, &crc16Modbus, sizeof(uint16_t));
    return frame;
}

Json::Value ems::miscellaneous::unserializedJson(const string& jsonstring)
{
    JSONCPP_STRING err;
    Json::Value root;
    Json::CharReaderBuilder builder;
    const unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(jsonstring.data(), jsonstring.data() + jsonstring.length(), &root, &err)){
        throw std::runtime_error("json string parse failed");
    }
    return root;
}

string ems::miscellaneous::createFixedSizeString(const string& content, const size_t len)
{
    string newObj(content);
    newObj.resize(len);
    return newObj;
}

/*
// 1, 构造消息结构 pair
// 2, 序列化为msgpack字节流
// 3, 转换为标准的vector<byte>字节流
*/
vector<byte> ems::miscellaneous::createRespondMessage(const bool status, const vector<byte>& content)
{
    const pair<bool, vector<byte>> msg{status, content};
    const auto serializedData = msgpackWrapper::pack(msg);

    const byte* bytePtr = reinterpret_cast<const byte*>(serializedData.data());
    const vector<byte> bytes(bytePtr, bytePtr + serializedData.size());
    return bytes;
}

vector<byte> ems::miscellaneous::serializedJsonAsBytes(const Json::Value& data)
{
    Json::StreamWriterBuilder builder;
    // builder["indentation"] = "";
    const string jsonString = Json::writeString(builder, data);
    return { reinterpret_cast<const byte*>(jsonString.data()),
                reinterpret_cast<const byte*>(jsonString.data()) + jsonString.size() };
}

vector<byte> ems::miscellaneous::convertStringToBytes(const string& data)
{
    return { reinterpret_cast<const byte*>(data.data()),
            reinterpret_cast<const byte*>(data.data()) + data.size() };
}

AssertUserPriority::AssertUserPriority()
{
    const uid_t rootUid{0};
    const uid_t selfUid{getuid()};
    BOOST_ASSERT(selfUid == rootUid);
}

