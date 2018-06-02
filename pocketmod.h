/* See end of file for license */

#ifndef POCKETMOD_H_INCLUDED
#define POCKETMOD_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pocketmod_context pocketmod_context;
int pocketmod_init(pocketmod_context *c, const void *data, int size, int rate);
int pocketmod_render(pocketmod_context *c, void *buffer, int size);
int pocketmod_loop_count(pocketmod_context *c);

#ifndef POCKETMOD_MAX_CHANNELS
#define POCKETMOD_MAX_CHANNELS 32
#endif

#ifndef POCKETMOD_MAX_SAMPLES
#define POCKETMOD_MAX_SAMPLES 31
#endif

typedef struct {
    signed char *data;          /* Sample data buffer                      */
    unsigned int length;        /* Data length (in bytes)                  */
} _pocketmod_sample;

typedef struct {
    unsigned char dirty;        /* Pitch/volume dirty flags                */
    unsigned char sample;       /* Sample number (0..31)                   */
    unsigned char volume;       /* Base volume without tremolo (0..64)     */
    unsigned char balance;      /* Stereo balance (0..255)                 */
    unsigned short period;      /* Note period (113..856)                  */
    unsigned short target;      /* Target period (for tone portamento)     */
    unsigned char finetune;     /* Note finetune (0..15)                   */
    unsigned char loop_count;   /* E6x loop counter                        */
    unsigned char loop_line;    /* E6x target line                         */
    unsigned char lfo_step;     /* Vibrato/tremolo LFO step counter        */
    unsigned char lfo_type[2];  /* LFO type for vibrato/tremolo            */
    unsigned char effect;       /* Current effect (0x0..0xf or 0xe0..0xef) */
    unsigned char param;        /* Raw effect parameter value              */
    unsigned char param3;       /* Parameter memory for 3xx                */
    unsigned char param4;       /* Parameter memory for 4xy                */
    unsigned char param7;       /* Parameter memory for 7xy                */
    unsigned char param9;       /* Parameter memory for 9xx                */
    unsigned char paramE1;      /* Parameter memory for E1x                */
    unsigned char paramE2;      /* Parameter memory for E2x                */
    unsigned char paramEA;      /* Parameter memory for EAx                */
    unsigned char paramEB;      /* Parameter memory for EBx                */
    unsigned char real_volume;  /* Volume (with tremolo adjustment)        */
    float position;             /* Position in sample data buffer          */
    float increment;            /* Position increment per output sample    */
} _pocketmod_chan;

struct pocketmod_context
{
    /* Read-only song data */
    _pocketmod_sample samples[POCKETMOD_MAX_SAMPLES];
    unsigned char *source;      /* Pointer to source MOD data              */
    unsigned char *order;       /* Pattern order table                     */
    unsigned char *patterns;    /* Start of pattern data                   */
    unsigned char length;       /* Patterns in the order (1..128)          */
    unsigned char reset;        /* Pattern to loop back to (0..127)        */
    unsigned char num_patterns; /* Patterns in the file (1..128)           */
    unsigned char num_samples;  /* Sample count (15 or 31)                 */
    unsigned char num_channels; /* Channel count (1..32)                   */

    /* Timing variables */
    int samples_per_second;     /* Sample rate (set by user)               */
    int ticks_per_line;         /* A.K.A. song speed (initially 6)         */
    float samples_per_tick;     /* Depends on sample rate and BPM          */

    /* Loop detection state */
    unsigned char visited[16];  /* Bit mask of previously visited patterns */
    int loop_count;             /* How many times the song has looped      */

    /* Render state */
    _pocketmod_chan channels[POCKETMOD_MAX_CHANNELS];
    unsigned char pattern_delay;/* EEx pattern delay counter               */
    unsigned int lfo_rng;       /* RNG used for the random LFO waveform    */

    /* Position in song (from least to most granular) */
    signed char pattern;        /* Current pattern in order                */
    signed char line;           /* Current line in pattern                 */
    short tick;                 /* Current tick in line                    */
    float sample;               /* Current sample in tick                  */
};

