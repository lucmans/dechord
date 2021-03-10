
#include "fourier.h"
#include "config.h"
#include "graphics.h"
#include "gensound.h"
#include "find_peaks.h"

#include <omp.h>
#include <fftw3.h>
#include <SDL2/SDL.h>

#include <iostream>
#include <cmath>

#include <vector>
#include <string>


const char *notes[12] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};


struct ThreadReturnData {
    int max_idx;
    double norm;
};


int freq = 1000;
void handle_input(Graphics &graphics, double &max_norm) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        switch(e.type) {
            case SDL_QUIT:
                set_quit();
                break;

            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        set_quit();
                        break;

                    case SDLK_EQUALS:
                        freq += 10;
                        break;

                    case SDLK_MINUS:
                        freq -= 10;
                        break;

                    case SDLK_r:
                        max_norm = 1.0;
                        break;
                }
                break;

            case SDL_WINDOWEVENT:
                switch(e.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        set_quit();
                        break;

                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        graphics.resize_window(e.window.data1, e.window.data2);
                        break;
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


int max_element(const fftwf_complex values[], double norms[(WINDOW_SAMPLES / 2) + 1]) {
    int max_idx = -1;
    double max_value = -1.0;

    for(int i = 1; i < (WINDOW_SAMPLES / 2) + 1; i++) {
        norms[i] = sqrt((values[i][0] * values[i][0]) + (values[i][1] * values[i][1]));
        if(norms[i] > max_value) {
            max_value = norms[i];
            max_idx = i;
        }
    }

    return max_idx;
}


// Cut of values below hz frequency
int low_pass(int hz) {
    return hz / ((double)SAMPLE_RATE / WINDOW_SAMPLES);
}


void read_window(SDL_AudioDeviceID &in_dev, float *in) {

    // return;


    if(settings.generate_sine) {
        write_sine(freq, in, 0, WINDOW_SAMPLES);
        return;
    }

    uint32_t read = 0;
    while(read < WINDOW_SAMPLES * sizeof(float)) {
        uint32_t ret = SDL_DequeueAudio(in_dev, in + (read / sizeof(float)), (WINDOW_SAMPLES * sizeof(float)) - read);
        if(ret > (WINDOW_SAMPLES * sizeof(float)) - read) {
            printf("Error! Read too big!\n");
            exit(EXIT_FAILURE);
        }
        else if(ret % 4 != 0) {
            printf("Error! Read part of a float\n");
            exit(EXIT_FAILURE);
        }

        read += ret;
    }
}


// TODO: Optimize by pre-calulating values for every i < WINDOW_SAMPLES and storing in array
inline double window_func(const int i) {
    // No window function
    // return 1.0;

    const double N = WINDOW_SAMPLES;
    const double x = i * M_PI;

    // Hamming window
    // const double a0 = 25.0 / 46.0;
    // return a0 - ((1.0 - a0) * cos((2.0 * x) / N));

    // Hann window
    // return sin(x / N) * sin(x / N);

    // Blackman window
    // const double a0 = 7938.0 / 18608.0;
    // const double a1 = 9240.0 / 18608.0;
    // const double a2 = 1430.0 / 18608.0;
    // return a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N));

    // Nuttall window
    const double a0 = 0.355768;
    const double a1 = 0.487396;
    const double a2 = 0.144232;
    const double a3 = 0.012604;
    return a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

    // Blackman-Nuttall window
    // const double a0 = 0.3635819;
    // const double a1 = 0.4891775;
    // const double a2 = 0.1365995;
    // const double a3 = 0.0106411;
    // return a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

    // Blackman-Harris window
    // const double a0 = 0.35875;
    // const double a1 = 0.48829;
    // const double a2 = 0.14128;
    // const double a3 = 0.01168;
    // return a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

    // Flat top window
    // const double a0 = 0.21557895;
    // const double a1 = 0.41663158;
    // const double a2 = 0.277263158;
    // const double a3 = 0.083578947;
    // const double a4 = 0.006947368;
    // return a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N)) + (a4 * cos((8.0 * x) / N));
}


void fourier(SDL_AudioDeviceID &in_dev, Graphics &graphics, SDL_AudioDeviceID &out_dev) {
    // Init fftw
    float *in = (float*)fftwf_malloc(WINDOW_SAMPLES * sizeof(float));
    fftwf_complex *out = (fftwf_complex*)fftwf_malloc(((WINDOW_SAMPLES / 2) + 1) * sizeof(fftwf_complex));
    fftwf_plan p = fftwf_plan_dft_r2c_1d(WINDOW_SAMPLES, in, out, FFTW_ESTIMATE);

    std::cout << "Fourier error: " << SAMPLE_RATE / (double)WINDOW_SAMPLES << "Hz" << std::endl << std::endl;

    // Main loop
    reset_quit();
    double max_norm = 1.0;  // Used for coloring the graph
    SDL_PauseAudioDevice(in_dev, 0);  // Unpause device
    // ThreadReturnData tr_data[omp_get_num_procs()];
    // std::vector<float> buffer;  // TODO: Remove; this is for saving all recorded audio
    while(!poll_quit()) {
        // Fill input (window) with samples
        read_window(in_dev, in);

        // TODO: Remove; this is for saving all recorded audio
        // buffer.reserve(buffer.size() + WINDOW_SAMPLES);
        // for(int i = 0; i < WINDOW_SAMPLES; i++)
        //     buffer.push_back(in[i]);

        // Apply window function to minimize spectral leakage
        for(int i = 0; i < WINDOW_SAMPLES; i++)
            in[i] *= window_func(i);

        // Do the actual transform
        fftwf_execute(p);

        // Determine loudest frequency
        double norms[(WINDOW_SAMPLES / 2) + 1];
        int max_idx = max_element(out, norms);

        // Onset detection
        double power = 0.0;
        for(int i = 1; i < (WINDOW_SAMPLES / 2) + 1; i++)
            power += norms[i];

        double envelope[(WINDOW_SAMPLES / 2) + 1];
        calc_envelope(norms, envelope);

        // Only find peaks if note is played
        std::vector<int> peaks;
        find_peaks(norms, envelope, peaks);
        // if(power > 20.0) {
        //     std::vector<float> f_norms(begin(norms), end(norms));
        //     Peaks::findPeaks(f_norms, peaks);
        // }

        // Print results
        // double f = ((double)SAMPLE_RATE / WINDOW_SAMPLES) * max_idx;
        double f = ((double)SAMPLE_RATE / WINDOW_SAMPLES) * interpolate_max(max_idx, norms);
        // std::cout << notes[((int)round(fmod(12 * log2(f / A4), 12)) + 12) % 12] << "    "
        //           << f << " Hz ±" << ((double)SAMPLE_RATE / WINDOW_SAMPLES) / 2.0
        //           << "    amp " << norms[max_idx]
        //           << "    power " << power
        //           << (settings.generate_sine ? "    playing " + std::to_string(freq) + " Hz" : "")
        //           << (!settings.generate_sine ? "    queue: " + std::to_string(SDL_GetQueuedAudioSize(in_dev) / WINDOW_SAMPLES) : "")
        //           << "              "  << '\r';
        // std::flush(std::cout);

        // Do front-end stuff
        if(norms[max_idx] > max_norm)  // Set the higher recorded value for coloring the graph
            max_norm = norms[max_idx];

        // Waterfall plot
        // graphics.add_line(norms, max_norm, low_pass(3000), peaks);
        // graphics.add_line(norms, max_norm, (WINDOW_SAMPLES / 2) + 1, {max_idx});

        // Normal graph
        graphics.graph_spectrogram(norms, max_norm, low_pass(2500), peaks, envelope);
        // graphics.graph_spectrogram(norms, max_norm, (WINDOW_SAMPLES / 2) + 1);

        handle_input(graphics, max_norm);
    }
    std::cout << std::endl;

    // TODO: Remove; this is for playing back all recorded audio
    // SDL_PauseAudioDevice(out_dev, 0);
    // if(SDL_QueueAudio(out_dev, &buffer[0], buffer.size() * sizeof(float)) != 0) {
    //     printf("Can't queue audio: %s\n", SDL_GetError());
    //     exit(EXIT_FAILURE);
    // }
    // SDL_Delay(1000 * ((double)buffer.size() / SAMPLE_RATE));

    fftwf_destroy_plan(p);
    fftwf_free(in); fftwf_free(out);

    return;


    // Prevent warning
    SDL_PauseAudioDevice(out_dev, 1);
}


    // Code from other project (fraccert) for reference of basic OMP threading code
    // #pragma omp parallel num_threads(cores)
    // {
    //     while(true) {
    //         int blocknum;
    //         #pragma omp critical
    //         {
    //            blocknum = lastBlock;
    //            lastBlock++;
    //         }
    //         if(blocknum >= totalBlocks)
    //             break;

    //         calcScreen(domain, res, blocks[blocknum], data, sharedPixels);
    //     }
    // }