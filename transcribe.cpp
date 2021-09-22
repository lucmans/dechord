
#include "transcribe.h"
#include "config.h"
#include "graphics.h"
#include "gensound.h"
#include "find_peaks.h"
#include "note_set.h"
#include "music_file.h"

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
static bool playback = false;
void handle_input(Graphics *graphics, double &max_norm, bool &waterfall, bool &monophonic) {
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

                    case SDLK_s:
                        waterfall = !waterfall;
                        break;

                    case SDLK_m:
                        monophonic = true;
                        break;

                    case SDLK_p:
                        monophonic = false;
                        break;

                    case SDLK_k:
                        playback = !playback;
                        break;
                }
                break;

            case SDL_WINDOWEVENT:
                switch(e.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        set_quit();
                        break;

                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        if(!settings.headless)
                            graphics->resize_window(e.window.data1, e.window.data2);
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
int calc_norms(const fftwf_complex values[], double norms[(FRAME_SIZE / 2) + 1], double &power) {
    int max_idx = -1;
    double max_value = -1.0;

    for(int i = 1; i < (FRAME_SIZE / 2) + 1; i++) {
        norms[i] = sqrt((values[i][0] * values[i][0]) + (values[i][1] * values[i][1]));
        power += norms[i];

        if(norms[i] > max_value) {
            max_value = norms[i];
            max_idx = i;
        }
    }

    return max_idx;
}


// Cut of values below hz frequency
int low_pass(int hz) {
    return hz / ((double)SAMPLE_RATE / FRAME_SIZE);
}


void read_window(SDL_AudioDeviceID &in_dev, float *in) {
    if(settings.generate_sine) {
        write_sinef(freq, in, 0, FRAME_SIZE);
        return;
    }

    if(settings.play_file) {
        if(!file_get_samples(in, FRAME_SIZE)) {
            std::cout << "Finished playing file; quitting after this frame" << std::endl;
            set_quit();
        }

        // SDL_Delay(1000.0 * (double)FRAME_SIZE / (double)SAMPLE_RATE);
        return;
    }

    uint32_t read = 0;
    while(read < FRAME_SIZE * sizeof(float)) {
        uint32_t ret = SDL_DequeueAudio(in_dev, in + (read / sizeof(float)), (FRAME_SIZE * sizeof(float)) - read);
        if(ret > (FRAME_SIZE * sizeof(float)) - read) {
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


void window_func(double window[FRAME_SIZE]) {
    const double N = FRAME_SIZE;
    for(int i = 0; i < FRAME_SIZE; i++) {
        const double x = i * M_PI;

        // No window function
        // window[i] = 1.0;

        // Hamming window
        // const double a0 = 25.0 / 46.0;
        // window[i] = a0 - ((1.0 - a0) * cos((2.0 * x) / N));

        // Hann window
        // window[i] = sin(x / N) * sin(x / N);

        // Blackman window
        // const double a0 = 7938.0 / 18608.0;
        // const double a1 = 9240.0 / 18608.0;
        // const double a2 = 1430.0 / 18608.0;
        // window[i] = a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N));

        // Nuttall window
        // const double a0 = 0.355768;
        // const double a1 = 0.487396;
        // const double a2 = 0.144232;
        // const double a3 = 0.012604;
        // window[i] = a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

        // Blackman-Nuttall window
        const double a0 = 0.3635819;
        const double a1 = 0.4891775;
        const double a2 = 0.1365995;
        const double a3 = 0.0106411;
        window[i] = a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

        // Blackman-Harris window
        // const double a0 = 0.35875;
        // const double a1 = 0.48829;
        // const double a2 = 0.14128;
        // const double a3 = 0.01168;
        // window[i] = a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N));

        // Flat top window
        // const double a0 = 0.21557895;
        // const double a1 = 0.41663158;
        // const double a2 = 0.277263158;
        // const double a3 = 0.083578947;
        // const double a4 = 0.006947368;
        // window[i] = a0 - (a1 * cos((2.0 * x) / N)) + (a2 * cos((4.0 * x) / N)) - (a3 * cos((6.0 * x) / N)) + (a4 * cos((8.0 * x) / N));
    }
}


void transcribe(SDL_AudioDeviceID &in_dev, Graphics *graphics, SDL_AudioDeviceID &out_dev) {
    // Init fftw
    float *in = (float*)fftwf_malloc(FRAME_SIZE * sizeof(float));
    fftwf_complex *out = (fftwf_complex*)fftwf_malloc(((FRAME_SIZE / 2) + 1) * sizeof(fftwf_complex));
    fftwf_plan p = fftwf_plan_dft_r2c_1d(FRAME_SIZE, in, out, FFTW_ESTIMATE);

    double window[FRAME_SIZE];
    window_func(window);

    std::cout << "Fourier bin size: " << SAMPLE_RATE / (double)FRAME_SIZE << "Hz" << std::endl << std::endl;

    // Measures time between receiving FRAME_SIZE samples from audio driver and outputting the answer
    std::chrono::steady_clock::time_point start, end;
    // Other clocks
    std::chrono::steady_clock::time_point transform_start, transform_end,
                                          peak_start, peak_end,
                                          noteset_start, noteset_end;

    // Main loop
    reset_quit();
    double max_norm = 1.0;  // Used for coloring the graph
    bool waterfall = false;  // Plot waterfall or spectrogram
    bool monophonic = true;  // Polyphonic or monophonic transcription
    SDL_PauseAudioDevice(in_dev, 0);  // Unpause device
    SDL_PauseAudioDevice(out_dev, 0);  // Unpause device
    while(!poll_quit()) {
        // Fill input (window) with samples
        read_window(in_dev, in);
        if(playback)
            SDL_QueueAudio(out_dev, in, FRAME_SIZE * sizeof(float));

        start = std::chrono::steady_clock::now();

        // Transform
        transform_start = std::chrono::steady_clock::now();
        // Apply window function to minimize spectral leakage
        for(int i = 0; i < FRAME_SIZE; i++)
            in[i] *= window[i];

        // Do the actual transform
        fftwf_execute(p);

        // Calculate the norms to get amplitude
        double norms[(FRAME_SIZE / 2) + 1];
        double power = 0.0;
        int max_idx = calc_norms(out, norms, power);
        transform_end = std::chrono::steady_clock::now();

        // Peak picking
        peak_start = std::chrono::steady_clock::now();
        // Calculating the envelope is done here, because graphics will also need it
        double envelope[(FRAME_SIZE / 2) + 1];
        calc_envelope(norms, envelope);

        std::vector<int> peaks;
        // TODO: Only find peaks if note is played
        if(power > POWER_THRESHOLD)
            find_peaks(norms, envelope, peaks);
        peak_end = std::chrono::steady_clock::now();

        // Note selection
        noteset_start = std::chrono::steady_clock::now();
        // Use the found peaks to find the played notes
        const NoteSet noteset(norms, peaks);

        if(monophonic) {
            // Monophonic note recognition
            const Note *note_like = noteset.get_likeliest_note();

            noteset_end = std::chrono::steady_clock::now();
            end = std::chrono::steady_clock::now();

            // Print results
            if(note_like != nullptr)
                in_range(note_like);
            
            if(note_like != nullptr)
                std::cout << std::fixed << std::setprecision(2) << "Likeliest " << *note_like << "   freq: " << note_like->freq << "   amp: " << note_like->amp << "   cent off " << note_like->error << std::setprecision(6) << std::endl;
            else
                std::cout << "Likeliest -" << std::endl;
        }
        else {
            // Polyphonic note recognition
            std::vector<const Note*> likely;
            noteset.get_likely_notes(likely);
            in_range(likely);  // Remove all fundamental candidates which are outside of the dynamic range of a guitar
            
            noteset_end = std::chrono::steady_clock::now();
            end = std::chrono::steady_clock::now();

            // Print results
            print_notevec(likely);
            std::cout << std::endl;
        }

        // Other kinds of result printing
        // std::cout << noteset << std::endl;
        // std::cout << std::endl;

        // int n_notes = noteset.get_notes()->size();
        // std::cout << "Transform: " << std::chrono::duration_cast<duration_t>(transform_end - transform_start).count() << " ms" << std::endl
        //           << "Peak: " << std::chrono::duration_cast<duration_t>(peak_end - peak_start).count() << " ms" << std::endl
        //           << "Noteset: " << std::chrono::duration_cast<duration_t>(noteset_end - noteset_start).count() << " ms" << std::endl
        //           << "Total: " << std::chrono::duration_cast<duration_t>(end - start).count() << " ms" << std::endl
        //           // << "#overtones filtered: " << n_notes - likely.size() << '/' << n_notes << "  (" << 100.0 * (double)(n_notes - likely.size()) / (double)n_notes << "%)" << std::endl
        //           << std::endl;

        // double amp;
        // // double f = ((double)SAMPLE_RATE / FRAME_SIZE) * max_idx;
        // // double f = ((double)SAMPLE_RATE / FRAME_SIZE) * interpolate_max(max_idx, norms);
        // const double f = ((double)SAMPLE_RATE / FRAME_SIZE) * interpolate_max(peaks[0], norms, amp);
        // Note note(f, amp);
        // std::cout << note << "   "
        //           << f << " Hz ±" << ((double)SAMPLE_RATE / FRAME_SIZE) / 2.0
        //           << "    amp " << amp
        //           << "    power " << power
        //           << (settings.generate_sine ? "    playing " + std::to_string(freq) + " Hz" : "")
        //           << (!settings.generate_sine ? "    queue: " + std::to_string(SDL_GetQueuedAudioSize(in_dev) / FRAME_SIZE) : "")
        //           << "              "  << '\r';
        // std::flush(std::cout);

        // SDL_QueueAudio(out_dev, in, FRAME_SIZE * sizeof(float));

        // std::string input;
        // std::cin >> input;
        // while(input != "n") {
        //     SDL_QueueAudio(out_dev, in, FRAME_SIZE * sizeof(float));

        //     std::cin >> input;
        // }

        // SDL_Delay(1000.0 * (double)FRAME_SIZE / (double)SAMPLE_RATE);

        // Do front-end stuff
        if(norms[max_idx] > max_norm)  // Set the higher recorded value for graph limits
            max_norm = norms[max_idx];

        if(!settings.headless) {
            // Update waterfall plot
            graphics->add_line(norms, max_norm, peaks, low_pass(2000));

            if(waterfall)
                graphics->graph_waterfall();
            else
                graphics->graph_spectrogram(norms, max_norm, peaks, envelope, low_pass(2500));

            handle_input(graphics, max_norm, waterfall, monophonic);
        }
    }
    std::cout << std::endl;

    fftwf_destroy_plan(p);
    fftwf_free(in); fftwf_free(out);
}