#ifdef POCKETMOD_IMPLEMENTATION

/* Memorize a parameter unless the new value is zero */
#define POCKETMOD_MEM(dst, src) do { \
        (dst) = (src) ? (src) : (dst); \
    } while (0)

/* Same thing, but memorize each nibble separately */
#define POCKETMOD_MEM2(dst, src) do { \
        (dst) = (((src) & 0x0f) ? ((src) & 0x0f) : ((dst) & 0x0f)) \
              | (((src) & 0xf0) ? ((src) & 0xf0) : ((dst) & 0xf0)); \
    } while (0)

/* Shortcut to sample metadata (sample must be nonzero) */
#define POCKETMOD_SAMPLE(c, sample) ((c)->source + 12 + 30 * (sample))

/* Channel dirty flags */
#define POCKETMOD_PITCH  0x01
#define POCKETMOD_VOLUME 0x02

/* Amiga period table (three octaves per finetune setting) */
static const short _pocketmod_amiga_period[16][36] = {
    {856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
     428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
     214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113},
    {850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450,
     425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 239, 225,
     213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113},
    {844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447,
     422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237, 224,
     211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112},
    {838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444,
     419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235, 222,
     209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111},
    {832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441,
     416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233, 220,
     208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110},
    {826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437,
     413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232, 219,
     206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109},
    {820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434,
     410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230, 217,
     205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109},
    {814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431,
     407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228, 216,
     204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108},
    {907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 504, 480,
     453, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
     226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120},
    {900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477,
     450, 425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 238,
     225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119},
    {894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474,
     447, 422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237,
     223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118},
    {887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470,
     444, 419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235,
     222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118},
    {881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467,
     441, 416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233,
     220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117},
    {875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463,
     437, 413, 390, 368, 347, 338, 309, 292, 276, 260, 245, 232,
     219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116},
    {868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460,
     434, 410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230,
     217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115},
    {862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457,
     431, 407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228,
     216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114}
};

/* Min/max helper functions */
static int _pocketmod_min(int x, int y) { return x < y ? x : y; }
static int _pocketmod_max(int x, int y) { return x > y ? x : y; }

/* Clamp a volume value to the 0..64 range */
static int _pocketmod_clamp_volume(int x)
{
    x = _pocketmod_max(x, 0x00);
    x = _pocketmod_min(x, 0x40);
    return x;
}

/* Zero out a block of memory */
static int _pocketmod_zero(void *data, int size)
{
    char *byte = data, *end = byte + size;
    while (byte != end) { *byte++ = 0; }
}

/* Convert a period (at finetune = 0) to a note index in 0..35 */
static int _pocketmod_period_to_note(int period)
{
    switch (period) {
        case 856: return  0; case 808: return  1; case 762: return  2;
        case 720: return  3; case 678: return  4; case 640: return  5;
        case 604: return  6; case 570: return  7; case 538: return  8;
        case 508: return  9; case 480: return 10; case 453: return 11;
        case 428: return 12; case 404: return 13; case 381: return 14;
        case 360: return 15; case 339: return 16; case 320: return 17;
        case 302: return 18; case 285: return 19; case 269: return 20;
        case 254: return 21; case 240: return 22; case 226: return 23;
        case 214: return 24; case 202: return 25; case 190: return 26;
        case 180: return 27; case 170: return 28; case 160: return 29;
        case 151: return 30; case 143: return 31; case 135: return 32;
        case 127: return 33; case 120: return 34; case 113: return 35;
        default: return 0;
    }
}

/* Apply finetune adjustment to a period */
static int _pocketmod_finetune(int period, int finetune)
{
    int note = _pocketmod_period_to_note(period);
    return _pocketmod_amiga_period[finetune][note];
}

