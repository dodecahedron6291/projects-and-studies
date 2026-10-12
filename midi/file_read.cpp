#include "file_read.hpp"
void check(const ifstream& file)
{
    if (file.fail())
    {
            cerr << "Nastąpił błąd w odczytywaniu pliku." << endl;
            exit(1);
    }
}
uint32_t read_varint(ifstream& file, size_t& count)  // zakladam ze var len int bedzie nie dluzszy niz 4 bajty
{
    uint32_t res = 0;
    byte b;
    my_read(file, &b, count, 1);
    if((uint32_t)b >= 128)
    {
        res = res | ((uint32_t)b-128);
    }
    else res = res | (uint32_t)b;
    for (uint32_t i = 1; b >= byte(0x80); i++)
    {
        my_read(file, &b, count, 1);
        res = res <<7;
        if((uint32_t)b >= 128)
        {
            res = res | ((uint32_t)b-128);
        }
        else res = res | (uint32_t)b;
        if (i > 3)
        {
            cerr << "Ten plik posiada liczbę o zmiennej długości dłuższą niż 4 bajty, co nie jest obsługiwane przez ten program.";
            exit(1);
        }
    } 
    return res;
}