#ifdef __cplusplus
extern "C" {
#endif

#ifndef POCKETMOD_H_INCLUDED
#define POCKETMOD_H_INCLUDED

typedef struct pocketmod_context pocketmod_context;
int pocketmod_init(pocketmod_context *ctx, const void *data, int size, int rate);
int pocketmod_render(pocketmod_context *ctx, void *buffer, int samples);

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
} _pocketmod_channel;

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
    int samples_per_tick;       /* Depends on sample rate and BPM          */
    int ticks_per_line;         /* A.K.A. song speed (initially 6)         */

    /* Playback state */
    _pocketmod_channel channels[POCKETMOD_MAX_CHANNELS];
    unsigned char pattern_delay;/* EEx pattern delay counter               */
    unsigned int lfo_rng;       /* RNG used for the random LFO waveform    */

    /* Playback position (from least to most granular) */
    signed char pattern;        /* Current pattern in order                */
    signed char line;           /* Current line in pattern                 */
    short tick;                 /* Current tick in line                    */
    int sample;                 /* Current sample in tick                  */
};

#endif /* #ifndef POCKETMOD_H_INCLUDED */
#ifdef POCKETMOD_IMPLEMENTATION

/* Memorize a parameter unless the new value is zero */
#define POCKETMOD_MEMORIZE(dst, src) do { \
        (dst) = (src) ? (src) : (dst); \
    } while (0)

/* Same thing, but memorize each nibble separately */
#define POCKETMOD_MEMORIZE2(dst, src) do { \
        (dst) = (((src) & 0x0f) ? ((src) & 0x0f) : ((dst) & 0x0f)) \
              | (((src) & 0xf0) ? ((src) & 0xf0) : ((dst) & 0xf0)); \
    } while (0)

/* Shortcut to sample metadata (sample must be nonzero) */
#define POCKETMOD_SAMPLE(ctx, sample) ((ctx)->source + 12 + 30 * (sample))

/* Channel dirty flags */
#define POCKETMOD_PITCH  0x01
#define POCKETMOD_VOLUME 0x02

/* Amiga period table. Three octaves per finetune setting. */
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

/* Various helper functions */
static int _pocketmod_min(int x, int y) { return x < y ? x : y; }
static int _pocketmod_max(int x, int y) { return x > y ? x : y; }
static int _pocketmod_clamp_volume(int x) { return _pocketmod_max(0, _pocketmod_min(64, x)); }

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
    return _pocketmod_amiga_period[finetune][_pocketmod_period_to_note(period)];
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
static int _pocketmod_lfo(pocketmod_context *ctx, _pocketmod_channel *chan, int step)
{
    switch (chan->lfo_type[chan->effect == 7] & 3) {
        case 0: return _pocketmod_sin(step & 0x3f);              /* Sine   */
        case 1: return 0xff - ((step & 0x3f) << 3);         /* Saw    */
        case 2: return (step & 0x3f) < 0x20 ? 0xff : -0xff; /* Square */
        case 3: return (ctx->lfo_rng & 0x1ff) - 0xff;       /* Random */
        default: return 0; /* Hush little compiler */
    }
}

static void _pocketmod_update_pitch(pocketmod_context *ctx, _pocketmod_channel *chan)
{
    /* Don't do anything if the period is zero */
    chan->increment = 0.0f;
    chan->dirty &= ~POCKETMOD_PITCH;
    if (chan->period) {
        float freq, period = chan->period, semi = 1.0f;

        /* Apply vibrato or arpeggio (if active) */
        if (chan->effect == 0x4 || chan->effect == 0x6) {
            int step = (chan->param4 >> 4) * chan->lfo_step;
            int rate = chan->param4 & 0x0f;
            period += _pocketmod_lfo(ctx, chan, step) * rate / 128.0f;
        } else if (chan->effect == 0x0) {
            static const float arpeggio[16] = { /* 2^(X/12) for X in 0..15 */
                1.000000f, 1.059463f, 1.122462f, 1.189207f,
                1.259921f, 1.334840f, 1.414214f, 1.498307f,
                1.587401f, 1.681793f, 1.781797f, 1.887749f,
                2.000000f, 2.118926f, 2.244924f, 2.378414f
            };
            semi = arpeggio[(chan->param >> ((2 - ctx->tick % 3) << 2)) & 0x0f];
        }

        /* Convert to Hz and update sample buffer increment */
        freq = 7093789.2f / (period * 2.0f) * semi;
        chan->increment = freq / ctx->samples_per_second;
    }
}

