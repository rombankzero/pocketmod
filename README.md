# About #

pocketmod is a small ANSI C library for turning ProTracker [MOD files][1] into
playable PCM audio. It comes as an STB-style [single-file library][2], with no
external dependencies (not even the C standard library). The library has been
tested on a wide range of MOD files, and should be fairly accurate. The code is
MIT-licensed.



# Using the library #

## Integration ##

`pocketmod.h` is meant to be dropped into your source tree and compiled along
with the rest of your code. To create the library implementation, #define
`POCKETMOD_IMPLEMENTATION` before including the header in one source file.



## Library API ##

This is the entire public API of the library:

```c
typedef struct pocketmod_context pocketmod_context;
int pocketmod_init(pocketmod_context *c, const void *data, int size, int rate);
int pocketmod_render(pocketmod_context *c, void *buffer, int size);
int pocketmod_loop_count(pocketmod_context *c);
```

Below is a detailed description of each part.



### pocketmod_context ###

```c
struct pocketmod_context {
    /* ... */
};
```

This structure holds state used during rendering. It's declared publicly so that
you can instantiate it anywhere you like, but its data fields should be
considered private. The structure is relatively small (less than 2 KiB), so you
can put it on the stack if you want.



### pocketmod_init ###

```c
int pocketmod_init(pocketmod_context *c, const void *data, int size, int rate);
```

This function initializes a context and prepares it for rendering from the start
of a song. `data` is a buffer containing MOD file data, and `size` is the size
of that buffer in bytes. `rate` is the desired sample rate in Hertz, for example
44100 for CD-quality audio. The function returns a nonzero value on success, or
zero if it was given an invalid MOD file.

Note that the library reads pattern and instrument data directly out of `data`
during rendering, so you should make sure that it remains valid until the last
call to `pocketmod_render()`.



### pocketmod_render ###

```c
int pocketmod_render(pocketmod_context *c, void *buffer, int size);
```

This function renders the next part of the song and writes the rendered PCM
audio data to `buffer`, which should be at least `size` bytes large. It returns
the number of bytes worth of sample data that was written to `buffer`.

Each sample consists of two `float` PCM values, first the left channel and then
the right channel. `pocketmod_render()` generally writes as many samples as it
can, but it may stop short when reaching the end of a pattern. This is done so
that you can check if the song looped, and decide for yourself if you want to
keep going or not.



### pocketmod_loop_count ###

```c
int pocketmod_loop_count(pocketmod_context *c);
```

This function returns the number of times the song has looped. The song is
considered to have looped when reaching a previously encountered pattern order
position.



# Configuration #

There are a few preprocessor symbols that may be #defined before including
`pocketmod.h` to change the behavior of the library:

- `POCKETMOD_MAX_CHANNELS`: The maximum number of channels. The default value is
  32 (the highest number for any MOD format). Lowering this value reduces the
  size of the `pocketmod_context` structure. The downside is that it will make
  `pocketmod_init()` fail for any songs that need more channels. This setting is
  useful if, for example, you know that all of your songs only use four
  channels.
- `POCKETMOD_MAX_SAMPLES`: Same as the above, but for samples (instruments). The
  default value is 31 (the highest number for any MOD format).
- `POCKETMOD_NO_INTERPOLATION`: You can define this to disable sample
  interpolation. This gives the rendered audio a sharper, rougher sound, which
  is sometimes preferred for chiptunes.

Note that if you define any of the above symbols, you should make sure that
they're defined the same in *every* source file that includes `pocketmod.h`. I
recommend that you use your build system to do this consistently across your
whole project.



# Example programs #

There are two small example programs included, to demostrate how the library is
used.



## MOD to WAV converter ##

This is a simple command-line tool for converting MOD files to WAV files. Here's
an example of what building and using it looks like:

    $ make converter
    cc examples/converter.c -o converter -I.
    $ ./converter songs/spacedeb.mod spacedeb.wav
    Writing: 'spacedeb.wav' [54.0 MB] [5:05] Press Ctrl + C to stop



## SDL2-based MOD player ##

This is a command-line MOD player. To build the example, you need to have
[SDL2][3] installed. On Debian-based Linux you can get it the usual way:

    $ sudo apt-get install libsdl2-dev

And here's what it looks like:

    $ make player
    cc examples/player.c -o player -I. -lSDL2main -lSDL2
    $ ./player songs/king.mod
    Playing 'king.mod' [00:03] Press Ctrl + C to stop



# Song credits #

A small collection of test songs is included in the `songs/` directory. All
credit for these go to their respective authors. I have shortened the filename
of some of the songs, but they are otherwise unaltered.

| Filename             | Author           | License                 |
|----------------------|------------------|-------------------------|
| [bananasplit.mod][6] | dizzy / cncd     | [ModArchive license][4] |
| [chill.mod][7]       | Chromag          | [ModArchive license][4] |
| [elysium.mod][8]     | Jester           | [CC BY-NC-SA 3.0][5]    |
| [king.mod][9]        | Dreamweaver      | [ModArchive license][4] |
| [nemesis.mod][10]    | radix            | [ModArchive license][4] |
| [overture.mod][11]   | Jogeir Liljedahl | [ModArchive license][4] |
| [spacedeb.mod][12]   | Markus Kaarlonen | [ModArchive license][4] |
| [stardstm.mod][13]   | Jester           | [CC BY-NC-SA 3.0][5]    |
| [sundance.mod][14]   | Purple Motion    | [ModArchive license][4] |
| [sundown.mod][15]    | Chromag          | [ModArchive license][4] |
| [supernova.mod][16]  | radix            | [ModArchive license][4] |



[1]: https://en.wikipedia.org/wiki/MOD_(file_format)
[2]: https://github.com/nothings/single_file_libs
[3]: https://libsdl.org/
[4]: https://modarchive.org/index.php?terms-upload
[5]: https://creativecommons.org/licenses/by-nc-sa/3.0/
[6]: https://modarchive.org/module.php?35151
[7]: https://modarchive.org/module.php?85693
[8]: https://modarchive.org/module.php?40475
[9]: https://modarchive.org/module.php?93821
[10]: https://modarchive.org/module.php?164417
[11]: https://modarchive.org/module.php?51549
[12]: https://modarchive.org/module.php?57925
[13]: https://modarchive.org/module.php?59344
[14]: https://modarchive.org/module.php?171453
[15]: https://modarchive.org/module.php?159847
[16]: https://modarchive.org/module.php?164025
