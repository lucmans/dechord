
#ifndef GRAPHICS_H
#define GRAPHICS_H


#include "config.h"

#include <SDL2/SDL.h>

#include <vector>


static const int GRAPHWIDTH = 1;


class Graphics {
    public:
        Graphics();
        ~Graphics();

        void add_line(const double data[(FRAME_SIZE / 2) + 1], const double max_value,
                      const std::vector<int> &peaks, const int n_data_points/* = (FRAME_SIZE / 2) + 1*/);
        void graph_waterfall();

        void graph_spectrogram(const double data[(FRAME_SIZE / 2) + 1], const double max_value,
                               const std::vector<int> &peaks, const double envelope[(FRAME_SIZE / 2) + 1],
                               const int n_data_points/* = (FRAME_SIZE / 2) + 1*/);

        void resize_window(const int x, const int y);


    private:
        int res_x, res_y;

        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *buffer;

        SDL_Texture *line;
};


#endif  // GRAPHICS_H
