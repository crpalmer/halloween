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
    io->laser->set(1);
    printf("   The laser should be on, hit enter to disable the laser: ");
    input();
    io->laser->set(0);

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
    io->start_light->set(1);
    printf("   Light should be on, hit enter to disable it: ");
    input();
    io->start_light->set(0);
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
    test_endstop(io->endstop_idler, "motor");
    test_endstop(io->endstop_idler, "idler");
    printf("Done testing endstops.\n\n");
}

static void
test_scores()
{
    printf("Testing score boards:\n");
    printf("   Setting P1 score to 10: ");
    input();
    io->score[0]->set(10);
    printf("   Setting P2 score to 10: ");
    input();
    io->score[1]->set(10);
    printf("   Setting high score to 10: ");
    input();
    io->high_score->set(10);
    printf("   Setting all scores back to 0: ");
    input();
    io->score[0]->set(0);
    io->score[1]->set(0);
    io->high_score->set(0);
    printf("   All scores should now be zero: ");
    input();
    printf("Done testing score boards.\n\n");
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

    return 0;
}

