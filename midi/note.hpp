#pragma once
#include <cstdint>
#include "assert_packed_struct.hpp"
using namespace std;
#pragma pack(push, 1)
struct Note
{
    int8_t  note;
    bool operator== (const Note& other) const;
};
struct Tempo
{
    uint32_t tempo;
};
struct Note_Off
{
    Note note;
    int8_t velocity;
};
struct Note_On
{
    Note note;
    int8_t velocity;
};
#pragma pack(pop)
ASSERT_PACKED_STRUCT(Note, 1);
ASSERT_PACKED_STRUCT(Note_Off, 2);
ASSERT_PACKED_STRUCT(Note_On, 2);
ASSERT_PACKED_STRUCT(Tempo, 4);
struct Channel
{
    int channel;
    bool operator== (const Channel& other) const;
};