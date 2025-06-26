(synth
  (def output (* 0.06 (sine :freq (* (+ 1600.0 (* 700.0 (sine :freq 0.1 :phase 0.0))) (+ 0.5 (* 0.5 (noise :seed 5)))) :phase (* 64.0 (sine :freq 0.1 :phase 0.0))))))
