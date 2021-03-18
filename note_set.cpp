
#include "note_set.h"
#include "config.h"

#include <ostream>


const char *note_string[12] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
const char *sub[10] = {"\xe2\x82\x80", "\xe2\x82\x81", "\xe2\x82\x82",
                       "\xe2\x82\x83", "\xe2\x82\x84", "\xe2\x82\x85",
                       "\xe2\x82\x86", "\xe2\x82\x87", "\xe2\x82\x88", "\xe2\x82\x89"};


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

    const double C1 = A4 * exp2(-45.0 / 12.0);
    octave = log2(_freq / C1) + 1;
    // Correct for notes slightly detuned below octave
    if(note == Notes::C && abs(_freq - (C1 * exp2(octave - 1))) > 0.3 * _freq)
        octave++;

    // error = 
};

std::ostream& operator<<(std::ostream &s, const Note &note) {
    s << note_string[static_cast<int>(note.note)] << sub[note.octave];
    return s;
}


NoteSet::NoteSet(const double norms[(WINDOW_SAMPLES / 2) + 1], const std::vector<int> &peaks) {
    for(int p : peaks) {
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
