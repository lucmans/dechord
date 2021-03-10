Before compiling, install FFTW3 and SDL2 libraries.

On Ubuntu:
    sudo apt install libsdl2-dev libfftw3-dev
On Arch:
    sudo pacman -S sdl2 fftw3
Note that SDL>2.0.5 is required for recording audio

To compile, run just run make
To run, use ./fourier

Information about CLI arguments can be obtained through ./fourier -h



Usage instructions:
The application uses the terminal along with a graphical interface.
The terminal is only used for text output. At the start, information about the available audio drivers and interfaces will be printed. Also the requested audio setting and received settings from the driver are printed. Lastly, information about the Fourier transform will be printed to the screen.
During the real-time transforming, the current guess for the played note is shown along with the detected loudest frequency and it's calculated error margin. The amplitude of the signal is also listed. The input buffer (recording) queue length is also printed to validate that the program remains real-time.
The program can be closed by sending SIGINT to the terminal (ctrl+c) or pressing q/ESC in the graphical interface.
When running the program with "-s", a sine wave is generated with 1000Hz. The frequency can be increased or decreased using the - and = (+ without shift) keys. The plot is colored relative to the highest measured amplitude. This value can be reset to 0 by pressing r inside the graphical interface.



Code structure:
    config.h: Contains all configuration globals, such as sampling rate and samples per Fourier transform window
    main.cpp: Initializes libraries and devices
    fourier.cpp: Performs actual Fourier analysis and contains related function. For now also handles input.
    graphics.cpp: Handles the Window with the waterfall plot of frequency domain
    gensound.cpp: Contains functions for generating test tones
