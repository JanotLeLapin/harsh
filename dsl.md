# Harsh DSL

Harsh's DSL docs

## Syntax

Harsh's DSL uses a Scheme-like syntax, here's an example:

```scm
(synth
  (def freq (midi->freq :in 72))
  (def mod (sine :freq (+ (ref freq) :phase (hardclip :in (* 34.0 (sine :freq 0.2)) :threshold 32.0))))
  (def output (sine :freq (ref freq) :phase (ref mod))))
```

This patch represents a sine wave (output) oscillating at a frequency of 523.25Hz (midi pitch 72).
Its phase is modulated by another sine wave (mod), oscillating between a frequency of 523.25Hz ± 32Hz.
The mod sine wave's frequency is modulated by yet another sine wave oscillating at 0.2Hz, scaled by 34 and hard clipped to ±32, creating frequency deviation of up to ±32Hz around the base frequency.

We can rewrite the same patch without verbose named arguments:

```scm
(synth
  (def freq (midi->freq 72))
  (def mod (sine (+ (ref freq) (hardclip (* 34.0 (sine 0.2)) 32.0))))
  (def output (sine (ref freq) (ref mod))))
```

Check out the examples directory for more examples.

### Parameters

Harsh's DSL supports flexible parameter passing - you can mix positional and named arguments in the same function call. Each parameter has a default position in the argument list, with the input signal (`:in`) always being the first parameter for signal processing functions:

- `(sine 440.0 0.0)`: both frequency and phase as positional arguments
- `(sine :freq 440.0 :phase 0.0)`: both as named arguments
- `(sine 440.0 :phase 0.0)`: frequency positional, phase named
- `(lowpass signal 650.0 :stages 2)`: input and cutoff positional, stages named
- `(lowpass :in signal :cutoff 650.0 :stages 2)`: all as named arguments

## Expressions

Harsh DSL expressions

### Math

Mathematical expressions

- `(+ a b c) => a + b + c`
- `(- a b c) => a - b - c`
- `(* a b c) => a * b * c`
- `(/ a b c) => a / b / c`
- `(pow a b c) => (a^b)^c`
- `(log a) => log(a)`
- `(log2 a) => log2(a)`
- `(log10 a) => log10(a)`
- `(exp a) => e^a`

### Comparison

- `(< a b) => a < b`
- `(<= a b) => a <= b`
- `(> a b) => a > b`
- `(>= a b) => a >= b`
- `(= a b) => a == b`
- `(!= a b) => a != b`

### Conversion

- `(freq->midi 440.0) = 69`: from frequency to MIDI pitch
- `(midi->freq 69) = 440.0`: from MIDI pitch to frequency
- `(db->amp -6.0) ~= 0.5`: from decibel to amplitude
- `(amp->db 0.5) ~= -6.0`: from amplitude to decibel

### Noise

`(noise :seed 42)`: normally distributed deterministic random value

- `:seed (required)`

### Oscillator

`(sine/square/sawtooth :freq 440.0 :phase 0.0)`

- `:freq (required)`: frequency
- `:phase (optional, default = 0)`: phase offset

### Diode

`(diode :in signal)`: evaluates to log(1 + exp(signal))

- `:in (required)`: input

### Clip

`(hardclip/foldback :in signal :threshold 0.75)`

- `:in (required)`: input
- `:threshold (required)`: threshold

### Filter

`(lowpass/highpass :in signal :cutoff 650.0 :stages 2)`

- `:in (required)`: input
- `:cutoff (required)`: frequency cutoff
- `:stages (optional, default = 1)`: sound attenuation beyond cutoff is `(6 * stages)dB/octave`

### Bitcrush

`(bitcrush :in signal :target_freq 8000.0 :bits 7.0)`: downsamples and quantizes the input signal

- `:in (required)`: input
- `:target_freq (optional, default = 44100)`: target frequency (downsample)
- `:bits (optional, default = 16)`: floating point sample value precision (quantize)

### Pan

`(pan :in signal :alpha 0.0)`

- `:in (required)`: input
- `:alpha (required)`: panning position (-1.0 = full left, 1.0 = full right, 0.0 = center)

### Envelope

`(envelope time0 amp0 time1 amp1 ... timen ampn)`: stores a sequence of points with linear smoothing
