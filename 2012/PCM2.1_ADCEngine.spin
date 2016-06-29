{{
+-----------------------------------------------------------------------------------------------------------------------------+               
¦ PCM2.1 Analog To Digital Converter Engine                                                                                   ¦
¦                                                                                                                             ¦
¦ Author: Kwabena W. Agyeman                                                                                                  ¦                              
¦ Updated: 6/2/2010                                                                                                           ¦
¦ Designed For: P8X32A @ 80Mhz                                                                                                ¦
¦ Version: 1.1                                                                                                                ¦
¦                                                                                                                             ¦
¦ Copyright (c) 2010 Kwabena W. Agyeman                                                                                       ¦              
¦ See end of file for terms of use.                                                                                           ¦
¦                                                                                                                             ¦
¦ Update History:                                                                                                             ¦
¦                                                                                                                             ¦
¦ v1.0 - Original release - 2/28/2010.                                                                                        ¦
¦ v1.1 - Added variable pin support - 6/2/2010.                                                                               ¦
¦                                                                                                                             ¦
¦ For each included copy of this object only one spin interpreter should access it at a time.                                 ¦
¦                                                                                                                             ¦
¦ Nyamekye,                                                                                                                   ¦
+-----------------------------------------------------------------------------------------------------------------------------+
+-----------------------------------------------------------------------------------------------------------------------------+
¦ Converter Circuit:                                                                                                          ¦
¦                                                                                                                             ¦
¦                                  3.3V                                                                                       ¦
¦                                   ¦                                                                                          ¦
¦                                  cap 1nF                                                                                      ¦
¦                            100Kohm¦                                                                                          ¦
¦ ADC Left Pin Out Number   ?-res---?  0.1microF                                                                                   ¦
¦                                   ?--cap- Left In                                                                             ¦
¦ ADC Left Pin In Number    ?-------?                                                                                          ¦
¦                                   ¦                                                                                          ¦
¦                                  cap 1nF                                                                                      ¦
¦                                   ¦                                                                                          ¦
¦                                  gnd                                                                                          ¦
¦                                                                                                                             ¦
¦                                   3.3V                                                                                      ¦
¦                                   ¦                                                                                          ¦
¦                                  cap 1nF                                                                                      ¦
¦                            100Kohm¦                                                                                          ¦
¦ ADC Right Pin Out Number  ?-res---?  0.1microF                                                                                   ¦
¦                                   ?--cap-? Right In                                                                            ¦
¦ ADC Right Pin In Number   ?-------?                                                                                          ¦
¦                                   ¦                                                                                          ¦
¦                                  cap 1nF                                                                                      ¦
¦                                   ¦                                                                                          ¦
¦                                  gnd                                                                                          ¦
+-----------------------------------------------------------------------------------------------------------------------------+
}}

VAR

  long cogNumber, sampleRate, sampleAdjust, callerPointer, callePointer
  word dataBlock[512]
  byte multiplyOrDivide, signedOrUnsigned, bitsPerSample, numberOfChannels

PUB transferData '' 3 Stack Longs

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Helps reliably transfers data to the stereo ADC to record.                                                               ¦
'' ¦                                                                                                                          ¦
'' ¦ Returns the address of the next data buffer to transfer data to.                                                         ¦
'' ¦                                                                                                                          ¦
'' ¦ At 8 bits per sample using 1 channel each data buffer is composed of 512 bytes with 512 samples per transfer.            ¦
'' ¦                                                                                                                          ¦
'' ¦ At 8 bits per sample using 2 channels each data buffer is composed of 512 bytes with 256 samples per transfer.           ¦
'' ¦                                                                                                                          ¦
'' ¦ At 16 bits per sample using 1 channel each data buffer is composed of 256 words with 256 samples per transfer.           ¦
'' ¦                                                                                                                          ¦
'' ¦ At 16 bits per sample using 2 channels each data buffer is composed of 256 words with 128 samples per transfer.          ¦
'' ¦                                                                                                                          ¦
'' ¦ This function waits indefiantly to sync up with the ADC driver.                                                          ¦    
'' ¦                                                                                                                          ¦
'' ¦ Use this function with the longmove/wordmove/bytemove commands and/or a file system to play data.                        ¦
'' +--------------------------------------------------------------------------------------------------------------------------+

                               
  repeat while(callerPointer == callePointer)
  result := @dataBlock[256 & callerPointer]
  not callerPointer    

