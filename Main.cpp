#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <future>
#include <thread>
#include <chrono>

#include <vector>
#include <tuple>
#include <array>

#include <SDL.h>

#include "SDLTools/Utilities.h"
#include "SDLTools/Timer.h"
#include "SDLTools/CommandLineParser.h"


uint8_t gThreshold = 0x88;
Uint8* gRecordingBuffer = NULL;
Uint32 gBufferByteSize = 0;
static bool monitor = true;
static bool run = true;

// TODO
void usage(char* argv[]) {
    std::cout << "Usage:" << std::endl << argv[0] << " -t <threshold> [options]" << std::endl;
    std::exit(-1);
}

static void getKeyboardInput() {
    while (true) {
        std::string input = sdl::auxiliary::ccin<std::string>("[USER] -/+: decrease/increase threshold, q: quit: ");
        if (input == "-") {
            std::cout << "[INFO] Decreasing threshold." << std::endl;
            if (gThreshold >= 5)
                gThreshold -= 5;
            std::cout << "[INFO] Threshold = " << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<unsigned int>(gThreshold) << std::dec << std::endl;
        } else if (input == "+") {
            std::cout << "[INFO] Increasing threshold." << std::endl;
            if (gThreshold <= 250)
                gThreshold += 5;
            std::cout << "[INFO] Threshold = " << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<unsigned int>(gThreshold) << std::dec << std::endl;
        } else if (input == "q") {
            std::cout << "[INFO] Exiting" << std::endl;
            run = false;
            monitor = false;
            break;
        }
    }
}

void audioRecordingCallback(void* userdata, Uint8* stream, int len) {
#if 0
    static uint32_t numberOfSamples = 0;
    std::cout << "in callback " << numberOfSamples << ": ";
    for (int i = 0; i < len; ++i)
        std::cout << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<unsigned int>(stream[i]);
    std::cout << std::dec << std::endl;
    numberOfSamples += len;
#endif

    for (int i = 0; i < len; ++i) {
        if (static_cast<unsigned int>(stream[i]) >= gThreshold) {
            system("osascript -e 'display notification \"Something loud is happening!\" with title\"Alarm!\" subtitle \"Notification from sound-awareness daemon\"'");

            // store the buffer for later output
            if (gBufferByteSize == len && gRecordingBuffer != NULL) {
                std::memcpy(gRecordingBuffer, stream, len);
            } else {
                if (gRecordingBuffer != NULL)
                    delete[] gRecordingBuffer;
                gBufferByteSize = len;
                gRecordingBuffer = new Uint8[len];
                std::memcpy(gRecordingBuffer, stream, len);
            }

            monitor = false;
            break;
        }
    }
}

void audioPlaybackCallback(void* userdata, Uint8* stream, int len) {
    // copy audio to stream
    if (gBufferByteSize >= len && gRecordingBuffer != NULL) {
#if 0
        for (int i = 0; i < len; ++i)
            std::cout << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<unsigned int>(gRecordingBuffer[i]);
        std::cout << std::dec << std::endl;
#endif

        std::memcpy(stream, gRecordingBuffer, len);
    }
    monitor = true;
}

int main(int argc, char* argv[]) {
    if(SDL_Init(SDL_INIT_AUDIO) != 0) {
        std::cerr << "[ERROR] Error in SDL_Init: " << SDL_GetError() << std::endl;
        return -1;
    }

    // recording audio specs
    SDL_AudioSpec receivedRecordingSpec;
    SDL_AudioSpec desiredRecordingSpec;
    SDL_zero(desiredRecordingSpec);
    desiredRecordingSpec.freq = 44100;
    desiredRecordingSpec.format = AUDIO_U8;
    desiredRecordingSpec.channels = 2;
    desiredRecordingSpec.samples = 16384;
    desiredRecordingSpec.callback = audioRecordingCallback;
    int recordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE);
    if(recordingDeviceCount < 1) {
        std::cerr << "[ERROR] Unable to get audio capture device: " << SDL_GetError() << std::endl;
        return -1;
    }

    // choose audio input
    unsigned int index;
    for(int i = 0; i < recordingDeviceCount; ++i) {
        // Get capture device name
        const std::string deviceName = std::string(SDL_GetAudioDeviceName(i, SDL_TRUE));
        std::cout << "[INFO] " << i << " - " << deviceName << std::endl;
    }
    do {
        index = sdl::auxiliary::ccin<int>("[USER] Choose audio: ");
    } while (index >= recordingDeviceCount);

    // Open recording device
    SDL_AudioDeviceID recordingDeviceId = 0;
    recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &receivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    // Device failed to open
    if(recordingDeviceId == 0) {
        // Report error
        std::cerr << "[ERROR] Failed to open recording device! SDL Error:" <<  SDL_GetError() << std::endl;
        return 1;
    }

    // playback audio spec
    SDL_AudioSpec receivedPlaybackSpec;
    SDL_AudioSpec desiredPlaybackSpec;
    SDL_zero(desiredPlaybackSpec);
    desiredPlaybackSpec.freq = 44100;
    desiredPlaybackSpec.format = AUDIO_U8; // AUDIO_F32;
    desiredPlaybackSpec.channels = 2;
    desiredPlaybackSpec.samples = 16384;
    desiredPlaybackSpec.callback = audioPlaybackCallback;

    // Open playback device
    SDL_AudioDeviceID playbackDeviceId = 0;
    playbackDeviceId = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desiredPlaybackSpec, &receivedPlaybackSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    //Device failed to open
    if(playbackDeviceId == 0) {
        std::cerr << "[ERROR] Failed to open playback device! SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // handle keyboard input in a separate thread
    std::thread t(getKeyboardInput);
    
    std::cout << "[INFO] Monitoring" << std::endl;
    while (run) {
        // monitoring
        SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);
        while (monitor) {
            // Lock callback
            SDL_LockAudioDevice(recordingDeviceId);

            // Unlock callback
            SDL_UnlockAudioDevice(recordingDeviceId);
        }
        // stop monitoring audio
        SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE);

        // playback
        SDL_PauseAudioDevice(playbackDeviceId, SDL_FALSE);
        while (!monitor) {
            // Lock callback
            SDL_LockAudioDevice(playbackDeviceId);

            // Unlock callback
            SDL_UnlockAudioDevice(playbackDeviceId);
        }
        // stop playback audio
        SDL_PauseAudioDevice(playbackDeviceId, SDL_TRUE);
    }

    // join and terminate thread
    t.join();

    if(gRecordingBuffer != NULL) {
        delete[] gRecordingBuffer;
        gRecordingBuffer = NULL;
    }

    SDL_Quit();
    return 0;
}
