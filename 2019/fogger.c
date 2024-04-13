#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "fogger.h"
#include "time-utils.h"
#include "wb.h"

#define FOGGER_PIN	2, 1

int
main(int argc, char **argv)
{
    fogger_args_t fogger_args = { 0.01, 0.01 };

    if (wb_init() < 0) {
	fprintf(stderr, "failed to initialize wb\n");
	exit(1);
    }

    fogger_run_in_background(FOGGER_PIN, &fogger_args);

    while (1) ms_sleep(1000*1000);

    return 0;
}