/* Table-based sine wave oscillator */
static int _pocketmod_sin(int step)
{
    static const unsigned char sin[16] = {
        0x00, 0x18, 0x31, 0x4a, 0x61, 0x78, 0x8d, 0xa1,
        0xb4, 0xc5, 0xd4, 0xe0, 0xeb, 0xf4, 0xfa, 0xfd
    };
    int x = sin[step & 0x0f];
    x = (step & 0x1f) < 0x10 ? x : 0xff - x;
    return step < 0x20 ? x : -x;
}

/* Oscillators for vibrato/tremolo effects */
static int _pocketmod_lfo(pocketmod_context *c, _pocketmod_chan *ch, int step)
{
    switch (ch->lfo_type[ch->effect == 7] & 3) {
        case 0: return _pocketmod_sin(step & 0x3f);         /* Sine   */
        case 1: return 0xff - ((step & 0x3f) << 3);         /* Saw    */
        case 2: return (step & 0x3f) < 0x20 ? 0xff : -0xff; /* Square */
        case 3: return (c->lfo_rng & 0x1ff) - 0xff;         /* Random */
        default: return 0; /* Hush little compiler */
    }
}

static void _pocketmod_update_pitch(pocketmod_context *c, _pocketmod_chan *ch)
{
    /* Don't do anything if the period is zero */
    ch->increment = 0.0f;
    if (ch->period) {
        float period = ch->period;

        /* Apply vibrato (if active) */
        if (ch->effect == 0x4 || ch->effect == 0x6) {
            int step = (ch->param4 >> 4) * ch->lfo_step;
            int rate = ch->param4 & 0x0f;
            period += _pocketmod_lfo(c, ch, step) * rate / 128.0f;

        /* Apply arpeggio (if active) */
        } else if (ch->effect == 0x0 && ch->param) {
            static const float arpeggio[16] = { /* 2^(X/12) for X in 0..15 */
                1.000000f, 1.059463f, 1.122462f, 1.189207f,
                1.259921f, 1.334840f, 1.414214f, 1.498307f,
                1.587401f, 1.681793f, 1.781797f, 1.887749f,
                2.000000f, 2.118926f, 2.244924f, 2.378414f
            };
            int step = (ch->param >> ((2 - c->tick % 3) << 2)) & 0x0f;
            period /= arpeggio[step];
        }

        /* Calculate sample buffer position increment */
        ch->increment = 3546894.6f / (period * c->samples_per_second);
    }

    /* Clear the pitch dirty flag */
    ch->dirty &= ~POCKETMOD_PITCH;
}

static void _pocketmod_update_volume(pocketmod_context *c, _pocketmod_chan *ch)
{
    int volume = ch->volume;
    if (ch->effect == 0x7) {
        int step = ch->lfo_step * (ch->param7 >> 4);
        volume += _pocketmod_lfo(c, ch, step) * (ch->param7 & 0x0f) >> 6;
    }
    ch->real_volume = _pocketmod_clamp_volume(volume);
    ch->dirty &= ~POCKETMOD_VOLUME;
}

static void _pocketmod_pitch_slide(_pocketmod_chan *ch, int amount)
{
    ch->period += amount;
    const short *pitch = _pocketmod_amiga_period[ch->finetune];
    ch->period = _pocketmod_max(ch->period, pitch[35]);
    ch->period = _pocketmod_min(ch->period, pitch[0]);
    ch->dirty |= POCKETMOD_PITCH;
}

static void _pocketmod_volume_slide(_pocketmod_chan *ch, int param)
{
    /* Undocumented quirk: If both x and y are nonzero, then the value of x */
    /* takes precedence. (Yes, there are songs that rely on this behavior.) */
    int change = (param & 0xf0) ? (param >> 4) : -(param & 0x0f);
    ch->volume = _pocketmod_clamp_volume(ch->volume + change);
    ch->dirty |= POCKETMOD_VOLUME;
}

