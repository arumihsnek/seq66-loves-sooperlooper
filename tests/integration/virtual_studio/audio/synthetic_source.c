/*
 * synthetic_audio_source.c — JACK client that generates deterministic audio.
 *
 * Build:
 *   gcc -std=c11 -O2 -o synthetic_source synthetic_audio_source.c -ljack -lm -lpthread
 */

#include <jack/jack.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    const char *mode;
    const char *client_name;
    const char *server_name;
    double tempo;
    int numerator;
    int denominator;
    int ppqn;
    unsigned int seed;
    int continuous;
    long max_frames;
    int dropout_at_frame;
    int clip_at_frame;
    int phase_invert_at_frame;
    int discontinuity_at_frame;
} config_t;

typedef struct {
    jack_client_t *client;
    jack_port_t *out_l;
    jack_port_t *out_r;
    config_t cfg;
    double sample_rate;
    long absolute_frame;
    int current_bar;
    int current_beat;
    int current_pulse;
    double frames_per_beat;
    double frames_per_bar;
    double frames_per_pulse;
    double phase;
    int marker_count;
    int downbeat_count;
    unsigned int noise_seed;
    volatile int shutdown;
} source_state_t;

static double generate_sine(source_state_t *s, double freq, int channel) {
    double val = sin(s->phase + channel * M_PI * 0.25);
    s->phase += 2.0 * M_PI * freq / s->sample_rate;
    if (s->phase > 2.0 * M_PI) s->phase -= 2.0 * M_PI;
    return val * 0.8;
}

static double generate_beat_marker(source_state_t *s, int channel) {
    int frame_in_beat = s->absolute_frame % (int)s->frames_per_beat;
    int frame_in_bar = s->absolute_frame % (int)s->frames_per_bar;
    int is_downbeat = (frame_in_bar < 3);
    int is_beat = (frame_in_beat < 3);

    if (is_downbeat) {
        double val = (channel == 0) ? 0.9 : 0.7;
        val *= (1.0 - (s->current_bar % 8) * 0.1);
        return val;
    } else if (is_beat) {
        return (channel == 0) ? 0.5 : 0.3;
    }
    return generate_sine(s, 440.0 + s->current_bar * 10.0, channel);
}

static double generate_bar_marker(source_state_t *s, int channel) {
    int frame_in_bar = s->absolute_frame % (int)s->frames_per_bar;
    double marker_width = s->frames_per_bar * 0.01;
    if (frame_in_bar < (int)marker_width) {
        double base = 0.7 + (s->current_bar % 10) * 0.03;
        return (channel == 0) ? base : base * 0.8;
    }
    return 0.0;
}

static double generate_noise(source_state_t *s) {
    s->noise_seed = s->noise_seed * 1103515245 + 12345;
    double val = ((s->noise_seed >> 16) & 0x7FFF) / 32767.0;
    return (val - 0.5) * 0.6;
}

static double generate_chirp(source_state_t *s) {
    double t = (double)s->absolute_frame / s->sample_rate;
    double f0 = 100.0, f1 = 2000.0, duration = 4.0;
    double freq = f0 + (f1 - f0) * (t / duration);
    return sin(2.0 * M_PI * freq * t) * 0.7;
}

static int process_callback(jack_nframes_t nframes, void *arg) {
    source_state_t *s = (source_state_t *)arg;
    jack_default_audio_sample_t *out_l = jack_port_get_buffer(s->out_l, nframes);
    jack_default_audio_sample_t *out_r = jack_port_get_buffer(s->out_r, nframes);

    for (jack_nframes_t i = 0; i < nframes; i++) {
        if (s->cfg.max_frames > 0 && s->absolute_frame >= s->cfg.max_frames) {
            out_l[i] = 0.0; out_r[i] = 0.0; continue;
        }

        int frame_in_beat = s->absolute_frame % (int)s->frames_per_beat;
        int frame_in_bar = s->absolute_frame % (int)s->frames_per_bar;

        if (frame_in_bar == 0 && s->absolute_frame > 0) {
            s->current_bar++; s->current_beat = 0; s->current_pulse = 0;
            s->downbeat_count++;
        }
        if (frame_in_beat == 0 && s->absolute_frame > 0) s->current_beat++;

        double val_l = 0.0, val_r = 0.0;

        if (strcmp(s->cfg.mode, "sine") == 0) {
            val_l = generate_sine(s, 440.0, 0);
            val_r = generate_sine(s, 440.0, 1);
        } else if (strcmp(s->cfg.mode, "beats") == 0) {
            val_l = generate_beat_marker(s, 0);
            val_r = generate_beat_marker(s, 1);
            if (frame_in_beat == 0) s->marker_count++;
        } else if (strcmp(s->cfg.mode, "markers") == 0) {
            val_l = generate_bar_marker(s, 0);
            val_r = generate_bar_marker(s, 1);
            if (frame_in_bar == 0) s->marker_count++;
        } else if (strcmp(s->cfg.mode, "noise") == 0) {
            val_l = generate_noise(s);
            val_r = generate_noise(s);
        } else if (strcmp(s->cfg.mode, "chirp") == 0) {
            val_l = generate_chirp(s);
            val_r = generate_chirp(s);
        }

        /* Fault injection */
        if (s->cfg.dropout_at_frame >= 0 &&
            s->absolute_frame >= s->cfg.dropout_at_frame &&
            s->absolute_frame < s->cfg.dropout_at_frame + (int)s->frames_per_beat) {
            val_l = 0.0; val_r = 0.0;
        }
        if (s->cfg.clip_at_frame >= 0 &&
            s->absolute_frame >= s->cfg.clip_at_frame &&
            s->absolute_frame < s->cfg.clip_at_frame + 100) {
            val_l = 1.0; val_r = -1.0;
        }
        if (s->cfg.phase_invert_at_frame >= 0 &&
            s->absolute_frame >= s->cfg.phase_invert_at_frame) {
            val_l = -val_l; val_r = -val_r;
        }
        if (s->cfg.discontinuity_at_frame >= 0 &&
            s->absolute_frame == s->cfg.discontinuity_at_frame) {
            val_l = -val_l; val_r = -val_r;
        }

        out_l[i] = (jack_default_audio_sample_t)val_l;
        out_r[i] = (jack_default_audio_sample_t)val_r;
        s->absolute_frame++;
    }
    return 0;
}