PUB clearData '' 3 Stack Longs

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Clears all data in all recorder data buffers.                                                                            ¦
'' ¦                                                                                                                          ¦
'' ¦ Helps prevent old data from being recorded when changing to a new data transfer destination.                             ¦                                                                
'' +--------------------------------------------------------------------------------------------------------------------------+

  bytefill(@dataBlock, ((not(signedOrUnsigned)) & $80), 1024)

PUB changeSampleSign(signed) '' 4 Stack Longs

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Changes whether samples are signed or not.                                                                               ¦
'' ¦                                                                                                                          ¦
'' ¦ Signed - True to make all samples be saved as signed. False to make all samples be saved as unsigned.                    ¦
'' +--------------------------------------------------------------------------------------------------------------------------+

  signedOrUnsigned := signed
    
PUB changeBitsPerSample(newWidth) '' 7 Stack Longs

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Changes the bits per sample, 8 bits or 16 bits.                                                                          ¦
'' ¦                                                                                                                          ¦
'' ¦ NewWidth - New bits per sample to input samples at. Samples are sized by this value. (8 or 16)                           ¦
'' +--------------------------------------------------------------------------------------------------------------------------+

  bitsPerSample := (newWidth == 16)

  computeSampleAdjust
  
PUB changeNumberOfChannels(newNumber) '' 4 Stack Longs

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Changes the number of channels, 1 channel or 2 channels.                                                                 ¦
'' ¦                                                                                                                          ¦
'' ¦ When set to 1 channel mode the driver will input samples from both channels at once.                                     ¦
'' ¦                                                                                                                          ¦
'' ¦ When set to 2 channel mode the driver will input samples from both channels at simultaneously.                           ¦
'' ¦                                                                                                                          ¦
'' ¦ NewNumber - New number of channels to input samples at. Samples are grouped by this value. (1 or 2)                      ¦
'' +--------------------------------------------------------------------------------------------------------------------------+

  numberOfChannels := (newNumber == 2)
 
