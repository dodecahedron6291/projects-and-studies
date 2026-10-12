#pragma once
#include "note.hpp"
#include <variant>
using namespace std;
struct Timeline_Event // wprowadzam oś czasu na ktorej beda wazne wydarzenia
{
    variant<Note_Off,Note_On,Tempo> type;
    size_t time;
    Channel channel;
    bool operator< (const Timeline_Event& other) const;
};