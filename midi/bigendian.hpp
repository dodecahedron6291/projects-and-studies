#pragma once
#include "assert_packed_struct.hpp"
#include <cassert>
#include <cstdint>
#include <concepts>
#include <string>
using namespace std;
#pragma pack(push, 1)
template <typename T> requires is_integral_v<T>
class BigEndian_
{
    private:
        T raw_;
    public:
        T get() const
        {
            static_assert(endian::native == endian::big || endian::native == endian::little);
            T res = T();
            if (endian::native == endian::big)//nie trzeba nic robic
                res=raw_;
            if (endian::native == endian::little)//basically odwracamy kolejnosc
            {
                res = byteswap(raw_);
            }
            return res;
        }
        string to_string() const//zamienia liczbe z get() na stringa
        {
            string res = "";
            for (uint32_t i = 0; i < sizeof(T); i++)
            {
                T temp = get();
                temp = temp << i*8;
                temp = temp >> (sizeof(T)-1)*8;
                res += (char)temp;
            }
            return res;
        }
};
template <typename T>
requires ( sizeof(BigEndian_<T>) == sizeof(T)
    && is_standard_layout_v<BigEndian_<T>>
    && is_trivially_copyable_v<BigEndian_<T>> )
using BigEndian = BigEndian_<T>;
#pragma pack(pop)
ASSERT_PACKED_STRUCT(BigEndian_<int32_t>, 4);
ASSERT_PACKED_STRUCT(BigEndian_<int16_t>, 2);