static void _pocketmod_next_line(pocketmod_context *c)
{
    unsigned char (*data)[4];
    int i, pos, pattern_break = -1;

    /* When entering a new pattern order index, mark it as "visited" */
    if (c->line == 0) {
        c->visited[c->pattern >> 3] |= 1 << (c->pattern & 7);
    }

    /* Move to the next pattern if this was the last line */
    if (++c->line == 64) {
        if (++c->pattern == c->length) {
            c->pattern = c->reset;
        }
        c->line = 0;
    }

    /* Find the pattern data for the current line */
    pos = (c->order[c->pattern] * 64 + c->line) * c->num_channels * 4;
    data = (unsigned char(*)[4]) (c->patterns + pos);
    for (i = 0; i < c->num_channels; i++) {

        /* Decode columns */
        int sample = (data[i][0] & 0xf0) | (data[i][2] >> 4);
        int period = ((data[i][0] & 0x0f) << 8) | data[i][1];
        int effect = ((data[i][2] & 0x0f) << 8) | data[i][3];

        /* Memorize effect parameter values */
        _pocketmod_chan *ch = &c->channels[i];
        ch->effect = (effect >> 8) != 0xe ? (effect >> 8) : (effect >> 4);
        ch->param = (effect >> 8) != 0xe ? (effect & 0xff) : (effect & 0x0f);

        /* Set sample */
        if (sample) {
            if (sample <= POCKETMOD_MAX_SAMPLES) {
                unsigned char *sample_data = POCKETMOD_SAMPLE(c, sample);
                ch->sample = sample;
                ch->finetune = sample_data[2] & 0x0f;
                if (ch->effect != 0xED) {
                    ch->volume = _pocketmod_min(sample_data[3], 0x40);
                    ch->dirty |= POCKETMOD_VOLUME;
                }
            } else {
                ch->sample = 0;
            }
        }

        /* Set note */
        if (period) {
            period = _pocketmod_finetune(period, ch->finetune);
            if (ch->effect != 0x3) {
                if (ch->effect != 0xED) {
                    ch->position = 0.0f;
                }
                ch->dirty |= POCKETMOD_PITCH;
                ch->period = period;
                ch->lfo_step = 0;
            }
        }

        /* Handle pattern effects */
        switch (ch->effect) {

            /* Memorize parameters */
            case 0x3: POCKETMOD_MEM(ch->param3, ch->param); /* Fall through */
            case 0x5: POCKETMOD_MEM(ch->target, period); break;
            case 0x4: POCKETMOD_MEM2(ch->param4, ch->param); break;
            case 0x7: POCKETMOD_MEM2(ch->param7, ch->param); break;
            case 0xE1: POCKETMOD_MEM(ch->paramE1, ch->param); break;
            case 0xE2: POCKETMOD_MEM(ch->paramE2, ch->param); break;
            case 0xEA: POCKETMOD_MEM(ch->paramEA, ch->param); break;
            case 0xEB: POCKETMOD_MEM(ch->paramEB, ch->param); break;

            /* 8xx: Set stereo balance (nonstandard) */
            case 0x8: {
                ch->balance = ch->param;
            } break;

            /* 9xx: Set sample offset */
            case 0x9: {
                if (period != 0 || sample != 0) {
                    ch->param9 = ch->param ? ch->param : ch->param9;
                    ch->position = ch->param9 << 8;
                }
            } break;

            /* Bxx: Jump to pattern */
            case 0xB: {
                c->pattern = ch->param < c->length ? ch->param : 0;
                c->line = -1;
            } break;

            /* Cxx: Set volume */
            case 0xC: {
                ch->volume = _pocketmod_clamp_volume(ch->param);
                ch->dirty |= POCKETMOD_VOLUME;
            } break;

            /* Dxy: Pattern break */
            case 0xD: {
                pattern_break = (ch->param >> 4) * 10 + (ch->param & 15);
            } break;

            /* E4x: Set vibrato waveform */
            case 0xE4: {
                ch->lfo_type[0] = ch->param;
            } break;

            /* E5x: Set sample finetune */
            case 0xE5: {
                ch->finetune = ch->param;
                ch->dirty |= POCKETMOD_PITCH;
            } break;

            /* E6x: Pattern loop */
            case 0xE6: {
                if (ch->param) {
                    if (!ch->loop_count) {
                        ch->loop_count = ch->param;
                        c->line = ch->loop_line;
                    } else if (--ch->loop_count) {
                        c->line = ch->loop_line;
                    }
                } else {
                    ch->loop_line = c->line - 1;
                }
            } break;

            /* E7x: Set tremolo waveform */
            case 0xE7: {
                ch->lfo_type[1] = ch->param;
            } break;

            /* E8x: Set stereo balance (nonstandard) */
            case 0xE8: {
                ch->balance = ch->param << 4;
            } break;

            /* EEx: Pattern delay */
            case 0xEE: {
                c->pattern_delay = ch->param;
            } break;

            /* Fxx: Set speed */
            case 0xF: {
                if (ch->param != 0) {
                    if (ch->param < 0x20) {
                        c->ticks_per_line = ch->param;
                    } else {
                        float rate = c->samples_per_second;
                        c->samples_per_tick = rate / (0.4f * ch->param);
                    }
                }
            } break;

            default: break;
        }
    }

    /* Pattern breaks are handled here, so that only one jump happens even when
     * multiple Dxy commands appear on the same line. (You guessed it: There are
     * songs that rely on this behavior!) */
    if (pattern_break != -1) {
        c->line = (pattern_break < 64 ? pattern_break : 0) - 1;
        if (++c->pattern == c->length) {
            c->pattern = c->reset;
        }
    }
}

