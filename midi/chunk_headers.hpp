#pragma once
#include <cstdint>       
#include "fstream"
#include "bigendian.hpp"

using namespace std;


struct Header_Chunk//nomenklatura ze specyfikacji midi
{
    BigEndian<uint32_t> chunk_type;
    BigEndian<uint32_t> length;
    BigEndian<uint16_t> format;
    BigEndian<uint16_t> ntrks;
    BigEndian<uint16_t> divisions;
};
struct Track_Chunk
{
    BigEndian<uint32_t> chunk_type;
    BigEndian<uint32_t> length;
};
void handle_header(ifstream& file, Header_Chunk& header);
void handle_track_header(ifstream&file, const size_t i, Track_Chunk& tc);