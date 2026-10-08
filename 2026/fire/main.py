from machine import Pin
from neopixel import NeoPixel
from random import randint
import time

def gamma(c, gamma=2.8):
    return int(pow(c / 255.0, gamma) * 255.0)

gamma_correction = [gamma(c) for c in range(256)]

class Log:
    def __init__(self, pin, n):
        self._neo = NeoPixel(pin, n)

    def _vary_the_colour(self, rgb, delta):
        rgb = int(rgb)
        rgb += randint(-delta, +delta)
        if rgb < 0:
            return 0
        elif rgb > 255:
            return 255
        else:
            return rgb
    
    def _v(self, rgb, delta=15):
        return gamma_correction[self._vary_the_colour(rgb, delta)]

    def _random_colour(self):
        pct = randint(0, 1000)
        if pct < 0:
            return tuple(self._v(c, 0) for c in (59, 0, 86))
        elif pct < 10:
            return tuple(self._v(c, 5) for c in (195, 97, 60))
        else:
            return tuple(self._v(c, 8) for c in (179, 82, 19))

    def flicker(self):
        self._neo.write()
        for i in range(len(self._neo)):
            self._neo[i] = self._random_colour()
        self._neo.write()
        
# l = Log(Pin(0), 2)
# neo = NeoPixel(Pin(2), 10)
# red = (223, 56, 26)
# orange = (150, 50, 33)
# d = 15
# neo[0] = tuple(gamma_correction[c] for c in red)
# neo[1] = tuple(gamma_correction[max(c - d, 0)] for c in red)
# neo[2] = tuple(gamma_correction[min(c + d, 255)] for c in red)
# neo[3] = tuple(gamma_correction[c] for c in orange)
# neo[4] = tuple(gamma_correction[max(c - d, 0)] for c in orange)
# neo[5] = tuple(gamma_correction[min(c + d, 255)] for c in orange)
# while True:
#     neo[7] = tuple(l._v(c) for c in red)
#     neo[8] = tuple(l._v(c) for c in orange)
#     neo.write()
#     time.sleep_ms(10)

# n = NeoPixel(Pin(1), 21)
# for i in range(21):
#     n[i] = (0, 0, 0)
# n.write()
# time.sleep(1000)

logs = []
logs.append(Log(Pin(0), 21))
logs.append(Log(Pin(1), 19))
logs.append(Log(Pin(2), 18))
logs.append(Log(Pin(3), 15))
logs.append(Log(Pin(4), 15))
logs.append(Log(Pin(5), 21))

while True:
    start = time.ticks_ms()
    delay = randint(10, 100)
    for log in logs:
        log.flicker()
    delta = time.ticks_diff(time.ticks_ms(), start)
    if delta < delay:
        time.sleep_ms(delay - delta)