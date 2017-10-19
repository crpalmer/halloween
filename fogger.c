#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "server.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#include "fogger.h"

#define DUTY_FILENAME "fogger.duty"
#define FOG_BURST_MS	5000

#define OFF_DELAY	5000

static double default_duty = .1;
static double delta_duty = 0.01;
static double duty = .10;
static pthread_mutex_t lock;
static pthread_mutex_t fog_lock;
static pthread_t fogger;

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

    pthread_mutex_lock(&lock);
    wb_set(fogger_bank, fogger_pin, 1);
    fprintf(stderr, "sleeping for ON\n");
    ms_sleep(ms);
    wb_set(fogger_bank, fogger_pin, 0);
    fprintf(stderr, "sleeping for OFF_DELAY\n");
    ms_sleep(OFF_DELAY);
    pthread_mutex_unlock(&lock);
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
    } else if (strcmp(command, "duty_set") == 0) {
	duty = atoi(&command[strlen("duty_set")]);
	if (duty < 0) duty = 0;
	write_duty_cycle(duty);
	return return_duty();
    } else if (strcmp(command, "fog") == 0) {
        if (pthread_mutex_trylock(&fog_lock) != 0) {
	    return strdup("prop is busy");
	}
	do_fog(FOG_BURST_MS);
	pthread_mutex_unlock(&fog_lock);
    } else {
        return strdup("invalid command");
    }

    return strdup("ok");
}

static void *
fogger_main(void *args_as_vp)
{
    server_args_t server_args;
    pthread_t server_thread;
    fogger_args_t *args = (fogger_args_t *) args_as_vp;

    if (args) {
	default_duty = args->default_duty;
	delta_duty = args->delta_duty;
    }

    duty = read_duty_cycle();

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&fog_lock, NULL);

    server_args.port = 5556;
    server_args.command = remote_event;
    server_args.state = NULL;

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);

    while(true) {
	fprintf(stderr, "sleeping for OFF\n");
	ms_sleep(5000 * (1 - duty));
	do_fog(5000 * duty);
    }

    return NULL;
}

void
fogger_run_in_background(unsigned bank, unsigned pin, fogger_args_t *args)
{
    fogger_bank = bank;
    fogger_pin = pin;

    pthread_create(&fogger, NULL, fogger_main, args);
}
