#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "pi-threads.h"
#include "server.h"
#include "track.h"
#include "time-utils.h"
#include "wb.h"

#include "fogger.h"

#define DUTY_FILENAME "fogger.duty"
#define FOG_BURST_MS	5000

#define OFF_DELAY	5000

static double default_duty = .1;
static double delta_duty = 0.01;
static double duty = .10;
static pi_mutex_t *lock;
static pi_mutex_t *fog_lock;

static unsigned fogger_bank, fogger_pin;


static double
read_duty_cycle(void)
{
    FILE *f = fopen(DUTY_FILENAME, "r");
    double x;

    if (f == NULL) {
	perror(DUTY_FILENAME);
	return default_duty;
    }

    if (fscanf(f, "%lf", &x) != 1) {
	fprintf(stderr, "Failed to ready duty cycle.\n");
	return default_duty;
    }

    fclose(f);

    return x;
}

static double
write_duty_cycle(double x)
{
    FILE *f = fopen(DUTY_FILENAME, "w");

    if (f == NULL) {
	perror(DUTY_FILENAME);
	exit(1);
    }

    fprintf(f, "%f", x);
    fclose(f);

    return x;
}

static void
do_fog(unsigned ms)
{
    if (ms == 0) return;

    pi_mutex_lock(lock);
    wb_set(fogger_bank, fogger_pin, 1);
    fprintf(stderr, "sleeping for ON %d\n", ms);
    ms_sleep(ms);
    wb_set(fogger_bank, fogger_pin, 0);
    fprintf(stderr, "sleeping for OFF_DELAY %d\n", OFF_DELAY);
    ms_sleep(OFF_DELAY);
    pi_mutex_unlock(lock);
}

static char *
return_duty(void)
{
    char foo[1000];

    sprintf(foo, "ok duty is now %f\n", duty);
    return strdup(foo);
}

static char *
remote_event(void *unused, const char *command, struct sockaddr_in *addr, size_t size)
{
    if (strcmp(command, "duty_up") == 0) {
	duty += delta_duty;
	if (duty >= 1) duty = 1;
	write_duty_cycle(duty);
	return return_duty();
    } else if (strcmp(command, "duty_down") == 0) {
	duty -= delta_duty;
	if (duty <= 0) duty = 0;
	write_duty_cycle(duty);
	return return_duty();
    } else if (strncmp(command, "duty_set", strlen("duty_set")) == 0) {
	duty = atof(&command[strlen("duty_set")]);
	if (duty < 0) duty = 0;
	write_duty_cycle(duty);
	return return_duty();
    } else if (strcmp(command, "fog") == 0) {
        if (pi_mutex_trylock(fog_lock) != 0) {
	    return strdup("prop is busy");
	}
	do_fog(FOG_BURST_MS);
	pi_mutex_unlock(fog_lock);
    } else {
        return strdup("invalid command");
    }

    return strdup("ok");
}

static void
fogger_main(void *args_as_vp)
{
    server_args_t server_args;
    fogger_args_t *args = (fogger_args_t *) args_as_vp;

    if (args) {
	default_duty = args->default_duty;
	delta_duty = args->delta_duty;
    }

    duty = read_duty_cycle();

    lock = pi_mutex_new();
    fog_lock = pi_mutex_new();

    server_args.port = 5556;
    server_args.command = remote_event;
    server_args.state = NULL;

    pi_thread_create_anonymous(server_thread_main, &server_args);

    while(true) {
	if (args->is_active && args->is_active()) {
	    unsigned ms = 5000 * (1 - duty);
	    fprintf(stderr, "sleeping for OFF %d\n", ms);
	    ms_sleep(ms);
	    do_fog(5000 * duty);
	}
    }
}

void
fogger_run_in_background(unsigned bank, unsigned pin, fogger_args_t *args)
{
    fogger_bank = bank;
    fogger_pin = pin;

    pi_thread_create_anonymous(fogger_main, args);
}
