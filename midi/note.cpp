#include "note.hpp"
using namespace std;
bool Note::operator== (const Note& other) const
{
    return note == other.note;
}
bool Channel::operator== (const Channel& other) const
{
    return channel == other.channel;
}