static void _pocketmod_next_tick(pocketmod_context *c)
{
    int i;

    /* Move to the next line if this was the last tick */
    if (++c->tick == c->ticks_per_line) {
        if (c->pattern_delay > 0) {
            c->pattern_delay--;
        } else {
            _pocketmod_next_line(c);
        }
        c->tick = 0;
    }

    /* Make per-tick adjustments for all channels */
    for (i = 0; i < c->num_channels; i++) {
        _pocketmod_chan *ch = &c->channels[i];
        int param = ch->param;

        /* Advance the LFO random number generator */
        c->lfo_rng = 0x0019660d * c->lfo_rng + 0x3c6ef35f;

        /* Handle effects that may happen on any tick of a line */
        switch (ch->effect) {

            /* 0xy: Arpeggio */
            case 0x0: {
                ch->dirty |= POCKETMOD_PITCH;
            } break;

            /* E9x: Retrigger note every x ticks */
            case 0xE9: {
                if (!(param && c->tick % param)) {
                    ch->position = 0.0f;
                    ch->lfo_step = 0;
                }
            } break;

            /* ECx: Cut note after x ticks */
            case 0xEC: {
                if (c->tick == param) {
                    ch->volume = 0;
                    ch->dirty |= POCKETMOD_VOLUME;
                }
            } break;

            /* EDx: Delay note for x ticks */
            case 0xED: {
                if (c->tick == param && ch->sample) {
                    unsigned char *data = POCKETMOD_SAMPLE(c, ch->sample);
                    ch->volume = _pocketmod_min(0x40, data[3]);
                    ch->dirty |= POCKETMOD_VOLUME;
                    ch->position = 0.0f;
                    ch->lfo_step = 0;
                }
            } break;

            default: break;
        }

        /* Handle effects that only happen on the first tick of a line */
        if (c->tick == 0) {
            switch (ch->effect) {
                case 0xE1: _pocketmod_pitch_slide(ch, -ch->paramE1); break;
                case 0xE2: _pocketmod_pitch_slide(ch, +ch->paramE2); break;
                case 0xEA: _pocketmod_volume_slide(ch, ch->paramEA << 4); break;
                case 0xEB: _pocketmod_volume_slide(ch, ch->paramEB & 15); break;
                default: break;
            }

        /* Handle effects that are not applied on the first tick of a line */
        } else {
            switch (ch->effect) {

                /* 1xx: Portamento up */
                case 0x1: {
                    _pocketmod_pitch_slide(ch, -param);
                } break;

                /* 2xx: Portamento down */
                case 0x2: {
                    _pocketmod_pitch_slide(ch, +param);
                } break;

                /* 5xy: Volume slide + tone portamento */
                case 0x5: {
                    _pocketmod_volume_slide(ch, param);
                } /* Fall through */

                /* 3xx: Tone portamento */
                case 0x3: {
                    int rate = ch->param3;
                    int order = ch->period < ch->target;
                    int closer = ch->period + (order ? rate : -rate);
                    int new_order = closer < ch->target;
                    ch->period = new_order == order ? closer : ch->target;
                    ch->dirty |= POCKETMOD_PITCH;
                } break;

                /* 6xy: Volume slide + vibrato */
                case 0x6: {
                    _pocketmod_volume_slide(ch, param);
                } /* Fall through */

                /* 4xy: Vibrato */
                case 0x4: {
                    ch->lfo_step++;
                    ch->dirty |= POCKETMOD_PITCH;
                } break;

                /* 7xy: Tremolo */
                case 0x7: {
                    ch->lfo_step++;
                    ch->dirty |= POCKETMOD_VOLUME;
                } break;

                /* Axy: Volume slide */
                case 0xA: {
                    _pocketmod_volume_slide(ch, param);
                } break;

                default: break;
            }
        }

        /* Update channel volume/pitch if either is out of date */
        if (ch->dirty & POCKETMOD_VOLUME) { _pocketmod_update_volume(c, ch); }
        if (ch->dirty & POCKETMOD_PITCH) { _pocketmod_update_pitch(c, ch); }
    }
}