static void _pocketmod_update_volume(pocketmod_context *ctx, _pocketmod_channel *chan)
{
    int volume = chan->volume;
    if (chan->effect == 0x7) {
        int step = chan->lfo_step * (chan->param7 >> 4);
        volume += _pocketmod_lfo(ctx, chan, step) * (chan->param7 & 0x0f) >> 6;
    }
    chan->real_volume = _pocketmod_clamp_volume(volume);
    chan->dirty &= ~POCKETMOD_VOLUME;
}

static void _pocketmod_pitch_slide(_pocketmod_channel *chan, int amount)
{
    int max = _pocketmod_amiga_period[chan->finetune][0];
    int min = _pocketmod_amiga_period[chan->finetune][35];
    chan->period = _pocketmod_max(min, _pocketmod_min(max, chan->period + amount));
    chan->dirty |= POCKETMOD_PITCH;
}

static void _pocketmod_volume_slide(_pocketmod_channel *chan, int param)
{
    /* Undocumented quirk: If both x and y are nonzero, then the value of x */
    /* takes precedence. (Yes, there are songs that rely on this behavior.) */
    int change = (param & 0xf0) ? (param >> 4) : -(param & 0x0f);
    chan->volume = _pocketmod_clamp_volume(chan->volume + change);
    chan->dirty |= POCKETMOD_VOLUME;
}

static void _pocketmod_next_line(pocketmod_context *ctx)
{
    unsigned char (*data)[4];
    int i, pos;

    /* Move to the next pattern if this was the last line */
    if (++ctx->line == 64) {
        if (++ctx->pattern == ctx->length) {
            ctx->pattern = ctx->reset;
        }
        ctx->line = 0;
    }

    /* Find the pattern data for the current line */
    pos = (ctx->order[ctx->pattern] * 64 + ctx->line) * ctx->num_channels * 4;
    data = (unsigned char(*)[4]) (ctx->patterns + pos);
    for (i = 0; i < ctx->num_channels; i++) {

        /* Decode columns */
        int sample = (data[i][0] & 0xf0) | (data[i][2] >> 4);
        int period = ((data[i][0] & 0x0f) << 8) | data[i][1];
        int effect = ((data[i][2] & 0x0f) << 8) | data[i][3];

        /* Memorize effect parameter values */
        _pocketmod_channel *chan = &ctx->channels[i];
        chan->effect = (effect >> 8) != 0xe ? (effect >> 8) : (effect >> 4);
        chan->param = (effect >> 8) != 0xe ? (effect & 0xff) : (effect & 0x0f);

        /* Set sample */
        if (sample) {
            if (sample <= POCKETMOD_MAX_SAMPLES) {
                unsigned char *sample_data = POCKETMOD_SAMPLE(ctx, sample);
                chan->sample = sample;
                chan->finetune = sample_data[2] & 0x0f;
                if (chan->effect != 0xED) {
                    chan->volume = _pocketmod_min(sample_data[3], 0x40);
                    chan->dirty |= POCKETMOD_VOLUME;
                }
            } else {
                chan->sample = 0;
            }
        }

        /* Set note */
        if (period) {
            period = _pocketmod_finetune(period, chan->finetune);
            if (chan->effect != 0x3) {
                if (chan->effect != 0xED) {
                    chan->position = 0.0f;
                }
                chan->dirty |= POCKETMOD_PITCH;
                chan->period = period;
                chan->lfo_step = 0;
            }
        }

        /* Handle pattern effects */
        switch (chan->effect) {

            /* Memorize parameters */
            case 0x3: POCKETMOD_MEMORIZE(chan->param3, chan->param); /* Fall through */
            case 0x5: POCKETMOD_MEMORIZE(chan->target, period); break;
            case 0x4: POCKETMOD_MEMORIZE2(chan->param4, chan->param); break;
            case 0x7: POCKETMOD_MEMORIZE2(chan->param7, chan->param); break;
            case 0xE1: POCKETMOD_MEMORIZE(chan->paramE1, chan->param); break;
            case 0xE2: POCKETMOD_MEMORIZE(chan->paramE2, chan->param); break;
            case 0xEA: POCKETMOD_MEMORIZE(chan->paramEA, chan->param); break;
            case 0xEB: POCKETMOD_MEMORIZE(chan->paramEB, chan->param); break;

            /* 8xx: Set stereo balance (nonstandard) */
            case 0x8: {
                chan->balance = chan->param;
            } break;

            /* 9xx: Set sample offset */
            case 0x9: {
                if (period != 0 || sample != 0) {
                    chan->param9 = chan->param ? chan->param : chan->param9;
                    chan->position = chan->param9 << 8;
                }
            } break;

            /* Bxx: Jump to pattern */
            case 0xB: {
                ctx->pattern = chan->param < ctx->length ? chan->param : 0;
                ctx->line = -1;
            } break;

            /* Cxx: Set volume */
            case 0xC: {
                chan->volume = _pocketmod_clamp_volume(chan->param);
                chan->dirty |= POCKETMOD_VOLUME;
            } break;

            /* Dxy: Pattern break */
            case 0xD: {
                int line = (chan->param >> 4) * 10 + (chan->param & 15);
                ctx->line = (line < 64 ? line : 0) - 1;
                if (++ctx->pattern == ctx->length) {
                    ctx->pattern = ctx->reset;
                }
            } break;

            /* E4x: Set vibrato waveform */
            case 0xE4: {
                chan->lfo_type[0] = chan->param;
            } break;

            /* E5x: Set sample finetune */
            case 0xE5: {
                chan->finetune = chan->param;
                chan->dirty |= POCKETMOD_PITCH;
            } break;

            /* E6x: Pattern loop */
            case 0xE6: {
                if (chan->param) {
                    if (!chan->loop_count) {
                        chan->loop_count = chan->param;
                        ctx->line = chan->loop_line;
                    } else if (--chan->loop_count) {
                        ctx->line = chan->loop_line;
                    }
                } else {
                    chan->loop_line = ctx->line - 1;
                }
            } break;

            /* E7x: Set tremolo waveform */
            case 0xE7: {
                chan->lfo_type[1] = chan->param;
            } break;

            /* E8x: Set stereo balance (nonstandard) */
            case 0xE8: {
                chan->balance = chan->param << 4;
            } break;

            /* EEx: Pattern delay */
            case 0xEE: {
                ctx->pattern_delay = chan->param;
            } break;

            /* Fxx: Set speed */
            case 0xF: {
                if (chan->param != 0) {
                    if (chan->param < 0x20) {
                        ctx->ticks_per_line = chan->param;
                    } else {
                        float rate = ctx->samples_per_second;
                        ctx->samples_per_tick = rate / (0.4f * chan->param);
                    }
                }
            } break;

            default: break;
        }
    }
}

