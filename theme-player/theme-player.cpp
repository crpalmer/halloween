#include <stdio.h>
#include "pi.h"
#include "audio.h"
#include "audio-player.h"
#include "pi-threads.h"
#include "time-utils.h"
#include "wav.h"

Audio *audio;
AudioPlayer *player;
AudioBuffer *audio_buffer;

static void play(AudioBuffer *audio_buffer) {
    printf("Playing\n");
    struct timespec start;
    nano_gettime(&start);
    player->play(audio_buffer);
    player->wait_all_done();
    delete audio_buffer;
    printf("Done: %d ms\n", nano_elapsed_ms_now(&start));
}

void threads_main(int argc, char **argv) {
    audio = Audio::create_instance();
    player = new AudioPlayer(audio);
    if ((audio_buffer = wav_open("theme.wav")) == NULL) abort();

    while (1) {
	play(audio_buffer);
	ms_sleep(15*1000);
    }
}

int main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
    return 0;
}
