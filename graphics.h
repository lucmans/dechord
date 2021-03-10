
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

        void add_line(const double data[(WINDOW_SAMPLES / 2) + 1], const double max_value, const int n_data_points,
                      const std::vector<int> &peaks);

        void graph_spectrogram(const double data[(WINDOW_SAMPLES / 2) + 1], const double max_value, const int n_data_points,
                               const std::vector<int> &peaks, const double envelope[(WINDOW_SAMPLES / 2) + 1]);

        void resize_window(const int x, const int y);


    private:
        int res_x, res_y;

        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *buffer;

        SDL_Texture *line;
};


#endif  // GRAPHICS_H
