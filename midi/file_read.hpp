#pragma once
#include <fstream>
#include <iostream>
#include <cstdint>
#include "bigendian.hpp"
using namespace std;

void check(const ifstream& file);

template <typename T>
void my_read(ifstream& file, T b, size_t& count, int x)//dla lepszej czytelności
{
    file.read((char*)b, x);
    count-=x;
    check(file);
}
uint32_t read_varint(ifstream& file, size_t& count);