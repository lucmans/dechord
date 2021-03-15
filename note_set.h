
#ifndef NOTESET_H
#define NOTESET_H


#include <ostream>


// extern const char *note_string[12];


// 'd' denotes #; the opposite of b
enum class Notes {
    A = 0,
    Ad = 1,
    B = 2,
    C = 3,
    Cd = 4,
    D = 5,
    Dd = 6,
    E = 7,
    F = 8,
    Fd = 9,
    G = 10,
    Gd = 11
};


struct Note {
    double freq;

    // Closest note
    Notes note;
    int octave;
    double error;  // In cents, so between -50 and 50

    Note(const double freq);
};

std::ostream& operator<<(std::ostream& s, const Note& note);


class NoteSet {
    public:
        NoteSet();
        ~NoteSet();


    private:
        //
};


#endif  // NOTE_H