static void shutdown_callback(void *arg) {
    ((source_state_t *)arg)->shutdown = 1;
}

int main(int argc, char *argv[]) {
    source_state_t state;
    memset(&state, 0, sizeof(state));
    state.cfg.mode = "beats";
    state.cfg.client_name = "synthetic_source";
    state.cfg.tempo = 120.0;
    state.cfg.numerator = 4;
    state.cfg.denominator = 4;
    state.cfg.ppqn = 480;
    state.cfg.seed = 42;
    state.cfg.continuous = 1;
    state.cfg.dropout_at_frame = -1;
    state.cfg.clip_at_frame = -1;
    state.cfg.phase_invert_at_frame = -1;
    state.cfg.discontinuity_at_frame = -1;
    state.noise_seed = 42;

    static struct option long_opts[] = {
        {"mode", required_argument, 0, 'm'}, {"name", required_argument, 0, 'n'},
        {"server", required_argument, 0, 's'}, {"tempo", required_argument, 0, 't'},
        {"numerator", required_argument, 0, 'N'}, {"denominator", required_argument, 0, 'D'},
        {"max-frames", required_argument, 0, 'f'}, {"dropout", required_argument, 0, 'd'},
        {"clip", required_argument, 0, 'c'}, {"invert", required_argument, 0, 'i'},
        {"discontinuity", required_argument, 0, 'x'}, {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "m:n:s:t:N:D:f:d:c:i:x:h", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'm': state.cfg.mode = optarg; break;
            case 'n': state.cfg.client_name = optarg; break;
            case 's': state.cfg.server_name = optarg; break;
            case 't': state.cfg.tempo = atof(optarg); break;
            case 'N': state.cfg.numerator = atoi(optarg); break;
            case 'D': state.cfg.denominator = atoi(optarg); break;
            case 'f': state.cfg.max_frames = atol(optarg); break;
            case 'd': state.cfg.dropout_at_frame = atoi(optarg); break;
            case 'c': state.cfg.clip_at_frame = atoi(optarg); break;
            case 'i': state.cfg.phase_invert_at_frame = atoi(optarg); break;
            case 'x': state.cfg.discontinuity_at_frame = atoi(optarg); break;
            case 'h':
                fprintf(stderr, "Usage: %s [--mode MODE] [--tempo BPM] [--numerator N] [--denominator D]\n", argv[0]);
                return 0;
            default: return 1;
        }
    }

    jack_status_t status;
    jack_options_t opts = JackNoStartServer;
    if (state.cfg.server_name) opts = (jack_options_t)(opts | JackServerName);

    state.client = jack_client_open(state.cfg.client_name, opts, &status, state.cfg.server_name);
    if (!state.client) { fprintf(stderr, "Cannot connect to JACK\n"); return 1; }

    state.sample_rate = jack_get_sample_rate(state.client);
    state.frames_per_beat = (60.0 / state.cfg.tempo) * state.sample_rate;
    state.frames_per_bar = state.frames_per_beat * state.cfg.numerator;

    fprintf(stderr, "synthetic_source: mode=%s tempo=%.1f %d/%d sr=%.0f fpb=%.0f fpb_bar=%.0f\n",
            state.cfg.mode, state.cfg.tempo, state.cfg.numerator, state.cfg.denominator,
            state.sample_rate, state.frames_per_beat, state.frames_per_bar);

    state.out_l = jack_port_register(state.client, "out_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    state.out_r = jack_port_register(state.client, "out_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!state.out_l || !state.out_r) { jack_client_close(state.client); return 1; }

    jack_set_process_callback(state.client, process_callback, &state);
    jack_on_shutdown(state.client, shutdown_callback, &state);

    if (jack_activate(state.client)) { jack_client_close(state.client); return 1; }

    fprintf(stderr, "synthetic_source: started pid=%d\n", getpid());

    while (!state.shutdown) {
        if (!state.cfg.continuous && state.cfg.max_frames > 0 &&
            state.absolute_frame >= state.cfg.max_frames) break;
        usleep(100000);
    }

    fprintf(stderr, "synthetic_source: stopping frames=%ld bars=%d markers=%d\n",
            state.absolute_frame, state.current_bar, state.marker_count);

    jack_deactivate(state.client);
    jack_client_close(state.client);
    return 0;
}
