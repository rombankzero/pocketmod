#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define POCKETMOD_IMPLEMENTATION
#include "pocketmod.h"

#define SAMPLE_RATE 44100

/* Write a 16-bit little-endian integer to a file */
static void fputw(unsigned short value, FILE *file)
{
    fputc(value & 0xff, file);
    fputc(value >> 8, file);
}

/* Write a 32-bit little-endian integer to a file */
static void fputl(unsigned long value, FILE *file)
{
    fputw(value & 0xffff, file);
    fputw(value >> 16, file);
}

/* Clip a floating point sample to the [-1, +1] range */
static float clip(float value)
{
    value = value < -1.0f ? -1.0f : value;
    value = value > +1.0f ? +1.0f : value;
    return value;
}

/* Print file size and duration statistics */
static void show_stats(char *filename, int samples)
{
    int seconds = (double) samples / SAMPLE_RATE;
    int filesize = samples * 4 + 44;
    printf("\rwriting: %s ", filename);
    printf("[%.1f MB] ", filesize / 1000000.0);
    printf("[%d:%02d]", seconds / 60, seconds % 60);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    pocketmod_context context;
    void *mod_data;
    int i, mod_size, samples = 0;
    clock_t time_now, time_prev = 0;
    FILE *file;

    /* Print usage if no file was given */
    if (argc != 3) {
        printf("usage: %s <infile> <outfile>\n", argv[0]);
        return -1;
    }

    /* Read the input file into a heap block */
    if (!(file = fopen(argv[1], "rb"))) {
        printf("error: can't open '%s' for reading\n", argv[1]);
        return -1;
    }
    fseek(file, 0, SEEK_END);
    mod_size = ftell(file);
    rewind(file);
    if (!(mod_data = malloc(mod_size))) {
        printf("error: %d-byte memory allocation failed\n", mod_size);
        return -1;
    } else if (!fread(mod_data, mod_size, 1, file)) {
        printf("error: error reading file '%s'\n", argv[1]);
        return -1;
    }
    fclose(file);

    /* Initialize the renderer */
    if (!pocketmod_init(&context, mod_data, mod_size, SAMPLE_RATE)) {
        printf("error: '%s' is not a valid MOD file\n", argv[1]);
        return -1;
    }

    /* Open the output file */
    if (!(file = fopen(argv[2], "wb"))) {
        printf("error: can't open '%s' for writing\n", argv[2]);
        return -1;
    }

    /* Write the WAV header */
    /* Follow along at home: http://soundfile.sapp.org/doc/WaveFormat/ */
    fputs("RIFF", file);          /* ChunkID       */
    fputl(0, file);               /* ChunkSize     */
    fputs("WAVE", file);          /* Format        */
    fputs("fmt ", file);          /* Subchunk1ID   */
    fputl(16, file);              /* Subchunk1Size */
    fputw(1, file);               /* AudioFormat   */
    fputw(2, file);               /* NumChannels   */
    fputl(SAMPLE_RATE, file);     /* SampleRate    */
    fputl(SAMPLE_RATE * 4, file); /* ByteRate      */
    fputw(4, file);               /* BlockAlign    */
    fputw(16, file);              /* BitsPerSample */
    fputs("data", file);          /* Subchunk2ID   */
    fputl(0, file);               /* Subchunk2Size */

    /* Write sample data */
    while (pocketmod_loops(&context) == 0) {

        /* Render samples and write them as signed 16-bit PCM */
        float buffer[512];
        int rendered = pocketmod_render(&context, buffer, sizeof(buffer));
        for (i = 0; i < rendered / sizeof(float); i++) {
            fputw(clip(buffer[i]) * 0x7fff, file);
        }
        samples += rendered / sizeof(float[2]);

        /* Print statistics at regular intervals */
        time_now = clock();
        if ((double) (time_now - time_prev) / CLOCKS_PER_SEC > 0.1) {
            show_stats(argv[2], samples);
            time_prev = time_now;
        }
    }
    show_stats(argv[2], samples);
    putchar('\n');

    /* Now that we know how many samples we got, we can go back and patch the */
    /* ChunkSize and Subchunk2Size fields in the WAV header */
    fseek(file, 4, SEEK_SET);
    fputl(samples * 4 + 36, file);
    fseek(file, 40, SEEK_SET);
    fputl(samples * 4, file);

    /* Tidy up before leaving */
    free(mod_data);
    fclose(file);
    return 0;
}
