#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <chrono>
#include <iomanip>

#include <vector>
#include <tuple>
#include <array>

#include <SDL.h>

#include "SDLTools/Utilities.h"
#include "SDLTools/Timer.h"
#include "SDLTools/CommandLineParser.h"


uint8_t threshold = 0xB0;

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
            std::cout << "ALARM!" << std::endl;
            system("osascript -e 'display notification \"Something loud is happening!\" with title\"Alarm!\" subtitle \"Notification from sound-awareness daemon\"'");
            break;
        }
}

int main(int argc, char* argv[]) {
    if(SDL_Init(SDL_INIT_AUDIO) != 0) {
        std::cerr << "Error in SDL_Init: " << SDL_GetError() << std::endl;
        return -1;
    }

    // audio specs
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
        std::cerr << "Unable to get audio capture device: " << SDL_GetError() << std::endl;
        return -1;
    }

    // choose audio input
    for(int i = 0; i < recordingDeviceCount; ++i) {
        // Get capture device name
        const std::string deviceName = std::string(SDL_GetAudioDeviceName(i, SDL_TRUE));
        std::cout << i << " - " << deviceName << std::endl;
    }
    int index;
    do {
        std::cout << "Choose audio" << std::endl;
        std::cin >> index;
    } while (index < 0 || index >= recordingDeviceCount);

    // Open recording device
    SDL_AudioDeviceID recordingDeviceId = 0;
    recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &receivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    // Device failed to open
    if(recordingDeviceId == 0) {
        // Report error
        printf("Failed to open recording device! SDL Error: %s", SDL_GetError());
        return 1;
    }

    // monitoring
    std::cout << "Monitoring" << std::endl;
    SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);
    while (true) { //(numberOfSamples == 0) {
        // Lock callback
        SDL_LockAudioDevice(recordingDeviceId);
        // Unlock callback
        SDL_UnlockAudioDevice(recordingDeviceId);
    }
    // Finished recording, stop recording audio
    SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE);

    SDL_Quit();
    return 0;
}
