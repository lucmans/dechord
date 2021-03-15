
#include "note_set.h"
#include "config.h"

#include <ostream>


const char *note_string[12] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
const char *sub[10] = {"\xe2\x82\x80", "\xe2\x82\x81", "\xe2\x82\x82",
                       "\xe2\x82\x83", "\xe2\x82\x84", "\xe2\x82\x85",
                       "\xe2\x82\x86", "\xe2\x82\x87", "\xe2\x82\x88", "\xe2\x82\x89"};


Note::Note(double freq) {
    note = static_cast<Notes>(((int)round(fmod(12.0 * log2(freq / A4), 12.0)) + 12) % 12);

    const double C1 = A4 * exp2(-45.0 / 12.0);
    octave = log2(freq / C1) + 1;
    if(note == Notes::C && abs(freq - (C1 * exp2(octave - 1))) > 0.3 * freq)
            octave++;
};

std::ostream& operator<<(std::ostream& s, const Note& note) {
    s << note_string[static_cast<int>(note.note)] << sub[note.octave];
    return s;
}


NoteSet::NoteSet() {
    
}

NoteSet::~NoteSet() {
    
}
