#include "pi.h"
#include "neopixel-pico.h"
#include "random-utils.h"
#include "time-utils.h"

int main() {
   pi_init();
   NeoPixelPico *neo = new NeoPixelPico(0);
   neo->set_mode(neopixel_mode_RGB);
   neo->set_n_leds(2);
   while (1) {
      neo->set_led(0, random_number_in_range(0, 64), random_number_in_range(0, 64), random_number_in_range(128, 255));
      neo->set_led(1, random_number_in_range(0, 64), random_number_in_range(0, 64), random_number_in_range(128, 255));
      neo->show();
      ms_sleep(random_number_in_range(1, 100));
   }
	
}
