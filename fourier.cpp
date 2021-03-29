
#include "fourier.h"
#include "config.h"
#include "graphics.h"
#include "gensound.h"
#include "find_peaks.h"
#include "note_set.h"

#include <omp.h>
#include <fftw3.h>
#include <SDL2/SDL.h>

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>


typedef std::chrono::duration<double, std::milli> duration_t;  // Omit ", std::milli" for second


static int freq = 1000;
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


// Also return the index of the maximum norm
int calc_norms(const fftwf_complex values[], double norms[(WINDOW_SAMPLES / 2) + 1]) {
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
    if(settings.generate_sine) {
        write_sinef(freq, in, 0, WINDOW_SAMPLES);
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
    // const double a0 = 0.355768;
    // const double a1 = 0.487396;
    // const double a2 = 0.144232;
    // const double a3 = 0.012604;
    // return a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

    // Blackman-Nuttall window
    const double a0 = 0.3635819;
    const double a1 = 0.4891775;
    const double a2 = 0.1365995;
    const double a3 = 0.0106411;
    return a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

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


void fourier(SDL_AudioDeviceID &in_dev, Graphics &graphics) {
    // Init fftw
    float *in = (float*)fftwf_malloc(WINDOW_SAMPLES * sizeof(float));
    fftwf_complex *out = (fftwf_complex*)fftwf_malloc(((WINDOW_SAMPLES / 2) + 1) * sizeof(fftwf_complex));
    fftwf_plan p = fftwf_plan_dft_r2c_1d(WINDOW_SAMPLES, in, out, FFTW_ESTIMATE);

    std::cout << "Fourier bin size: " << SAMPLE_RATE / (double)WINDOW_SAMPLES << "Hz" << std::endl << std::endl;

    // Main loop
    reset_quit();
    double max_norm = 1.0;  // Used for coloring the graph
    SDL_PauseAudioDevice(in_dev, 0);  // Unpause device
    std::chrono::steady_clock::time_point start, end;  // Measures time between receiving WINDOW_SAMPLES samples from audio driver and outputting the answer
    while(!poll_quit()) {
        // Fill input (window) with samples
        read_window(in_dev, in);

        start = std::chrono::steady_clock::now();

        // Apply window function to minimize spectral leakage
        for(int i = 0; i < WINDOW_SAMPLES; i++)
            in[i] *= window_func(i);

        // Do the actual transform
        fftwf_execute(p);
        // end = std::chrono::steady_clock::now();
        // std::cout << std::chrono::duration_cast<duration_t>(end - start).count() << " ms" << std::endl
        //           << std::endl;

        // Calculate the norms to get amplitude
        double norms[(WINDOW_SAMPLES / 2) + 1];
        int max_idx = calc_norms(out, norms);

        // Onset detection
        double power = 0.0;
        for(int i = 1; i < (WINDOW_SAMPLES / 2) + 1; i++)
            power += norms[i];

        // Peak picking
        // Calculating the envelope is done here, because graphics will also need it
        double envelope[(WINDOW_SAMPLES / 2) + 1];
        calc_envelope(norms, envelope);

        std::vector<int> peaks;
        // TODO: Only find peaks if note is played
        if(power > POWER_THRESHOLD)
            find_peaks(norms, envelope, peaks);

        // Use the found peaks to find the played notes
        NoteSet noteset(norms, peaks);

        // Print results
        // const Note *note_loud = noteset.get_loudest_peak();
        // if(note_loud != nullptr)
        //     std::cout << "Loud " << *note_loud << "  " << note_loud->freq << "  " << note_loud->amp << std::endl;

        // const Note *note_low = noteset.get_lowest_peak();
        // if(note_low != nullptr)
        //     std::cout << "Low " << *note_low << "  " << note_low->freq << "  " << note_low->amp << std::endl;

        // const Note *note_like = noteset.get_likeliest_note();
        // if(note_like != nullptr)
        //     std::cout << std::fixed << std::setprecision(2) << "Likeliest " << *note_like << "   freq: " << note_like->freq << "   amp: " << note_like->amp << "   cent off " << note_like->error << std::endl;

        std::vector<const Note*> likely;
        noteset.get_likely_notes(likely);
        print_notevec(likely);

        // std::cout << noteset << std::endl;
        // std::cout << std::endl;

        end = std::chrono::steady_clock::now();
        std::cout << std::chrono::duration_cast<duration_t>(end - start).count() << " ms" << std::endl
                  << std::endl;

        // double amp;
        // // double f = ((double)SAMPLE_RATE / WINDOW_SAMPLES) * max_idx;
        // // double f = ((double)SAMPLE_RATE / WINDOW_SAMPLES) * interpolate_max(max_idx, norms);
        // const double f = ((double)SAMPLE_RATE / WINDOW_SAMPLES) * interpolate_max(peaks[0], norms, amp);
        // Note note(f, amp);
        // std::cout << note << "   "
        //           << f << " Hz ±" << ((double)SAMPLE_RATE / WINDOW_SAMPLES) / 2.0
        //           << "    amp " << amp
        //           << "    power " << power
        //           << (settings.generate_sine ? "    playing " + std::to_string(freq) + " Hz" : "")
        //           << (!settings.generate_sine ? "    queue: " + std::to_string(SDL_GetQueuedAudioSize(in_dev) / WINDOW_SAMPLES) : "")
        //           << "              "  << '\r';
        // std::flush(std::cout);

        // Do front-end stuff
        if(norms[max_idx] > max_norm)  // Set the higher recorded value for graph limits
            max_norm = norms[max_idx];

        // Waterfall plot
        // graphics.add_line(norms, max_norm, peaks, low_pass(2000));

        // Normal graph
        graphics.graph_spectrogram(norms, max_norm, peaks, envelope, low_pass(2500));

        handle_input(graphics, max_norm);
    }
    std::cout << std::endl;

    fftwf_destroy_plan(p);
    fftwf_free(in); fftwf_free(out);
}
