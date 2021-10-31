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

#include "talker.h"

#define N_HISTORY 50
#define MICROPHONE_HISTORY_EPSILON 5
#define MICROPHONE_GAIN_TARGET     65
#define MICROPHONE_MAX_GAIN	   5
#define OTHER_MAX_GAIN		   100
#define OTHER_GAIN_TARGET          65
#define OTHER_HISTORY_EPSILON      0

#define MAX_ANY_AUDIO 10
#define ANY_AUDIO_THRESHOLD 2

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
    int vol;
} event_t;

stats_t stats[MAX_STATS];

static pthread_mutex_t speak_lock;
static talking_skull_t *skull;

static unsigned any_audio;
static struct timespec last_audio;

static producer_consumer_t *pc;
size_t size;

#define MAX_IDLE_TRACKS		20
#define IDLE_TRACK_PREFIX	"talker"
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
    if (any_audio > ANY_AUDIO_THRESHOLD) nano_gettime(&last_audio);
}

static void
update_servo_and_eyes(talker_args_t *args, double pos)
{
    static int eyes = -1;
    int new_eyes;
    stats_t *s = &stats[current_stats_who];

    if (pos < s->epsilon) {
	pos = 0;
    }

    pos *= s->gain;
    if (pos > 100) pos = 100;
    if (args->m) {
	maestro_set_servo_pos(args->m, args->servo_id, pos);
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
	if (args->eyes) args->eyes->set(eyes);
     }
}

static void
servo_update(void *args, double pos)
{
    if (current_stats_who != STATS_AUTO) {
	update_history_and_gain(&stats[current_stats_who], pos);
    }
    update_any_audio_check(pos);
    update_servo_and_eyes((talker_args_t *) args, pos);
}

static void
play_one_buffer(audio_t *out, void *buffer, int vol, size_t size)
{
    talking_skull_play(skull, (unsigned char *) buffer, size);
    audio_set_volume(out, vol);
    if (! audio_play_buffer(out, (unsigned char *) buffer, size)) {
	fprintf(stderr, "Failed to play buffer!\n");
	exit(1);
    }
}

static void
produce(producer_consumer_t *pc, int stats_who, int vol, unsigned char *buffer)
{
    event_t *e = (event_t *) fatal_malloc(sizeof(*e));

    e->stats_who = stats_who;
    e->buffer = buffer;
    e->vol = vol;

    producer_consumer_produce(pc, e);
}

static char *
remote_event(void *args_as_vp, const char *command, struct sockaddr_in *addr, size_t addr_size)
{
    talker_args_t *args = (talker_args_t *) args_as_vp;
    unsigned char *data;
    size_t i, j;
    int ip = addr->sin_addr.s_addr>>24 % 256;

    if (strncmp(command, "play ", 5) != 0) {
	return strdup("Invalid command");
    }

    /* Expect sample rate 16000 mono and playing 48000 stereo */
    data = (unsigned char *) fatal_malloc(size + 100);

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
	    produce(pc, ip, args->remote_vol, data);
	    data = (unsigned char *) fatal_malloc(size + 100);
	    j = 0;
	}
    }

    if (j > 0) {
	while (j < size) data[j++] = 0;
	produce(pc, ip, args->remote_vol, data);
    } else {
	free(data);
    }

    pthread_mutex_unlock(&speak_lock);

    return strdup(SERVER_OK);
}

static void *
play_thread_main(void *out_as_vp)
{
    audio_t *out = (audio_t *) out_as_vp;
    unsigned seq_unused;
    event_t *e;

    while ((e = (event_t *) producer_consumer_consume(pc, &seq_unused)) != NULL) {
	current_stats_who = e->stats_who;
	play_one_buffer(out, e->buffer, e->vol, size);
	free(e->buffer);
	free(e);
    }

    return NULL;
}

