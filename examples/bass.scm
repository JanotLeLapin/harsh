(synth
  (def noise-amp (+ 0.004 (* 0.002 (sine :freq 0.06 :phase 0.0))))
  (def bits (* 4.0 (+ 2.0 (sawtooth :freq 0.2 :phase 0.0))))
  (def env (ad 1.0 40.0))
  (def carrier (square :freq (midi->freq (+ 27.0 (* 27.0 (ref env)))) :phase (* (ref noise-amp) (noise :seed 420.0))))
  (def output (lowpass 650.0 2.0 (bitcrush (ref carrier) :target_freq 12000.0 :bits (ref bits))))))
