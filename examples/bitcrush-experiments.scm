(synth
  (def target-freq (+ 8000.0 (* 4000.0 (sine :freq 0.1 :phase 0.0))))
  (def bits (* 4 (+ 1.2 (sine :freq 0.15 :phase 0.0))))
  (def output (bitcrush (diode (sine :freq 55.0 :phase 0.0)) :target_freq (ref target-freq) :bits (ref bits))))