static void _pocketmod_next_tick(pocketmod_context *ctx)
{
    int i;

    /* Move to the next line if this was the last tick */
    if (++ctx->tick == ctx->ticks_per_line) {
        if (ctx->pattern_delay > 0) {
            ctx->pattern_delay--;
        } else {
            _pocketmod_next_line(ctx);
        }
        ctx->tick = 0;
    }

    /* Make per-tick adjustments for all channels */
    for (i = 0; i < ctx->num_channels; i++) {
        _pocketmod_channel *chan = &ctx->channels[i];
        int param = chan->param;

        /* Advance the LFO random number generator */
        ctx->lfo_rng = 0x0019660d * ctx->lfo_rng + 0x3c6ef35f;

        /* Handle effects that may happen on any tick of a line */
        switch (chan->effect) {

            /* 0xy: Arpeggio */
            case 0x0: {
                chan->dirty |= POCKETMOD_PITCH;
            } break;

            /* E9x: Retrigger note every x ticks */
            case 0xE9: {
                if (!(param && ctx->tick % param)) {
                    chan->position = 0.0f;
                    chan->lfo_step = 0;
                }
            } break;

            /* ECx: Cut note after x ticks */
            case 0xEC: {
                if (ctx->tick == param) {
                    chan->volume = 0;
                    chan->dirty |= POCKETMOD_VOLUME;
                }
            } break;

            /* EDx: Delay note for x ticks */
            case 0xED: {
                if (ctx->tick == param && chan->sample) {
                    unsigned char *data = POCKETMOD_SAMPLE(ctx, chan->sample);
                    chan->volume = _pocketmod_min(0x40, data[3]);
                    chan->dirty |= POCKETMOD_VOLUME;
                    chan->position = 0.0f;
                    chan->lfo_step = 0;
                }
            } break;

            default: break;
        }

        /* Handle effects that only happen on the first tick of a line */
        if (ctx->tick == 0) {
            switch (chan->effect) {
                case 0xE1: _pocketmod_pitch_slide(chan, -chan->paramE1); break;
                case 0xE2: _pocketmod_pitch_slide(chan, +chan->paramE2); break;
                case 0xEA: _pocketmod_volume_slide(chan, chan->paramEA << 4); break;
                case 0xEB: _pocketmod_volume_slide(chan, chan->paramEB & 15); break;
                default: break;
            }

        /* Handle effects that are not applied on the first tick of a line */
        } else {
            switch (chan->effect) {

                /* 1xx: Portamento up */
                case 0x1: {
                    _pocketmod_pitch_slide(chan, -param);
                } break;

                /* 2xx: Portamento down */
                case 0x2: {
                    _pocketmod_pitch_slide(chan, +param);
                } break;

                /* 5xy: Volume slide + tone portamento */
                case 0x5: {
                    _pocketmod_volume_slide(chan, param);
                } /* Fall through */

                /* 3xx: Tone portamento */
                case 0x3: {
                    int rate = chan->param3;
                    int order = chan->period < chan->target;
                    int closer = chan->period + (order ? rate : -rate);
                    int new_order = closer < chan->target;
                    chan->period = new_order == order ? closer : chan->target;
                    chan->dirty |= POCKETMOD_PITCH;
                } break;

                /* 6xy: Volume slide + vibrato */
                case 0x6: {
                    _pocketmod_volume_slide(chan, param);
                } /* Fall through */

                /* 4xy: Vibrato */
                case 0x4: {
                    chan->lfo_step++;
                    chan->dirty |= POCKETMOD_PITCH;
                } break;

                /* 7xy: Tremolo */
                case 0x7: {
                    chan->lfo_step++;
                    chan->dirty |= POCKETMOD_VOLUME;
                } break;

                /* Axy: Volume slide */
                case 0xA: {
                    _pocketmod_volume_slide(chan, param);
                } break;

                default: break;
            }
        }

        /* Update channel volume/pitch if either is out of date */
        if (chan->dirty & POCKETMOD_VOLUME) { _pocketmod_update_volume(ctx, chan); }
        if (chan->dirty & POCKETMOD_PITCH) { _pocketmod_update_pitch(ctx, chan); }
    }
}

