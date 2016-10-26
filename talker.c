#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "audio.h"
#include "maestro.h"
#include "mem.h"
#include "pi-usb.h"
#include "producer-consumer.h"
#include "server.h"
#include "talking-skull.h"
#include "wav.h"
#include "wb.h"

static maestro_t *maestro;

#define SERVO_ID 0
#define EYES_PIN 1, 1

#define MICROPHONE_HISTORY_EPSILON 5
#define OTHER_HISTORY_EPSILON 0
#define N_HISTORY 20
#define GAIN_TARGET 75

#define MAX_ANY_AUDIO 10
#define ANY_AUDIO_THRESHOLD 2
#define IDLE_AUDIO_SECS 1*1000*1000*1000

#define STATS_MICROPHONE 0
#define STATS_AUTO	 256
#define MAX_STATS (256+1)

typedef struct {
    double history[N_HISTORY];
    size_t history_i;
    double sum_history;
    bool history_full;
    double gain;
    double epsilon;
} stats_t;

typedef struct {
    int stats_who;
    unsigned char *buffer;
} event_t;

stats_t stats[MAX_STATS];

static pthread_mutex_t speak_lock;
static audio_t *out;
static talking_skull_t *skull;

static unsigned any_audio;
static time_t last_audio;

static producer_consumer_t *pc;
size_t size;

/* Only used by the single threaded consumer thread */
static int current_stats_who;

static void
update_history_and_gain(stats_t *s, double pos)
{
    if (pos > s->epsilon) {
        s->sum_history = s->sum_history - s->history[s->history_i] + pos;
        s->history[s->history_i] = pos;
        s->history_i = (s->history_i + 1) % N_HISTORY;
	if (s->history_i == 0) {
	    s->history_full = true;
	}
	if (s->history_full) {
	    s->gain = GAIN_TARGET / (s->sum_history / N_HISTORY);
	}
    }
}

static void
update_any_audio_check(double pos)
{
    stats_t *s = &stats[current_stats_who];

    if (pos > s->epsilon) {
	if (any_audio < MAX_ANY_AUDIO) any_audio++;
    } else {
	if (any_audio > 0) any_audio--;
    }
    if (any_audio > ANY_AUDIO_THRESHOLD) last_audio = time(NULL);
}

static void
update_servo_and_eyes(double pos)
{
    static int eyes = -1;
    int new_eyes;
    stats_t *s = &stats[current_stats_who];

    if (pos < s->epsilon) {
	pos = 0;
    }

    pos *= s->gain;
    if (pos > 100) pos = 100;
    if (maestro) {
	maestro_set_servo_pos(maestro, SERVO_ID, pos);
    } else {
	static double last_pos = -1;
	if (pos != last_pos) {
	    printf("servo: %.2f %.0f\n", s->gain, pos);
	    last_pos = pos;
	}
    }

    new_eyes = (pos >= 50);
    if (new_eyes != eyes) {
	eyes = new_eyes;
	wb_set(EYES_PIN, eyes);
     }
}

static void
servo_update(void *unused, double pos)
{
    if (current_stats_who != STATS_AUTO) {
	update_history_and_gain(&stats[current_stats_who], pos);
    }
    update_any_audio_check(pos);
    update_servo_and_eyes(pos);
}

static void
play_one_buffer(void *buffer, size_t size)
{
    talking_skull_play(skull, buffer, size);
    if (! audio_play_buffer(out, buffer, size)) {
	fprintf(stderr, "Failed to play buffer!\n");
	exit(1);
    }
}

static void
produce(producer_consumer_t *pc, int stats_who, unsigned char *buffer)
{
    event_t *e = fatal_malloc(sizeof(*e));

    e->stats_who = stats_who;
    e->buffer = buffer;

    producer_consumer_produce(pc, e);
}

static char *
remote_event(void *unused, const char *command, struct sockaddr_in *addr, size_t addr_size)
{
    unsigned char *data;
    size_t i, j;
    int ip = addr->sin_addr.s_addr>>24 % 256;

fprintf(stderr, "remote: %d %.5s %d bytes\n", ip, command, strlen(command));

    if (strncmp(command, "play ", 5) != 0) {
	return strdup("Invalid command");
    }

    /* Expect sample rate 16000 mono and playing 48000 stereo */
    data = fatal_malloc(size + 100);

    pthread_mutex_lock(&speak_lock);

    for (i = 5, j = 0; command[i]; i++) {
	if (command[i] == '\\') {
	    i++;
	    switch(command[i]) {
	    case '\\': data[j++] = '\\'; break;
	    case 'n': data[j++] = '\n'; break;
	    case 'r': data[j++] = '\r'; break;
	    case '0': data[j++] = '\0'; break;
	    default:
		free(data);
		return strdup("corrupt data");
	    }
	} else {
	    data[j++] = command[i];
	}

	if ((j % 2) == 0) {
	    size_t p = j-2, k;
	    for (k = 1; k < 3*2; k++) {
		data[j++] = data[p];
		data[j++] = data[p+1];
	    }
	}
	if (j >= size) {
	    produce(pc, ip, data);
	    data = fatal_malloc(size + 100);
	    j = 0;
	}
    }

    if (j > 0) {
	while (j < size) data[j++] = 0;
	produce(pc, ip, data);
    } else {
	free(data);
    }

    pthread_mutex_unlock(&speak_lock);

    return strdup(SERVER_OK);
}

