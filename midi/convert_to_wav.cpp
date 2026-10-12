#include <cmath>
#include "convert_to_wav.hpp"
#include "meta_event.hpp"


bool Timeline_Event::operator< (const Timeline_Event& other) const 
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
static const int SamplingRate = 44100;
double frequency(Note note)
{
        const double d = note.note-69;
        const double dos = 2;
        return 440 * pow(dos,d/12);
}
static vector<size_t> create_timetable(const vector<Timeline_Event>& timeline, uint16_t divisions)
{  
    Tempo tempo = {500000};
    vector<size_t> res;
    res.push_back(0);
    size_t x = 0;
    for(size_t i = 0; i+1 < timeline.size(); i++)
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
            size_t j;
            for(j= i+1;j<timeline.size(); j++)//zakladam ze kazde note on ma odpowiadające note off
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
                        const double freq = 2*pi*frequency(pval->note)/SamplingRate;
                        for(size_t k = note_start_time;k < note_end_time + SamplingRate/100;k++)
                        {
                            if (k >= music.size())
                                music.resize(note_end_time + SamplingRate/100);
                            if (k < note_end_time)
                                music[k]+= pval->velocity * sin(freq * (k-note_start_time));
                            else 
                            {
                                double release_duration = SamplingRate / 100;//wykladniczy release envelope
                                double t = (k - note_end_time) / release_duration;
                                music[k] += pval->velocity * sin(freq * (k - note_start_time)) * exp(-3 * t);
                            }
                        }
                        break;
                    }
                }  
            }
            if (j == timeline.size())
            {
                cerr << "Plik zawiera niekończącą się nutę: " << to_string(pval->note) << endl;
            }
        }
    }
}//beep boop schodki z klamerek
static long double my_abs(const long double& x)
{
    if (x < 0) return -1 * x;
    return x;
}
void handle_timeline(string path, vector<Timeline_Event>& timeline, const uint16_t& divisions)
{
    vector<long double> music;
    handle_timeline_events(timeline, music, divisions);
    vector<int16_t> real_music(music.size());
    if (!music.empty())
    {
        long double max_val = *max_element(music.begin(), music.end());
        long double min_val = *min_element(music.begin(), music.end());
        long double max_amplitude = max(my_abs(max_val), my_abs(min_val));
        for(size_t i = 0; i< real_music.size(); i++)
        {
            real_music[i] = floor(music[i] / max_amplitude * 32760);//to zawsze jest w dobrym przedziale
        }
        assert(path.size()>=4);
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
        cout << "Utworzono nowy plik: " << path << endl;
    }
}