static void _pocketmod_next_sample(pocketmod_context *ctx, float *output)
{
    int i;

    /* Move to the next tick if we were on the last sample */
    if (++ctx->sample == ctx->samples_per_tick) {
        _pocketmod_next_tick(ctx);
        ctx->sample = 0;
    }

    /* Mix channels */
    output[0] = 0.0f;
    output[1] = 0.0f;
    for (i = 0; i < ctx->num_channels; i++) {
        _pocketmod_channel *chan = &ctx->channels[i];
        if (chan->position >= 0.0f && chan->sample) {
            _pocketmod_sample *sample = &ctx->samples[chan->sample - 1];

            /* Resample instrument */
#ifndef POCKETMOD_NO_INTERPOLATION
            float s0, s1, t;
            unsigned int x1;
#endif
            float balance, value;
            unsigned char *data = POCKETMOD_SAMPLE(ctx, chan->sample);
            unsigned int x0 = (unsigned int) chan->position;
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
            t = chan->position - x0;
            s0 = x0 < sample->length ? (float) sample->data[x0] : 0.0f;
            s1 = x1 < sample->length ? (float) sample->data[x1] : 0.0f;
            value = s0 * (1.0f - t) + s1 * t;
#endif

            /* Apply volume and stereo balance */
            value *= chan->real_volume / (128.0f * 64.0f) * 0.25f;
            balance = chan->balance / 255.0f;
            output[0] += value * (1.0f - balance);
            output[1] += value * (0.0f + balance);

            /* Increment sample position, respecting loops */
            chan->position += chan->increment;
            if (loop_length > 2) {
                while (chan->position >= loop_end) {
                    chan->position -= loop_length;
                }
            }

            /* Cut sample if the end is reached */
            if (chan->position >= sample->length) {
                chan->position = -1.0f;
            }
        }
    }
}

