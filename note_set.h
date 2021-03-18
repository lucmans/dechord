
#ifndef NOTESET_H
#define NOTESET_H


#include "config.h"

#include <ostream>
#include <vector>


// extern const char *note_string[12];


// Given a peak at max_idx (can't be first or last of norms), interpolate actual peak location
double interpolate_max(const int max_idx, const double norms[(WINDOW_SAMPLES / 2) + 1]);
double interpolate_max(const int max_idx, const double norms[(WINDOW_SAMPLES / 2) + 1], double &amp);


// 'd' denotes #; the opposite of b (from diesis)
enum class Notes {
    A = 0, Ad = 1, B = 2, C = 3, Cd = 4, D = 5,
    Dd = 6, E = 7, F = 8, Fd = 9, G = 10, Gd = 11
};


struct Note {
    double freq;
    double amp;

    // Closest note
    Notes note;
    int octave;
    // double error;  // In cents, so between -50 and 50

    Note(const double freq, const double amp);
};

std::ostream& operator<<(std::ostream &s, const Note &note);


class NoteSet {
    public:
        NoteSet(const double norms[(WINDOW_SAMPLES / 2) + 1], const std::vector<int> &peaks);
        ~NoteSet();

        const std::vector<Note>* get_notes() const;
        const Note* get_loudest() const;
        const Note* get_lowest() const;


    private:
        std::vector<Note> notes;
};

std::ostream& operator<<(std::ostream &s, const NoteSet &noteset);


#endif  // NOTE_H
