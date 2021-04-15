
#ifndef CONFIG_H
#define CONFIG_H


#include <SDL2/SDL.h>


// Audio related configs
const int SAMPLE_RATE = 96000 * 2;
const int FRAME_SIZE = 1024 * 16 /** 2*/;  // Number of samples in Fourier frame

// Central A in Hz
const double A4 = 440.0;

// Threshold of channel power before finding peaks
const double POWER_THRESHOLD = 15.0;

// Gaussian average settings (for peak picking)
const int KERNEL_WIDTH = 47;  // Choose odd value
const double SIGMA = 1.2;  // Higher values of sigma make values close to kernel center weight more

// Error in cents that an detected overtone may have compared to the theoretical overtone
const double OVERTONE_ERROR = 10.0;

// Frequency range of guitar
const double MIN_FREQ = 79.0;
const double MAX_FREQ = 1350.0;


// Audio driver config
const SDL_AudioFormat FORMAT = AUDIO_F32SYS;
const unsigned int N_CHANNELS = 1;
const unsigned int SAMPLES_PER_BUFFER = 512;


// Graphics related configs
const int DEFAULT_RES[2] = {1024, 768};


// Struct with all settings that can be changed through CLI arguments
struct Settings {
    bool test_tone = false;
    bool generate_sine = false;

    bool play_file = false;

    bool headless = false;
};
extern Settings settings;


// Functions relating to signals and gracefully quitting through interrupt
void signalHandler(const int signum);

bool poll_quit();
void set_quit();
void reset_quit();


#endif  // CONFIG_H
