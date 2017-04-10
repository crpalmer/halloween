#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "wb.h"
#include "107-utils.h"

#define BUTTON 1

static void *
thread_main(void *state_as_vp)
{
    unsigned state = (unsigned) state_as_vp;

    wb_wait_for_pin(BUTTON, state);
    exit(1);
}

void
room_107_die_thread(unsigned state)
{
    pthread_t thread;
    pthread_create(&thread, NULL, thread_main, (void *) state);
}