static int _pocketmod_identify(pocketmod_context *ctx, unsigned char *data, int size)
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
                ctx->num_channels = tags[i].channels;
                ctx->length = data[950];
                ctx->reset = data[951];
                ctx->order = &data[952];
                ctx->patterns = &data[1084];
                ctx->num_samples = 31;
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
    ctx->length = data[470];
    ctx->reset = data[471];
    ctx->order = &data[472];
    ctx->patterns = &data[600];
    ctx->num_samples = 15;
    ctx->num_channels = 4;
    return 1;
}

int pocketmod_init(pocketmod_context *ctx, const void *data, int size, int rate)
{
    int i, remaining, header_bytes, pattern_bytes;
    unsigned char *byte;
    signed char *sample_data;

    /* Start by zeroing out the whole context, for simplicity's sake */
    byte = (unsigned char*) ctx;
    for (i = 0; i < (int) sizeof(pocketmod_context); i++) {
        byte[i] = 0;
    }

    /* Check that arguments look more or less sane */
    ctx->source = (unsigned char*) data;
    if (!ctx || !data || rate <= 0 || size <= 0
     || !_pocketmod_identify(ctx, (unsigned char*) data, size)) {
        return 0;
    }

    /* Check that we are compiled with support for enough channels */
    if (ctx->num_channels > POCKETMOD_MAX_CHANNELS) {
        return 0;
    }

    /* Check that we have enough sample slots for this file */
    if (POCKETMOD_MAX_SAMPLES < 31) {
        byte = (unsigned char*) data + 20;
        for (i = 0; i < ctx->num_samples; i++) {
            unsigned int length = 2 * ((byte[22] << 8) | byte[23]);
            if (i >= POCKETMOD_MAX_SAMPLES && length > 2) {
                return 0; /* Can't fit this sample */
            }
            byte += 30;
        }
    }

    /* Check that the song length is in valid range (1..128) */
    if (ctx->length == 0 || ctx->length > 128) {
        return 0;
    }

    /* Make sure that the reset pattern doesn't take us out of bounds */
    if (ctx->reset >= ctx->length) {
        ctx->reset = 0;
    }

    /* Count how many patterns there are in the file */
    ctx->num_patterns = 0;
    for (i = 0; i < 128 && ctx->order[i] < 128; i++) {
        ctx->num_patterns = _pocketmod_max(ctx->num_patterns, ctx->order[i]);
    }
    pattern_bytes = 256 * ctx->num_channels * ++ctx->num_patterns;
    header_bytes = (int) ((char*) ctx->patterns - (char*) data);

    /* Check that each pattern in the order is within file bounds */
    for (i = 0; i < ctx->length; i++) {
        if (header_bytes + 256 * ctx->num_channels * ctx->order[i] > size) {
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
    for (i = 0; i < ctx->num_samples; i++) {
        unsigned char *data = POCKETMOD_SAMPLE(ctx, i + 1);
        unsigned int length = ((data[0] << 8) | data[1]) << 1;
        _pocketmod_sample *sample = &ctx->samples[i];
        sample->data = sample_data;
        sample->length = _pocketmod_min(length > 2 ? length : 0, remaining);
        sample_data += sample->length;
        remaining -= sample->length;
    }

    /* Set up ProTracker default panning for all channels */
    for (i = 0; i < ctx->num_channels; i++) {
        ctx->channels[i].balance = 0x80 + ((((i + 1) >> 1) & 1) ? 0x20 : -0x20);
    }

    /* Prepare for rendering from the start */
    ctx->ticks_per_line = 6;
    ctx->samples_per_second = rate;
    ctx->samples_per_tick = rate / 50;
    ctx->lfo_rng = 0xbaadc0de;
    ctx->line = -1;
    ctx->tick = ctx->ticks_per_line - 1;
    ctx->sample = ctx->samples_per_tick - 1;
    return 1;
}

int pocketmod_render(pocketmod_context *ctx, void *buffer, int count)
{
    if (ctx && buffer && count > 0) {
        int i;
        float (*samples)[2] = (float(*)[2]) buffer;
        for (i = 0; i < count; i++) {
            _pocketmod_next_sample(ctx, samples[i]);
        }
        return count;
    }
    return 0;
}

#endif /* #ifdef POCKETMOD_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif
