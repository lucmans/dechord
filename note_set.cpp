
#include "note_set.h"
#include "config.h"

#include <iostream>
#include <ostream>
#include <string>
#include <cctype>


const char *note_string[12] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
const char *sub[10] = {"\xe2\x82\x80", "\xe2\x82\x81", "\xe2\x82\x82",
                       "\xe2\x82\x83", "\xe2\x82\x84", "\xe2\x82\x85",
                       "\xe2\x82\x86", "\xe2\x82\x87", "\xe2\x82\x88", "\xe2\x82\x89"};

const std::string stringify_sub(int n) {
    if(n < 0)
        return "";
    if(n < 10)
        return sub[n];

    std::string out;
    while(n != 0) {
        out = sub[n % 10] + out;
        n /= 10;
    }

    return out;
}


double interpolate_max(const int max_idx, const double norms[(FRAME_SIZE / 2) + 1]) {
    const double a = log2(norms[max_idx - 1]),
                 b = log2(norms[max_idx]),
                 c = log2(norms[max_idx + 1]);
    const double p = 0.5 * ((a - c) / (a - (2.0 * b) + c));

    return max_idx + p;
}

double interpolate_max(const int max_idx, const double norms[(FRAME_SIZE / 2) + 1], double &amp) {
    const double a = log2(norms[max_idx - 1]),
                 b = log2(norms[max_idx]),
                 c = log2(norms[max_idx + 1]);
    const double p = 0.5 * ((a - c) / (a - (2.0 * b) + c));

    amp = b - (0.25 * (a - c) * p);

    return max_idx + p;
}


Note::Note(const double _freq, const double _amp) : freq(_freq), amp(_amp) {
    note = static_cast<Notes>(((int)round(fmod(12.0 * log2(_freq / A4), 12.0)) + 12) % 12);
    // note = ((int)round(fmod(12.0 * log2(_freq / A4), 12.0)) + 12) % 12;

    const double tuned_freq = A4 * exp2(round(12.0 * log2(_freq / A4)) / 12.0);
    error = 1200.0 * log2(_freq / tuned_freq);

    const double C1 = A4 * exp2(-45.0 / 12.0);
    octave = log2(_freq / C1) + 1;
    // Correct for notes slightly detuned below octave
    if(note == Notes::C && _freq < tuned_freq)
        octave++;
};

Note::Note(const Notes _note, const int _octave) : note(_note), octave(_octave) {}


std::ostream& operator<<(std::ostream &s, const Note &note) {
    s << note_string[static_cast<int>(note.note)] << stringify_sub(note.octave);
    // s << note_string[note.note] << sub[note.octave];
    return s;
}


NoteSet::NoteSet(const double norms[(FRAME_SIZE / 2) + 1], const std::vector<int> &peaks) {
    for(int p : peaks) {
        // TODO: Check if the interpolation will be in-bounds
        // if(p == 0 || p == FRAME_SIZE / 2)
        //     continue;
        // else if(p > FRAME_SIZE / 2)
        //     exit(-1);  // TODO: Error here

        double amp;
        const double freq = ((double)SAMPLE_RATE / FRAME_SIZE) * interpolate_max(p, norms, amp);
        notes.push_back(Note(freq, amp));
    }
}

NoteSet::~NoteSet() {
    
}


const std::vector<Note>* NoteSet::get_notes() const {
    return &notes;
}

const Note* NoteSet::get_loudest_peak() const {
    const int n_notes = notes.size();
    if(n_notes == 0)
        return nullptr;
    else if(n_notes == 1)
        return &notes[0];

    const Note *out = &notes[0];
    for(int i = 1; i < n_notes; i++) {
        if(notes[i].amp > out->amp)
            out = &notes[i];
    }

    return out;
}

// TODO: Could be optimized by returning first peak, as the peak picking algorithms sorts the peaks
const Note* NoteSet::get_lowest_peak() const {
    const int n_notes = notes.size();
    if(n_notes == 0)
        return nullptr;
    else if(n_notes == 1)
        return &notes[0];

    const Note *out = &notes[0];
    for(int i = 1; i < n_notes; i++) {
        if(notes[i].freq < out->freq)
            out = &notes[i];
    }

    return out;
}


const Note* NoteSet::get_likeliest_note() const {
    const int n_notes = notes.size();
    if(n_notes == 0)
        return nullptr;
    else if(n_notes == 1 && notes[0].amp > 0)
        return &notes[0];

    std::vector<int> n_harmonics(n_notes, 0);
    for(int i = 0; i < n_notes; i++) {
        for(int j = i + 1; j < n_notes; j++) {
            const double detected_freq = notes[j].freq;
            const double theoretical_freq = notes[i].freq * round(notes[j].freq / notes[i].freq);
            const double cent_error = 1200 * log2(detected_freq / theoretical_freq);
            if(cent_error > -OVERTONE_ERROR && cent_error < OVERTONE_ERROR)
                n_harmonics[i]++;
        }
    }

    int max_idx = 0;
    for(int i = 1; i < n_notes; i++) {
        if(n_harmonics[i] > n_harmonics[max_idx])
            max_idx = i;
    }

    // Filter noise
    if(notes[max_idx].amp < 0)
        return nullptr;

    return &notes[max_idx];
}


void NoteSet::get_likely_notes(std::vector<const Note*> &out) const {
    out.clear();

    const int n_notes = notes.size();
    if(n_notes == 0)
        return;
    else if(n_notes == 1) {
        out.push_back(&notes[0]);
        return;
    }

    // for(auto &a : notes)
    //     std::cout << a << ' ';
    // std::cout << std::endl;

    // First find the number of possible harmonics for each note
    std::vector<int> n_harmonics(n_notes, 0);
    std::vector<bool> explained(n_notes, false);
    for(int i = 0; i < n_notes; i++) {
        for(int j = i + 1; j < n_notes; j++) {
            const double detected_freq = notes[j].freq;
            const double theoretical_freq = notes[i].freq * round(notes[j].freq / notes[i].freq);
            const double cent_error = 1200 * log2(detected_freq / theoretical_freq);
            // std::cout << notes[i].freq << ' ' << detected_freq << ' ' << theoretical_freq << ' ' << cent_error << std::endl;
            if(cent_error > -OVERTONE_ERROR && cent_error < OVERTONE_ERROR) {
                n_harmonics[i]++;
                // std::cout << notes[j] << " explained by " << notes[i] << std::endl;
                explained[j] = true;
            }
        }
        // std::cout << std::endl;
    }

    // Test if has more harmonics then others
    for(int i = 0; i < n_notes; i++) {
        if(!explained[i])
            out.push_back(&notes[i]);
    }

    // TODO
    // Subtract harmonics from signal and see if other harmonics are still left
    // Repeat till no peaks are left
}


std::ostream& operator<<(std::ostream &s, const NoteSet &noteset) {
    const std::vector<Note> *notes = noteset.get_notes();
    const int n_notes = notes->size();
    if(n_notes == 0)
        return s << "{}";

    s << '{' << (*notes)[0];
    for(int i = 1; i < n_notes; i++)
        s << ", " << (*notes)[i];
    s << '}';

    std::cout << std::endl;

    s << '{' << (*notes)[0].freq;
    for(int i = 1; i < n_notes; i++)
        s << ", " << (*notes)[i].freq;
    s << '}';

    return s;
}

// std::ostream& operator<<(std::ostream &s, const NoteSet &noteset) {
//     const std::vector<Note> *notes = noteset.get_notes();
//     const int n_notes = notes->size();
//     if(n_notes == 0)
//         return s << "{}";

//     s << '{' << (*notes)[0].freq;
//     for(int i = 1; i < n_notes; i++)
//         s << ", " << (*notes)[i].freq;
//     s << '}';

//     return s;
// }

void print_notevec(const std::vector<const Note*> &note_vec) {
    const int n_notes = note_vec.size();
    if(n_notes == 0) {
        std::cout << "{}" << std::endl;
        return;
    }

    std::cout << '{' << *note_vec[0];
    for(int i = 1; i < n_notes; i++)
        std::cout << ", " << *note_vec[i];
    std::cout << '}' << std::endl;

    std::cout << '{' << note_vec[0]->freq;
    for(int i = 1; i < n_notes; i++)
        std::cout << ", " << note_vec[i]->freq;
    std::cout << '}' << std::endl;
}

void in_range(std::vector<const Note*> &note_vec) {
    for(int i = note_vec.size() - 1; i >= 0; i--) {
        if(note_vec[i]->freq < MIN_FREQ || note_vec[i]->freq > MAX_FREQ)
            note_vec.erase(note_vec.begin() + i);
    }
}

void in_range(const Note* &note) {
    if(note->freq < MIN_FREQ || note->freq > MAX_FREQ)
        note = nullptr;
}
