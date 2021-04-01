
#include "gensound.h"
#include "fourier.h"
#include "config.h"
#include "graphics.h"
#include "note_set.h"
#include "music_file.h"

#include <omp.h>
#include <SDL2/SDL.h>

#include <iostream>
#include <iomanip>
#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <csignal>
#include <string>

#include <limits>
#include <ios>


void play_test_tone(SDL_AudioDeviceID &out_dev) {
    const int samples = SAMPLE_RATE * 4;
    float wave[samples];

    std::cout << "Playing 440Hz and then 400Hz" << std::endl << std::endl;

    write_sinef(440, wave, 0, (samples / 2));
    write_sinef(400, wave, (samples / 2), samples);

    SDL_PauseAudioDevice(out_dev, 0);
    if(SDL_QueueAudio(out_dev, wave, samples * sizeof(float)) < 0) {
        printf("Can't queue audio: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}


void print_overtones(std::string note_string, const int n = 5) {
    int note_distance = 0,
        octave_distance = 0;
    int modifier = 0;
    if(note_string[1] == '#')
        modifier = 1;
    else if(note_string[1] == 'b')
        modifier = -1;

    switch(tolower(note_string[0])) {
        case 'b':
            note_distance += 2;
            __attribute__ ((fallthrough));
        case 'a':
            note_distance += 2;
            __attribute__ ((fallthrough));
        case 'g':
            note_distance += 2;
            __attribute__ ((fallthrough));
        case 'f':
            note_distance += 1;
            __attribute__ ((fallthrough));
        case 'e':
            note_distance += 2;
            __attribute__ ((fallthrough));
        case 'd':
            note_distance += 2;
            __attribute__ ((fallthrough));
        case 'c':
            break;

        default:
            std::cout << "Error: Incorrect note" << std::endl;
            exit(-1);
    }
    note_distance += modifier;

    try {
        if(modifier == 0)
            octave_distance = std::stoi(note_string.substr(1)) - 1;
        else
            octave_distance = std::stoi(note_string.substr(2)) - 1;
    }
    catch(...) {
        std::cout << "Error: Incorrect note format" << std::endl;
        exit(-1);
    }

    const double C1 = A4 * exp2(-45.0 / 12.0);
    const double freq = C1 * exp2(octave_distance) * exp2((double)note_distance / 12.0);

    std::cout << std::left << std::setw(12) << "f_harmonic" << std::setw(14) << "closest note" << std::setw(11) <<  "f_closest" << std::setw(11) << "cent error" << std::endl << std::right;
    for(int i = 1; i <= n; i++) {
        Note note(freq * i, 0);
        std::cout << std::fixed << std::setprecision(3) << std::setw(10) << freq * i << std::setw(13) << note << std::setw(11) << A4 * exp2(round(12.0 * log2((freq * i) / A4)) / 12.0) << std::setw(12) << note.error << std::endl;
    }
    std::cout << std::endl;
}

void print_audio_settings(SDL_AudioSpec &specs, bool input) {
    std::cout << "Audio " << (input ? "input" : "output") << " config:" << std::endl
              << "Sample rate: " << SAMPLE_RATE << " " << specs.freq << std::endl
              << "Format: " << FORMAT << " " << specs.format << std::endl
              << "Channels: " << N_CHANNELS << " " << specs.channels << std::endl
              << "Samples per buffer: " << SAMPLES_PER_BUFFER << " " << specs.samples << std::endl
              << "Buffer size:  -  " << specs.size << " bytes" << std::endl
              << "Silence value:  -  " << specs.silence << std::endl
              << std::endl;
}

void parse_args(int argc, char *argv[]) {
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-f") == 0 && argc > i + 1) {
            settings.play_file = true;
            load_file(argv[i + 1]);
            std::cout << "Finished reading samples from " << argv[i + 1] << std::endl;

            i++;
        }
        else if(strcmp(argv[i], "-o") == 0 && argc > i + 1) {
            std::cout << "File output not implemented" << std::endl;
            i++;
        }
        else if(strcmp(argv[i], "-p") == 0 && argc > i + 1) {
            if(argc > i + 2) {
                int n;
                try {
                    n = std::stoi(argv[i + 2]);
                }
                catch(...) {
                    std::cout << "Error: Incorrect note formatting" << std::endl;
                    exit(-1);
                }
                print_overtones(argv[i + 1], n);
            }
            else
                print_overtones(argv[i + 1]);
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[i], "-s") == 0)
            settings.generate_sine = true;
        else if(strcmp(argv[i], "-t") == 0)
            settings.test_tone = true;
        else {
            if(strcmp(argv[i], "-h") != 0 && strcmp(argv[i], "--help") != 0)
                std::cout << "Incorrect usage.\n" << std::endl;

            std::cout << "Flags:\n"
                      // << "  -c    - Compute "
                      << "  -f <file>     - Get samples from file (.wav) instead of input device"
                      << "  -o <file>     - File to which the output gets written"
                      << "  -p <note> <n> - Print n overtones of note\n"
                      << "  -s            - Generate sine instead of listening to input device\n"
                      << "  -t            - Play test sound over speakers\n"
                      << std::endl;
            exit(EXIT_SUCCESS);
        }
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signalHandler);

    parse_args(argc, argv);

    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize!\nSDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    std::cout << "Using audio driver: " << SDL_GetCurrentAudioDriver() << std::endl << std::endl;
    Graphics graphics;

    // Print available audio devices
    int count = SDL_GetNumAudioDevices(0);
    std::cout << "Playback devices:" << std::endl;
    for(int i = 0; i < count; i++)
        printf("  Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    std::cout << std::endl;

    count = SDL_GetNumAudioDevices(1);
    std::cout << "Recording devices:" << std::endl;
    for(int i = 0; i < count; i++)
        printf("  Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    std::cout << std::endl;

    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = SAMPLE_RATE;
    want.format = FORMAT;
    want.channels = N_CHANNELS;
    want.samples = SAMPLES_PER_BUFFER;
    want.callback = NULL;

    SDL_AudioDeviceID out_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0/*SDL_AUDIO_ALLOW_ANY_CHANGE*/);
    if(out_dev == 0) {
        printf("Failed to open audio: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    print_audio_settings(have, false);

    SDL_AudioDeviceID in_dev = SDL_OpenAudioDevice(NULL, 1, &want, &have, 0/*SDL_AUDIO_ALLOW_ANY_CHANGE*/);
    if(in_dev == 0) {
        printf("Failed to open audio: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    print_audio_settings(have, true);

    if(settings.test_tone)
        play_test_tone(out_dev);

    std::cout << "Cores: " << omp_get_num_procs() << std::endl
              << "Samples per window: " << WINDOW_SAMPLES << std::endl
              << "Window length: " << (double)WINDOW_SAMPLES / SAMPLE_RATE << 's' << std::endl;

    fourier(in_dev, graphics);

    SDL_CloseAudioDevice(in_dev);
    SDL_CloseAudioDevice(out_dev);
    SDL_Quit();

    return EXIT_SUCCESS;
}
