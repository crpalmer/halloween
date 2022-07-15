; homex.g
; called to home the X axis
G91               ; relative positioning
G1 H1 X+500 F1800 ; move quickly to X axis endstop and stop there (first pass)
G1 H2 X-5 F6000   ; go back a few mm
G1 H1 X+20 F360   ; move slowly to X axis endstop once more (second pass)
G90               ; absolute positioning