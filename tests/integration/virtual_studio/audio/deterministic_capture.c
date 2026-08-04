#define _POSIX_C_SOURCE 199309L
/*
 * deterministic_capture.c — JACK client that captures audio to WAV.
 *
 * Build:
 *   gcc -std=c11 -O2 -o deterministic_capture deterministic_capture.c -ljack -lm -lpthread
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

typedef struct {
    char riff[4]; uint32_t file_size; char wave[4];
    char fmt[4]; uint32_t fmt_size; uint16_t audio_format;
    uint16_t num_channels; uint32_t sample_rate; uint32_t byte_rate;
    uint16_t block_align; uint16_t bits_per_sample;
    char data[4]; uint32_t data_size;
} __attribute__((packed)) wav_header_t;

#define RING_CAP (1 << 20)

typedef struct {
    float *buf; int wpos, rpos, count, cap;
    pthread_mutex_t mtx; pthread_cond_t cond;
} ring_t;

static void ring_init(ring_t *r) {
    r->buf = calloc(RING_CAP, sizeof(float));
    r->wpos = r->rpos = r->count = 0; r->cap = RING_CAP;
    pthread_mutex_init(&r->mtx, NULL);
    pthread_cond_init(&r->cond, NULL);
}
static void ring_free(ring_t *r) {
    free(r->buf); pthread_mutex_destroy(&r->mtx); pthread_cond_destroy(&r->cond);
}
static void ring_write(ring_t *r, const float *d, int n) {
    pthread_mutex_lock(&r->mtx);
    for (int i = 0; i < n && r->count < r->cap; i++) {
        r->buf[r->wpos] = d[i]; r->wpos = (r->wpos+1) % r->cap; r->count++;
    }
    pthread_cond_signal(&r->cond); pthread_mutex_unlock(&r->mtx);
}
static int ring_read(ring_t *r, float *d, int mx, int ms) {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms/1000; ts.tv_nsec += (ms%1000)*1000000;
    if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
    pthread_mutex_lock(&r->mtx);
    while (r->count == 0) {
        if (pthread_cond_timedwait(&r->cond, &r->mtx, &ts) != 0) {
            pthread_mutex_unlock(&r->mtx); return 0;
        }
    }
    int n = r->count < mx ? r->count : mx;
    for (int i = 0; i < n; i++) {
        d[i] = r->buf[r->rpos]; r->rpos = (r->rpos+1) % r->cap; r->count--;
    }
    pthread_mutex_unlock(&r->mtx); return n;
}
static int ring_avail(ring_t *r) {
    pthread_mutex_lock(&r->mtx); int n = r->count; pthread_mutex_unlock(&r->mtx); return n;
}

typedef struct {
    jack_client_t *client; jack_port_t *in_l, *in_r;
    const char *output; ring_t ring;
    long total_frames; int cb_count, xrun_count;
    jack_nframes_t period_size; int sr;
    volatile int shutdown;
    FILE *trace;
} cap_t;

static int process_cb(jack_nframes_t nframes, void *arg) {
    cap_t *c = (cap_t *)arg;
    jack_default_audio_sample_t *il = jack_port_get_buffer(c->in_l, nframes);
    jack_default_audio_sample_t *ir = jack_port_get_buffer(c->in_r, nframes);
    float interleaved[nframes * 2];
    for (jack_nframes_t i = 0; i < nframes; i++) {
        interleaved[i*2] = il[i]; interleaved[i*2+1] = ir[i];
    }
    ring_write(&c->ring, interleaved, nframes * 2);
    c->period_size = nframes; c->total_frames += nframes; c->cb_count++;
    if (c->trace)
        fprintf(c->trace, "{\"frame\":%ld,\"cb\":%d,\"n\":%u,\"xr\":%d}\n",
                c->total_frames, c->cb_count, nframes, c->xrun_count);
    return 0;
}
static int xrun_cb(void *arg) { ((cap_t *)arg)->xrun_count++; }
static void shutdown_cb(void *arg) { ((cap_t *)arg)->shutdown = 1; }

static void *writer_func(void *arg) {
    cap_t *c = (cap_t *)arg;
    FILE *w = fopen(c->output, "wb");
    if (!w) { fprintf(stderr, "capture: cannot open %s\n", c->output); return NULL; }
    wav_header_t h; memset(&h, 0, sizeof(h));
    memcpy(h.riff,"RIFF",4); memcpy(h.wave,"WAVE",4);
    memcpy(h.fmt,"fmt ",4); h.fmt_size=16; h.audio_format=3;
    h.num_channels=2; h.sample_rate=c->sr; h.bits_per_sample=32;
    h.block_align=8; h.byte_rate=c->sr*8;
    memcpy(h.data,"data",4);
    fwrite(&h, sizeof(h), 1, w);
    long bytes=0; float buf[4096];
    while (!c->shutdown || ring_avail(&c->ring) > 0) {
        int n = ring_read(&c->ring, buf, 4096, 100);
        if (n > 0) { fwrite(buf, sizeof(float), n, w); bytes += n*sizeof(float); }
    }
    while (ring_avail(&c->ring) > 0) {
        int n = ring_read(&c->ring, buf, 4096, 10);
        if (n > 0) { fwrite(buf, sizeof(float), n, w); bytes += n*sizeof(float); }
    }
    h.file_size = 36 + bytes; h.data_size = bytes;
    fseek(w, 0, SEEK_SET); fwrite(&h, sizeof(h), 1, w); fclose(w);
    fprintf(stderr, "capture: wrote %s (%ld bytes)\n", c->output, bytes);
    return NULL;
}

int main(int argc, char *argv[]) {
    cap_t c; memset(&c, 0, sizeof(c));
    c.output = "/tmp/captured.wav";
    const char *client_name = "deterministic_capture";
    const char *trace_file = NULL;

    static struct option lo[] = {
        {"output",required_argument,0,'o'}, {"server",required_argument,0,'s'},
        {"name",required_argument,0,'n'}, {"trace",required_argument,0,'t'},
        {"help",no_argument,0,'h'},{0,0,0,0}};
    int opt;
    while ((opt = getopt_long(argc, argv, "o:s:n:t:h", lo, NULL)) != -1) {
        switch(opt) {
            case 'o': c.output=optarg; break; case 's': break;
            case 'n': client_name=optarg; break; case 't': trace_file=optarg; break;
            case 'h': return 0; default: return 1;
        }
    }

    ring_init(&c.ring);
    jack_status_t status;
    c.client = jack_client_open(client_name, JackNoStartServer, &status, NULL);
    if (!c.client) { fprintf(stderr, "Cannot connect to JACK\n"); return 1; }
    c.sr = jack_get_sample_rate(c.client);
    c.in_l = jack_port_register(c.client, "in_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    c.in_r = jack_port_register(c.client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    if (!c.in_l || !c.in_r) { jack_client_close(c.client); return 1; }
    jack_set_process_callback(c.client, process_cb, &c);
    jack_set_xrun_callback(c.client, xrun_cb, &c);
    jack_on_shutdown(c.client, shutdown_cb, &c);
    if (trace_file) c.trace = fopen(trace_file, "w");

    pthread_t wt; pthread_create(&wt, NULL, writer_func, &c);
    if (jack_activate(c.client)) { jack_client_close(c.client); return 1; }
    fprintf(stderr, "capture: started pid=%d output=%s\n", getpid(), c.output);

    while (!c.shutdown) usleep(100000);
    fprintf(stderr, "capture: stopping frames=%ld cbs=%d xruns=%d\n",
            c.total_frames, c.cb_count, c.xrun_count);
    pthread_join(wt, NULL);
    if (c.trace) fclose(c.trace);
    ring_free(&c.ring);
    jack_deactivate(c.client); jack_client_close(c.client);
    return 0;
}
