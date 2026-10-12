#include "bigendian.hpp"
#include "chunk_headers.hpp"
#include "file_read.hpp"
#include <iostream>
#include <format>

using namespace std;
void handle_header(ifstream& file, Header_Chunk& header)
{
    file.read((char*)&header, sizeof(Header_Chunk));
    check(file);
    if (header.chunk_type.get() != 0x4d546864 || header.length.get() != 6 || header.divisions.get()>>15 == 1 || header.format.get() > 2
    || (header.format.get() == 0 && header.ntrks.get() !=1))
    {
        cerr << "Podany plik nie jest poprawnym plikiem formatu MIDI. Ma niepoprawny nagłówek." << endl;
        exit(1);// tu zostaly sprawdzone podstawowe wlasnosci naglowka: MThd na poczatku, dlugosc 6, mozliwe wartosci formatu, format 0 ma zawsze tylko 1 sciezke
    }
    check(file);
    string file_format;
    switch(header.format.get())
    {
        case 0:
            file_format = "jedna wielokanałowa ścieżka."; break;
        case 1:
            file_format = "wiele ścieżek odtwarzanych równocześnie."; break;
        case 2:
            file_format = "wiele niezależnych jednościeżkowych wzorców."; break;

    }
    cout << format("Magiczna liczba nagłówka to {}\n", header.chunk_type.to_string());
        cout << format("Rozmiar nagłówka (w bajtach): {}. \nFormat pliku: {}, czyli {} \nLiczba ścieżek w pliku: {} \nLiczba ticków w ćwierćnucie (divisions): {}\n",
        header.length.get()+8, header.format.get(), file_format, header.ntrks.get(), header.divisions.get());
}
void handle_track_header(ifstream&file, const size_t i, Track_Chunk& tc)
{
    file.read((char*) &tc, sizeof(tc));
    check(file);
    if (tc.chunk_type.get() != 0x4d54726b)
    {
        cerr << "Podany plik nie jest poprawnym plikiem formatu MIDI. Ścieżka " << i+1<< ". jest niepoprawna." << endl;
        exit(1); // tu zostaly sprawdzone podstawowe wlasnosci sciezki: MTrk na poczatku
    }  
    cout << format("Magiczna liczba {} ścieżki to {}\n", i+1, tc.chunk_type.to_string());
    cout << format("Rozmiar {}. ścieżki (w bajtach): {}.", i+1, tc.length.get()+8) << endl;
}