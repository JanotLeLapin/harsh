(synth
  (def term-freq (* 0.25 (sine :freq 0.03 :phase 0.0)))
  (def term (* 2.0 (+ 1.0 (* 0.5 (sine :freq (ref term-freq) :phase 0.0)))))
  (def noise-amp (+ 0.054 (* 0.05 (sine :freq (noise :seed 4.0) :phase 0.0) (square :freq 0.5 :phase 0.0))))
  (def carrier (sine :freq 40.0 :phase (* (ref noise-amp) (noise :seed 723.0))))
  (def output (log (+ (ref term) (ref carrier)))))
