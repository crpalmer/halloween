{{
}}

CON

  _clkfreq = 80_000_000
  _clkmode = xtal1 + pll16x

  ZERO_CHAR = 48

  N_TRACKS = 5

  MIN_INTER_WAIT = 5000
  MAX_INTER_WAIT = 20000
  
  SD_P0 = 12
  AUDIO_P0 = 24
  MOUTH_SERVO = 4
  HEAD_SERVO = 5
  TAIL_SERVO = 6
  LEDS = 3
  PIR = 0
  
OBJ

  skull : "talking_skull"
  play_file : "play_file"
  'debug : "Parallax Serial Terminal"

VAR
  LONG     head_stack[32]
  LONG     tail_stack[32]
  LONG     head_speed, head_range
  LONG     tail_speed, tail_range
  BYTE     filename[10]
  
PUB Baxter | check, P, seed, this, last, last2, ms, pir_passes

  'debug.start(250000)
  
  ByteMove(@filename, STRING("00.wav"), 7)

'  dira[MOUTH_SERVO] := 1
'  ms := clkfreq / 1_000_000
'      last := cnt + 20_000 * ms
'  repeat
'    last2 := cnt + 1100 * ms
'    outa[MOUTH_SERVO] := 1
'    waitcnt(last2)
'    outa[MOUTH_SERVO] := 0
'    waitcnt(last)
'    last := last + 20_000 * ms
                                                                                
  play_file.start(SD_P0, AUDIO_P0)
  skull.start(1050, 750, play_file.sampleAddress, MOUTH_SERVO, LEDS, 1)
  cognew(background_mover(HEAD_SERVO, @head_speed, @head_range), @head_stack)
  cognew(background_mover(TAIL_SERVO, @tail_speed, @tail_range), @tail_stack)

  seed := 1234567
  ms := clkfreq / 1000

  repeat
    pir_passes := 0
'    repeat
'      if ina[PIR] == 0
'        pir_passes := 0
'      else
'        pir_passes ++
'      waitcnt(cnt + ms)
'    until pir_passes > 100

    repeat
      ?seed
      this := seed
      ||this
      this := (this//N_TRACKS)
    until this <> last and this <> last2
    last2 := last
    last := this

    head_speed := WORD[@movements][this*4]
    head_range := WORD[@movements][this*4+1]
    tail_speed := WORD[@movements][this*4+2]
    tail_range := WORD[@movements][this*4+3]
    
    filename[0] := this / 10 + ZERO_CHAR
    filename[1] := this // 10 + ZERO_CHAR

    'debug.dec(this)
    'debug.str(@filename)
    'debug.str(STRING("   "))

    play_file.synchronously(@filename, 1)
    head_speed := 0
    tail_speed := 0
    ?seed
    P := ||seed
    P := P // (MAX_INTER_WAIT - MIN_INTER_WAIT) + MIN_INTER_WAIT
    waitcnt(cnt + ms * P)

PRI background_mover(servo, speed_addr, range_addr) | position, dir, speed, range, us, x, seed
   seed := 20111031+servo
   dira[servo] := 1
   dir := 1
   position := 1500
   us := clkfreq / 1_000_000
   x := cnt + 20_000 * us
   repeat
    repeat while LONG[speed_addr] == 0
      outa[servo] := 1
      waitcnt(cnt + position * us)
      outa[servo] := 0
      waitcnt(x)
      ' Keep the timing tight by including the logic
      x := cnt + 20_000 * us
      speed := LONG[speed_addr]
      
    dir := -1*dir
    range := random(LONG[range_addr], @seed)
    speed := random(LONG[speed_addr], @seed)
    position := position #> (1500 - range) <# (1500 + range)
    repeat
      ?seed
      position := position + speed*dir
      outa[servo] := 1
      waitcnt(cnt + position * us)
      outa[servo] := 0
      waitcnt(x)
      ' Keep the timing tight by including the logic
      x := cnt + 20_000 * us
    until (position < 1500 - range) or (position > 1500 + range) or LONG[speed_addr] == 0


PRI random(range, seed_addr) | x
  x := LONG[seed_addr]
  ?x
  LONG[seed_addr] := x
  ||x
  range := range / 2
  return range + (x // range) + 1
  
                         ' Head:speed/range   Tail:speed/range
DAT
movements  WORD             10, 200,   5,  80
           WORD             5,  500,   2,  50
           WORD             40, 400,  20, 200
           WORD             20, 300,  40, 500
           WORD             20, 100,  10,  70