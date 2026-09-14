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

    def _vary_the_colour(self, rgb):
        rgb = int(rgb)
        rgb += randint(-27, +27)
        if rgb < 0:
            return 0
        elif rgb > 255:
            return 255
        else:
            return rgb
    
    def _v(self, rgb):
        return gamma_correction[self._vary_the_colour(rgb)]

    def flicker(self):
        pct = randint(0, 99)
        for i in range(len(self._neo)):
            if pct < 0:
                self._neo[i] = (self._v(131), self._v(56), self._v(154))
            elif pct < 12:
                self._neo[i] = (self._v(255), self._v(15), self._v(15))
            else:
                self._neo[i] = (self._v(223), self._v(56), self._v(25))
        self._neo.write()

logs = []
logs.append(Log(Pin(0), 21))
logs.append(Log(Pin(1), 19))
logs.append(Log(Pin(2), 19))
logs.append(Log(Pin(3), 14))
logs.append(Log(Pin(4), 14))
logs.append(Log(Pin(5), 14))

while True:
    start = time.ticks_ms()
    delay = randint(10, 100)
    for log in logs:
        log.flicker()
    delta = time.ticks_diff(time.ticks_ms(), start)
    if delta < delay:
        time.sleep_ms(delay - delta)