void *
talker_main(void *args_as_vp)
{
    talker_args_t *args = (talker_args_t *) args_as_vp;
    unsigned char *buffer;
    audio_t *in;
    audio_config_t cfg;
    audio_meta_t meta;
    server_args_t server_args;
    pthread_t server_thread;
    pthread_t play_thread;
    unsigned char *auto_play_buffer = NULL;
    size_t auto_play_bytes_left = 0;
    audio_t *out;
    size_t i;

    for (n_idle_tracks = 0; n_idle_tracks < MAX_IDLE_TRACKS; n_idle_tracks++) {
	char fname[128];

	sprintf(fname, "%s-%d.wav", IDLE_TRACK_PREFIX, n_idle_tracks+1);
	if ((idle_tracks[n_idle_tracks] = wav_new(fname)) == NULL) break;
    }

    fprintf(stderr, "Using %d idle tracks\n", n_idle_tracks);

    pthread_mutex_init(&speak_lock, NULL);
    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = args;

    pc = producer_consumer_new(1);

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);

    audio_config_init_default(&cfg);
    cfg.channels = 2;
    cfg.rate = 48000;

    out = audio_new(&cfg, &args->out_dev);
    pthread_create(&play_thread, NULL, play_thread_main, out);

    if (args->no_input) in = NULL;
    else {
	if ((in = audio_new(&cfg, &args->in_dev)) == NULL) {
	    perror("audio in");
	    exit(1);
	}
    }

    size = audio_get_buffer_size(out);
    buffer = (unsigned char *) fatal_malloc(size);

    if (! out) {
	perror("out");
	fprintf(stderr, "Failed to initialize playback\n");
	exit(1);
    }

    if (in) {
	audio_set_volume(in, 50);
	assert(audio_get_buffer_size(in) == size);
        fprintf(stderr, "Copying from capture to play using %u byte buffers\n", size);
    } else {
	fprintf(stderr, "Processing remote speech commands\n");
    }

    audio_set_volume(out, 100);

    audio_meta_init_from_config(&meta, &cfg);
    skull = talking_skull_new(&meta, servo_update, args);

    nano_gettime(&last_audio);

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

    while (! in || audio_capture_buffer(in, buffer)) {
	if (auto_play_bytes_left == 0 && nano_elapsed_ms_now(&last_audio) >= (int) args->idle_ms && n_idle_tracks > 0) {
	    auto_play_buffer = wav_get_raw_data(idle_tracks[random_number_in_range(0, n_idle_tracks-1)], &auto_play_bytes_left);
	}

	if (auto_play_bytes_left) {
	    size_t n = auto_play_bytes_left > size ? size : auto_play_bytes_left;
	    memcpy(buffer, auto_play_buffer, n);
	    auto_play_buffer += n;
	    auto_play_bytes_left -= n;

	    pthread_mutex_lock(&speak_lock);
	    produce(pc, STATS_AUTO, args->track_vol, buffer);
	    pthread_mutex_unlock(&speak_lock);

	    buffer = (unsigned char *) fatal_malloc(size);
	} else if (! in) {
	    ms_sleep(100);
	} else {
	    pthread_mutex_lock(&speak_lock);
	    produce(pc, STATS_MICROPHONE, args->mic_vol, buffer);
	    pthread_mutex_unlock(&speak_lock);
	    buffer = (unsigned char *) fatal_malloc(size);
	}
    }

    fprintf(stderr, "Failed to capture buffer!\n");

    audio_destroy(in);
    audio_destroy(out);
    free(buffer);

    return 0;
}

void
talker_run_in_background(talker_args_t *args)
{
    pthread_t thread;

    pthread_create(&thread, NULL, talker_main, args);
}

void
talker_args_init(talker_args_t *args)
{
    memset(args, 0, sizeof(*args));
    args->idle_track_prefix = "talker";
    args->idle_ms = 5*60*1000;
    args->port = 5555;
    audio_device_init(&args->out_dev, 1, 0, true);
    audio_device_init(&args->in_dev, 2, 0, false);
    args->mic_vol = args->remote_vol = args->track_vol = 100;
}
