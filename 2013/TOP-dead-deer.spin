{{
}}

CON

  _clkfreq = 80_000_000
  _clkmode = xtal1 + pll16x

  MIN_POS = 1100
  RANGE = 450
  
  VOLUME = 100
  SD_P0 = 0
  AUDIO_P0 = 22
  SKULL_SERVO = 7
  SKULL_LEDS = 6

  WHEELS_COUNTDOWN_N_TRACKS = 5

OBJ

  skull : "talking_skull"
  play_file : "play_file"
  debug : "Parallax Serial Terminal"
 
PUB Deer | P, seed, ms, wheels_countdown

  debug.start(250000)
  play_file.start(SD_P0, AUDIO_P0)
  skull.start(MIN_POS, RANGE, play_file.sampleAddress, SKULL_SERVO, SKULL_LEDS, 1)
  
  seed := 1234567
  ms := clkfreq / 1000
  wheels_countdown := 2
  
  repeat
    wheels_countdown := wheels_countdown - 1
    debug.str(STRING(" wheels_countdown="))
    debug.dec(wheels_countdown)
    if wheels_countdown == 0
      wheels_countdown := WHEELS_COUNTDOWN_N_TRACKS
      play_file.synchronously(STRING("wheels.wav"), 1)
    else
      play_file.synchronously(STRING("manson.wav"), 1)

    ?seed
    P := ||seed              
    P := P // 7000 + 3000
    debug.str(STRING(" sleep1="))
    debug.dec(P)
    waitcnt(cnt + ms * P)

    play_file.synchronously(STRING("blink.wav"), 1)
    
    ?seed
    P := ||seed              
    P := P // 7000 + 3000
    debug.str(STRING(" sleep2="))
    debug.dec(P)
    waitcnt(cnt + ms * P)            