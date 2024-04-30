#include <stdio.h>
#include "wb.h"
#include "ween2020.h"

static ween_time_constraint_t wt[] = {
     { 0, 9, 0, 12+10, 30 },
     { -1, 12+3, 0, 12+6, 0 },
     { -1, 12+7, 0, 12+8, 0 },
};

#define WT_SIZE	(sizeof(wt) / sizeof(wt[0]))

int main(int argc, char **argv)
{
     int is_valid = -1;
     wb_init();

     while (1) {
	int new_valid = ween_time_is_valid(wt, WT_SIZE) || ween2020_is_ignored();

	if (new_valid != is_valid) {
	    printf("switching from %d to %d\n", is_valid, new_valid);
	    is_valid = new_valid;
	    wb_set(2, 4, is_valid);
	    ms_sleep(100);
	    wb_set(2, 1, is_valid);
	}
	ms_sleep(1*1000);
     }
}

