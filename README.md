This project is here for archival purposes. See [Digistring](https://github.com/lucmans/digistring) for a better version.

Note that this project is just a prototype. In the branch "paper", a report ("paper.pdf") can be found which explains what has been implemented and what has to be implemented for this program to actually perform real-time polyphonic guitar to MIDI translation.  

# Requirements
On Ubuntu:  
    sudo apt install libsdl2-dev libfftw3-dev  
On Arch:  
    sudo pacman -S sdl2 fftw3  
Note that SDL>2.0.5 is required for recording audio

# Build and usage instruction
Run from the command line in the root directory of the project:  
`make`  
`./dechord`

Information about CLI arguments can be obtained through ./dechord -h

# Usage instructions
The application uses the terminal along with a graphical interface.  
The terminal is only used for text output. At the start, information about the available audio drivers and interfaces will be printed. Also the requested audio setting and received settings from the driver are printed. Lastly, information about the Fourier transform will be printed to the screen. After this, real-time information on the current estimate are continuously printed.  
The program can be closed by sending SIGINT to the terminal (ctrl+c) or pressing q/ESC in the graphical interface.  
You can switch between monophonic and polyphonic transcription by pressing "m" or "p" respectively.  
There are two plotting modes, spectrogram plot and waterfall plot. You can switch between them using the "s" key in the GUI.  
When running the program with "-s", a sine wave is generated with 1000Hz. The frequency can be increased or decreased using the - and = (+ without shift) keys. The plot height/colors are relative to the highest measured amplitude. This value can be reset to 0 by pressing r inside the graphical interface.

# Code structure
`config.h`: Contains all configuration globals (parameters), such as sampling rate and samples per Fourier transform frame.  
`main.cpp`: Initializes libraries and devices.  
`transcribe.cpp`: Contains the main transcription loop and user interaction code. Specific steps in the transcription process are split into their own files.  
`graphics.cpp`: Handles the window which can show a spectrogram and waterfall plot. In the spectrogram, green represents the detected frequencies, red the rolling Gaussian mean and in blue the peaks. In the waterfall plot, peaks are highlighted with a white filter.  
`note_set.cpp`: Makes sets of notes given the peak locations. It also needs the spectrogram for interpolation, which is done in this step instead of the peak picking step for efficiency. Based on the note sets, f0 estimation can be performed. Polyphony can be detected using "overtone sieves".  
`find_peaks.cpp`: Contains code for finding peaks in the spectrum, so also calculates the rolling Gaussian mean of a spectrum.  
`music_file.cpp`: Contains code for loading .wav files. Only supports 192kHz with float encoding, but could easily be extended to support 24bit int (see comments).  
`gensound.cpp`: Contains functions for generating test tones.
