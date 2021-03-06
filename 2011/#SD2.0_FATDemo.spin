{{
┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐               
│ SD2.0 FATEngine Demo                                                                                                        │
│                                                                                                                             │
│ Author: Kwabena W. Agyeman                                                                                                  │                              
│ Updated: 6/16/2010                                                                                                          │
│ Designed For: P8X32A @ 80Mhz                                                                                                │
│ Version: 1.0                                                                                                                │
│                                                                                                                             │
│ Copyright (c) 2010 Kwabena W. Agyeman                                                                                       │              
│ See end of file for terms of use.                                                                                           │
│                                                                                                                             │
│ Update History:                                                                                                             │
│                                                                                                                             │
│ v1.0 - Original release - 6/16/2010.                                                                                        │
│                                                                                                                             │
│ Type "help" into the serial terminal and press enter after connecting to display the command list.                          │
│                                                                                                                             │
│ Nyamekye,                                                                                                                   │
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
}}

CON

  _clkfreq = 80_000_000
  _clkmode = xtal1 + pll16x

  _leftChannelVolume = 70
  _rightChannelVolume = 70
  _micSampleFrequency = 8_000
  _micBitsPerSample = 8
  
  _baudRateSpeed = 115_200
  _newLineCharacter = com#carriage_return
  _clearScreenCharacter = 16
  _homeCursorCharacter = 1

  _receiverPin = 31
  _transmitterPin = 30
  
  _clockDataPin = 29
  _clockClockPin = 28
  
  _cardDataOutPin = 16
  _cardClockPin = 17
  _cardDataInPin = 18
  _cardChipSelectPin = 19

  _leftChannelAudioPin = 24
  _rightChannelAudioPin = 23
  
  _leftMicInputPin = 7
  _leftMicFeedBackPin = 6
  _rightMicInputPin = -1
  _rightMicFeedBackPin = -1

OBJ

  com: "RS232_COMEngine.spin"
  rtc: "DS1307_RTCEngine.spin"
  fat: "SD2.0_FATEngine.spin"
  dac: "PCM2.1_DACEngine.spin"
  adc: "PCM2.1_ADCEngine.spin" 
  str: "ASCII0_STREngine.spin"
 
PUB shell

  dira[15] := 1
  outa[15] := 1
  
  ifnot(com.COMEngineStart(_receiverPin, _transmitterPin, _baudRateSpeed))
    reboot
      
  ifnot(rtc.RTCEngineStart(_clockDataPin, _clockClockPin))
    reboot

  ifnot(fat.FATEngineStart(_cardDataOutPin, _cardClockPin, _cardDataInPin, _cardChipSelectPin, _clockDataPin, _clockClockPin))
    reboot

  ifnot(dac.DACEngineStart(_leftChannelAudioPin, _rightChannelAudioPin, 0))
    reboot

  ifnot(adc.ADCEngineStart(_leftMicInputPin, _leftMicFeedBackPin, _rightMicInputPin, _rightMicFeedBackPin, 0))
    reboot  
                                                                                                                               
  repeat
  
    result := shellLine(string(">_ "))
    com.transmitString(string("Running command: "))
    com.transmitString(result)
    com.transmitByte(_newLineCharacter)

    result := \shellCommands(result)
    com.transmitString(result)
    com.transmitByte(_newLineCharacter)

    if(fat.checkErrorNumber)
      com.transmitByte(_newLineCharacter) 

PRI shellCommands(stringPointer)

  stringPointer := str.tokenizeString(stringPointer)
  
  programClear(stringPointer)
  programEcho(stringPointer)
  
  programReboot(stringPointer)
  programHelp(stringPointer)  

  programMount(stringPointer)
  programUnmount(stringPointer)

  programFreeSpace(stringPointer)
  programUsedSpace(stringPointer)
  
  programChangeDirectory(stringPointer)
  programChangeAttributes(stringPointer)

  programRename(stringPointer)
  programRemove(stringPointer)

  programMakeFile(stringPointer)
  programMakeDirectory(stringPointer)

  programBoot(stringPointer)
  programFormat(stringPointer)
  
  programList(stringPointer)
  programPrintWorkingDirectory(stringPointer)
  
  programConcatenate(stringPointer)
  programTest(stringPointer)

  programDate(stringPointer)
  programTime(stringPointer)

  programPlayWavFile(stringPointer)
  programRecordWavFile(stringPointer)

  abort string("Command Not Found!", _newLineCharacter)

