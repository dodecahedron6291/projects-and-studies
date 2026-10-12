#pragma once
#include <fstream>
#include <vector>
#include "note.hpp"
#include "assert_packed_struct.hpp"
#include "timeline.hpp"
using namespace std;
struct Meta_Event
{
    byte type;
    uint32_t length;
    vector<byte> bytes;
};
#pragma pack(push, 1)
struct SF
{
    int8_t  sf_;
};
struct MI
{
    uint8_t  mi_;
};
struct Key
{
    SF sf;
    MI mi;
};
#pragma pack(pop)
ASSERT_PACKED_STRUCT(Key, 2);
ASSERT_PACKED_STRUCT(SF, 1);
ASSERT_PACKED_STRUCT(MI, 1);
enum class MetaEventType : uint8_t 
{
    SequenceNumber      = 0x00,
    TextEvent           = 0x01,
    CopyrightNotice     = 0x02,
    SequenceTrackName   = 0x03,
    InstrumentName      = 0x04,
    Lyric               = 0x05,
    Marker              = 0x06,
    CuePoint            = 0x07,
    MidiChannelPrefix   = 0x20,
    EndOfTrack          = 0x2F,
    SetTempo            = 0x51,
    SmpteOffset         = 0x54,
    TimeSignature       = 0x58,
    KeySignature        = 0x59,
    SequencerSpecific   = 0x7F
};
string to_string(MetaEventType event);
string decodeKeySignature(SF sf, MI mi);
string to_string(Note note);
void handle_meta_event (ifstream& file, size_t& count, const size_t i, Tempo& tempo, const Channel channel,
    const size_t total_time, vector<Timeline_Event>& timeline);