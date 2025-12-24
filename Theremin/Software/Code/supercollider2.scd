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
~ampMax = 0.8;
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

    if(~ctrl.notNil) { ~ctrl.stop; };
    if(~sine.notNil) { ~sine.free; };

    ~sine = Synth(\tofTheremin, [
        \freq, ~defaultFreqNoHand,
        \amp,  ~defaultAmpNoHand,
        \lagUp, ~lagUp,
        \lagDown, ~lagDown
    ]);

    ~lastFreq = ~defaultFreqNoHand;
    ~lastGoodD1 = nil;

    ~ctrl = Routine({
        var d1raw, d2, d1use, norm, octMax, oct, targetFreq, diff, freq, amp;

        octMax = log2(~pitchAt400mm / 1.0);

        inf.do({
            d1raw = ~d1;
            d2    = ~d2;

            d1use = d1raw;

            if(d1raw == ~noHand) {
                d1use = ~noHand;
            }{
                if(~lastGoodD1.notNil and: { ((~lastGoodD1 - d1raw).abs > ~maxDropMM) }) {
                    d1use = ~lastGoodD1;
                };

                if(d1use < ~minValidPitchMM) {
                    d1use = (~lastGoodD1.isNil).if({ ~noHand }, { ~lastGoodD1 });
                };
            };

            if(d1use != ~noHand) { ~lastGoodD1 = d1use; };

            targetFreq = (d1use == ~noHand).if({
                ~defaultFreqNoHand
            },{
                norm = d1use.clip(~dPitchMin, ~dPitchMax).linlin(~dPitchMin, ~dPitchMax, 0.0, 1.0);
                oct  = (norm * octMax);
                targetFreq = (1.0 * (2 ** oct)).clip(~pitchAt0mm, ~pitchAt400mm);
                if(d1use <= 1) { targetFreq = 0.0 };
                targetFreq
            });

            diff = targetFreq - ~lastFreq;

            freq = (diff.abs < ~deadbandHz).if(
                ~lastFreq,
                ~lastFreq + diff.clip(0 - ~maxStepHz, 0 + ~maxStepHz)
            );

            ~lastFreq = freq;

            amp = (d2 == ~noHand).if({
                ~defaultAmpNoHand
            },{
                d2.clip(~dAmpMin, ~dAmpMax)
                  .linlin(~dAmpMin, ~dAmpMax, ~ampMin, ~ampMax)
                  .clip(~ampMin, ~ampMax)
            });

            ~sine.set(\freq, freq, \amp, amp);

            ~dt.wait;
        });
    }).play;

    "Theremin running.".postln;
});
)
