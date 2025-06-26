(synth
  (def sine-freq (* (+ 1600.0 (* 700.0 (sine :freq 0.1 :phase 0.0))) (+ 0.5 (* 0.5 (noise :seed 5)))))
  (def sine-phase (* 64.0 (sine :freq 0.1 :phase 0.0)))
  (def output (* 0.06 (sine :freq (ref sine-freq) :phase (ref sine-phase)))))
