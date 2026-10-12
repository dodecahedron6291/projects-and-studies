#pragma once
#include <fstream>
#include <vector>
#include "note.hpp"
#include "assert_packed_struct.hpp"
#include "timeline.hpp"
using namespace std;
#pragma pack(push, 1)
struct Pitch_bend_change
{
    private:
        uint16_t raw_;
    public:
    int16_t get();
};

struct Polyphonic_key_pressure
{
    Note note;
    int8_t value;
};
struct Control_Change
{
    int8_t type;
    int8_t value;
};
struct Program_Change
{
    int8_t type;
};
struct Channel_pressure
{
    int8_t value;
};
#pragma pack(pop)
ASSERT_PACKED_STRUCT(Polyphonic_key_pressure, 2);
ASSERT_PACKED_STRUCT(Control_Change, 2);
ASSERT_PACKED_STRUCT(Program_Change, 1);
ASSERT_PACKED_STRUCT(Channel_pressure, 1);
ASSERT_PACKED_STRUCT(Pitch_bend_change, 2);

void handle_midi_event (ifstream& file, size_t& count, const int status_b, const Channel channel, const size_t total_time, vector<Timeline_Event>& timeline);