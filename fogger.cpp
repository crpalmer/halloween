#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "pi.h"
#include "file.h"
#include "mem.h"
#include "server.h"
#include "time-utils.h"
#include "wb.h"

#include "fogger.h"

#define DUTY_FILENAME "fogger.duty"
#define FOG_BURST_MS	5000
#define OFF_DELAY	5000

void Fogger::load_duty(void) {
    file_t *f = file_open(DUTY_FILENAME, "r");

    duty = default_duty;

    if (f == NULL) {
	perror(DUTY_FILENAME);
	return;
    }

    char line[32];
    if (file_gets(f, line, 128) > 0) {
        if (sscanf(line, "%lf", &duty) != 1) {
	    fprintf(stderr, "Failed to ready duty cycle.\n");
	}
    }

    file_close(f);
}

void Fogger::save_duty()
{
    file_t *f = file_open(DUTY_FILENAME, "w");

    if (f == NULL) {
	perror(DUTY_FILENAME);
    }

    file_printf(f, "%f", duty);
    file_close(f);
}

bool Fogger::fog(unsigned ms, bool non_blocking) {
    if (ms == 0) return false;

    if (non_blocking) {
	if (! lock->trylock()) return false;
    } else {
        lock->lock();
    }

    fogger->set(1);
    fprintf(stderr, "sleeping for ON %d\n", ms);
    ms_sleep(ms);

    fogger->set(0);
    fprintf(stderr, "sleeping for OFF_DELAY %d\n", OFF_DELAY);
    ms_sleep(OFF_DELAY);

    lock->unlock();

    return true;
}

void Fogger::main() {
    load_duty();
    while(true) {
	if (is_active()) {
	    unsigned ms = 5000 * (1 - duty);
	    fprintf(stderr, "sleeping for OFF %d\n", ms);
	    ms_sleep(ms);
	    fog(5000 * duty);
	} else {
	    ms_sleep(1000);
	}
    }
}

void FoggerConsole::process_cmd(const char *cmd) {
    if (is_command(cmd, "duty_up")) {
	fogger->duty += fogger->delta_duty;
	if (fogger->duty >= 1) fogger->duty = 1;
	fogger->save_duty();
	printf("ok duty is now %lf\n", fogger->duty);
    } else if (is_command(cmd, "duty_down")) {
	fogger->duty -= fogger->delta_duty;
	if (fogger->duty <= 0) fogger->duty = 0;
	fogger->save_duty();
	printf("ok duty is now %lf\n", fogger->duty);
    } else if (is_command(cmd, "duty_set")) {
	fogger->duty = atof(&cmd[strlen("duty_set")]);
	if (fogger->duty < 0) fogger->duty = 0;
	fogger->save_duty();
	printf("ok duty is now %lf\n", fogger->duty);
    } else if (strcmp(cmd, "fog") == 0) {
        if (! fogger->fog(FOG_BURST_MS, true)) {
	    printf("failed prop is busy\n");
	}
    } else {
        ThreadsConsole::process_cmd(cmd);
    }
}

void FoggerConsole::usage() {
    ThreadsConsole::usage();
    printf("duty_up\nduty_down\nduty_set <duty>\nfog - generate a burst of fog\n");
}
