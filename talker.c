#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/stat.h>
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

#define SERVO_ID 1
#define RIGHT_EYE_PIN 2, 1
#define LEFT_EYE_PIN 2, 2

#define N_HISTORY 50
#define MICROPHONE_HISTORY_EPSILON 5
#define MICROPHONE_GAIN_TARGET     65
#define MICROPHONE_MAX_GAIN	   5
#define OTHER_MAX_GAIN		   100
#define OTHER_GAIN_TARGET          65
#define OTHER_HISTORY_EPSILON      0

#define MAX_ANY_AUDIO 10
#define ANY_AUDIO_THRESHOLD 2
#define IDLE_AUDIO_SECS (5*60)

#define STATS_MICROPHONE 0
#define STATS_AUTO	 256
#define MAX_STATS (STATS_AUTO+1)

typedef struct {
    double history[N_HISTORY];
    size_t history_i;
    double sum_history;
    bool history_full;
    double gain;
    double gain_target;
    double max_gain;
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

#define MAX_IDLE_TRACKS		20
#define IDLE_TRACK_PREFIX	"toothfairy-talker"
static wav_t *idle_tracks[MAX_IDLE_TRACKS];
static int n_idle_tracks;

/* Only used by the single threaded consumer thread */
static int current_stats_who;

static void
update_history_and_gain(stats_t *s, double pos)
{
    if (pos > s->epsilon) {
	int n_avg;

        s->sum_history = s->sum_history - s->history[s->history_i] + pos;
        s->history[s->history_i] = pos;
        s->history_i = (s->history_i + 1) % N_HISTORY;
	if (s->history_i == 0) {
	    s->history_full = true;
	}
	n_avg = s->history_full ? N_HISTORY : s->history_i;
        s->gain = s->gain_target / (s->sum_history / n_avg);
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
	wb_set(RIGHT_EYE_PIN, eyes);
	wb_set(LEFT_EYE_PIN, eyes);
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
    int no_input;
    struct stat stat_unused;
    size_t i;

    no_input = (stat("/tmp/talker-no-input", &stat_unused) >= 0);
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
	if (SERVO_ID == 0) {
	    maestro_set_servo_physical_range(maestro, SERVO_ID, 1600, 1900);
	} else {
	    maestro_set_servo_range(maestro, SERVO_ID, EXTENDED_SERVO);
	    maestro_set_servo_range_pct(maestro, SERVO_ID, 35, 53);
	}
	maestro_set_servo_is_inverted(maestro, SERVO_ID, 1);
    }

    for (n_idle_tracks = 0; n_idle_tracks < MAX_IDLE_TRACKS; n_idle_tracks++) {
	char fname[128];

	sprintf(fname, "%s-%d.wav", IDLE_TRACK_PREFIX, n_idle_tracks+1);
	if ((idle_tracks[n_idle_tracks] = wav_new(fname)) == NULL) break;
    }

    fprintf(stderr, "Using %d idle tracks\n", n_idle_tracks);

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

    audio_device_init(&out_dev, 1, 0, true);
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
	audio_set_volume(in, 50);
	assert(audio_get_buffer_size(in) == size);
        fprintf(stderr, "Copying from capture to play using %u byte buffers\n", size);
    } else {
	fprintf(stderr, "Processing remote speech commands\n");
    }

    audio_set_volume(out, 100);

    audio_meta_init_from_config(&meta, &cfg);
    skull = talking_skull_new(&meta, servo_update, NULL);

    last_audio = time(NULL);

    for (i = 0; i < MAX_STATS; i++) {
	stats[i].gain = 5;
	stats[i].gain_target = OTHER_GAIN_TARGET;
	stats[i].max_gain = OTHER_MAX_GAIN;
	stats[i].epsilon = OTHER_HISTORY_EPSILON;
    }
    stats[STATS_MICROPHONE].gain_target = MICROPHONE_GAIN_TARGET;
    stats[STATS_MICROPHONE].max_gain = MICROPHONE_MAX_GAIN;
    stats[STATS_MICROPHONE].epsilon = MICROPHONE_HISTORY_EPSILON;
    stats[STATS_AUTO].gain = 3;

    while (no_input || audio_capture_buffer(in, buffer)) {
	if (auto_play_bytes_left == 0 && time(NULL) - last_audio >= IDLE_AUDIO_SECS && n_idle_tracks > 0) {
	    auto_play_buffer = wav_get_raw_data(idle_tracks[random_number_in_range(0, n_idle_tracks-1)], &auto_play_bytes_left);
	}

	if (auto_play_bytes_left) {
	    size_t n = auto_play_bytes_left > size ? size : auto_play_bytes_left;
	    memcpy(buffer, auto_play_buffer, n);
	    auto_play_buffer += n;
	    auto_play_bytes_left -= n;

	    pthread_mutex_lock(&speak_lock);
	    produce(pc, STATS_AUTO, buffer);
	    pthread_mutex_unlock(&speak_lock);

	    buffer = fatal_malloc(size);
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

