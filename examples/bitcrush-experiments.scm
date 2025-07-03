(synth
  (def target-freq (+ 8000.0 (* 4000.0 (sine 0.1))))
  (def bits (* 8 0.5 (+ 1.2 (sine 0.15))))
  (def output (bitcrush (diode (sine 55.0)) (ref target-freq) (ref bits))))
