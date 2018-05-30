#include <stdio.h>
#include <stdlib.h>

#define POCKETMOD_IMPLEMENTATION
#include "pocketmod.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 1024

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

/* Clamp a floating point value to the [-1, +1] range */
static float saturate(float value)
{
    value = value < -1.0f ? -1.0f : value;
    value = value > +1.0f ? +1.0f : value;
    return value;
}

int main(int argc, char **argv)
{
    pocketmod_context context;
    void *mod_data;
    int mod_size, samples, seconds, filesize, i;
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

    /* Initialize the renderer */
    if (!pocketmod_init(&context, mod_data, mod_size, SAMPLE_RATE)) {
        printf("error: '%s' is not a valid MOD file\n", argv[1]);
        return -1;
    }

    /* Render the MOD file and write it as signed 16-bit PCM */
    while (pocketmod_loops(&context) == 0) {
        float buffer[BUFFER_SIZE][2];
        int rendered = pocketmod_render(&context, buffer, BUFFER_SIZE);
        for (i = 0; i < rendered; i++) {
            fputw(saturate(buffer[i][0]) * 0x7fff, file);
            fputw(saturate(buffer[i][1]) * 0x7fff, file);
        }
        samples += rendered;
    }

    /* Now that we know how many samples we got, we can go back and patch the */
    /* ChunkSize and Subchunk2Size fields in the WAV header */
    fseek(file, 4, SEEK_SET);
    fputl(36 + samples * 4, file);
    fseek(file, 40, SEEK_SET);
    fputl(samples * 4, file);

    /* Let the user know how big the output file is */
    printf("%s: ", argv[2]);
    filesize = 40 + samples * 4;
    if (filesize < 1000) {
        printf("%d bytes, ", filesize);
    } else if (filesize < 1000000) {
        printf("%.0f kB, ", (double) filesize / 1000);
    } else {
        printf("%.0f MB, ", (double) filesize / 1000000);
    }
    seconds = (int) ((double) samples / SAMPLE_RATE);
    printf("%d min %02d s\n", seconds / 60, seconds % 60);

    /* Tidy up before leaving */
    fclose(file);
    free(mod_data);
    return 0;
}
