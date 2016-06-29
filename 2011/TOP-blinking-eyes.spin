CON

  _clkmode        = xtal1 + pll16x           ' Feedback and PLL multiplier
  _xinfreq        = 5_000_000                ' External oscillator = 5 MHz

  MIN_TIME_BETWEEN_BLINKS = 2000
  MAX_TIME_BETWEEN_BLINKS = 8000
  MIN_TIME_OF_BLINK = 150
  MAX_TIME_OF_BLINK = 500   
  STACK_SIZE = 10
  
VAR
  long stack1[STACK_SIZE]
  long stack2[STACK_SIZE]
  long stack3[STACK_SIZE]
  long stack4[STACK_SIZE]
  long stack5[STACK_SIZE]
  long stack6[STACK_SIZE]
  long stack7[STACK_SIZE]
  
PUB BlinkingEyes
  cognew(OneSetOfEyes(1), @stack1)
  waitcnt(clkfreq / 1000 * 100 + cnt)
  cognew(OneSetOfEyes(2), @stack2)
  waitcnt(clkfreq / 1000 * 100 + cnt)
  cognew(OneSetOfEyes(3), @stack3)
  waitcnt(clkfreq / 1000 * 100 + cnt)
  cognew(OneSetOfEyes(4), @stack4)
  waitcnt(clkfreq / 1000 * 100 + cnt)
  cognew(OneSetOfEyes(5), @stack5)
  waitcnt(clkfreq / 1000 * 100 + cnt)
  cognew(OneSetOfEyes(6), @stack6)
  waitcnt(clkfreq / 1000 * 100 + cnt)
  cognew(OneSetOfEyes(7), @stack7)
  waitcnt(clkfreq / 1000 * 100 + cnt)
  OneSetOfEyes(0)

PUB OneSetOfEyes(start) | seed, P, eye

  seed := 1234567+start

  dira[start]~~
  outa[start]~~
  dira[start+8]~~
  outa[start+8]~~
  dira[start+16]~~
  outa[start+16]~~

  repeat                                     ' Endless loop
    ?seed
    P := ||seed
    eye := (P // 3)*8 + start
    P := P // (MAX_TIME_BETWEEN_BLINKS - MIN_TIME_BETWEEN_BLINKS + 1) + MIN_TIME_BETWEEN_BLINKS
    waitcnt(clkfreq / 1000 * P + cnt)
    outa[eye] := 0
    ?seed
    P := ||seed
    P := P // (MAX_TIME_OF_BLINK - MIN_TIME_OF_BLINK + 1) + MIN_TIME_OF_BLINK
    waitcnt(clkfreq / 1000 * P + cnt)
    outa[eye] := 1