PRI shellLine(prompt)

  com.transmitString(prompt)

  repeat
    result := com.receivedByte
    str.putCharacter(result)    
  while((result <> com#carriage_return) and (result <> com#line_feed) and (result <> com#null))

  return str.trimString(str.getCharacters(true))
  
PRI shellDecision(prompt, characterToFind, otherCharacterToFind)

  com.transmitString(prompt)  

  repeat
    prompt := com.receivedByte
    result or= ((prompt == characterToFind) or (prompt == otherCharacterToFind))
    if(prompt == com#backspace)
      return false
    
  while((prompt <> com#carriage_return) and (prompt <> com#line_feed) and (prompt <> com#null))  

PRI programClear(stringPointer)

  ifnot(str.stringCompareCI(string("clear"), stringPointer))
    rtc.pauseForMilliseconds(500)  
    com.transmitString(string(_clearScreenCharacter, _homeCursorCharacter))
    abort 0

PRI programEcho(stringPointer)

  ifnot(str.stringCompareCI(string("echo"), stringPointer))   
    com.transmitString(str.trimString(stringPointer += 5))
    com.transmitByte(_newLineCharacter)
    abort 0

PRI programReboot(stringPointer)

  ifnot(str.stringCompareCI(string("reboot"), stringPointer))
    com.transmitString(string("Rebooting"))
    
    repeat 3
      rtc.pauseForMilliseconds(500)  
      com.transmitString(string(" ."))

    com.transmitByte(_newLineCharacter)
    rtc.pauseForMilliseconds(500)
      
    reboot    

PRI programHelp(stringPointer)

  ifnot(str.stringCompareCI(string("help"), stringPointer))
    com.transmitString(@shellProgramHelpStrings)
    abort 0

DAT shellProgramHelpStrings

  byte _newLineCharacter
  byte "Command Listing"
  byte _newLineCharacter, _newLineCharacter

  byte "<clear>                          - Clear the screen."
  byte _newLineCharacter
  byte "<echo> <string>                  - Echo the string argument."
  byte _newLineCharacter
  byte "<reboot>                         - Reboot the propeller chip."
  byte _newLineCharacter 
  byte "<help>                           - Show the command listing."
  byte _newLineCharacter
  byte "<mount> <partition#> <check@>    - Mount the file system. C=Checkdisk."
  byte _newLineCharacter
  byte "<unmount>                        - Unmount the file system."
  byte _newLineCharacter
  byte "<df> <fast>                      - Compute free sectors. F=Fast. 1 Sector = 512 Bytes."
  byte _newLineCharacter
  byte "<du> <fast>                      - Compute used sectors. F=Fast. 1 Sector = 512 Bytes."
  byte _newLineCharacter  
  byte "<cd> <directory>                 - Change directory."
  byte _newLineCharacter
  byte "<attrib> <entry> <newAttributes> - Change attributes. R=ReadOnly, H=Hidden, S=System, A=Archive."
  byte _newLineCharacter
  byte "<rename> <oldName> <newName>     - Rename a file or directory."
  byte _newLineCharacter
  byte "<rm> <entry>                     - Remove a file or directory."
  byte _newLineCharacter
  byte "<mkfil> <name>                   - Make a new file."
  byte _newLineCharacter               
  byte "<mkdir> <name>                   - Make a new directory."
  byte _newLineCharacter
  byte "<boot> <file>                    - Reboot the propeller chip from a file."
  byte _newLineCharacter
  byte "<format> <partition> <newLabel>  - Format the selected partition and give it a new label."
  byte _newLineCharacter
  byte "<ls>                             - List the contents of the working directory."
  byte _newLineCharacter
  byte "<pwd>                            - Print the working directory path."
  byte _newLineCharacter  
  byte "<cat> <file>                     - Print the ASCII interpreted contents of a file."
  byte _newLineCharacter
  byte "<test>                           - Test file system functions and read and write speed."
  byte _newLineCharacter  
  byte "<date>                           - Display the current date and time."
  byte _newLineCharacter 
  byte "<time>                           - Change the current date and time."
  byte _newLineCharacter
  byte "<play> <file>                    - Play a WAV file."
  byte _newLineCharacter 
  byte "<record> <file>                  - Record a WAV file."
  byte _newLineCharacter, 0

PRI programMount(stringPointer)

  ifnot(str.stringCompareCI(string("mount"), stringPointer))
    fat.mountPartition(str.decimalToInteger(str.tokenizeString(0)), byte[str.tokenizeString(0)])
    
    com.transmitString(string("Disk 0x"))
    com.transmitString(str.integerToHexadecimal(fat.checkDiskSignature, 8))                                               
    com.transmitString(string(" with volume ID 0x"))
    com.transmitString(str.integerToHexadecimal(fat.checkVolumeIdentification, 8))
    com.transmitString(string(" a.k.a "))
    com.transmitString(str.trimString(fat.checkVolumeLabel))
    com.transmitString(string(" ready", _newLineCharacter))                           
    abort 0
 
PRI programUnmount(stringPointer)

  ifnot(str.stringCompareCI(string("unmount"), stringPointer))
    fat.unmountPartition
    com.transmitString(string("Disk ready for removal", _newLineCharacter))
    abort 0

PRI programFreeSpace(stringPointer)

  ifnot(str.stringCompareCI(string("df"), stringPointer))
  
    com.transmitString(string("The "))
    com.transmitString(str.trimString(fat.checkFileSystemType))
    com.transmitByte("_")
    com.transmitString(str.trimString(fat.checkVolumeLabel))
    com.transmitString(string(" parition has "))
    com.transmitString(1 + str.integerToDecimal(fat.checkCountOfClusters, 10))
    com.transmitString(string(" clusters and "))
    com.transmitString(1 + str.integerToDecimal(fat.checkTotalSectors, 10))
    com.transmitString(string(" total sectors.", _newLineCharacter, "Where each cluster is ")) 
    com.transmitString(1 + str.integerToDecimal(fat.checkSectorsPerCluster, 3))
    com.transmitString(string(" sectors large and each sector is "))
    com.transmitString(1 + str.integerToDecimal(fat.checkBytesPerSector, 3))
    com.transmitString(string(" bytes.", _newLineCharacter))     

    com.transmitString(string("And there are... (please wait)... "))
    com.transmitString(1 + str.integerToDecimal(fat.checkFreeSectorCount(byte[str.tokenizeString(0)]), 10))
    com.transmitString(string(" Free Sectors.", _newLineCharacter))
    abort 0

PRI programUsedSpace(stringPointer)

  ifnot(str.stringCompareCI(string("du"), stringPointer))

    com.transmitString(string("The "))
    com.transmitString(str.trimString(fat.checkFileSystemType))
    com.transmitByte("_")
    com.transmitString(str.trimString(fat.checkVolumeLabel))
    com.transmitString(string(" parition has "))
    com.transmitString(1 + str.integerToDecimal(fat.checkCountOfClusters, 10))
    com.transmitString(string(" clusters and "))
    com.transmitString(1 + str.integerToDecimal(fat.checkTotalSectors, 10))
    com.transmitString(string(" total sectors.", _newLineCharacter, "Where each cluster is ")) 
    com.transmitString(1 + str.integerToDecimal(fat.checkSectorsPerCluster, 3))
    com.transmitString(string(" sectors large and each sector is "))
    com.transmitString(1 + str.integerToDecimal(fat.checkBytesPerSector, 3))
    com.transmitString(string(" bytes.", _newLineCharacter))     

    com.transmitString(string("And there are... (please wait)... ")) 
    com.transmitString(1 + str.integerToDecimal(fat.checkUsedSectorCount(byte[str.tokenizeString(0)]), 10))
    com.transmitString(string(" Used Sectors.", _newLineCharacter))
    abort 0
 
PRI programChangeDirectory(stringPointer)

  ifnot(str.stringCompareCI(string("cd"), stringPointer))
    com.transmitString(fat.changeDirectory(str.tokenizeString(0)))
    com.transmitByte(_newLineCharacter)
    abort 0

PRI programChangeAttributes(stringPointer)

  ifnot(str.stringCompareCI(string("attrib"), stringPointer))
    com.transmitString(str.trimString(fat.changeAttributes(str.tokenizeString(0), str.tokenizeString(0))))
    com.transmitString(string("changed", _newLineCharacter))
    abort 0

PRI programRename(stringPointer)

  ifnot(str.stringCompareCI(string("rename"), stringPointer))
    com.transmitString(str.trimString(fat.renameEntry(str.tokenizeString(0), str.tokenizeString(0))))
    com.transmitString(string(" renamed", _newLineCharacter))
    abort 0

PRI programRemove(stringPointer)

  ifnot(str.stringCompareCI(string("rm"), stringPointer))
    com.transmitString(str.trimString(fat.deleteEntry(str.tokenizeString(0))))
    com.transmitString(string(" deleted", _newLineCharacter))                           
    abort 0    

PRI programMakeFile(stringPointer)

  ifnot(str.stringCompareCI(string("mkfil"), stringPointer))
    com.transmitString(str.trimString(fat.newFile(str.tokenizeString(0))))
    com.transmitString(string(" made", _newLineCharacter)) 
    abort 0

PRI programMakeDirectory(stringPointer)

  ifnot(str.stringCompareCI(string("mkdir"), stringPointer))
    com.transmitString(str.trimString(fat.newDirectory(str.tokenizeString(0))))
    com.transmitString(string(" made", _newLineCharacter)) 
    abort 0

PRI programBoot(stringPointer)

  ifnot(str.stringCompareCI(string("boot"), stringPointer))
    stringPointer := str.tokenizeString(0)
  
    com.transmitString(string("Are you sure you want to reboot from "))
    com.transmitString(stringPointer)

    if(shellDecision(string(". [y]es or [n]o? : "), "Y", "y"))    
      fat.bootPartition(stringPointer)

    com.transmitString(string(_newLineCharacter, "Guess not", _newLineCharacter))
    abort 0

PRI programFormat(stringPointer)

  ifnot(str.stringCompareCI(string("format"), stringPointer))

    if(shellDecision(string("Are you sure? [y]es or [n]o: "), "Y", "y"))
      if(shellDecision(string("Are you really sure? [y]es or [n]o: "), "Y", "y"))
        com.transmitString(string("Formating Partition...", _newLineCharacter)) 
        fat.formatPartition(str.decimalToInteger(str.tokenizeString(0)), str.tokenizeString(0)) 

    abort 0 

PRI programList(stringPointer)
  
  ifnot(str.stringCompareCI(string("ls"), stringPointer))
    fat.listName("T")
  
    repeat
      result := fat.listName(0)
      ifnot(result)
        abort 0
      
      com.transmitString(result)
      
      if(fat.listIsReadOnly)
        com.transmitString(string(" [ R"))
      else
        com.transmitString(string(" [ _"))

      if(fat.listIsHidden)
        com.transmitByte("H")
      else
        com.transmitByte("_")

      if(fat.listIsSystem)
        com.transmitByte("S")
      else
        com.transmitByte("_")

      if(fat.listIsDirectory)
        com.transmitByte("D")
      else
        com.transmitByte("_")

      if(fat.listIsArchive)
        com.transmitByte("A")
      else
        com.transmitByte("_")

      com.transmitString(string(" ] [ "))
      com.transmitString(1 + str.integerToDecimal(fat.listSize, 10))
      
      com.transmitString(string(" ] [ Accessed "))
      com.transmitString(1 + str.integerToDecimal(fat.listAccessMonth, 2))
      com.transmitByte("/")
      com.transmitString(1 + str.integerToDecimal(fat.listAccessDay, 2))
      com.transmitByte("/")
      com.transmitString(1 + str.integerToDecimal(fat.listAccessYear, 4)) 
      
      com.transmitString(string(" ] [ Created "))
      com.transmitString(1 + str.integerToDecimal(fat.listCreationHours, 2))
      com.transmitByte(":")
      com.transmitString(1 + str.integerToDecimal(fat.listCreationMinutes, 2))
      com.transmitByte(":")
      com.transmitString(1 + str.integerToDecimal(fat.listCreationSeconds, 2))                                      

      com.transmitByte(" ")
      com.transmitString(1 + str.integerToDecimal(fat.listCreationMonth, 2))
      com.transmitByte("/")
      com.transmitString(1 + str.integerToDecimal(fat.listCreationDay, 2))
      com.transmitByte("/")
      com.transmitString(1 + str.integerToDecimal(fat.listCreationYear, 4)) 
      
      com.transmitString(string(" ] [ Modified "))
      com.transmitString(1 + str.integerToDecimal(fat.listModificationHours, 2))
      com.transmitByte(":")
      com.transmitString(1 + str.integerToDecimal(fat.listModificationMinutes, 2))
      com.transmitByte(":")
      com.transmitString(1 + str.integerToDecimal(fat.listModificationSeconds, 2))                                      

      com.transmitByte(" ") 
      com.transmitString(1 + str.integerToDecimal(fat.listModificationMonth, 2))
      com.transmitByte("/")
      com.transmitString(1 + str.integerToDecimal(fat.listModificationDay, 2))
      com.transmitByte("/")
      com.transmitString(1 + str.integerToDecimal(fat.listModificationYear, 4)) 
      
      com.transmitString(string(" ]", _newLineCharacter))

PRI programPrintWorkingDirectory(stringPointer)

  ifnot(str.stringCompareCI(string("pwd"), stringPointer))
    com.transmitString(fat.listWorkingDirectory)
    com.transmitByte(_newLineCharacter)
    abort 0   
    
PRI programConcatenate(stringPointer) | fileTypeFlag, nextNewLine

  ifnot(str.stringCompareCI(string("cat"), stringPointer))
    fat.openFile(str.tokenizeString(0), "R")

    nextNewLine := false 
    fileTypeFlag := false
    if((fat.readByte == $FF) and (fat.readByte == $FE))
      fileTypeFlag := true

    fat.changeFilePosition(2 & fileTypeFlag)

    if(fileTypeFlag)
      repeat ((fat.listSize / 2) - 1)

        result := ($FF & fat.readShort)
        if(result == com#line_feed)
          nextNewLine := true
          next

        if((result == com#carriage_return) and nextNewLine~)
          com.transmitByte(_newLineCharacter)
          rtc.pauseForMilliseconds(100)
          next 

        case result
          com#backspace .. com#carriage_return, " " .. com#Delete: com.transmitByte(result)  

    else
      repeat fat.listSize
      
        result := fat.readByte
        if(result == com#line_feed)
          nextNewLine := true
          next

        if((result == com#carriage_return) and nextNewLine~)
          com.transmitByte(_newLineCharacter)
          rtc.pauseForMilliseconds(100)
          next  

        case result
          com#backspace .. com#carriage_return, " " .. com#Delete: com.transmitByte(result)

    com.transmitString(string(_newLineCharacter, _newLineCharacter, "End of File", _newLineCharacter)) 

    fat.closeFile
    abort 0  

PRI programTest(stringPointer) | startCNT, stopCNT, sectorBuffer[128]

  ifnot(str.stringCompareCI(string("test"), stringPointer))
    com.transmitString(string("Creating test file ", com#Quotation_Marks, "testfile", com#Quotation_Marks, " ... "))

    '////////Preallocate///////////////////////////////////////////////////////////////////////////////////////////////////////  

    fat.openFile(fat.newFile(string("testfile")), "W")

    startCNT := cnt
    
    repeat constant(131_072 / 512)     
      fat.writeData(@sectorBuffer, 512)

    stopCNT := cnt

    com.transmitString(string("Success", _newLineCharacter, _newLineCharacter))

    com.transmitString(string("Wrote 131,072 bytes at "))
    com.transmitString(1 + str.integerToDecimal( {
                      }((((131_072 * (^^clkfreq)) / ((stopCNT - startCNT) >> 8)) * (^^clkfreq)) >> 8) {
                      }, 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))

    '////////Byte Writing//////////////////////////////////////////////////////////////////////////////////////////////////////

    fat.openFile(string("testFile"), "W")
    com.transmitString(string("Running byte stride write test...", _newLineCharacter))

    startCNT := cnt
    
    repeat result from 0 to constant(32_768 - 1)    
      fat.writeByte(result)
      
    stopCNT := cnt

    com.transmitString(string("Wrote 32,768 bytes at "))
    com.transmitString(1 + str.integerToDecimal( {
                      }((((32_768 * (^^clkfreq)) / ((stopCNT - startCNT) >> 8)) * (^^clkfreq)) >> 8) {
                      }, 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))
    
    '////////Byte Reading//////////////////////////////////////////////////////////////////////////////////////////////////////

    fat.openFile(string("testFile"), "R")
    com.transmitString(string("Running byte stride read test...", _newLineCharacter))

    startCNT := cnt
    
    repeat result from 0 to constant(32_768 - 1)    
      if(fat.readByte <> (result & $FF))
        com.transmitString(string("Read back failed at byte "))
        com.transmitString(1 + str.integerToDecimal(result, 10))
        com.transmitByte(_newLineCharacter)
        abort 0
      
    stopCNT := cnt

    com.transmitString(string("Read 32,768 bytes at "))
    com.transmitString(1 + str.integerToDecimal( {
                      }((((32_768 * (^^clkfreq)) / ((stopCNT - startCNT) >> 8)) * (^^clkfreq)) >> 8) {
                      }, 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))  
    
    '////////Word Writing////////////////////////////////////////////////////////////////////////////////////////////////////// 
    
    fat.openFile(string("testFile"), "W")
    com.transmitString(string("Running word stride write test...", _newLineCharacter))

    startCNT := cnt
    
    repeat result from 0 to constant((65_536 / 2) - 1)    
      fat.writeShort(result)
      
    stopCNT := cnt

    com.transmitString(string("Wrote 65,5536 bytes at "))
    com.transmitString(1 + str.integerToDecimal( {
                      }((((65_536 * (^^clkfreq)) / ((stopCNT - startCNT) >> 8)) * (^^clkfreq)) >> 8) {
                      }, 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))
    
    '////////Word Reading//////////////////////////////////////////////////////////////////////////////////////////////////////

    fat.openFile(string("testFile"), "R")
    com.transmitString(string("Running word stride read test...", _newLineCharacter))

    startCNT := cnt
    
    repeat result from 0 to constant((65_536 / 2) - 1)    
      if(fat.readShort <> (result & $FFFF))
        com.transmitString(string("Read back failed at word "))
        com.transmitString(1 + str.integerToDecimal(result, 10))
        com.transmitByte(_newLineCharacter)
        abort 0
      
    stopCNT := cnt

    com.transmitString(string("Read 65,5536 bytes at "))
    com.transmitString(1 + str.integerToDecimal( {
                      }((((65_536 * (^^clkfreq)) / ((stopCNT - startCNT) >> 8)) * (^^clkfreq)) >> 8) {
                      }, 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter)) 
    
    '////////Long Writing//////////////////////////////////////////////////////////////////////////////////////////////////////
    
    fat.openFile(string("testFile"), "W")
    com.transmitString(string("Running long stride write test...", _newLineCharacter))

    startCNT := cnt
    
    repeat result from 0 to constant((131_072 / 4) - 1)    
      fat.writeLong(result)
      
    stopCNT := cnt

    com.transmitString(string("Wrote 131,072 bytes at "))
    com.transmitString(1 + str.integerToDecimal( {
                      }((((131_072 * (^^clkfreq)) / ((stopCNT - startCNT) >> 8)) * (^^clkfreq)) >> 8) {
                      }, 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))
    
    '////////Long Reading//////////////////////////////////////////////////////////////////////////////////////////////////////

    fat.openFile(string("testFile"), "R")
    com.transmitString(string("Running long stride read test...", _newLineCharacter))

    startCNT := cnt
    
    repeat result from 0 to constant((131_072 / 4) - 1)    
      if(fat.readLong <> result)
        com.transmitString(string("Read back failed at long "))
        com.transmitString(1 + str.integerToDecimal(result, 10))
        com.transmitByte(_newLineCharacter)
        abort 0
      
    stopCNT := cnt

    com.transmitString(string("Read 131,072 bytes at "))
    com.transmitString(1 + str.integerToDecimal( {
                      }((((131_072 * (^^clkfreq)) / ((stopCNT - startCNT) >> 8)) * (^^clkfreq)) >> 8) {
                      }, 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))
    
    '////////Speed Writing/////////////////////////////////////////////////////////////////////////////////////////////////////
    
    fat.openFile(string("testFile"), "W")
    com.transmitString(string("Running speed writing test (512 bytes at a time)...", _newLineCharacter))

    startCNT := cnt
    
    repeat constant(131_072 / 512)    
      fat.writeData(@sectorBuffer, 512)
      
    stopCNT := cnt

    com.transmitString(string("Wrote 131,072 bytes at "))
    com.transmitString(1 + str.integerToDecimal((((131_072 * (^^clkfreq)) / (stopCNT - startCNT)) * (^^clkfreq)), 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))

    '////////Speed Reading///////////////////////////////////////////////////////////////////////////////////////////////////// 

    fat.openFile(string("testFile"), "R")
    com.transmitString(string("Running speed reading test (512 bytes at a time)...", _newLineCharacter))

    startCNT := cnt
    
    repeat constant(131_072 / 512)      
      fat.readData(@sectorBuffer, 512)     
      
    stopCNT := cnt

    com.transmitString(string("Read 131,072 bytes at "))
    com.transmitString(1 + str.integerToDecimal((((131_072 * (^^clkfreq)) / (stopCNT - startCNT)) * (^^clkfreq)), 10))
    com.transmitString(string(" bytes per second", _newLineCharacter, _newLineCharacter))

    '////////Append Test///////////////////////////////////////////////////////////////////////////////////////////////////////

    fat.openFile(string("testFile"), "W")
    com.transmitString(string("Running append test... "))

    repeat result from 0 to constant((131_072 / 4) - 1)    
      fat.writeLong(result)

    fat.openFile(string("testFile"), "A")

    repeat result from constant(131_072 / 4) to constant((262_144 / 4) - 1)    
      fat.writeLong(result)         

    fat.openFile(string("testFile"), "R")

    repeat result from 0 to constant((262_144 / 4) - 1)    
      if(fat.readLong <> result)
        com.transmitString(string("Read back failed at long "))
        com.transmitString(1 + str.integerToDecimal(result, 10))
        com.transmitByte(_newLineCharacter)
        abort 0

    com.transmitString(string("Success", _newLineCharacter, _newLineCharacter))
    
    '////////Seek Test/////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    fat.openFile(string("testFile"), "R")
    com.transmitString(string("Running seek test... ")) 

    repeat result from (constant(262_144 / 4) - 1) to 0  
      fat.changeFilePosition(result * 4)
      if(fat.readLong <> result) 
        com.transmitString(string("Read back failed at long "))
        com.transmitString(1 + str.integerToDecimal(result, 10))
        com.transmitByte(_newLineCharacter)
        abort 0

    com.transmitString(string("Success", _newLineCharacter, _newLineCharacter)) 
    
    '//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    com.transmitString(string("Deleting test file ", com#Quotation_Marks, "testfile", com#Quotation_Marks, " ... "))
    fat.deleteEntry(string("testfile"))
    com.transmitString(string("Success", _newLineCharacter))    
    abort 0

PRI programDate(stringPointer)
    
  ifnot(str.stringCompareCI(string("date"), stringPointer))
    ifnot(rtc.checkTime)
      abort string("Operation Failed", _newLineCharacter)    

    com.transmitString(lookup(rtc.checkDay: string("Sunday"), {
                                              } string("Monday"), {
                                              } string("Tuesday"),  {
                                              } string("Wednesday"), {
                                              } string("Thursday"), {
                                              } string("Friday"), {
                                              } string("Saturday")))
    com.transmitString(string(", ")) 
    com.transmitString(lookup(rtc.checkMonth: string("January"), {
                                                } string("Feburary"), {
                                                } string("March"), {
                                                } string("April"), {
                                                } string("May"), {
                                                } string("June"), {
                                                } string("July"),  {
                                                } string("August"), {
                                                } string("September"), {
                                                } string("October"), {
                                                } string("November"), {
                                                } string("December")))

    com.transmitByte(" ")                                             
    com.transmitString(1 + str.integerToDecimal(rtc.checkDate, 2))
    com.transmitString(string(", "))
    com.transmitString(1 + str.integerToDecimal(rtc.checkYear, 4))
    com.transmitByte(" ")  
    com.transmitString(1 + str.integerToDecimal(rtc.checkMeridiemHour, 2))
    com.transmitByte(":")
    com.transmitString(1 + str.integerToDecimal(rtc.checkMinute, 2))
    com.transmitByte(":") 
    com.transmitString(1 + str.integerToDecimal(rtc.checkSecond, 2))
    com.transmitByte(" ")

    if(rtc.checkMeridiemTime)
      com.transmitString(string("PM", _newLineCharacter)) 
    else
      com.transmitString(string("AM", _newLineCharacter))
      
    abort 0 

PRI programTime(stringPointer) | time[6]

  ifnot(str.stringCompareCI(string("time"), stringPointer))
    com.transmitString(string("Please enter the exact time,", _newLineCharacter)) 

    time[0] := str.decimalToInteger(shellLine(string("Year (2000 - 2127): ")))
    time[1] := str.decimalToInteger(shellLine(string("Month (1 - 12): ")))
    time[2] := str.decimalToInteger(shellLine(string("Date (1 - 31): ")))
    time[3] := str.decimalToInteger(shellLine(string("Day (1 - 7): ")))
    time[4] := str.decimalToInteger(shellLine(string("Hours (0 - 23): ")))    
    time[5] := str.decimalToInteger(shellLine(string("Minutes (0 - 59): ")))
    ifnot(rtc.changeTime(str.decimalToInteger(shellLine(string("Seconds (0 - 59): "))), {
                       } time[5], {
                       } time[4], {
                       } time[3], {
                       } time[2], {
                       } time[1], {
                       } time[0]))
      abort string("Operation Failed", _newLineCharacter)

    com.transmitByte(_newLineCharacter)  
    programDate(string("date"))
    com.transmitString(string(_newLineCharacter, "Operation Successful", _newLineCharacter))
    abort 0

PRI programPlayWavFile(stringPointer) | numberOfChannels, sampleRate

  ifnot(str.stringCompareCI(string("play"), stringPointer))
    com.transmitString(string("Please wait...", _newLineCharacter)) 
    stringPointer := fat.openFile(str.tokenizeString(0), "R")

    if(fat.readLong <> $46_46_49_52)
      abort string("Not RIFF file", _newLineCharacter)

    if((fat.readlong + 8) <> fat.listSize)
      abort string("WAV file corrupt", _newLineCharacter)

    if(fat.readLong <> $45_56_41_57)
      abort string("Not WAVE file", _newLineCharacter)       

    if(fat.readLong <> $20_74_6D_66)
      abort string("Wav file corrupt", _newLineCharacter)

    if(fat.readLong <> 16)
      abort string("Wav file corrupt", _newLineCharacter)
      
    if(fat.readShort <> 1)
      abort string("Not LPCM file", _newLinecharacter)

    numberOfChannels := fat.readShort  
    dac.changeNumberOfChannels(numberOfChannels)
    sampleRate := fat.readLong
    dac.changeSampleRate(sampleRate)
    
    fat.changeFilePosition(34)
    result := fat.readShort

    dac.changeBitsPerSample(result)    
    dac.changeSampleSign(result == 16)

    fat.changeFilePosition(28)

    if(fat.readLong <> (sampleRate * numberOfChannels * result / 8))
      abort string("Wav file corrupt", _newLineCharacter)

    if(fat.readShort <> (numberOfChannels * result / 8))
      abort string("Wav file corrupt", _newLineCharacter)    

    fat.changeFilePosition(36)
      
    if(fat.readLong <> $61_74_61_64)
      abort string("No DATA", _newLineCharacter) 

    com.transmitString(string("Now playing "))
    com.transmitString(stringPointer)
    com.transmitString(string(_newLineCharacter, "Press enter to quit"))

    dac.changeLeftChannelVolume(_leftChannelVolume)
    dac.changeRightChannelVolume(_rightChannelVolume) 
   
    dac.clearData 
    dac.startPlayer
  
    repeat (fat.readLong / 512)
      fat.readData(dac.transferData, 512)

      if(com.receivedNumber)
        result := com.receivedByte
        if((result == com#carriage_return) or (result == com#line_feed) or (result == com#null))
          quit

    dac.stopPlayer
    com.transmitString(string(_newLineCharacter, "End of File", _newLineCharacter))
    fat.closeFile
    abort 0         

PRI programRecordWavFile(stringPointer)

  ifnot(str.stringCompareCI(string("record"), stringPointer))
    com.transmitString(string("Please wait...", _newLineCharacter)) 

    stringPointer := fat.openFile(fat.newFile(str.tokenizeString(0)), "W")
    com.transmitString(string("Now recording "))
    com.transmitString(stringPointer)
    com.transmitString(string(_newLineCharacter, "Press enter to quit"))

    fat.writeString(string("RIFF____WAVEfmt ")) ' Chunk Identification and ChunkSize and format and Subchunk 1 Identification  
    fat.writeLong(16) ' Subchunk 1 Size
    
    fat.writeShort(1) ' Audio Format
    fat.writeShort(1) ' Number of Channels

    fat.writeLong(constant(_micSampleFrequency #> 0)) ' Sample Rate
    fat.writeLong(constant((_micSampleFrequency #> 0) * (((_micBitsPerSample <# 16) #> 8) / 8))) ' Byte Rate

    fat.writeShort(constant(((_micBitsPerSample <# 16) #> 8) / 8)) ' Block Align
    fat.writeShort(constant((((_micBitsPerSample <# 16) #> 8) / 8) * 8)) ' Bits per Sample

    fat.writeString(string("data____")) ' Subchunk 2 Identification and Subchunk2Size
    
    adc.changeSampleRate(constant(_micSampleFrequency #> 0))
    adc.changeNumberOfChannels(1)
    adc.changeBitsPerSample(constant((((_micBitsPerSample <# 16) #> 8) / 8) * 8))
    adc.changeSampleSign(constant((((_micBitsPerSample <# 16) #> 8) / 8) * 8) == 16)

    adc.clearData
    repeat while(result < fat#Max_File_Size)
      fat.writeData(adc.transferData, 512)
      result += 512

      if(com.receivedNumber)
        stringPointer := com.receivedByte
        if((stringPointer == com#carriage_return) or (stringPointer == com#line_feed) or (stringPointer == com#null))
          quit

    fat.changeFilePosition(40)
    fat.writeLong(result * constant(((_micBitsPerSample <# 16) #> 8) / 8)) ' Subchunk 2 Size
    
    fat.changeFilePosition(4)
    fat.writeLong((result * constant(((_micBitsPerSample <# 16) #> 8) / 8)) + 36) ' Chunk Size

    com.transmitString(string(_newLineCharacter, "End of File", _newLineCharacter))
    
    fat.closeFile
    abort 0
   
{{

┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                                   TERMS OF USE: MIT License                                                 │                                                            
├─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation   │ 
│files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,   │
│modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the        │
│Software is furnished to do so, subject to the following conditions:                                                         │         
│                                                                                                                             │
│The above copyright notice and this permission notice shall be included in all copies or substantial portions of the         │
│Software.                                                                                                                    │
│                                                                                                                             │
│THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE         │
│WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR        │
│COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,  │
│ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                        │
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
}}                        