static int _pocketmod_next_sample(pocketmod_context *c, float *output)
{
    int i, pattern_changed = 0;
    const float volume_scale = 1.0f / (128 * 64 * 4);
    const float balance_scale = 1.0f / 255.0f;

    /* Move to the next tick if we were on the last sample */
    if ((c->sample += 1.0f) >= c->samples_per_tick) {
        _pocketmod_next_tick(c);
        c->sample -= c->samples_per_tick;
        pattern_changed = c->line == 0 && c->tick == 0;
    }

    /* Mix channels */
    output[0] = 0.0f;
    output[1] = 0.0f;
    for (i = 0; i < c->num_channels; i++) {
        _pocketmod_chan *ch = &c->channels[i];
        if (ch->position >= 0.0f && ch->sample) {
            _pocketmod_sample *sample = &c->samples[ch->sample - 1];

            /* Resample instrument */
#ifndef POCKETMOD_NO_INTERPOLATION
            float s0, s1, t;
            unsigned int x1;
#endif
            float balance, value;
            unsigned char *data = POCKETMOD_SAMPLE(c, ch->sample);
            unsigned int x0 = (unsigned int) ch->position;
            unsigned int loop_start = ((data[4] << 8) | data[5]) << 1;
            unsigned int loop_length = ((data[6] << 8) | data[7]) << 1;
            unsigned int loop_end = loop_start + loop_length;
#ifdef POCKETMOD_NO_INTERPOLATION
            value = x0 < sample->length ? (float) sample->data[x0] : 0.0f;
#else
            x1 = x0 + 1;
            if (loop_length > 2 && x1 >= loop_end) {
                x1 -= loop_length;
            }
            t = ch->position - x0;
            s0 = x0 < sample->length ? (float) sample->data[x0] : 0.0f;
            s1 = x1 < sample->length ? (float) sample->data[x1] : 0.0f;
            value = s0 * (1.0f - t) + s1 * t;
#endif

            /* Apply volume and stereo balance */
            value *= ch->real_volume * volume_scale;
            balance = ch->balance * balance_scale;
            output[0] += value * (1.0f - balance);
            output[1] += value * (0.0f + balance);

            /* Increment sample position, respecting loops */
            ch->position += ch->increment;
            if (loop_length > 2) {
                while (ch->position >= loop_end) {
                    ch->position -= loop_length;
                }
            }

            /* Cut sample if the end is reached */
            if (ch->position >= sample->length) {
                ch->position = -1.0f;
            }
        }
    }
    return pattern_changed;
}

