CON

VAR
    long              position
    long              stack[32], stack2[32], stack3[32]

OBJ
    debug : "Parallax Serial Terminal"
    
PUB start(min_pos, range, addr, servo, eyes, sample_track)
    cognew(background_worker(eyes, min_pos, range, addr, @position, sample_track), @stack)
    cognew(background_servo(@position, servo), @stack2)
'    cognew(background_printer, @stack3)

PUB background_worker(eyes, min_pos, range, sample_addr, position_addr, sample_track) | x,y
  dira[eyes] := 1
  LONG[sample_addr][sample_track] := 0
  repeat
    x := LONG[sample_addr][sample_track]
    ||x
    if x > 32768 / 4
      outa[eyes] := 1
    else
      outa[eyes] := 0
    position := ((x * range) / 32768) + min_pos

PUB background_servo(addr, servo) | us, x, pulse
  dira[servo] := 1
  us := clkfreq / 1_000_000
  x := cnt + 20_000 * us
  repeat
    pulse := cnt + LONG[addr] * us
    outa[servo] := 1
    waitcnt(pulse)
    outa[servo] := 0
    waitcnt(x)
    x := x + 20_000 * us
    

PUB background_printer
    debug.start(250_000)
    repeat
      debug.dec(position)
      debug.str(string(" "))
      