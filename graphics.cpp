
#include "graphics.h"
#include "config.h"

#include <SDL2/SDL.h>

#include <vector>
#include <algorithm>
#include <iostream>


Graphics::Graphics() : res_x(DEFAULT_RES[0]), res_y(DEFAULT_RES[1]) {
    window = SDL_CreateWindow("dechord", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, res_x, res_y, SDL_WINDOW_UTILITY | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Set the texture that will hold the waterfall plot of frequency domain and fill with black pixels
    buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, res_x, res_y);
    SDL_SetRenderTarget(renderer, buffer);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    const SDL_Rect rect = {0, 0, res_x, res_y};
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderTarget(renderer, NULL);

    // Initial render
    SDL_RenderCopy(renderer, buffer, NULL, NULL);
    SDL_RenderPresent(renderer);

    line = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (FRAME_SIZE / 2) + 1, 1);
    if(line == NULL)
        printf("Warning: SDL could not create texture for line! Try compiling with a lower FRAME_SIZE (config.h)\nSDL Error: %s\n", SDL_GetError());
}

Graphics::~Graphics() {
    SDL_DestroyTexture(buffer);
    SDL_DestroyTexture(line);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}


uint32_t calc_color(const double data, const double max_value) {
    uint32_t rgba = 0xff;
    // const double t = log(data) / log(max_value);
    const double t = (data / max_value) * 0.8;

    rgba |= (uint8_t)(9 * (1 - t) * t * t * t * 255) << 24;  // r
    rgba |= (uint8_t)(15 * (1 - t) * (1 - t) * t * t * 255) << 16;  // g
    rgba |= (uint8_t)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255) << 8;  // b

    return rgba;
}

void Graphics::add_line(const double data[(FRAME_SIZE / 2) + 1], const double max_value,
                        const std::vector<int> &peaks, const int n_data_points = (FRAME_SIZE / 2) + 1) {
    if(line == NULL)
        return;

    uint32_t *pixels;
    int pitch;
    SDL_LockTexture(line, NULL, (void **)&pixels, &pitch);
    // Generate next line
    for(int i = 0; i < n_data_points; i++)
        pixels[i] = calc_color(data[i], max_value);

    // Color found peaks
    // for(unsigned int i = 0; i < peaks.size(); i++) {
    //     if(peaks[i] >= n_data_points)
    //         continue;
    //     // pixels[peaks[i]] = 0xff00ffff;
    //     pixels[peaks[i]] |= 0x7f7f7fff;
    // }
    SDL_UnlockTexture(line);

    // Shift all lines one place down
    const SDL_Rect repeat_src = {0, 0, res_x, res_y - 1};
    const SDL_Rect repeat_dst = {0, 1, res_x, res_y - 1};
    SDL_SetRenderTarget(renderer, buffer);
    SDL_RenderCopy(renderer, buffer, &repeat_src, &repeat_dst);

    // Draw new line
    const SDL_Rect line_src = {0, 0, n_data_points, 1};
    const SDL_Rect line_dst = {0, 0, res_x, 1};
    SDL_RenderCopy(renderer, line, &line_src, &line_dst);
    SDL_SetRenderTarget(renderer, NULL);
}

void Graphics::graph_waterfall() {
    if(line == NULL)
        return;

    // Render
    SDL_RenderCopy(renderer, buffer, NULL, NULL);
    SDL_RenderPresent(renderer);
}


void Graphics::graph_spectrogram(const double data[(FRAME_SIZE / 2) + 1], const double max_value,
                                 const std::vector<int> &peaks, const double envelope[(FRAME_SIZE / 2) + 1],
                                 const int n_data_points = (FRAME_SIZE / 2) + 1) {
    // TODO: Remove this memory leak
    static SDL_Texture *graf = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, res_x, res_y);
    SDL_SetRenderTarget(renderer, graf);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderFillRect(renderer, NULL);

    // Color found peaks
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xff, 0xff);
    for(unsigned int i = 0; i < peaks.size(); i++) {
        if(peaks[i] >= n_data_points)
            continue;
        SDL_RenderDrawLine(renderer, peaks[i], 0, peaks[i], res_y);
    }

    // Plot envelope
    SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);
    int prev_y = res_y - ((log10(envelope[0] + 1) / log10(max_value + 1)) * res_y);
    // int prev_y = res_y - ((envelope[0] / max_value) * res_y);
    // for(int i = 1; i < (FRAME_SIZE / 2) + 1; i++) {
    for(int i = 1; i < n_data_points; i++) {
        int y = res_y - ((log10(envelope[i] + 1) / log10(max_value + 1)) * res_y);
        // int y = res_y - ((envelope[i] / max_value) * res_y);
        SDL_RenderDrawLine(renderer, i - 1, prev_y, i, y);
        prev_y = y;
    }

    // Plot line of spectrum
    SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, 0xff);
    prev_y = res_y - ((log10(data[0] + 1) / log10(max_value + 1)) * res_y);
    // prev_y = res_y - ((data[0] / max_value) * res_y);
    // for(int i = 1; i < (FRAME_SIZE / 2) + 1; i++) {
    for(int i = 1; i < n_data_points; i++) {
        int y = res_y - ((log10(data[i] + 1) / log10(max_value + 1)) * res_y);
        // int y = res_y - ((data[i] / max_value) * res_y);
        SDL_RenderDrawLine(renderer, i - 1, prev_y, i, y);
        prev_y = y;
    }

    // SDL_RenderDrawLine(renderer, 0, res_y - ((3.0 / max_value) * res_y), res_x, res_y - ((3.0 / max_value) * res_y));

    SDL_SetRenderTarget(renderer, NULL);
    SDL_Rect src_graf = {0, 0, n_data_points, res_y};
    SDL_RenderCopy(renderer, graf, &src_graf, NULL);
    SDL_RenderPresent(renderer);
}


// TODO: Keep the last n lines in memory for better resizing
void Graphics::resize_window(const int x, const int y) {
    res_x = x; res_y = y;
    SDL_Texture *new_buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, res_x, res_y);

    SDL_SetRenderTarget(renderer, new_buffer);
    SDL_RenderCopy(renderer, buffer, NULL, NULL);
    SDL_DestroyTexture(buffer);
    buffer = new_buffer;
}
