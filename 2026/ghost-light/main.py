from neopixel import NeoPixel
from machine import Pin
import random
import time

neo = NeoPixel(Pin(0, Pin.OUT), 24)

cur = 255
delta = -1
while True:
    cur += delta
    if cur < 100:
        cur = 100
        delta = +2
        time.sleep_ms(250)
    elif cur > 255:
        cur = 255
        delta = -2
        time.sleep_ms(250)
    neo.fill((cur, cur, cur))
    neo.write()
    time.sleep_ms(15) # + random.randint(-5, +5))