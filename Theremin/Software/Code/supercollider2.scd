(
~oscPort = 57120;
~noHand  = 1000;

~ctrlHz = 200;
~dt = 1.0/~ctrlHz;

~dPitchMin = 0;
~dPitchMax = 350;

~dAmpMin   = 0;
~dAmpMax   = 350;

~pitchAt0mm   = 0;
~pitchAt400mm = 3000;
~defaultFreqNoHand = 440;

~ampMin = 0.001;
~ampMax = 0.6;
~defaultAmpNoHand = ~ampMax;

~deadbandHz = 0.8;
~maxStepHz  = 4000.0;

~lagUp   = 0.1;
~lagDown = 0.1;

~minValidPitchMM = 5;     // treat 0..4mm as glitch unless you're truly at the sensor
~maxDropMM = 80;          // ignore sudden drops bigger than this (glitch guard)

thisProcess.openUDPPort(~oscPort);

~d1 = ~noHand;
~d2 = ~noHand;

OSCdef(\sensorsIn, { |msg|
    ~d1 = msg[1].asInteger;
    ~d2 = msg[2].asInteger;
}, "/sensors", recvPort: ~oscPort);

s.options.sampleRate = 48000;
s.options.blockSize = 512;
s.options.numOutputBusChannels = 2;

s.waitForBoot({

    SynthDef(\tofTheremin, { |out=0, freq=440, amp=0, lagUp=0.004, lagDown=0.008|
        var f, a, sig;
        f = VarLag.kr(freq.clip(0, 2000), lagUp, lagDown, \sine);
        a = Lag.kr(amp.clip(0.0, 0.8), 0.02);
        sig = SinOsc.ar(f) + (SinOsc.ar(f*1.001)*0.1) + (SinOsc.ar(f*0.999)*0.1);
        sig = (sig * 1.5).tanh * a;
        Out.ar(out, sig!2);
    }).add;

    s.sync;

)); "Theremin running.".postln;eq, \amp, amp);~ampMin, ~ampMax)tepHz)tchAt400mm);tchMax, 0.0, 1.0);