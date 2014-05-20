#ifdef PER_MODULE_ITEM
    // synthesizers
    PER_MODULE_ITEM(monosynth, true, "monosynth")
    PER_MODULE_ITEM(organ, true, "organ")
#ifdef ENABLE_EXPERIMENTAL
    PER_MODULE_ITEM(fluidsynth, true, "fluidsynth")
    PER_MODULE_ITEM(wavetable, true, "wavetable")
#endif
    // modulation
    PER_MODULE_ITEM(multichorus, false, "multichorus")
    PER_MODULE_ITEM(phaser, false, "phaser")
    PER_MODULE_ITEM(flanger, false, "flanger")
    PER_MODULE_ITEM(pulsator, false, "pulsator")
    PER_MODULE_ITEM(ringmodulator, false, "ringmodulator")
    PER_MODULE_ITEM(rotary_speaker, false, "rotaryspeaker")
    
    // delay/reverb
    PER_MODULE_ITEM(reverb, false, "reverb")
    PER_MODULE_ITEM(vintage_delay, false, "vintagedelay")
    PER_MODULE_ITEM(comp_delay, true, "compdelay")
    
    // dynamics
    PER_MODULE_ITEM(compressor, false, "compressor")
    PER_MODULE_ITEM(sidechaincompressor, false, "sidechaincompressor")
    PER_MODULE_ITEM(multibandcompressor, false, "multibandcompressor")
    PER_MODULE_ITEM(monocompressor, false, "monocompressor")
    PER_MODULE_ITEM(deesser, false, "deesser")
    PER_MODULE_ITEM(gate, false, "gate")
    PER_MODULE_ITEM(sidechaingate, false, "sidechaingate")
    PER_MODULE_ITEM(multibandgate, false, "multibandgate")
    PER_MODULE_ITEM(limiter, false, "limiter")
    PER_MODULE_ITEM(multibandlimiter, false, "multibandlimiter")
    PER_MODULE_ITEM(sidechainlimiter, false, "sidechainlimiter")
    PER_MODULE_ITEM(transientdesigner, false, "transientdesigner")
    
    // filters
    PER_MODULE_ITEM(filter, false, "filter")
    PER_MODULE_ITEM(filterclavier, false, "filterclavier")
    PER_MODULE_ITEM(equalizer5band, false, "eq5")
    PER_MODULE_ITEM(equalizer8band, false, "eq8")
    PER_MODULE_ITEM(equalizer12band, false, "eq12")
    PER_MODULE_ITEM(emphasis, false, "emphasis")
    PER_MODULE_ITEM(xover2, false, "xover2")
    PER_MODULE_ITEM(xover3, false, "xover3")
    PER_MODULE_ITEM(xover4, false, "xover4")
    PER_MODULE_ITEM(vocoder, false, "vocoder")
    
    // distortion
    PER_MODULE_ITEM(saturator, false, "saturator")
    PER_MODULE_ITEM(exciter, false, "exciter")
    PER_MODULE_ITEM(bassenhancer, false, "bassenhancer")
    PER_MODULE_ITEM(tapesimulator, false, "tapesimulator")
    PER_MODULE_ITEM(crusher, false, "crusher")
    
    // tools
    PER_MODULE_ITEM(mono, false, "mono")
    PER_MODULE_ITEM(stereo, false, "stereo")
    PER_MODULE_ITEM(analyzer, false, "analyzer")

#undef PER_MODULE_ITEM
#endif
