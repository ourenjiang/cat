#include "Miscellaneous.h"
#include <regex>
// #include <unistd.h>
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "crc16.h"

using namespace std;

optional<pair<string, string>> miscellaneous::parseTcpAddress(const string& address)
{
    regex pattern(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d{1,5}))");
    smatch match;
    if(regex_search(address, match, pattern)){
        const string ip = match[1].str();
        const string port = match[2].str();
        return make_pair(ip, port);
    }
    return {};
}

//解析BCD码（压缩BCD码）
int32_t miscellaneous::parseBCDGroupWithSymbol(void *bcd_mem, uint16_t byte_num, bool is_signed)
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
