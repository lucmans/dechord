
#include "music_file.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>


float *song_samples = nullptr;
unsigned int song_size;
unsigned int song_n_samples;


struct RIFF_chunk {
    char chunk_id[4];
    uint32_t chunk_size;
    char chunk_format[4];
};

struct fmt_chunk {
    char subchunk_id[4];
    uint32_t subchunk_size;
    uint16_t audio_format;
    uint16_t n_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

// struct data_chunk {
//     char subchunk_id[4];
//     uint32_t subchunk_size;
// };


bool file_get_samples(float *in, const int n_samples) {
    static unsigned int progress = 0;  // samples
    
    // TODO: Remove check
    if(progress * sizeof(float) > song_size) {
        memset(in, 0, n_samples * sizeof(float));
        return true;
        // std::cout << "Error: Reading samples after file" << std::endl;
        // exit(-1);
    }

    // Fill end with zeros if not enough samples left in song
    if(progress + n_samples > song_n_samples) {
        memcpy(in, song_samples + progress, song_n_samples - progress);
        memset(in + (song_n_samples - progress), 0, (n_samples * sizeof(float)) - (song_size - (progress * sizeof(float))));
        progress = song_n_samples + 1;
        return false;
    }

    memcpy(in, song_samples + progress, n_samples * sizeof(float));
    progress += n_samples;
    // std::cout << progress << ' ' << song_size << ' ' << n_samples << std::endl;
    return true;
}


void load_file(const std::string &name) {
    std::ifstream file(name);
    if(!file.is_open()) {
        std::cout << "Error: Couldn't open file" << std::endl;
        exit(-1);
    }

    // Sanity check
    if(sizeof(float) != 4) {
        std::cout << "Unsupported architecture (wrong float size)" << std::endl;
        exit(-1);
    }

    RIFF_chunk rc;
    file.read((char *)&rc, 12);
    if(strncmp(rc.chunk_id, "RIFF", 4) != 0) {
        std::cout << "Error: Not a valid WAV file (no RIFF magic)" << std::endl;
        exit(-1);
    }
    if(strncmp(rc.chunk_format, "WAVE", 4) != 0) {
        std::cout << "Error: Not a valid WAV file (no WAVE format)" << std::endl;
        exit(-1);
    }

    fmt_chunk fc;
    file.read((char *)&fc, 24);
    if(strncmp(fc.subchunk_id, "fmt ", 4) != 0) {
        std::cout << "Error: Not a valid WAV file (not fmt format)" << std::endl;
        exit(-1);
    }
    if(fc.subchunk_size != 16) {
        std::cout << "Error: Invalid subchunk size (likely not PCM encoded)" << std::endl;
        exit(-1);
    }
    if(fc.audio_format != 3) {
        std::cout << "Error: Invalid audio format (not IEEE Float)" << std::endl;
        exit(-1);
    }
    if(fc.n_channels != 1) {
        std::cout << "Error: WAV file is not mono" << std::endl;
        exit(-1);
    }
    if(fc.sample_rate != 192000) {
        std::cout << "Error: Sample rate is not 192000 Hz" << std::endl;
        exit(-1);
    }
    if(fc.bits_per_sample != 32) {
        std::cout << "Error: Incorrect number of bits per sample" << std::endl;
        exit(-1);
    }

    // Skip all other fields until data field
    char buf[4];
    file.read(buf, 4);
    while(strncmp(buf, "data", 4) != 0) {
        uint32_t size;
        file.read((char *)&size, 4);
        file.seekg(size, file.cur);
        file.read(buf, 4);
    }

    uint32_t size;
    file.read((char *)&size, 4);
    if(size % 4 != 0) {
        std::cout << "Error: File ends with incomplete sample" << std::endl;
        exit(-1);
    }
    song_size = size;
    song_n_samples = size / 4;
    
    song_samples = new (std::nothrow) float[size / 4];
    if(song_samples == nullptr) {
        std::cout << "Error: Couldn't allocate buffer for song samples" << std::endl;
        exit(-1);
    }
    file.read((char *)song_samples, size);

    file.read(buf, 1);
    if(!file.eof())
        std::cout << "Trailing data is ignored" << std::endl;
}
