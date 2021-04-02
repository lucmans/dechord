
#include <SDL2/SDL.h>

#include <iostream>
#include <cstdlib>
#include <csignal>


const int SAMPLE_RATE = 96000 * 2;
const SDL_AudioFormat FORMAT = AUDIO_F32SYS;
const unsigned int N_CHANNELS = 1;
const unsigned int SAMPLES_PER_BUFFER = 512;


volatile int latency = 0;


volatile bool quit = false;
void signalHandler(const int signum) {
    std::cout << std::endl << "Quitting..." << std::endl;

    quit = true;
    return;

    // Prevent warning
    int i = signum; i++;
}


void read_buffer(SDL_AudioDeviceID &in_dev, float *buffer) {
    // long unsigned int read = 0;
    long unsigned int ret = 0;
    // while(read < SAMPLES_PER_BUFFER * sizeof(float)) {
    while(ret == 0) {
        // long unsigned int ret = SDL_DequeueAudio(in_dev, buffer + (read / sizeof(float)), (SAMPLES_PER_BUFFER * sizeof(float)) - read);
        /*long unsigned int */ret = SDL_DequeueAudio(in_dev, buffer, SAMPLES_PER_BUFFER * sizeof(float));
        // if(ret > (SAMPLES_PER_BUFFER * sizeof(float)) - read) {
        //     std::cout << "Error! Read too big!\n" << std::endl;
        //     exit(EXIT_FAILURE);
        // }
        // else if(ret % sizeof(float) != 0) {
        //     std::cout << "Error! Read part of a float\n" << std::endl;
        //     exit(EXIT_FAILURE);
        // }

        if(ret != SAMPLES_PER_BUFFER * sizeof(float) && ret != 0) {
            std::cout << "Error" << std::endl
                      << ret << std::endl;
            exit(EXIT_FAILURE);
        }

        // read += ret;
    }
}

void play_buffer(SDL_AudioDeviceID &out_dev, float *buffer) {
    if(SDL_QueueAudio(out_dev, buffer, SAMPLES_PER_BUFFER * sizeof(float)) != 0) {
        printf("Can't queue audio: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}


void handle_input(SDL_AudioDeviceID &in_dev, SDL_AudioDeviceID &out_dev) {
    SDL_Event e;
    float buffer[SAMPLES_PER_BUFFER] = {};
    float buffer2[SAMPLES_PER_BUFFER];
    while(SDL_PollEvent(&e)) {
        switch(e.type) {
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        quit = true;
                        break;

                    case SDLK_EQUALS:
                        latency += 1;
                        play_buffer(out_dev, buffer);
                        break;

                    case SDLK_MINUS:
                        if(latency > 0)
                            latency -= 1;
                        read_buffer(in_dev, buffer2);
                        break;
                }
                break;

            case SDL_WINDOWEVENT:
                switch(e.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        quit = true;
                        break;

                    // case SDL_WINDOWEVENT_SIZE_CHANGED:
                    //     graphics.resize_window(e.window.data1, e.window.data2);
                    //     break;
                }
                break;

            // case SDL_AUDIODEVICEADDED:
            // case SDL_AUDIODEVICEREMOVED:
            //     std::cout << "Warning: Audio devices may have changed!" << std::endl;
            //     std::cout << SDL_GetNumAudioDevices(0) << std::endl;
            //     std::cout << SDL_GetNumAudioDevices(1) << std::endl;
            //     break;
        }
    }
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

int main() {
    signal(SIGINT, signalHandler);

    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize!\nSDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    std::cout << "Using audio driver: " << SDL_GetCurrentAudioDriver() << std::endl << std::endl;
    SDL_Window *window = SDL_CreateWindow("latency", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_UTILITY | SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

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

    if(have.samples != SAMPLES_PER_BUFFER) {
        std::cout << "Error" << std::endl;
        exit(EXIT_FAILURE);
    }

    float *buffer = new (std::nothrow) float[SAMPLES_PER_BUFFER];
    if(buffer == nullptr) {
        std::cout << "Error" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Note that this latency is only added artificial latency! We are still limited by the buffer size etc." << std::endl;
    std::cout << "Delay can be changed using the equals/plus and minus keys" << std::endl;
    SDL_PauseAudioDevice(in_dev, 0);
    SDL_PauseAudioDevice(out_dev, 0);
    while(!quit) {
        handle_input(in_dev, out_dev);
        std::cout << "Current latency: " << (double)latency * ((double)SAMPLES_PER_BUFFER / (double)SAMPLE_RATE) * 1000.0 << " ms         \r";
        std::flush(std::cout);

        read_buffer(in_dev, buffer);
        play_buffer(out_dev, buffer);
    }
    std::cout << std::endl;

    SDL_CloseAudioDevice(in_dev);
    SDL_CloseAudioDevice(out_dev);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
