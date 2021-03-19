
#include "note_set.h"
#include "config.h"

#include <ostream>
#include <string>


const char *note_string[12] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
const char *sub[10] = {"\xe2\x82\x80", "\xe2\x82\x81", "\xe2\x82\x82",
                       "\xe2\x82\x83", "\xe2\x82\x84", "\xe2\x82\x85",
                       "\xe2\x82\x86", "\xe2\x82\x87", "\xe2\x82\x88", "\xe2\x82\x89"};

const std::string stringify_sub(int n) {
    if(n < 10)
        return sub[n];

    std::string out;
    while(n != 0) {
        out = sub[n % 10] + out;
        n /= 10;
    }

    return out;
}


double interpolate_max(const int max_idx, const double norms[(WINDOW_SAMPLES / 2) + 1]) {
    const double a = norms[max_idx - 1],
                 b = norms[max_idx],
                 c = norms[max_idx + 1];
    const double p = 0.5 * ((a - c) / (a - (2.0 * b) + c));

    return max_idx + p;
}

double interpolate_max(const int max_idx, const double norms[(WINDOW_SAMPLES / 2) + 1], double &amp) {
    const double a = norms[max_idx - 1],
                 b = norms[max_idx],
                 c = norms[max_idx + 1];
    const double p = 0.5 * ((a - c) / (a - (2.0 * b) + c));

    amp = b - (0.25 * (a - c) * p);

    return max_idx + p;
}


Note::Note(const double _freq, const double _amp) : freq(_freq), amp(_amp) {
    note = static_cast<Notes>(((int)round(fmod(12.0 * log2(_freq / A4), 12.0)) + 12) % 12);
    // note = ((int)round(fmod(12.0 * log2(_freq / A4), 12.0)) + 12) % 12;

    const double C1 = A4 * exp2(-45.0 / 12.0);
    octave = log2(_freq / C1) + 1;
    // Correct for notes slightly detuned below octave
    if(note == Notes::C && abs(_freq - (C1 * exp2(octave - 1))) > 0.3 * _freq)
        octave++;

    // error = 
};

Note::Note(const Notes _note, const int _octave) : note(_note), octave(_octave) {}

std::ostream& operator<<(std::ostream &s, const Note &note) {
    s << note_string[static_cast<int>(note.note)] << stringify_sub(note.octave);
    // s << note_string[note.note] << sub[note.octave];
    return s;
}


NoteSet::NoteSet(const double norms[(WINDOW_SAMPLES / 2) + 1], const std::vector<int> &peaks) {
    for(int p : peaks) {
        // TODO: Check if the interpolation will be in-bounds
        // if(p == 0 || p == WINDOW_SAMPLES / 2)
        //     continue;
        // else if(p > WINDOW_SAMPLES / 2)
        //     exit(-1);  // TODO: Error here

        double amp;
        const double freq = ((double)SAMPLE_RATE / WINDOW_SAMPLES) * interpolate_max(p, norms, amp);
        notes.push_back(Note(freq, amp));
    }
}

NoteSet::~NoteSet() {
    
}


const std::vector<Note>* NoteSet::get_notes() const {
    return &notes;
}

const Note* NoteSet::get_loudest_peak() const {
    if(notes.size() == 0)
        return nullptr;
    else if(notes.size() == 1)
        return &notes[0];

    const Note *out = &notes[0];
    for(size_t i = 1; i < notes.size(); i++) {
        if(notes[i].amp > out->amp)
            out = &notes[i];
    }

    return out;
}

const Note* NoteSet::get_lowest_peak() const {
    if(notes.size() == 0)
        return nullptr;
    else if(notes.size() == 1)
        return &notes[0];

    const Note *out = &notes[0];
    for(size_t i = 1; i < notes.size(); i++) {
        if(notes[i].freq < out->freq)
            out = &notes[i];
    }

    return out;
}


const Note* NoteSet::get_lowest_note() const {
    if(notes.size() == 0)
        return nullptr;
    else if(notes.size() == 1)
        return &notes[0];

    std::vector<int> n_harmonics(notes.size(), 0);
    for(size_t i = 0; i < notes.size(); i++) {
        for(size_t j = i + 1; j < notes.size(); j++) {
            if(fmod(notes[j].freq / notes[i].freq, 1.0) < 0.1)
                n_harmonics[i]++;
        }
    }

    size_t max_idx = 0;
    for(size_t i = 1; i < n_harmonics.size(); i++) {
        if(n_harmonics[i] > n_harmonics[max_idx])
            max_idx = i;
    }

    return &notes[max_idx];
}


std::ostream& operator<<(std::ostream &s, const NoteSet &noteset) {
    const std::vector<Note> *notes = noteset.get_notes();
    if(notes->size() == 0)
        return s << "{}";

    s << '{' << (*notes)[0];
    for(size_t i = 1; i < notes->size(); i++)
        s << ", " << (*notes)[i];
    s << '}';

    return s;
}