PUB changeSampleRate(newRate) '' 7 Stack Longs

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Changes the sample rate. The stereo ADC supportes samples rates from 1HZ to 80KHZ @ 80Mhz.                               ¦
'' ¦                                                                                                                          ¦
'' ¦ NewRate - New sample rate to input samples at. Samples are inputted at the frequency specified by this value.            ¦
'' +--------------------------------------------------------------------------------------------------------------------------+

  sampleRate := (clkfreq / ((newRate <# (clkfreq / constant(80_000_000 / 80_000))) #> 1))   
  
  computeSampleAdjust

PUB ADCEngineStart(leftPinInNumber, leftPinOutNumber, rightPinInNumber, rightPinOutNumber, newRate) '' 15 Stack Longs 

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Starts up the ADC driver running on a cog.                                                                               ¦
'' ¦                                                                                                                          ¦
'' ¦ Returns true on success and false on failure.                                                                            ¦
'' ¦                                                                                                                          ¦
'' ¦ LeftPinInNumber - Pin used to record left channel audio according to the converter circuit. -1 to disable.               ¦
'' ¦ LeftPinOutNumber - Pin used to record left channel audio according to the converter circuit. -1 to disable.              ¦
'' ¦ RightPinInNumber - Pin used to record right channel audio according to the converter circuit. -1 to disable.             ¦
'' ¦ RightPinOutNumber - Pin used to record right channel audio according to the converter circuit. -1 to disable.            ¦
'' ¦ NewRate - New sample rate to input samples at. Samples are inputted at the frequency specified by this value.            ¦ 
'' +--------------------------------------------------------------------------------------------------------------------------+

  ADCEngineStop
  changeSampleRate(newRate)

  if((chipver == 1) and (clkmode > 1))

    result := ((leftPinOutNumber <# 31) #> 0)
    newRate := ((rightPinOutNumber <# 31) #> 0)

    outputMask := (((leftPinOutNumber <> -1) & (|<result)) | ((rightPinOutNumber <> -1) & (|<newRate)))
    
    leftCounterSetup := (constant(%01001 << 26) + (result << 9) + ((leftPinInNumber <# 31) #> 0))  
    rightCounterSetup := (constant(%01001 << 26) + (newRate << 9) + ((rightPinInNumber <# 31) #> 0))   

    dataBlockAddress := @dataBlock
    callePointerAddress := @callePointer
    multiplyOrDivideAddress := @multiplyOrDivide
    unsignedOrSignedAddress := @signedOrUnsigned
    bitsPerSampleAddress := @bitsPerSample
    numberOfChannelsAddress := @numberOfChannels
    sampleAdjustAddress := @sampleAdjust
    
    cogNumber := cognew(@initialization, @sampleRate)  
    return (not(not(++cogNumber)))

PUB ADCEngineStop '' 3 Stack Longs

'' +--------------------------------------------------------------------------------------------------------------------------+
'' ¦ Shuts down the ADC driver running on a cog.                                                                              ¦                
'' +--------------------------------------------------------------------------------------------------------------------------+

  if(cogNumber)
    cogstop(-1 + cogNumber~) 

PRI computeSampleAdjust ' 3 Stack Longs

  result := ($FF | ($FFFF & (not(not(bitsPerSample)))))
  
  if(sampleRate =< result)
    sampleAdjust := (result / sampleRate)
    multiplyOrDivide := false
  else
    sampleAdjust := (sampleRate / result)
    multiplyOrDivide := true 
                                                              
DAT

' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
'                       ADC Driver
' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  

                        org     0

' //////////////////////Initialization/////////////////////////////////////////////////////////////////////////////////////////

initialization          mov     ctra,            leftCounterSetup           ' Setup counter I/O pins.
                        mov     ctrb,            rightCounterSetup          '
                        mov     frqa,            #1                         '
                        mov     frqb,            #1                         '

                        mov     dira,            outputMask                 ' Setup I/O pin directions.

                        mov     recorderPointer, dataBlockAddress           ' Setup data block pointer.

                        rdlong  timeCounter,     par                        ' Setup timing.  
                        add     timeCounter,     cnt                        '    

' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
'                       Recorder
' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

outerLoop               rdbyte  buffer,          numberOfChannelsAddress wz ' Setup recorder mode.
                        muxz    recorderMode,    #$1                        '
                        rdbyte  buffer,          bitsPerSampleAddress wz    '
                        muxz    recorderMode,    #$2                        '

                        test    recorderMode,    #$1 wc                     ' Setup counter.
                        mov     counter,         #128                       '
if_c_or_z               add     counter,         #128                       '
if_c_and_z              add     counter,         #256                       '

' //////////////////////Inner Loop/////////////////////////////////////////////////////////////////////////////////////////////                               

innerLoop               rdlong  buffer,          par                        ' Wait until next sample input period.
                        waitcnt timeCounter,     buffer                     '
  
                        mov     sampleBuffer,    phsa                       ' Input and set value.
                        sub     sampleBuffer,    leftDelta                  '
                        add     leftDelta,       sampleBuffer               '                               
                        call    #encode                                     '  

                        mov     sampleBuffer,    phsb                       ' Input and set value.
                        sub     sampleBuffer,    rightDelta                 '
                        add     rightDelta,      sampleBuffer               '
                        test    recorderMode,    #1 wc                      '
if_nc                   call    #encode                                     '
                                                                               
                        djnz    counter,         #innerLoop                 ' Loop.
                                                                                
' //////////////////////Outer Loop/////////////////////////////////////////////////////////////////////////////////////////////

                        rdlong  buffer,          callePointerAddress wz     ' Switch data block pointer.
if_z                    neg     buffer,          #1                         '
if_nz                   mov     buffer,          #0                         '
                        wrlong  buffer,          callePointerAddress        '

if_nz                   mov     recorderPointer, dataBlockAddress           ' Setup data block pointer.

                        jmp     #outerLoop                                  ' Loop.

' ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
'                       Encode Value
' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

encode                  rdbyte  buffer,          multiplyOrDivideAddress wz ' Scale up or down the sample.
                        rdlong  mathArgument,    sampleAdjustAddress        '
if_z                    call    #multiply                                   '
if_z                    mov     sampleBuffer,    mathResult                 '
if_nz                   call    #divide                                     '

                        test    recorderMode,    #$2 wc                     ' Modify data depending on sign and size.
                        rdbyte  buffer,          unsignedOrSignedAddress wz ' 
if_c                    max     sampleBuffer,    #$FF                       ' 
if_nz_and_c             sub     sampleBuffer,    #$80                       '
if_nc                   max     sampleBuffer,    wordMax                    '
if_nz_and_nc            sub     sampleBuffer,    wordAdjust                 '
                         
if_c                    wrbyte  sampleBuffer,    recorderPointer            ' Write data depending on size.
if_c                    add     recorderPointer, #1                         '
if_nc                   wrword  sampleBuffer,    recorderPointer            '
if_nc                   add     recorderPointer, #2                         '
                     
encode_ret              ret                                                 ' Return.

' ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
'                       Multiply
' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

multiply                mov     mathResult,      #0                         ' Setup multiply.

multiplyLoop            shr     sampleBuffer,    #1 wc                      ' Preform multiplication.
if_c                    add     mathResult,      mathArgument               '
                        shl     mathArgument,    #1                         '
                        tjnz    sampleBuffer,    #multiplyLoop              '

multiply_ret            ret                                                 ' Return.

' ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
'                       Divide
' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

divide                  shl     mathArgument,    #15                        ' Setup divide.
                        mov     mathResult,      #16                        '

divideLoop              cmpsub  sampleBuffer,    mathArgument wc            ' Preform division.
                        rcl     sampleBuffer,    #1                         '
                        djnz    mathResult,      #divideLoop                '

                        shl     sampleBuffer,    #16                        ' Throw away remainder.
                        shr     sampleBuffer,    #16                        '
                        
divide_ret              ret                                                 ' Return.

' ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
'                       Data
' ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

leftDelta               long    0                                           ' Delta initialized to zero.
rightDelta              long    0                                           ' Delta initialized to zero.
wordMax                 long    $FFFF                                       ' Edits word unsigned value. 
wordAdjust              long    $8000                                       ' Edits word signed value. 

' //////////////////////Configuration Settings/////////////////////////////////////////////////////////////////////////////////

leftCounterSetup        long    0
rightCounterSetup       long    0

outputMask              long    0 

' //////////////////////Addresses//////////////////////////////////////////////////////////////////////////////////////////////

dataBlockAddress        long    0
callePointerAddress     long    0
multiplyOrDivideAddress long    0
unsignedOrSignedAddress long    0          
bitsPerSampleAddress    long    0
numberOfChannelsAddress long    0
sampleAdjustAddress     long    0
                     
' //////////////////////Run Time Variables/////////////////////////////////////////////////////////////////////////////////////

buffer                  res     1
counter                 res     1

recorderPointer         res     1
recorderMode            res     1

sampleBuffer            res     1
timeCounter             res     1

mathResult              res     1
mathArgument            res     1

' /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        fit     496
 
{{

+-----------------------------------------------------------------------------------------------------------------------------+
¦                                                   TERMS OF USE: MIT License                                                 ¦                                                            
+-----------------------------------------------------------------------------------------------------------------------------¦
¦Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation   ¦ 
¦files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,   ¦
¦modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the        ¦
¦Software is furnished to do so, subject to the following conditions:                                                         ¦         
¦                                                                                                                             ¦
¦The above copyright notice and this permission notice shall be included in all copies or substantial portions of the         ¦
¦Software.                                                                                                                    ¦
¦                                                                                                                             ¦
¦THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE         ¦
¦WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR        ¦
¦COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,  ¦
¦ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                        ¦
+-----------------------------------------------------------------------------------------------------------------------------+
}}                        