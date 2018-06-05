#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#define POCKETMOD_IMPLEMENTATION
#include "pocketmod.h"

static void audio_callback(void *userdata, Uint8 *buffer, int bytes)
{
    int i = 0;
    while (i < bytes) {
        i += pocketmod_render(userdata, buffer + i, bytes - i);
    }
}

int main(int argc, char **argv)
{
    const Uint32 allowed_changes = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE;
    Uint32 start_time;
    pocketmod_context context;
    SDL_AudioSpec format;
    SDL_AudioDeviceID device;
    SDL_RWops *mod_file;
    char *mod_data, *slash;
    size_t mod_size;

    /* Print usage if no file was given */
    if (argc != 2) {
        printf("usage: %s <modfile>\n", argv[0]);
        return -1;
    }

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("error: SDL_Init() failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Initialize the audio subsystem */
    format.freq = 44100;
    format.format = AUDIO_F32;
    format.channels = 2;
    format.samples = 4096;
    format.callback = audio_callback;
    format.userdata = &context;
    device = SDL_OpenAudioDevice(NULL, 0, &format, &format, allowed_changes);
    if (!device) {
        printf("error: SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Read the MOD file into a heap block */
    if (!(mod_file = SDL_RWFromFile(argv[1], "rb"))) {
        printf("error: can't open '%s' for reading\n", argv[1]);
        return -1;
    } else if (!(mod_data = SDL_malloc(mod_size = SDL_RWsize(mod_file)))) {
        printf("error: can't allocate memory for '%s'\n", argv[1]);
        return -1;
    } else if (!SDL_RWread(mod_file, mod_data, mod_size, 1)) {
        printf("error: can't read file '%s'\n", argv[1]);
        return -1;
    }
    SDL_RWclose(mod_file);

    /* Initialize the renderer */
    if (!pocketmod_init(&context, mod_data, mod_size, format.freq)) {
        printf("error: '%s' is not a valid MOD file\n", argv[1]);
        return -1;
    }

    /* Strip the directory part from the source file's path */
    while ((slash = strpbrk(argv[1], "/\\"))) {
        argv[1] = slash + 1;
    }

    /* Start playback */
    SDL_PauseAudioDevice(device, 0);
    start_time = SDL_GetTicks();
    for (;;) {

        /* Print some information during playback */
        int seconds = (SDL_GetTicks() - start_time) / 1000;
        printf("\rPlaying '%s' ", argv[1]);
        printf("[%d:%02d] ", seconds / 60, seconds % 60);
        printf("Press Ctrl + C to stop");
        fflush(stdout);
        SDL_Delay(500);
    }
    return 0;
}
