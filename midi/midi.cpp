#include <iostream>
#include <fstream>
#include "bigendian.hpp"
#include "file_read.hpp"
#include "handles.cpp"

using namespace std;


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cerr << "Błędna liczba argumentów lub formuła komendy. Poprawne użycie programu to: program plik.mid" << endl;
        return 1;
    }
    string path = argv[1];
    ifstream file(path, ios::binary);
    if (!file.is_open())
    {
        cerr << "Nie można otworzyć pliku. Sprawdź, czy ścieżka jest poprawna. Poprawne użycie programu to: program plik.mid" << endl;
        return 1;
    }
    Header_Chunk header;
    handle_header(file, header);
    vector<Track_Chunk> v(header.ntrks.get());
    vector<Timeline_Event> timeline;
    for (size_t i = 0; i < header.ntrks.get(); i++)
    {
        handle_track_header(file, i, v[i]);
        size_t count = v[i].length.get();
        Tempo tempo = {500000};//w mikrosekundach na ćwierćnutę(10^-6 s/qn)
        byte last_b = (byte)0;
        size_t total_time = 0;//zmienna przechowująca na którym ticku jesteśmy bezwzglednie
        while(count>0)
        {
            uint32_t delta_time = read_varint(file, count);
            delta_time = (int32_t)delta_time;
            total_time += delta_time;
            // const double delta_time_s = tempo.tempo / (header.divisions.get()*1e6) * delta_time;
            // cout << dec << format("--Wydarzenie--\n");                 // nie wypisuje tego bo wiekszosc eventow to beda nuty, a wiec puste delta time
            // cout << format("Delta czasu: {} ticków ({} s)\n", delta_time, delta_time_s);
            byte b;
            if((int)file.peek() >= 0x80)
            {
                my_read(file, &b, count, 1);
            }
            else 
            {
                assert(last_b != (byte)0);
                b = last_b;   
            }
            last_b = b;
            const Channel channel = {((int)b)%16};
            const int status_b = ((int)b)/16;//status byte
            assert(status_b != 15 || (status_b == 15 && (channel.channel ==0 || channel.channel == 7 || channel.channel == 15)));
            if (status_b == 0x0F && (channel.channel == 0 || channel.channel == 7))
                handle_sysex_event(file, count);
            else if (status_b == 15 && channel.channel == 15)
                handle_meta_event(file, count, i, tempo, channel, total_time, timeline);
            else 
                handle_midi_event(file, count, status_b, channel, total_time, timeline);//wyróżniam midi eventy przez status_b
        }
    }
    if (file.peek() != EOF)
    {
        cerr << "Plik posiada zawartość po oczekiwanym końcu pliku. Może to nie być poprawny plik MIDI." << endl;
        return 1;
    }
    handle_timeline(path, timeline, header.divisions.get());
    return 0;
}
