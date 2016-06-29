CON
  NO_WORKER = 255
  
VAR
  
OBJ

  fat: "SD2.0_FATEngine.spin"
  dac: "PCM2.1_DACEngine.spin"

PUB start(sd_p0, audio_p0) | check

  ifnot(fat.FATEngineStart(sd_p0, sd_p0+1, sd_p0+2, sd_p0+3))
    reboot

  ifnot(dac.DACEngineStart(audio_p0, audio_p0+1, 0))
    reboot

  check := \fat.mountPartition(0, 0)
  if check < 0
    reboot

PUB sampleAddress
  return dac.sampleAddress

PUB synchronously(stringPointer, numTracksToPlay) | numberOfChannels, sampleRate

    fat.openFile(stringPointer, "R")

    if(fat.readLong <> $46_46_49_52)
      return string("Not RIFF file")

    if((fat.readlong + 8) <> fat.listSize)
      return string("WAV file corrupt")

    if(fat.readLong <> $45_56_41_57)
      return string("Not WAVE file")       

    if(fat.readLong <> $20_74_6D_66)
      return string("Wav file corrupt")

    if(fat.readLong <> 16)
      return string("Wav file corrupt")
      
    if(fat.readShort <> 1)
      return string("Not LPCM file")

    numberOfChannels := fat.readShort
\   dac.changeNumberOfChannels(numberOfChannels)
    dac.changeNumberOfTracksToPlay(numTracksToPlay)
    sampleRate := fat.readLong
    dac.changeSampleRate(sampleRate)
    
    fat.changeFilePosition(34)
    result := fat.readShort

    if result <> 16
      return STRING("We only support 16-bit samples")
      
    fat.changeFilePosition(28)

    if(fat.readLong <> (sampleRate * numberOfChannels * result / 8))
      return string("Wav file corrupt")

    if(fat.readShort <> (numberOfChannels * result / 8))
      return string("Wav file corrupt")    

    fat.changeFilePosition(36)

    ' There may be other sections of information, such as the LIST section.
    ' Skip over the sections until we hit the DATA section.
    repeat while fat.readLong <> $61_74_61_64
      fat.changeFilePosition(fat.readLong + fat.checkFilePosition)

    dac.clearData
    dac.startPlayer

    repeat (fat.readLong / 512) ' fat.readLong / 512)
       fat.readData(\dac.transferData, 512)

    dac.stopPlayer
    fat.closeFile

    return -1   