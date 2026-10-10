from machine import Pin
import time

class Actuator:
    def __init__(self, dir1, dir2):
        self.dir1 = dir1
        self.dir2 = dir2
        self.stop()

    def stop(self):
        self.dir1.value(0)
        self.dir2.value(0)

    def push(self):
        self.dir1.value(1)
        self.dir2.value(0)
        time.sleep_ms(100)
        self.stop()

    def retract(self):
        self.dir1.value(0)
        self.dir2.value(1)
        time.sleep_ms(100)
        self.stop()

light = Pin(0, Pin.OUT)
actuator = Actuator(Pin(2), Pin(3))

while True:
    light.value(1)
    print("on")
    actuator.push()
    time.sleep_ms(500)
    actuator.retract()
    light.value(0)
    time.sleep_ms(1000)
