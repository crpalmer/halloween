#include <stdio.h>
#include <string.h>
#include "earth-invaders-io.h"

static earth_invaders_io_t *io;

static void
input()
{
    static char buf[100*1024];

    fgets(buf, sizeof(buf), stdin);
}

static void
test_button(input_t *t, const char *w)
{
   if (t->get() == BUTTON_PUSHED) printf("   **** FAILURE: button is pushed\n");
   printf("    Push the %s: ", w); fflush(stdout);
   while (t->get() != BUTTON_PUSHED) {}
   printf("button push recorded.\n");
}

static void
test_blasters()
{
    printf("Testing blasters:\n");

    printf("   The laser should be off, hit enter to enable the laser: ");
    input();
    io->laser->on();
    printf("   The laser should be on, hit enter to disable the laser: ");
    input();
    io->laser->off();

    test_button(io->triggers[0], "green trigger");
    test_button(io->triggers[1], "red trigger");

    printf("Done testing blaster.\n\n");
}

static void
test_start()
{
    printf("Testing the start button:\n");
    printf("   Light should be off, hit enter to enable it: ");
    input();
    io->start_light->on();
    printf("   Light should be on, hit enter to disable it: ");
    input();
    io->start_light->off();
    test_button(io->start_button, "start button");
    printf("Done testing the start button.\n\n");
}

static void
test_endstop(input_t *i, const char *w)
{
    if (i->get() == ENDSTOP_TRIGGERED) printf("    *** ENDSTOP IS TRIGGERED WHEN IT SHOULDN'T BE!!\n");
    printf("   Trigger the %s endstop: ", w);
    fflush(stdout);
    while (i->get() != ENDSTOP_TRIGGERED) {}
    printf("endstop triggered.\n");
}

static void
test_endstops()
{
    printf("Testing both endstops:\n");
    test_endstop(io->endstop_motor, "motor");
    test_endstop(io->endstop_idler, "idler");
    printf("Done testing endstops.\n\n");
}

static void
test_scores()
{
    printf("Testing score boards:\n");
    printf("   Setting P1 score to 100: ");
    input();
    io->score[0]->set(100);
    printf("   Setting P2 score to 100: ");
    input();
    io->score[1]->set(100);
    printf("   Setting high score to 100: ");
    input();
    io->high_score->set(100);
    printf("   Setting all scores back to 0: ");
    input();
    io->score[0]->set(0);
    io->score[1]->set(0);
    io->high_score->set(0);
    printf("   All scores should now be zero: ");
    input();
    printf("Done testing score boards.\n\n");
}

static void
test_lights()
{
    printf("   Enabling player 1 light 1");
    input();
    io->lights[0][0]->on();
    printf("   Enabling player 1 light 2");
    input();
    io->lights[0][0]->off();
    io->lights[0][1]->on();
    printf("   Enabling player 1 light 3");
    input();
    io->lights[0][1]->off();
    io->lights[0][2]->on();
    printf("   Enabling player 2 light 1");
    input();
    io->lights[0][2]->off();
    io->lights[1][0]->on();
    printf("   Enabling player 2 light 2");
    input();
    io->lights[1][0]->off();
    io->lights[1][1]->on();
    printf("   Enabling player 2 light 3");
    input();
    io->lights[1][1]->off();
    io->lights[1][2]->on();
    printf("   Turn off light: ");
    input();
    io->lights[1][2]->off();
}

static void
test_target(int player, int target)
{
    if (io->targets[player][target]->get() == TARGET_HIT) {
	printf("   *** TARGET HIT BEFORE SHOOTING IT!\n");
    }
    printf("   Shoot player %d target %d: ", player, target);
    fflush(stdout);
    io->lights[player][target]->on();
    while (io->targets[player][target]->get() != TARGET_HIT) {}
    io->lights[player][target]->off();
    printf("target hit.\n");
}

static void
test_targets()
{
    printf("Testing targets:\n");
    io->laser->on();
    for (int player = 0; player < 2; player++) {
	for (int target = 0; target < 3; target++) {
	     test_target(player, target);
	}
    }
    io->laser->off();
    printf("Done testing targets.\n");
}

static bool
test_is_enabled(int argc, char **argv, const char *test)
{
    if (argc == 1) return true;
    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], test) == 0) return true;
    }
    printf("%s: skipping test.\n", test);
    return false;
}

int main(int argc, char **argv)
{
    gpioInitialise();;
    wb_init();

    io = new earth_invaders_io_t();

    if (test_is_enabled(argc, argv, "blaster")) test_blasters();
    if (test_is_enabled(argc, argv, "start")) test_start();
    if (test_is_enabled(argc, argv, "endstops")) test_endstops();
    if (test_is_enabled(argc, argv, "scores")) test_scores();
    if (test_is_enabled(argc, argv, "lights")) test_lights();
    if (test_is_enabled(argc, argv, "targets")) test_targets();

    return 0;
}
