#include <format>
#include <cassert>
#include <cstdint>
#include <string>
#include "bigendian.hpp"
#include "file_read.hpp"
#include "midi_event.hpp"
#include "meta_event.hpp"
#include "sysex_event.hpp"
#include "timeline.hpp"
using namespace std;

int16_t Pitch_bend_change::get()
{
    int16_t res = raw_ - (2<<15);//usuwamy wiodące 1
    res = res >>8;
    res = res << 7;//zerujemy prawą część i ustawiamy 
    res = res | (raw_%(2<<7));//dodajemy prawą czesc na dobre miejsce
    return res;
}
string to_string(Note note)
{
    const string names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H"};
    int octave = ((int)note.note / 12) - 1;
    int index = (int)note.note % 12;
    return names[index] + to_string(octave);
}
string to_string(MetaEventType event)
{
    switch (event) {
        case MetaEventType::SequenceNumber:      return "Sequence Number";
        case MetaEventType::TextEvent:           return "Text Event";
        case MetaEventType::CopyrightNotice:     return "Copyright Notice";
        case MetaEventType::SequenceTrackName:   return "Sequence/Track Name";
        case MetaEventType::InstrumentName:      return "Instrument Name";
        case MetaEventType::Lyric:               return "Lyric";
        case MetaEventType::Marker:              return "Marker";
        case MetaEventType::CuePoint:            return "Cue Point";
        case MetaEventType::MidiChannelPrefix:   return "MIDI Channel Prefix";
        case MetaEventType::EndOfTrack:          return "End of Track";
        case MetaEventType::SetTempo:            return "Set Tempo";
        case MetaEventType::SmpteOffset:         return "SMPTE Offset";
        case MetaEventType::TimeSignature:       return "Time Signature";
        case MetaEventType::KeySignature:        return "Key Signature";
        case MetaEventType::SequencerSpecific:   return "Sequencer-Specific Meta-Event";
        default:                                 return "Nieznane/Niestandardowe Meta Wydarzenie";//np 0x21 - nie ma go w specyfkacji
    }
}
string decodeKeySignature(SF sf, MI mi)//funkcje napisal chatgpt, nie ma tu nic odkrywczego
{
    const string majorKeys[] = {
        "Ces", "Ges", "Des", "As", "Es", "B", "F",  
        "C",                                       
        "G", "D", "A", "E", "H", "Fis", "Cis"     
    };
    
    const string minorKeys[] = {
        "as", "es", "b", "f", "c", "g", "d",      
        "a",                                         
        "e", "h", "fis", "cis", "gis", "dis", "ais"
    };
    
    int index = sf.sf_ + 7;
    
    if (mi.mi_ == 0)
        return majorKeys[index] + "-dur";
    else
        return minorKeys[index] + "-moll";
}
void handle_sysex_event (ifstream& file, size_t& count)
{
    Sysex_Event e1;
    e1.length = read_varint(file, count);
    e1.bytes = vector<byte>(e1.length);
    my_read(file, e1.bytes.data(), count, e1.length);
    cout << "Sysex Event. Hex dump:\n";
    for (uint32_t j = 0; j < e1.length; j++)
    {
        cout << hex << (int)e1.bytes[j] << ' '; 
    }
    cout << endl;
}
void handle_meta_event (ifstream& file, size_t& count, const size_t i, Tempo& tempo, const Channel channel, const size_t total_time, vector<Timeline_Event>& timeline)
{
    Meta_Event e2;
    my_read(file, &(e2.type), count, 1);
    e2.length = read_varint(file, count);
    MetaEventType MET = static_cast<MetaEventType>(e2.type);
    if ((int)e2.type != 0x51)//zeby nie wypisywalo set tempo
        cout << dec << format("{}, rozmiar: {} bajtów", to_string(MET), e2.length)<<endl;
    switch((int)e2.type)
    {
        case 0x0:
            BigEndian<uint16_t> b4;
            my_read(file, &b4, count, 2);
            cout << "Liczba: "<<dec<<b4.get();
            break;
        case 0x20:
            byte b5;
            my_read(file, &b5, count, 1);
            cout << "Kanał: "<<dec<<(int)b5;
            break;
        case 0x2F:
            cout << "Koniec ścieżki "<< dec << i+1<<endl; break;
        case 0x51:
            assert(e2.length == 3);
            tempo.tempo = 0;
            for(int j = 0; j < 3; j++)
            {
                tempo.tempo = tempo.tempo << 8;
                byte b6;
                my_read(file, &b6, count, 1);
                tempo.tempo = tempo.tempo | (uint32_t)b6;
            }
            // cout << "Tempo: " << dec << tempo.tempo << " mikrosekund na ćwierćnutę"<<endl;  
            timeline.push_back({tempo, total_time, channel});
            break;
        case 0x54:{
            assert(e2.length == 5);
            byte offset[5];
            my_read(file, offset, count, 5);
            uint8_t fps = (int)offset[0];
            fps = fps >>5;
            cout << dec << format("Opóźnienie: {} h {} min {} s {} klatek i {} części setnych klatki, w ",(int)offset[0],(int)offset[1],(int)offset[2],(int)offset[3],(int)offset[4]);
            switch(fps)
            {
                case 0:
                    cout << "24";break;
                case 1:
                    cout << "25";break;
                case 2:
                    cout << "29.97";break;
                case 3:
                    cout << "30";break;
            }
            cout << " klatkach na sekundę" <<endl;
            break;}
        case 0x58:
            assert(e2.length == 4);
            byte time[4];
            my_read(file, time, count, 4);
            cout << "Metrum: "<<(int)time[0]<<"/"<<(1<<((int)time[1]))<< endl;
            cout << "MIDI clocks na klik metronomu: " << (int)time[2]<<endl;
            cout << "32-ki na ćwierćnutę: " << (int)time[3]<<endl;
            break;
        case 0x59:
            assert(e2.length == 2);
            Key key;
            my_read(file, &key, count, 2);
            cout << "Gama: " << decodeKeySignature(key.sf, key.mi)<<endl;
            break;
        case 0x7F:{
            vector<byte> data(e2.length);
            my_read(file, data.data(), count, e2.length);
            cout << "Specyficzne dane programu/producenta (hex dump): ";
            for (uint32_t j=0;j<e2.length;j++)
            {
                cout << hex<< (int)data[j] << ' ';
            }
            cout << endl;
            break;}          
        case 1://na szczescie liczby 0-9 są takie same w hex i dec
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:{
            string text;
            text.resize(e2.length);
            assert(text.size() == e2.length);
            my_read(file, &text[0], count, e2.length);
            cout << "Tekst: "<<text<< endl;
            break;}
        default://jak jest jakis dziwny
            file.ignore(e2.length);
            check(file);
            count -= e2.length;
    }
}
void handle_midi_event (ifstream& file, size_t& count, const int status_b, const Channel channel, const size_t total_time, vector<Timeline_Event>& timeline)
{
    switch (status_b)
    {   
        case 8:
            Note_Off n_off;
            my_read(file, &n_off, count, 2);//te i inne couty zostawiam zakomentowane dla ewentualnego debugu
            // cout << format("Note Off: {} ({}), prędkość: {}, kanał: {}\n", 
            //     to_string(n_off.note), n_off.note.note, n_off.velocity, channel.channel+1);
            timeline.push_back({n_off, total_time, channel});
            break;
        case 9:
            Note_On n_on;
            my_read(file, &n_on, count, 2);
            // cout << format("Note On: {} ({}), prędkość: {}", 
            //     to_string(n_on.note), n_on.note.note, n_on.velocity);
            // if (n_on.velocity == 0)
            // {
            //     cout <<", czyli tak naprawdę Note Off";
            // }
            // cout <<", kanał: "<<channel.channel+1<< '\n';
            if (n_on.velocity != 0)
                timeline.push_back({n_on, total_time, channel});
            else 
            {
                Note_Off n_onf = {n_on.note, n_on.velocity};//n_on.velocity == 0
                timeline.push_back({n_onf, total_time, channel});
            }
            break;
        case 10:
            Polyphonic_key_pressure pol_key_pr;
            my_read(file, &pol_key_pr, count, 2);
            cout << format("Polyphonic key pressure/Aftertouch: {}, ({}), wartość: {}, kanał: {}\n", 
                to_string(pol_key_pr.note), pol_key_pr.note.note, pol_key_pr.value, channel.channel+1);
            break;
        case 11:
            Control_Change control_change;
            my_read(file, &control_change, count, 2);
            cout << format("Control Change. Typ urządzenia: {}, wartość: {}, kanał: {}\n",
                control_change.type, control_change.value, channel.channel+1);
            break;
        case 12:
            Program_Change program_change;
            my_read(file, &program_change, count, 1);
            cout << format("Program Change. Numer instrumentu: {}, kanał: {}\n", 
                program_change.type, channel.channel+1);
            break;
        case 13:
            Channel_pressure channel_pressure;
            my_read(file, &channel_pressure, count, 1);
            cout << format("Channel key pressure/Aftertouch. Wartość: {} kanał: {}\n", 
                channel_pressure.value, channel.channel+1);
            break;
        case 14:
            Pitch_bend_change pitch_bend;
            my_read(file, &pitch_bend, count, 2);
            cout << format("Pitch Bend Change. Wartość: {}, kanał: {}\n",
                pitch_bend.get(), channel.channel+1);
            break;
    }
}