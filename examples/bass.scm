(synth
  (def noise-amp (+ 0.004 (* 0.002 (sine :freq 0.06 :phase 0.0))))
  (def bits (* 4.0 (+ 2.0 (sawtooth :freq 0.2 :phase 0.0))))
  (def carrier (square :freq 36.0 :phase (* (ref noise-amp) (noise :seed 420.0))))
  (def output (bitcrush (ref carrier) :target_freq 12000.0 :bits (ref bits)))))
