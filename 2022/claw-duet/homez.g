; homez.g
; called to home the Z axis
G91               ; relative positioning
G1 H1 Z-600 F1800 ; move quickly to X axis endstop and stop there (first pass)
G1 H2 Z+5 F6000   ; go back a few mm
G1 H1 Z-20 F360   ; move slowly to X axis endstop once more (second pass)
G90               ; absolute positioning