#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <cassert>
#include <cstdint>
#include <concepts>
#include <format>
#include <variant>
#include <algorithm>
#include <cmath>
using namespace std;
#pragma pack(push, 1)
template <typename T> requires is_integral_v<T>
class BigEndian_
{
    private:
        T raw_;
    public:
        T get() const
        {
            static_assert(endian::native == endian::big || endian::native == endian::little);
            T res = T();
            if (endian::native == endian::big)//nie trzeba nic robic
                res=raw_;
            if (endian::native == endian::little)//basically odwracamy kolejnosc
            {
                res = byteswap(raw_);
            }
            return res;
        }
        string to_string() const//zamienia liczbe z get() na stringa
        {
            string res = "";
            for (uint32_t i = 0; i < sizeof(T); i++)
            {
                T temp = get();
                temp = temp << i*8;
                temp = temp >> (sizeof(T)-1)*8;
                res += (char)temp;
            }
            return res;
        }
};
template <typename T>
requires ( sizeof(BigEndian_<T>) == sizeof(T)
    && is_standard_layout_v<BigEndian_<T>>
    && is_trivially_copyable_v<BigEndian_<T>> )
using BigEndian = BigEndian_<T>;
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
struct Pitch
{
    private:
        uint16_t raw_;
    public:
    int16_t get()
    {
        int16_t res = raw_ - (2<<15);//usuwamy wiodące 1
        res = res >>8;
        res = res << 7;//zerujemy prawą część i ustawiamy 
        res = res | (raw_%(2<<7));//dodajemy prawą czesc na dobre miejsce
        return res;
    }
};
struct Note
{
    int8_t  note;
    bool operator== (const Note& other) const
    {
        return note == other.note;
    }
};
struct Note_Off
{
    Note note;
    int8_t velocity;
};
struct Note_On
{
    Note note;
    int8_t velocity;
};
struct Polyphonic_key_pressure
{
    Note note;
    int8_t value;
};
struct Control_Change
{
    int8_t type;
    int8_t value;
};
struct Program_Change
{
    int8_t type;
};
struct Channel_pressure
{
    int8_t value;
};
struct Pitch_bend_change
{
    Pitch pitch;
};
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

#define ASSERT_PACKED_STRUCT(Type, ExpectedSize) \
    static_assert(sizeof(Type) == ExpectedSize); \
    static_assert(is_standard_layout_v<Type>); \
    static_assert(is_trivially_copyable_v<Type>)

ASSERT_PACKED_STRUCT(Header_Chunk, 14);
ASSERT_PACKED_STRUCT(Track_Chunk, 8);
ASSERT_PACKED_STRUCT(Pitch, 2);
ASSERT_PACKED_STRUCT(Note, 1);
ASSERT_PACKED_STRUCT(Note_Off, 2);
ASSERT_PACKED_STRUCT(Note_On, 2);
ASSERT_PACKED_STRUCT(Polyphonic_key_pressure, 2);
ASSERT_PACKED_STRUCT(Control_Change, 2);
ASSERT_PACKED_STRUCT(Program_Change, 1);
ASSERT_PACKED_STRUCT(Channel_pressure, 1);
ASSERT_PACKED_STRUCT(Pitch_bend_change, 2);
ASSERT_PACKED_STRUCT(BigEndian<int32_t>, 4);
ASSERT_PACKED_STRUCT(BigEndian<int16_t>, 2);
ASSERT_PACKED_STRUCT(Key, 2);
ASSERT_PACKED_STRUCT(SF, 1);
ASSERT_PACKED_STRUCT(MI, 1);
ASSERT_PACKED_STRUCT(Wav_Header, 44);

