#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pi-threads.h"
#include <sys/stat.h>
#include "audio.h"
#include "maestro.h"
#include "mem.h"
#include "pi-usb.h"
#include "producer-consumer.h"
#include "random-wavs.h"
#include "server.h"
#include "talking-skull.h"
#include "talking-skull-from-audio.h"
#include "wav.h"
#include "wb.h"

#include "talker.h"

#define MAX_ANY_AUDIO 10
#define ANY_AUDIO_THRESHOLD 2

#define N_GAIN_HISTORY 50
#define N_TALKER_GAIN 256

static PiMutex *speak_lock;

static unsigned any_audio;
static struct timespec last_audio;

#define MAX_IDLE_TRACKS		20
#define IDLE_TRACK_PREFIX	"talker"
static RandomWavs random_wavs;

class TalkerGain {
public:
    void set_gain(double new_gain) { gain = new_gain; }
    void set_gain_target(double new_target) { gain_target = new_target; }
    void set_max_gain(double new_max_gain) { max_gain = new_max_gain; }
    void set_epsilon(double new_epsilon) { epsilon = new_epsilon; }

    double get_gain() { return gain; }

    void add_sample(double pos) {
	update_any_audio_count(pos);
	if (is_audible(pos)) update_gain(pos);
    }

    bool is_audible(double pos) { return pos > epsilon; }

private:
    void update_gain(double pos) {
        sum_history = sum_history - history[history_i] + pos;
        history[history_i] = pos;
        history_i = (history_i + 1) % N_GAIN_HISTORY;
	if (history_i == 0) history_full = true;

	int n_avg = history_full ? N_GAIN_HISTORY : history_i;
        gain = gain_target / (sum_history / n_avg);
    }

    void update_any_audio_count(double pos) {
	if (is_audible(pos)) {
	    if (any_audio < MAX_ANY_AUDIO) any_audio++;
	} else {
	    if (any_audio > 0) any_audio--;
	}
	if (any_audio > ANY_AUDIO_THRESHOLD) nano_gettime(&last_audio);
    }

private:
    double gain = 3;
    double gain_target = 65;
    double max_gain = 100;
    double epsilon = 0;

    double history[N_GAIN_HISTORY];
    size_t history_i = 0;
    double sum_history = 0;
    bool history_full = false;
};

static TalkerGain talker_gain[N_TALKER_GAIN];
static TalkerGain talker_gain_mic;

class TalkerTalkingSkull : public TalkingSkull {
public:
    TalkerTalkingSkull(TalkerGain *who, TalkingSkullOps *ops) : who(who), TalkingSkull(ops, "talker-talking-skull") {}

    void update_pos(double pos) override {
        bool new_eyes;

	who->add_sample(pos);

        if (! who->is_audible(pos)) pos = 0;

	pos *= who->get_gain();
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

private:
    TalkerGain *who;
    bool eyes = false;
};

/* TODO: Can this be moved into Talker */

class AudioEvent {
public:
    AudioEvent(Audio *audio, AudioConfig *audio_config, TalkerGain *who, void *data, size_t size, int vol) :
           audio(audio), who(who), data(data), size(size), vol(vol) {
	audio_buffer = new AudioBuffer(new Buffer(data, size), audio_config);
	talking_skull = new TalkerTalkingSkull(who, new TalkingSkullAudioOps(audio_buffer));
    }

    void play() {
	if (! audio->configure(audio_buffer->get_audio_config())) {
	    consoles_fatal_printf("Could not configure audio device.\n");
	}

        // audio_set_volume(out, vol);

	talking_skull->play();
        if (! audio->play(buffer, size)) {
	    consoles_fatal_printf(stderr, "Failed to play audio buffer!\n");
        }
    }

private:
    Audio *audio;
    AudioBuffer *audio_buffer;
    TalkerGain *who;
    TalkingSkull *talking_skull;
    void *data;
    size_t size;
    int vol;
};

class TalkerBackgroundThread : PiThread {
public:
    TalkerBackgroundThread(Audio *audio, AudioConfig *audio_config) : audio(audio), audio_config(audio_config) {
	pc = producer_consumer_new(1);
	start();
    }

