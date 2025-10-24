#ifndef __EARTH_INVADERS_IO_H__
#define __EARTH_INVADERS_IO_H_

/* IMPORTANT:
 *
 * The wiring is labelled from the backside.  That means:
 *
 * Green = "Player 1" in labels and in the code
 * Red   = "Player 2" in labels and in the code
 *
 * Lights and targets are numbered from the back.  That means that light 1
 * is the furthest to the right when looking at it from the front.
 */

#include "digital-counter.h"
#include "l298n.h"
#include "mcp23017.h"
#include "pca9685.h"
#include "wb.h"

/* PLAYER_x : see the comment in *-io to explain the numbering */
#define PLAYER_1        1
#define PLAYER_2        0

#define BUTTON_PUSHED   0
#define TARGET_HIT      1
#define ENDSTOP_TRIGGERED 1

#define TO_MOTOR	1
#define TO_IDLER	0

class EarthInvadersIO {
public:
     EarthInvadersIO() {
	wb = new WeenBoard();
	mcp = new MCP23017();
	pca = new PCA9685();

	ms_sleep(500);		// let the relays stabilize after resetting pca

	laser = mcp->get_output(1, 5);
	triggers[0] = mcp->get_input(1, 3);
	triggers[1] = mcp->get_input(1, 4);

	start_light = wb->get_output(1, 8);
	start_button = mcp->get_input(1, 2);

	endstop_motor = mcp->get_input(1, 0);
	endstop_idler = mcp->get_input(1, 1);
	motor = new L298N(pca->get_output(13), pca->get_output(14), pca->get_output(15));
	motor->speed(0);

	for (int target = 0; target < 6; target++) {
	    targets[target / 3][target % 3] = mcp->get_input(0, 2+target);
	}

	for (int light = 0; light < 6; light++) {
	    lights[light / 3][light % 3] = wb->get_output(1, light+1);
        }

	high_score = new DigitalCounter(pca->get_output(3), NULL, pca->get_output(2));
	high_score->set_pause(20, 500, 750);

	score[0] = new DigitalCounter(pca->get_output(5), NULL, pca->get_output(4));
	score[0]->set_pause(20, 500, 750);

	score[1] = new DigitalCounter(pca->get_output(1), NULL, pca->get_output(0));
	score[1]->set_pause(20, 1000, 1000);

	triggers[0]->set_pullup_up();
	triggers[1]->set_pullup_up();
	start_button->set_pullup_up();
     }

    void change_motor(bool direction, double speed)
    {
        motor->speed(0);
        ms_sleep(50);
        motor->direction(direction);
        motor->speed(speed);
    }

    void stop_motor()
    {
	motor->speed(0);
    }

     Output	*laser;
     Input	*triggers[2];

     Output	*start_light;
     Input	*start_button;

     Input	*endstop_motor, *endstop_idler;
     Motor	*motor;

     Output   *lights[3][3];
     Input	*targets[3][3];

     DigitalCounter *score[2];
     DigitalCounter *high_score;

private:
     WeenBoard  *wb;
     MCP23017	*mcp;
     PCA9685	*pca;
};

#endif
