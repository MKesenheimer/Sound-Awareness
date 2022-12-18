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


uint8_t threshold = 0xB0;

void usage(char* argv[]) {
    std::cout << "Usage:" << std::endl << argv[0] << " -t <threshold> [options]" << std::endl;
    std::exit(-1);
}

static int getKeyboardInput() {
    std::string input = sdl::auxiliary::ccin<std::string>("[USER] -/+: decrease/increase threshold, q: quit: ");
    if (input == "-") {
        std::cout << "[INFO] Decreasing threshold." << std::endl;
        threshold -= 10;
        std::cout << "[INFO] Threshold = " << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<unsigned int>(threshold) << std::dec << std::endl;
    } else if (input == "+") {
        std::cout << "[INFO] Increasing threshold." << std::endl;
        threshold += 10;
        std::cout << "[INFO] Threshold = " << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<unsigned int>(threshold) << std::dec << std::endl;
    } else if (input == "q") {
        std::cout << "[INFO] Exiting" << std::endl;
        return 1;
    }
    return 0;
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

    for (int i = 0; i < len; ++i)
        if (static_cast<unsigned int>(stream[i]) >= threshold) {
            //std::cout << "ALARM!" << std::endl;
            system("osascript -e 'display notification \"Something loud is happening!\" with title\"Alarm!\" subtitle \"Notification from sound-awareness daemon\"'");
            break;
        }
}

void audioPlaybackCallback(void* userdata, Uint8* stream, int len) {
    /*//Copy audio to stream
    std::memcpy(stream, &gRecordingBuffer[ gBufferBytePosition ], len);

    //Move along buffer
    gBufferBytePosition += len;
    */
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
    desiredRecordingSpec.samples = 4096;
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
    desiredPlaybackSpec.samples = 4096;
    desiredPlaybackSpec.callback = audioPlaybackCallback;

    // Open playback device
    SDL_AudioDeviceID playbackDeviceId = 0;
    playbackDeviceId = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desiredPlaybackSpec, &receivedPlaybackSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    //Device failed to open
    if(playbackDeviceId == 0) {
        //Report error
        std::cerr << "[ERROR] Failed to open playback device! SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }


    // monitoring
    std::cout << "[INFO] Monitoring" << std::endl;
    SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);
    bool run = true;
    while (run) { //(numberOfSamples == 0) {
        // Lock callback
        SDL_LockAudioDevice(recordingDeviceId);

        std::chrono::seconds timeout(10);
        std::future<int> future = std::async(getKeyboardInput);
        if (future.wait_for(timeout) == std::future_status::ready)
            if(future.get() == 1)
                run = false;

        // Unlock callback
        SDL_UnlockAudioDevice(recordingDeviceId);
    }
    // stop monitoring audio
    SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE);

    SDL_Quit();
    return 0;
}
