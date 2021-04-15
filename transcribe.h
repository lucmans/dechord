
#ifndef FOURIER_H
#define FOURIER_H


#include "graphics.h"

#include <SDL2/SDL.h>


void transcribe(SDL_AudioDeviceID &in_dev, Graphics *graphics, SDL_AudioDeviceID &out_dev);


#endif  // FOURIER_H
