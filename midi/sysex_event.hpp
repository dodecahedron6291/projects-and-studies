#pragma once
using namespace std;
struct Sysex_Event
{
    uint32_t length;
    vector<byte> bytes;
};
void handle_sysex_event (ifstream& file, size_t& count);