static int _pocketmod_ident(pocketmod_context *c, unsigned char *data, int size)
{
    int i, j;

    /* 31-instrument files are at least 1084 bytes long */
    if (size >= 1084) {

        /* The format tag is located at offset 1080 */
        unsigned char *tag = data + 1080;

        /* List of recognized format tags (possibly incomplete) */
        static const struct {
            char name[5];
            char channels;
        } tags[] = {
            /* TODO: FLT8 intentionally omitted because I haven't been able */
            /* to find a specimen to test its funky pattern pairing format  */
            {"M.K.",  4}, {"M!K!",  4}, {"FLT4",  4}, {"4CHN",  4},
            {"OKTA",  8}, {"OCTA",  8}, {"CD81",  8}, {"FA08",  8},
            {"1CHN",  1}, {"2CHN",  2}, {"3CHN",  3}, {"4CHN",  4},
            {"5CHN",  5}, {"6CHN",  6}, {"7CHN",  7}, {"8CHN",  8},
            {"9CHN",  9}, {"10CH", 10}, {"11CH", 11}, {"12CH", 12},
            {"13CH", 13}, {"14CH", 14}, {"15CH", 15}, {"16CH", 16},
            {"17CH", 17}, {"18CH", 18}, {"19CH", 19}, {"20CH", 20},
            {"21CH", 21}, {"22CH", 22}, {"23CH", 23}, {"24CH", 24},
            {"25CH", 25}, {"26CH", 26}, {"27CH", 27}, {"28CH", 28},
            {"29CH", 29}, {"30CH", 30}, {"31CH", 31}, {"32CH", 32}
        };

        /* Check the format tag to determine if this is a 31-sample MOD */
        for (i = 0; i < (int) (sizeof(tags) / sizeof(*tags)); i++) {
            if (tags[i].name[0] == tag[0] && tags[i].name[1] == tag[1]
             && tags[i].name[2] == tag[2] && tags[i].name[3] == tag[3]) {
                c->num_channels = tags[i].channels;
                c->length = data[950];
                c->reset = data[951];
                c->order = &data[952];
                c->patterns = &data[1084];
                c->num_samples = 31;
                return 1;
            }
        }
    }

    /* A 15-instrument MOD has to be at least 600 bytes long */
    if (size < 600) {
        return 0;
    }

    /* Check that the song title only contains ASCII bytes (or null) */
    for (i = 0; i < 20; i++) {
        if (data[i] != '\0' && (data[i] < ' ' || data[i] > '~')) {
            return 0;
        }
    }

    /* Check that sample names only contain ASCII bytes (or null) */
    for (i = 0; i < 15; i++) {
        for (j = 0; j < 22; j++) {
            char chr = data[20 + i * 30 + j];
            if (chr != '\0' && (chr < ' ' || chr > '~')) {
                return 0;
            }
        }
    }

    /* It looks like we have an older 15-instrument MOD */
    c->length = data[470];
    c->reset = data[471];
    c->order = &data[472];
    c->patterns = &data[600];
    c->num_samples = 15;
    c->num_channels = 4;
    return 1;
}

