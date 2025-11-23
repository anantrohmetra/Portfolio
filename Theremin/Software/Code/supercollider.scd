
// Open your serial port (adjust the device path if needed)

(
~pot = SerialPort.new("/dev/ttyUSB0", 115200);
)

// ---- Shared state for two sensors ----

(
~d1 = nil;    // latest distance #1 (mm) or nil if OOR
~d2 = nil;    // latest distance #2 (mm) or nil if OOR
~t1 = nil;    // timestamp of #1 update
~t2 = nil;    // timestamp of #2 update
)



(
// Parser working vars
~buf  = List.new;  // collects digits
~sawN = false;     // true after 'N' (for "Na"/"Nb")
)



// ---- Reader Routine ----
// Expects Arduino to print e.g. "523a612b\n", or "Na"/"Nb" for out-of-range
(
~reader = Routine({
    var byte, ch, val;

    inf.do({
        byte = ~pot.read; // returns -1 if nothing available
        if(byte >= 0, {
            ch = byte.asAscii;

            if(ch.isDecDigit, {
                ~buf.add(ch.digit);
                ~sawN = false;
            }, {
                // Mark out-of-range
                if(ch == $N, {
                    ~buf = List.new;
                    ~sawN = true;
                });

                // Sensor #1 terminator
                if(ch == $a, {
                    if(~sawN, {
                        ~d1 = nil;
                    }, {
                        if(~buf.size > 0, {
                            val = ~buf.asArray.convertDigits;
                            ~d1 = val; ~t1 = Main.elapsedTime;
                        });
                    });
                    ~buf = List.new; ~sawN = false;
                });

                // Sensor #2 terminator
                if(ch == $b, {
                    if(~sawN, {
                        ~d2 = nil;
                    }, {
                        if(~buf.size > 0, {
                            val = ~buf.asArray.convertDigits;
                            ~d2 = val; ~t2 = Main.elapsedTime;
                        });
                    });
                    ~buf = List.new; ~sawN = false;
                });

                // ignore newlines / others silently
            });
        });

        0.0001.wait; // gentle polling
    });
}).play;
)

(
SynthDef(\tofSine, { |out = 0, freq = 440, amp = 0|
    var f, a;
    var base, base2, base3, mod, sig;

    // Same smoothing and clamping as before
    f = Lag.kr(freq.clip(100, 1000), 0.10);   // smooth pitch
    a = Lag.kr(amp.clip(0, 1), 0.10);         // smooth amplitude

    // Your theremin-ish core, now using `f`
    base  = SinOsc.ar(f);
    base2 = SinOsc.ar(f * 1.001, 0, 0.1);
    base3 = SinOsc.ar(f * 0.999, 0, 0.1);

    // Audio-rate harmonic / ring-mod brightness
    mod = SinOsc.ar(f * 2, 0, 1.2);

    // Mix your original timbre + detuned layer
    sig = Mix([
        (base + base2 + base3) * 0.25,   // thick core
        (base * mod) * 0.3               // ring-mod layer
    ]);

    // Soft clipping / tube-ish saturation
    sig = (sig * 1.5).tanh * a;

    // Stereo out
    Out.ar(out, sig ! 2);
}).add;
)


~sine = Synth(\tofSine, [\freq, 440, \amp, 0.5]);


(
~ctrl = Routine({
    var d1, d2, freq, amp;

    inf.do({
        d1 = ~d1;  // sensor #1
        d2 = ~d2;  // sensor #2

        // pitch from d1
        freq = if(d1.isNil, { 440 }, {
            d1.linlin(30, 400, 100, 1000).clip(100, 1000)
        });

        // amplitude from d2:
        // d2 = 30  -> amp = 0.7 (max)
        // d2 = 400 -> amp = 0   (min)
        amp = if(d2.isNil, { 0 }, {
            d2.linlin(200, 400, 0.7, 0).clip(0, 0.7)
        });

        // If freq source is nil, also mute
        if(d1.isNil) { amp = 0 };

        ~sine.set(\freq, freq, \amp, amp);
        0.0001.wait; // ~50 Hz updates
    });
}).play;
)
