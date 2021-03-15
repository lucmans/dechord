
#ifndef CONFIG_H
#define CONFIG_H


#include <SDL2/SDL.h>


// Audio related configs
const int SAMPLE_RATE = 96000 * 2;
const int WINDOW_SAMPLES = 1024 * 16/* * 2*/;


const SDL_AudioFormat FORMAT = AUDIO_F32SYS;
const unsigned int N_CHANNELS = 1;
const unsigned int SAMPLES_PER_BUFFER = 512;


// Central A in Hz
const double A4 = 440.0;


// Graphics related configs
const int DEFAULT_RES[2] = {1024, 768};


// Struct with all settings that can be changed through CLI arguments
struct Settings {
    bool test_tone = false;
    bool generate_sine = false;
};
extern Settings settings;


// Functions relating to signals and gracefully quitting through interrupt
void signalHandler(const int signum);

bool poll_quit();
void set_quit();
void reset_quit();


#endif  // CONFIG_H
