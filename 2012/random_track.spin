CON

    ZERO_CHAR = 48

VAR
    LONG last, last2
    LONG seed
    BYTE filename[10]
        
PUB init
  seed := 1234567
  last := -1
  last2 := -1
  ByteMove(@filename, STRING("00.wav"), 7)
  
PUB random_track(N_TRACKS) | this
    repeat
      ?seed
      this := seed
      ||this
      this := (this//N_TRACKS)
    until this <> last and this <> last2
    last2 := last
    last := this
    return this

PUB random_wave_file(N) | this
  this := random_track(N)
  filename[0] := this / 10 + ZERO_CHAR
  filename[1] := this // 10 + ZERO_CHAR
  return @filename
