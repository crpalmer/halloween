#ifndef __EARTH_INVADERS_IO_H__
#define __EARTH_INVADERS_IO_H_

#include "digital-counter.h"
#include "l298n.h"
#include "mcp23017.h"
#include "pca9685.h"
#include "util.h"
#include "wb.h"

#define BUTTON_PUSHED   0
#define TARGET_HIT      1
#define ENDSTOP_TRIGGERED 1

#define TO_MOTOR	1
#define TO_IDLER	0

class earth_invaders_io_t {
public:
     earth_invaders_io_t() {
	mcp = new MCP23017();
	pca = new PCA9685();

	ms_sleep(500);		// let the relays stabilize after resetting pca

	laser = mcp->get_output(1, 5);
	triggers[0] = mcp->get_input(1, 3);
	triggers[1] = mcp->get_input(1, 4);

	start_light = wb_get_output(1, 8);
	start_button = mcp->get_input(1, 2);

	endstop_motor = mcp->get_input(1, 0);
	endstop_idler = mcp->get_input(1, 1);
	motor = new L298N(pca->get_output(13), pca->get_output(14), pca->get_output(15));

	for (int target = 0; target < 6; target++) {
	    targets[target / 3][target % 3] = mcp->get_input(0, 2+target);
	}

	for (int light = 0; light < 6; light++) {
	    lights[light / 3][light % 3] = wb_get_output(1, light+1);
        }

	score[1] = new digital_counter_t(pca->get_output(1), NULL, pca->get_output(0));
	high_score = new digital_counter_t(pca->get_output(3), NULL, pca->get_output(2));
	score[0] = new digital_counter_t(pca->get_output(5), NULL, pca->get_output(4));

	score[0]->set_pause(25, 500, 500);
	high_score->set_pause(25, 500, 500);
	score[1]->set_pause(25, 500, 500);

	triggers[0]->set_pullup_up();
	triggers[1]->set_pullup_up();
	start_button->set_pullup_up();
     }

     output_t	*laser;
     input_t	*triggers[2];

     output_t	*start_light;
     input_t	*start_button;

     input_t	*endstop_motor, *endstop_idler;
     motor_t	*motor;

     output_t   *lights[3][3];
     input_t	*targets[3][3];

     digital_counter_t *score[2];
     digital_counter_t *high_score;

private:
     MCP23017	*mcp;
     PCA9685	*pca;
};

#endif
