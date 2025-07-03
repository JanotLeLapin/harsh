(synth
  (def noise-amp (+ 0.004 (* 0.002 (sine 0.06))))
  (def bits (* 4.0 (+ 2.0 (sawtooth 0.2))))
  (def env (ad 1.0 40.0))
  (def carrier (square :freq (midi->freq (+ 27.0 (* 27.0 (ref env)))) :phase (* (ref noise-amp) (noise :seed 420.0))))
  (def output (lowpass (bitcrush (ref carrier) :target_freq 12000.0 :bits (ref bits)) :cutoff 650.0 :stages 2.0))))
