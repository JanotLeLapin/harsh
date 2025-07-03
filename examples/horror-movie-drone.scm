(synth
  (def term-freq (* 0.25 (sine 0.03)))
  (def term (* 2.0 (+ 1.0 (* 0.5 (sine (ref term-freq))))))
  (def noise-amp (+ 0.054 (* 0.05 (sine :freq (noise :seed 4.0)) (square 0.5))))
  (def carrier (sine 40.0 :phase (* (ref noise-amp) (noise :seed 723.0))))
  (def output (log (+ (ref term) (ref carrier)))))