int pocketmod_init(pocketmod_context *c, const void *data, int size, int rate)
{
    int i, remaining, header_bytes, pattern_bytes;
    unsigned char *byte = (unsigned char*) c;
    signed char *sample_data;

    /* Check that arguments look more or less sane */
    if (!c || !data || rate <= 0 || size <= 0) {
        return 0;
    }

    /* Zero out the whole context and identify the MOD type */
    _pocketmod_zero(c, sizeof(pocketmod_context));
    c->source = (unsigned char*) data;
    if (!_pocketmod_ident(c, c->source, size)) {
        return 0;
    }

    /* Check that we are compiled with support for enough channels */
    if (c->num_channels > POCKETMOD_MAX_CHANNELS) {
        return 0;
    }

    /* Check that we have enough sample slots for this file */
    if (POCKETMOD_MAX_SAMPLES < 31) {
        byte = (unsigned char*) data + 20;
        for (i = 0; i < c->num_samples; i++) {
            unsigned int length = 2 * ((byte[22] << 8) | byte[23]);
            if (i >= POCKETMOD_MAX_SAMPLES && length > 2) {
                return 0; /* Can't fit this sample */
            }
            byte += 30;
        }
    }

    /* Check that the song length is in valid range (1..128) */
    if (c->length == 0 || c->length > 128) {
        return 0;
    }

    /* Make sure that the reset pattern doesn't take us out of bounds */
    if (c->reset >= c->length) {
        c->reset = 0;
    }

    /* Count how many patterns there are in the file */
    c->num_patterns = 0;
    for (i = 0; i < 128 && c->order[i] < 128; i++) {
        c->num_patterns = _pocketmod_max(c->num_patterns, c->order[i]);
    }
    pattern_bytes = 256 * c->num_channels * ++c->num_patterns;
    header_bytes = (int) ((char*) c->patterns - (char*) data);

    /* Check that each pattern in the order is within file bounds */
    for (i = 0; i < c->length; i++) {
        if (header_bytes + 256 * c->num_channels * c->order[i] > size) {
            return 0; /* Reading this pattern would be a buffer over-read! */
        }
    }

    /* Check that the pattern data doesn't extend past the end of the file */
    if (header_bytes + pattern_bytes > size) {
        return 0;
    }

    /* Load sample payload data, truncating ones that extend outside the file */
    remaining = size - header_bytes - pattern_bytes;
    sample_data = (signed char*) data + header_bytes + pattern_bytes;
    for (i = 0; i < c->num_samples; i++) {
        unsigned char *data = POCKETMOD_SAMPLE(c, i + 1);
        unsigned int length = ((data[0] << 8) | data[1]) << 1;
        _pocketmod_sample *sample = &c->samples[i];
        sample->data = sample_data;
        sample->length = _pocketmod_min(length > 2 ? length : 0, remaining);
        sample_data += sample->length;
        remaining -= sample->length;
    }

    /* Set up ProTracker default panning for all channels */
    for (i = 0; i < c->num_channels; i++) {
        c->channels[i].balance = 0x80 + ((((i + 1) >> 1) & 1) ? 0x20 : -0x20);
    }

    /* Prepare for rendering from the start */
    c->ticks_per_line = 6;
    c->samples_per_second = rate;
    c->samples_per_tick = rate / 50.0f;
    c->lfo_rng = 0xbadc0de;
    c->line = -1;
    c->tick = c->ticks_per_line - 1;
    _pocketmod_next_tick(c);
    return 1;
}

int pocketmod_render(pocketmod_context *c, void *buffer, int buffer_size)
{
    int bytes_per_sample = sizeof(float[2]), i, j;
    int samples_to_render = buffer_size / bytes_per_sample;
    if (c && buffer && samples_to_render > 0) {
        float (*samples)[2] = (float(*)[2]) buffer;
        for (i = 0; i < samples_to_render; i++) {

            /* Generate another sample */
            if (_pocketmod_next_sample(c, samples[i])) {

                /* Increment loop counter if we've seen this pattern before */
                if (c->visited[c->pattern >> 3] & (1 << (c->pattern & 7))) {
                    _pocketmod_zero(c->visited, sizeof(c->visited));
                    c->loop_count++;
                }

                /* Return early so the caller can decide whether to continue */
                break;
            }
        }
        return i * bytes_per_sample;
    }
    return 0;
}

int pocketmod_loop_count(pocketmod_context *c)
{
    return c->loop_count;
}

#endif /* #ifdef POCKETMOD_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef POCKETMOD_H_INCLUDED */

/*******************************************************************************

MIT License

Copyright (c) 2018 rombankzero

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*******************************************************************************/
