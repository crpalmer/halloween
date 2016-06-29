{{
}}

CON

  _clkfreq = 80_000_000
  _clkmode = xtal1 + pll16x

  ANNIE_MIN_POS = 1100
  ANNIE_RANGE = 850
  
  VOLUME = 100
  SD_P0 = 0
  AUDIO_P0 = 22
  SKULL_SERVO = 7
  SKULL_LEDS = 6

  WHEELS_COUNTDOWN_N_TRACKS = 25
  N_TRACKS = 8 

OBJ

  skull : "talking_skull"
  play_file : "play_file"
  random_track : "random_track"
  debug : "Parallax Serial Terminal"
 
PUB Deer | P, seed, ms, wheels_countdown, wav

  debug.start(250000)
  play_file.start(SD_P0, AUDIO_P0)
  skull.start(ANNIE_MIN_POS, ANNIE_RANGE, play_file.sampleAddress, SKULL_SERVO, SKULL_LEDS, 1)
  random_track.init
  
  seed := 1234567
  ms := clkfreq / 1000
  wheels_countdown := WHEELS_COUNTDOWN_N_TRACKS
  
  repeat
    wheels_countdown := wheels_countdown - 1
    debug.str(STRING(" wheels_countdown="))
    debug.dec(wheels_countdown)
    if wheels_countdown == 0
      wheels_countdown := WHEELS_COUNTDOWN_N_TRACKS
      'play_file.synchronously(STRING("wheels.wave"), 1)
    else
      wav := random_track.random_wave_file(N_TRACKS)
      debug.str(wav) 
      play_file.synchronously(wav, 1)
    ?seed
    P := ||seed
    P := P // 10000 + 3000
    debug.str(STRING(" sleep="))
    debug.dec(P)
    waitcnt(cnt + ms * P)            