static void *
play_thread_main(void *unused)
{
    unsigned seq_unused;
    event_t *e;

    while ((e = (event_t *) producer_consumer_consume(pc, &seq_unused)) != NULL) {
	current_stats_who = e->stats_who;
	play_one_buffer(e->buffer, size);
	free(e->buffer);
	free(e);
    }

    return NULL;
}

int
main(int argc, char **argv)
{
    unsigned char *buffer;
    audio_t *in;
    audio_config_t cfg;
    audio_meta_t meta;
    audio_device_t in_dev, out_dev;
    server_args_t server_args;
    pthread_t server_thread;
    pthread_t play_thread;
    unsigned char *auto_play_buffer = NULL;
    size_t auto_play_bytes_left = 0;
    wav_t *auto_wav;
    int no_input = 0;
    size_t i;

    if (argc > 1) {
	if (strcmp(argv[1], "--no-input") == 0) {
	    no_input = 1;
	} else {
	    fprintf(stderr, "usage: [--no-input]\n");
	    exit(1);
	}
    }

    pi_usb_init();
    wb_init();

    if ((maestro = maestro_new()) == NULL) {
        fprintf(stderr, "couldn't find a recognized device, disabling skull.\n");
    } else {
        maestro_set_servo_is_inverted(maestro, SERVO_ID, true);
	maestro_set_servo_range(maestro, SERVO_ID, TALKING_SKULL2);
    }

    if ((auto_wav = wav_new("chant1.wav")) == NULL) {
	perror("chant1.wav");
	exit(1);
    }

    pthread_mutex_init(&speak_lock, NULL);
    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    pc = producer_consumer_new(1);

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);
    pthread_create(&play_thread, NULL, play_thread_main, NULL);

    audio_config_init_default(&cfg);
    cfg.channels = 2;
    cfg.rate = 48000;

    audio_device_init_playback(&out_dev);
    audio_device_init_capture(&in_dev);

    out = audio_new(&cfg, &out_dev);
    in = audio_new(&cfg, &in_dev);

    size = audio_get_buffer_size(out);
    buffer = fatal_malloc(size);

    if (! out) {
	perror("out");
	fprintf(stderr, "Failed to initialize playback\n");
	exit(1);
    }

    if (! in && ! no_input) {
	perror("in");
	fprintf(stderr, "Failed to initialize capture\n");
	exit(1);
    } else if (in) {
	audio_set_volume(in, 100);
	assert(audio_get_buffer_size(in) == size);
        fprintf(stderr, "Copying from capture to play using %u byte buffers\n", size);
    } else {
	fprintf(stderr, "Processing remote speech commands\n");
    }

    audio_set_volume(out, 100);

    audio_meta_init_from_config(&meta, &cfg);
    skull = talking_skull_new(&meta, false, servo_update, NULL);

    last_audio = time(NULL);

    for (i = 0; i < MAX_STATS; i++) {
	stats[i].gain = 1;
	stats[i].epsilon = OTHER_HISTORY_EPSILON;
    }
    stats[STATS_MICROPHONE].gain = 10;
    stats[STATS_MICROPHONE].epsilon = MICROPHONE_HISTORY_EPSILON;
    stats[STATS_AUTO].gain = 3;

    while (no_input || audio_capture_buffer(in, buffer)) {
	if (auto_play_bytes_left == 0 && time(NULL) - last_audio >= IDLE_AUDIO_SECS) {
	    auto_play_buffer = wav_get_raw_data(auto_wav, &auto_play_bytes_left);
	    while (auto_play_bytes_left > 0) {
		size_t n = auto_play_bytes_left > size ? size : auto_play_bytes_left;
		memcpy(buffer, auto_play_buffer, n);
		auto_play_buffer += n;
		auto_play_bytes_left -= n;

		pthread_mutex_lock(&speak_lock);
		produce(pc, STATS_AUTO, buffer);
		pthread_mutex_unlock(&speak_lock);

		buffer = fatal_malloc(size);
	    }
	} else if (no_input) {
	    sleep(1);
	} else {
	    pthread_mutex_lock(&speak_lock);
	    produce(pc, STATS_MICROPHONE, buffer);
	    pthread_mutex_unlock(&speak_lock);
	    buffer = fatal_malloc(size);
	}
    }

    fprintf(stderr, "Failed to capture buffer!\n");

    audio_destroy(in);
    audio_destroy(out);
    free(buffer);

    return 0;
}