struct Meta_Event
{
    byte type;
    uint32_t length;
    vector<byte> bytes;
};
struct Sysex_Event
{
    uint32_t length;
    vector<byte> bytes;
};
struct Tempo
{
    uint32_t tempo;
};
struct Channel
{
    int channel;
    bool operator== (const Channel& other) const
    {
        return channel == other.channel;
    }
};
struct Timeline_Event // wprowadzam oś czasu na ktorej beda wazne wydarzenia
{
    variant<Note_Off,Note_On,Tempo> type;
    size_t time;
    Channel channel;
    bool operator< (const Timeline_Event& other) const 
    { 
        if (time != other.time) 
            return time < other.time;
        
        auto priority = [](const variant<Note_Off, Note_On, Tempo>& v) -> int //zeby po posortowaniu mialy sensowna kolejnosc
        {
            if (get_if<Tempo>(&v)) return 0;
            if (get_if<Note_On>(&v)) return 1;
            if (get_if<Note_Off>(&v)) return 2;
            return 3;
        };
        
        return priority(type) < priority(other.type);
    }
};
void check(const ifstream& file)
{
    if (file.fail())
    {
            cerr << "Nastąpił błąd w odczytywaniu pliku." << endl;
            exit(1);
    }
}
template <typename T>
void my_read(ifstream& file, T b, size_t& count, int x)//dla optymalizacji czytelności kodu
{
    file.read((char*)b, x);
    count-=x;
    check(file);
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
string to_string(Note note)
{
    const string names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H"};
    int octave = ((int)note.note / 12) - 1;
    int index = (int)note.note % 12;
    return names[index] + to_string(octave);
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
constexpr string to_string(MetaEventType event) 
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
        default:                                 return "Nieznane/Niestandardowe Meta Wydarzenie";//np 0x21 -  nie ma go w specyfkacji a się zdarza że występuje
    }
}
double frequency(Note note)
{
        const double d = note.note-69;
        const double dos = 2;
        return 440 * pow(dos,d/12);
}
void handle_header(ifstream& file, Header_Chunk& header)
{
    file.read((char*)&header, sizeof(Header_Chunk));
    check(file);
    if (header.chunk_type.get() != 0x4d546864 || header.length.get() != 6 || header.divisions.get()>>15 == 1 || header.format.get() > 2
    || (header.format.get() == 0 && header.ntrks.get() !=1))
    {
        cerr << "Podany plik nie jest poprawnym plikiem formatu MIDI. Ma niepoprawny nagłówek." << endl;
        exit(1);// tu zostaly sprawdzone podstawowe  wlasnosci naglowka: MThd na poczatku, dlugosc 6, mozliwe wartosci formatu, format 0 ma zawsze tylko 1 sciezke
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
            cout << "Tempo: " << dec << tempo.tempo << " mikrosekund na ćwierćnutę"<<endl;  
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
        default://jak jest jakis dziwny, w ramach ciekawostki moj poprzedni kod wylapywal 0x21 jako przypadek z tekstem i wypisywal jakis niewypisywalny znak
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
            my_read(file, &n_off, count, 2);
            cout << format("Note Off: {} ({}), prędkość: {}, kanał: {}\n", 
                to_string(n_off.note), n_off.note.note, n_off.velocity, channel.channel+1);
            timeline.push_back({n_off, total_time, channel});
            break;
        case 9:
            Note_On n_on;
            my_read(file, &n_on, count, 2);
            cout << format("Note On: {} ({}), prędkość: {}", 
                to_string(n_on.note), n_on.note.note, n_on.velocity);
            if (n_on.velocity == 0)
            {
                cout <<", czyli tak naprawdę Note Off";
            }
            cout <<", kanał: "<<channel.channel+1<< '\n';
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
                pitch_bend.pitch.get(), channel.channel+1);
            break;
    }
}
const int SamplingRate = 44100;
vector<size_t> create_timetable(const vector<Timeline_Event>& timeline, uint16_t divisions)
{  
    Tempo tempo = {500000};
    vector<size_t> res;
    res.push_back(0);
    size_t x = 0;
    
    for(size_t i = 0; i < timeline.size() - 1; i++)
    {
        if (const Tempo* pval = get_if<Tempo>(&timeline[i].type))
        {
            tempo.tempo = pval->tempo;
        }
        
        size_t timediff = timeline[i+1].time - timeline[i].time;
        x += (double)tempo.tempo / 1e6 * timediff * SamplingRate / divisions;
        res.push_back(x);
    }
    return res;
}
void handle_timeline_events(vector<Timeline_Event>& timeline, vector<long double>& music, uint16_t divisions)
{
    sort(timeline.begin(), timeline.end());
    vector<size_t> timetable = create_timetable(timeline, divisions);
    for(size_t i = 0; i< timeline.size(); i++)
    {   
        assert(get_if<Note_On>(&timeline[i].type) || get_if<Note_Off>(&timeline[i].type) || get_if<Tempo>(&timeline[i].type));
        if (const Note_On* pval = get_if<Note_On>(&timeline[i].type))
        {
            for(size_t j = i;j<timeline.size(); j++)
            {
                if (const Note_Off* pval2 = get_if<Note_Off>(&timeline[j].type))
                {
                    if (pval2->note == pval->note && timeline[i].channel == timeline[j].channel)
                    {
                        size_t note_start_time = timetable[i];
                        size_t note_end_time = timetable[j];
                        assert(pval->velocity != 0);
                        assert(note_start_time <= note_end_time);
                        const double pi = acos(-1);
                        const double freq = 2*pi*frequency(pval->note)/SamplingRate;//frequency podawane jest na sekunde a trzeba na unit
                        for(size_t k = note_start_time;k < note_end_time + SamplingRate/100;k++)
                        {
                            if (k >= music.size())
                                music.resize(note_end_time + SamplingRate/100);
                            if (k < note_end_time)
                                music[k]+= pval->velocity * sin(freq * (k-note_start_time));
                            else 
                            {
                                double release_duration = SamplingRate / 100;//wykładniczy release envelope
                                double t = (k - note_end_time) / release_duration;
                                music[k] += pval->velocity * sin(freq * (k - note_start_time)) * exp(-3 * t);
                            }
                        }
                        break;
                    }
                }  
            }
        }
    }

}
void handle_timeline(string path, vector<Timeline_Event>& timeline, const uint16_t& divisions)
{
    vector<long double> music;
    handle_timeline_events(timeline, music, divisions);
    vector<int16_t> real_music(music.size());
    if (!music.empty())
    {
        long double  max_amplitude = *max_element(music.begin(), music.end());
        for(size_t i = 0; i< real_music.size(); i++)
        {
            real_music[i] = floor(music[i] / max_amplitude * 32760);
        }
        path.erase(path.size()-4, 4);
        path += ".wav";
        Wav_Header wav_h;
        wav_h.Frequency = SamplingRate;
        wav_h.BitsPerSample = 16;
        wav_h.BytePerBloc = wav_h.NbrChannels * wav_h.BitsPerSample / 8;
        wav_h.BytePerSec = wav_h.Frequency * wav_h.BytePerBloc;
        wav_h.FileSize = 36 + real_music.size()*2;
        wav_h.DataSize = real_music.size()*2;
        ofstream output(path, ios::binary);
        output.write((char*)(&wav_h), sizeof(wav_h));
        output.write((char*)(real_music.data()), 2*real_music.size());
        cout << format("Wygenerowano plik dźwiękowy: {}", path) << endl;
    }
}
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
