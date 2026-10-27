from machine import Pin
import time
import random

class Actuator:
    def __init__(self, push_pin, retract_pin, actuate_ms = 200):
        self.actuate_ms = actuate_ms
        self.push_pin = push_pin
        self.retract_pin = retract_pin
        self.push_pin.value(0)
        self.retract_pin.value(0)
 
    def push(self):
        self.push_pin.value(1)
        time.sleep_ms(self.actuate_ms)
        self.push_pin.value(0)

    def retract(self):
        self.retract_pin.value(1)
        time.sleep_ms(self.actuate_ms)
        self.retract_pin.value(0)

actuator = Actuator(Pin(0, Pin.OUT), Pin(1, Pin.OUT))
light = Pin(2, Pin.OUT)

while True:
    time.sleep_ms(random.randint(5000, 10000))
    light.value(1)
    print("on")
    for bang in range(0, random.randint(2, 4)):
        if bang > 0:
            time.sleep_ms(random.randint(200, 500))
        print("bang")
        actuator.push()
        time.sleep_ms(50)
        actuator.retract()
    light.value(0)
 
