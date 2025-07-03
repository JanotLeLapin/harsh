(synth
  (def mod-amp-freq (sine 0.04))
  (def mod-amp (sine (* 8.0 (ref mod-amp-freq))))
  (def mod (+ 0.0 (* 0.5 (noise 777.0)) (* 0.5 (sine 440.0))))
  (def output (sine 110.0 (* (ref mod-amp) (ref mod)))))
