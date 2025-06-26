(synth
  (def mod-amp-freq (sine :freq 0.04 :phase 0.0))
  (def mod-amp (sine :freq (* 8.0 (ref mod-amp-freq)) :phase 0.0))
  (def mod (+ 0.0 (* 0.5 (noise :seed 777.0)) (* 0.5 (sine :freq 440.0 :phase 0.0))))
  (def output (sine :freq 110.0 :phase (* (ref mod-amp) (ref mod)))))
