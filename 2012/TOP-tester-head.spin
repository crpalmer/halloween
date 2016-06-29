{{
}}

CON

  _clkfreq = 80_000_000
  _clkmode = xtal1 + pll16x

  AUDIO_P0 = 0
  
OBJ

  skull : "talking_skull"
' play_file : "play_file"
  debug : "Parallax Serial Terminal"
  adc : "PCM2.1_ADCEngine"

VAR
  LONG     head_stack[32]
  LONG     tail_stack[32]
  LONG     head_speed, head_range
  LONG     tail_speed, tail_range
  BYTE     filename[10]
  
PUB TesterHead | addr, end_addr, pos 

  debug.start(250000)

  adc.ADCEngineStart(AUDIO_P0, AUDIO_P0+1, AUDIO_P0+2, AUDIO_P0+3, 22050) '' 15 Stack Longs
  adc.changeSampleSign(0)
  adc.changeBitsPerSample(8)
  adc.changeNumberOfChannels(1)
   
  repeat
    addr := adc.transferData
    end_addr := addr + 256
    pos := 0
    repeat while addr < end_addr
      pos := BYTE[addr] #> pos
      addr := addr + 1        
    debug.dec(pos) 
    debug.str(string(" "))
  