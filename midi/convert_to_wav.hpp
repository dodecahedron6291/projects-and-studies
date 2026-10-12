#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include "note.hpp"
#include "assert_packed_struct.hpp"
#include <algorithm>
#include <cassert>
#include "midi_event.hpp"
#include <variant>
#include "timeline.hpp"


using namespace std;
#pragma pack(push, 1)
struct Wav_Header
{
    char FileTypeBlocID[4] = {'R', 'I', 'F', 'F'};
    uint32_t FileSize;
    char FileFormatID[4] = {'W', 'A', 'V', 'E'};
    
    char FormatBlocID[4] = {'f', 'm', 't', ' '};
    uint32_t BlocSize = 16;
    uint16_t AudioFormat = 1;
    uint16_t NbrChannels = 1;
    uint32_t Frequency;
    uint32_t BytePerSec;
    uint16_t BytePerBloc;
    uint16_t BitsPerSample;
    
    char DataBlocID[4] = {'d', 'a', 't', 'a'};
    uint32_t DataSize;
};
#pragma pack(pop)
ASSERT_PACKED_STRUCT(Wav_Header, 44);

double frequency(Note note);

void handle_timeline_events(vector<Timeline_Event>& timeline, vector<long double>& music, uint16_t divisions);

void handle_timeline(string path, vector<Timeline_Event>& timeline, const uint16_t& divisions);
