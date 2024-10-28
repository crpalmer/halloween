#include "pi.h"
#include "neopixel-pico.h"
#include "random-utils.h"
#include "time-utils.h"

const int N_LEDS = 6;

int main() {
   pi_init();
   NeoPixelPico *neo = new NeoPixelPico(0);
   neo->set_mode(neopixel_mode_RGB);
   neo->set_n_leds(N_LEDS);
   while (1) {
      for (int i = 0; i < N_LEDS; i++) {
          neo->set_led(i, random_number_in_range(0, 64), random_number_in_range(0, 64), random_number_in_range(160, 255));
      }
      neo->show();
      ms_sleep(random_number_in_range(10, 200));
   }
	
}