    ~Talker() {
	producer_consumer_destroy(pc);
    }

    void enqueue_buffer(TalkerGain *who, AudioConfig *audio_config, void *data, size_t size, int vol) {
	producer_consumer_produce(new AudioEvent(audio, audio_config, who, data, size, vol));
    }

    void main(void) {
	AudioEvent *e;

	while ((e = (AudioEvent *) producer_consumer_consume(pc, &seq_unused)) != NULL) e->play(e);
    }

private:
    Audio *audio;
    producer_consumer *pc;
};

#if 0
static char *
remote_event(void *args_as_vp, const char *command, struct sockaddr_in *addr, size_t addr_size)
{
    talker_args_t *args = (talker_args_t *) args_as_vp;
    unsigned char *data;
    size_t i, j;
    int ip = addr->sin_addr.s_addr>>24 % (N_AUTO_GAIN);

    if (strncmp(command, "play ", 5) != 0) {
	return strdup("Invalid command");
    }

    /* Expect sample rate 16000 mono and playing 48000 stereo */
    data = (unsigned char *) fatal_malloc(size + 100);

    speak_lock->lock();

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
	    produce(pc, &talker_gain[ip], args->remote_vol, data);
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

    speak_lock->unlock();

    return strdup(SERVER_OK);
}

#endif

class TalkerMainThread : public PiThread {
public:
    TalkerMainThread() {
        start();
    }
};

void
talker_main(void *args_as_vp)
{
    talker_args_t *args = (talker_args_t *) args_as_vp;
    unsigned char *buffer;
    audio_t *in;
    audio_config_t cfg;
    audio_meta_t meta;
    server_args_t server_args;
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

#if 0
    speak_lock = new PiMutex();
    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = args;

    pi_thread_create("server", server_thread_main, &server_args);
#endif

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
        fprintf(stderr, "Copying from capture to play using %u byte buffers\n", (unsigned) size);
    } else {
	fprintf(stderr, "Processing remote speech commands\n");
    }

    audio_set_volume(out, 100);

    audio_meta_init_from_config(&meta, &cfg);
    skull = talking_skull_new(&meta, servo_update, args);

    nano_gettime(&last_audio);

    talker_gain_mic.set_gain(3);
    talker_gain_mic.set_epsilon(5);
    talker_gain_mic.set_max_gain(5);

    while (! in || audio_capture_buffer(in, buffer)) {
	if (auto_play_bytes_left == 0 && nano_elapsed_ms_now(&last_audio) >= (int) args->idle_ms && n_idle_tracks > 0 && args->is_valid()) {
	    auto_play_buffer = wav_get_raw_data(idle_tracks[random_number_in_range(0, n_idle_tracks-1)], &auto_play_bytes_left);
	}

	if (auto_play_bytes_left) {
	    size_t n = auto_play_bytes_left > size ? size : auto_play_bytes_left;
	    memcpy(buffer, auto_play_buffer, n);
	    auto_play_buffer += n;
	    auto_play_bytes_left -= n;

	    speak_lock->lock();
	    produce(pc, NULL, args->track_vol, buffer);
	    speak_lock->unlock();

	    buffer = (unsigned char *) fatal_malloc(size);
	} else if (! in) {
	    ms_sleep(100);
	} else {
	    speak_lock->lock();
	    produce(pc, &talker_gain_mic, args->mic_vol, buffer);
	    speak_lock->unlock();
	    buffer = (unsigned char *) fatal_malloc(size);
	}
    }

    fprintf(stderr, "Failed to capture buffer!\n");

    audio_destroy(in);
    audio_destroy(out);
    free(buffer);
}

void
talker_run_in_background(talker_args_t *args)
{
    pi_thread_create("talker", talker_main, args);
}

static bool
always_valid(void)
{
    return true;
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
    args->is_valid = always_valid;
}
