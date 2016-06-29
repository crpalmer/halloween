{{
}}

CON

  _clkfreq = 80_000_000
  _clkmode = xtal1 + pll16x

  CYLINDER = 8
  MISTER = 9

  VOLUME = 100
  SD_P0 = 17
  AUDIO_P0 = 24

  MODE = 1
  BUTTON = 0

VAR
   long stack[10]

OBJ

  'play_file : "play_file"
 
PUB Snake | P, seed, go, ms, mode_transition

  'play_file.start(SD_P0, AUDIO_P0, VOLUME)

  go := 0
  cognew(attack(@go), @stack)
  
  seed := 1234567
  ms := clkfreq / 1000
  
  repeat
    mode_transition := 0
    
    if ina[MODE] == 0
      ?seed
      P := ||seed
      P := P // 20 + 5
      repeat
        waitcnt(cnt + clkfreq)
        P := P-1
      until ina[MODE] == 1 OR ina[BUTTON] == 1 OR P == 0
      if ina[MODE] == 1
        mode_transition := 1
    else
      repeat
      until ina[BUTTON] == 1 OR ina[MODE] == 0
      if ina[MODE] == 0
        mode_transition := 1

    if mode_transition == 0
      go := 1      
      'play_file.synchronously(STRING("snake.wav"), 2)

PUB attack(addr)
  outa[CYLINDER] := 0
  outa[MISTER] := 0
  dira[CYLINDER] := 1
  dira[MISTER] := 1
  repeat
    repeat
    until LONG[addr] <> 0
    LONG[addr] := 0
    'waitcnt(clkfreq / 10 + cnt)
    outa[MISTER] := 1
    outa[CYLINDER] := 1
    waitcnt(clkfreq + cnt)
    outa[MISTER] := 0
    outa[CYLINDER] := 0