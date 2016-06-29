{{
***********************************
* Casa Fear Zombie                *
*                                 *
* Author: Christopher R. Palmer   *
*                                 *
***********************************
}}

CON

  _clkfreq = 80_000_000
  _clkmode = xtal1 + pll16x

  ZERO_CHAR = 48

  SD_P0 = 16
  AUDIO_P0 = 22
  PIR = 0
  CYLINDER_P0 = 8

  VOLUME = 100
  N_TRACKS = 5
  MOVE_PAUSE_MIN = 100
  MOVE_PAUSE_MAX = 350
  INTER_PAUSE_MIN = 10000
  INTER_PAUSE_MAX = 30000
  
VAR
  long       ms
  byte       filename[10]
  byte       thrashing
  long       stack[32]
  
OBJ

  play_file : "play_file"
  debug : "Parallax Serial Terminal"
 
PUB CasaFearZombie | seed, P, last, last2, track, pause, error

  debug.start(250000)

  ByteMove(@filename, STRING("00.wav"), 7)

  debug.str(STRING("Loading SDcard", 13))  
  play_file.start(SD_P0, AUDIO_P0)

  cognew(backgroundThrashing, @stack)
  
  last := -1
  last2 := -1
  seed := 1234567
  ms := clkfreq / 1000

  repeat
    debug.str(STRING("Waiting", 13))
    seed := wait_pir_active(seed)
    repeat
      ?seed
      P := ||seed
      track := P // N_TRACKS
      debug.str(STRING("Trying track: "))
      debug.dec(track)
      debug.str(STRING("last two were "))
      debug.dec(last)
      debug.str(STRING(" "))
      debug.dec(last2)
      debug.char(13)
    until track <> last and track <> last2
    last := last2
    last2 := track

    filename[0] := track / 10 + ZERO_CHAR
    filename[1] := track // 10 + ZERO_CHAR

    thrashing := 1
    play_file.synchronously(@filename, 2)
    thrashing := 0

    ?seed
    P := ||seed
    pause := P // (INTER_PAUSE_MAX - INTER_PAUSE_MIN) + INTER_PAUSE_MIN
    debug.str(STRING("inter-run pause of "))
    debug.dec(pause)
    debug.char(13)
    waitcnt(cnt + pause * ms)
  
PRI wait_pir_active(seed) | n
  n := 0
  repeat
    ?seed
    if ina[PIR]
      n++
    else
      n := 0
  until n > 5
   
  return seed

PRI backgroundThrashing | seed, P, MYms
  seed := 1234567
  dira[CYLINDER_P0] := 1
  dira[CYLINDER_P0+1] := 1
  
  myms := clkfreq / 1000

  repeat
    ?seed
    P := ||seed
    if thrashing
     outa[CYLINDER_P0] := P // 2
     outa[CYLINDER_P0+1] := (P / 2) // 2
    ?seed
    P := ||seed
    P := P // (MOVE_PAUSE_MAX - MOVE_PAUSE_MIN) + MOVE_PAUSE_MIN
    waitcnt(cnt + (P * myms))
   