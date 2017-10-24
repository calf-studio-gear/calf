#ifdef PER_MODULE_ITEM
    PER_MODULE_ITEM(monosynth,           true,  "monosynth")
    PER_MODULE_ITEM(organ,               true,  "organ")
#ifdef ENABLE_EXPERIMENTAL
    PER_MODULE_ITEM(fluidsynth,          true,  "fluidsynth")
    PER_MODULE_ITEM(wavetable,           true,  "wavetable")
#endif
    // Modulator
    PER_MODULE_ITEM(multichorus,         false, "multichorus")
    PER_MODULE_ITEM(phaser,              false, "phaser")
    PER_MODULE_ITEM(flanger,             false, "flanger")
    PER_MODULE_ITEM(pulsator,            false, "pulsator")
    PER_MODULE_ITEM(ringmodulator,       false, "ringmodulator")
    
    // Simulator
    PER_MODULE_ITEM(rotary_speaker,      false, "rotaryspeaker")
    PER_MODULE_ITEM(tapesimulator,       false, "tapesimulator")
    PER_MODULE_ITEM(vinyl,               false, "vinyl")
    
    // Reverb
    PER_MODULE_ITEM(reverb,              false, "reverb")
    
    // Delay
    PER_MODULE_ITEM(vintage_delay,       false, "vintagedelay")
    PER_MODULE_ITEM(comp_delay,          false, "compdelay")
    PER_MODULE_ITEM(reverse_delay,       false, "reversedelay")
    
    // Compressor
    PER_MODULE_ITEM(compressor,          false, "compressor")
    PER_MODULE_ITEM(sidechaincompressor, false, "sidechaincompressor")
    PER_MODULE_ITEM(multibandcompressor, false, "multibandcompressor")

    PER_MODULE_ITEM(monocompressor,      false, "monocompressor")
    PER_MODULE_ITEM(deesser,             false, "deesser")
    
    // Expander
    PER_MODULE_ITEM(gate,                false, "gate")
    PER_MODULE_ITEM(sidechaingate,       false, "sidechaingate")
    PER_MODULE_ITEM(multibandgate,       false, "multibandgate")
    
    // Limiter
    PER_MODULE_ITEM(limiter,             false, "limiter")
    PER_MODULE_ITEM(multibandlimiter,    false, "multibandlimiter")
    PER_MODULE_ITEM(sidechainlimiter,    false, "sidechainlimiter")
    
    // Envelope
    PER_MODULE_ITEM(transientdesigner,   false, "transientdesigner")
    
    // Filter
    PER_MODULE_ITEM(filter,              false, "filter")
    PER_MODULE_ITEM(filterclavier,       false, "filterclavier")
    PER_MODULE_ITEM(envelopefilter,      false, "envelopefilter")
    PER_MODULE_ITEM(emphasis,            false, "emphasis")
    PER_MODULE_ITEM(vocoder,             false, "vocoder")
    
    // Equalizer
    PER_MODULE_ITEM(equalizer5band,      false, "eq5")
    PER_MODULE_ITEM(equalizer8band,      false, "eq8")
    PER_MODULE_ITEM(equalizer12band,     false, "eq12")
    PER_MODULE_ITEM(equalizer30band,     false, "eq30")
    
    // Distrotion
    PER_MODULE_ITEM(saturator,           false, "saturator")
    PER_MODULE_ITEM(crusher,             false, "crusher")
    
    // Spectral
    PER_MODULE_ITEM(exciter,             false, "exciter")
    PER_MODULE_ITEM(bassenhancer,        false, "bassenhancer")
    
    // Spatial
    PER_MODULE_ITEM(stereo,              false, "stereo")
    PER_MODULE_ITEM(haas_enhancer,       false, "haasenhancer")
    PER_MODULE_ITEM(multibandenhancer,   false, "multibandenhancer")
    PER_MODULE_ITEM(multispread,         false, "multispread")
    
    // Utility
    PER_MODULE_ITEM(mono,                false, "mono")
    PER_MODULE_ITEM(xover2,              false, "xover2")
    PER_MODULE_ITEM(xover3,              false, "xover3")
    PER_MODULE_ITEM(xover4,              false, "xover4")
    
    // Analyser
    PER_MODULE_ITEM(analyzer,            false, "analyzer")

#ifdef ENABLE_EXPERIMENTAL
    // Pitch tools
    PER_MODULE_ITEM(pitch,               false, "pitch")
    // Widget tester
    //PER_MODULE_ITEM(widgets,             false, "widgets")
#endif

#undef PER_MODULE_ITEM
#endif
