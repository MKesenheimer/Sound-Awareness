// g++ $(sdl2-config --cflags --libs) SDL_Audio.cpp -o SDL_Audio

#include <SDL.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>
#include <iomanip>

const int MAX_RECORDING_DEVICES = 10;

//Maximum recording time
const int MAX_RECORDING_SECONDS = 60;

//Maximum recording time plus padding
const int RECORDING_BUFFER_SECONDS = MAX_RECORDING_SECONDS + 1;

//Recieved audio spec
SDL_AudioSpec gReceivedRecordingSpec;
SDL_AudioSpec gReceivedPlaybackSpec;

//Recording data buffer
Uint8* gRecordingBuffer = NULL;

//Size of data buffer
Uint32 gBufferByteSize = 0;

//Position in data buffer
Uint32 gBufferBytePosition = 0;

//Maximum position in data buffer for recording
Uint32 gBufferByteMaxPosition = 0;

int gRecordingDeviceCount = 0;

char y;

void audioRecordingCallback(void* userdata, Uint8* stream, int len )
{
    std::cout << "in callback " << gBufferBytePosition << ": ";
    for (int i = 0; i < len; ++i)
        std::cout << std::hex << std::setfill('0') << std::setw(2) << std::right << static_cast<unsigned int>(stream[i]);
    std::cout << std::dec << std::endl;

    //Copy audio from stream
    std::memcpy(&gRecordingBuffer[ gBufferBytePosition ], stream, len);

    //Move along buffer
    gBufferBytePosition += len;
}

void audioPlaybackCallback(void* userdata, Uint8* stream, int len )
{
    //Copy audio to stream
    std::memcpy(stream, &gRecordingBuffer[ gBufferBytePosition ], len);

    //Move along buffer
    gBufferBytePosition += len;
}

int main()
{
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioDeviceID recordingDeviceId = 0;
    SDL_AudioDeviceID playbackDeviceId = 0;

    SDL_AudioSpec desiredRecordingSpec;
    SDL_zero(desiredRecordingSpec);
    desiredRecordingSpec.freq = 44100;
    desiredRecordingSpec.format = AUDIO_S16;
    desiredRecordingSpec.channels = 2;
    desiredRecordingSpec.samples = 4096;
    desiredRecordingSpec.callback = audioRecordingCallback;

    gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE);

    if(gRecordingDeviceCount < 1)
    {
        printf( "Unable to get audio capture device! SDL Error: %s\n", SDL_GetError() );
        return 0;
    }

    int index;

    for(int i = 0; i < gRecordingDeviceCount; ++i)
    {
        //Get capture device name
        const char* deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);

        printf("%d - %s\n", i, deviceName);
    }

    printf("Choose audio\n");
    scanf("%d", &index);

    //Open recording device
    recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &gReceivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    
    // Device failed to open
    if(recordingDeviceId == 0)
    {
        //Report error
        printf("Failed to open recording device! SDL Error: %s", SDL_GetError() );
        return 1;
    }

    //Default audio spec
    SDL_AudioSpec desiredPlaybackSpec;
    SDL_zero(desiredPlaybackSpec);
    desiredPlaybackSpec.freq = 44100;
    desiredPlaybackSpec.format = AUDIO_S16; // AUDIO_F32;
    desiredPlaybackSpec.channels = 2;
    desiredPlaybackSpec.samples = 4096;
    desiredPlaybackSpec.callback = audioPlaybackCallback;

    //Open playback device
    playbackDeviceId = SDL_OpenAudioDevice( NULL, SDL_FALSE, &desiredPlaybackSpec, &gReceivedPlaybackSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );

    //Device failed to open
    if(playbackDeviceId == 0)
    {
        //Report error
        printf("Failed to open playback device! SDL Error: %s", SDL_GetError());
        return 1;
    }

    // HERE

    //Calculate per sample bytes
    int bytesPerSample = gReceivedRecordingSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedRecordingSpec.format) / 8);

    //Calculate bytes per second
    int bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;

    //Calculate buffer size
    gBufferByteSize = RECORDING_BUFFER_SECONDS * bytesPerSecond;

    //Calculate max buffer use
    gBufferByteMaxPosition = MAX_RECORDING_SECONDS * bytesPerSecond;

    //Allocate and initialize byte buffer
    gRecordingBuffer = new Uint8[gBufferByteSize];
    std::memset(gRecordingBuffer, 0, gBufferByteSize);

    // FILE* f = fopen("output.txt", "rb");
    // size_t result = fread(gRecordingBuffer, sizeof(Uint8), gBufferByteSize, f);
    // if (result != gBufferByteSize)
    // {
    //     printf("Something wrong? Res: %lu & gBufferByteSize %u\n", result, gBufferByteSize);
    // }

    std::printf("Recoding\n");
    //std::cin >> y;

    // begin recording

    //Go back to beginning of buffer
    gBufferBytePosition = 0;

    SDL_PauseAudioDevice( recordingDeviceId, SDL_FALSE );

    while(true)
    {
        //Lock callback
        SDL_LockAudioDevice( recordingDeviceId );

        //Finished recording
        if(gBufferBytePosition > gBufferByteMaxPosition)
        {
            //Stop recording audio
            SDL_PauseAudioDevice( recordingDeviceId, SDL_TRUE );
            break;
        }

        //Unlock callback
        SDL_UnlockAudioDevice( recordingDeviceId );
    }

    std::printf("Playback\n");
    //std::cin >> y;

    gBufferBytePosition = 0;
    SDL_PauseAudioDevice(playbackDeviceId, SDL_FALSE);

    const clock_t begin_time = clock();
    while(true)
    {
        //Lock callback
        SDL_LockAudioDevice(playbackDeviceId);

        //Finished playback
        if(gBufferBytePosition > gBufferByteMaxPosition)
        {
            //Stop playing audio
            SDL_PauseAudioDevice(playbackDeviceId, SDL_TRUE);
            std::cout << "Time elapsed: " << float(clock() - begin_time) / CLOCKS_PER_SEC << '\n';
            break;
        }

        //Unlock callback
        SDL_UnlockAudioDevice(playbackDeviceId);
    }

    if( gRecordingBuffer != NULL )
    {
        delete[] gRecordingBuffer;
        gRecordingBuffer = NULL;
    }

    SDL_Quit();
    return 0;
}
