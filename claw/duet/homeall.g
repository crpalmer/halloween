; homeall.g
; called to home all axes
G91                           ; relative positioning
G1 H1 X+500 Y+500 Z-600 F1800 ; move quickly to X and Y axis endstops and stop there (first pass)
G1 H2   X-5   Y-5   Z+5 F6000 ; go back a few mm
G1 H1  X+20  Y+20  Z-20 F360  ; move slowly to X and Y axis endstops once more (second pass)
G90                           ; absolute positioning